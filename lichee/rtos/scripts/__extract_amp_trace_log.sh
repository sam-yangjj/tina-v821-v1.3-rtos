#!/bin/bash

function usage()
{
	echo "Usage: $0 <coredump_file> <trace_log_buf_addr>"
	echo -e "\nexample: $0 coredump.elf 0x10000000"
}

function dump_segment_info()
{
	index=0
	echo "Segment Info:"
	for item in ${segment_size_hex_arr[@]}
	do
		segment_size=$(printf %d 0x${item})

		segment_start_addr=$(printf %d 0x${segment_addr_hex_arr[${index}]})
		segment_end_addr=$[ ${segment_start_addr} + ${segment_size} - 1]

		echo "segment${index}: $(printf 0x%x ${segment_start_addr}) - $(printf 0x%x ${segment_end_addr})"
		let index++
	done
}

if [[ $# -ne 2 ]]
then
	usage
	exit 1
fi

coredump_file=$1
buf_addr_hex=$2

if [[ ! -f ${coredump_file} ]]
then
	echo "Error: cordump file '${coredump_file}' not exist!"
	exit 2
fi

current_date=$(date +%Y%m%d_%H%M%S)

buf_addr=$(printf %u ${buf_addr_hex})

echo "coredump file: ${coredump_file}"
echo "trace log buffer addr: ${buf_addr}($(printf 0x%x ${buf_addr}))"

orig_section_info=$(objdump -h ${coredump_file})
section_info=$(echo "${orig_section_info}" | grep load)

# segment size
segment_size_hex_arr=($(echo "${section_info}" | awk '{print $3}'))

# segment start addr
segment_addr_hex_arr=($(echo "${section_info}" | awk '{print $4}'))

# segment offset in file
segment_offset_hex_arr=($(echo "${section_info}" | awk '{print $6}'))


is_segment_found=0
index=0
for item in ${segment_size_hex_arr[@]}
do
	segment_size=$(printf %d 0x${item})

	segment_start_addr=$(printf %d 0x${segment_addr_hex_arr[${index}]})
	segment_end_addr=$[ ${segment_start_addr} + ${segment_size} - 1]

	#echo "segment_start_addr: ${segment_start_addr}($(printf 0x%x ${segment_start_addr}))"
	#echo "segment_end_addr: ${segment_end_addr}($(printf 0x%x ${segment_end_addr}))"

	if [[ ${buf_addr} -ge ${segment_start_addr} && ${buf_addr} -le ${segment_end_addr} ]]
	then
		segment_offset_in_file=$(printf %d 0x${segment_offset_hex_arr[${index}]})
		echo "segment_offset_in_file: ${segment_offset_in_file}($(printf 0x%x ${segment_offset_in_file}))"

		buf_offset_in_file=$[ ${buf_addr} - ${segment_start_addr} + ${segment_offset_in_file} ]
		echo "buf_offset_in_file: ${buf_offset_in_file}($(printf 0x%x ${buf_offset_in_file}))"
		is_segment_found=1
		break
	fi
	
	let index++
done

if [[ ${is_segment_found} -eq 0 ]]
then
	echo "There is no segment found for trace log buffer addr: ${buf_addr_hex}, total segment count: ${index}"
	dump_segment_info
	exit 3
fi

output_dir=${coredump_file%/*}

if [[ ${output_dir} == ${coredump_file} ]]
then
	output_dir=.
fi

output_dir=${output_dir}/

echo "output directory: ${output_dir}"

trace_buf_payload_file="${output_dir}trace_buf_payload_${current_date}.bin"
trace_buf_header_file="${output_dir}trace_buf_header_${current_date}.bin"
trace_log_file="${output_dir}trace_log_${current_date}.txt"

if [[ -f ${trace_buf_header_file} ]]
then
	rm -fv ${trace_buf_header_file}
fi

if [[ -f ${trace_buf_payload_file} ]]
then
	rm -fv ${trace_buf_payload_file}
fi

if [[ -f ${trace_log_file} ]]
then
	rm -fv ${trace_log_file}
fi

echo -e "\nExtract trace buffer header:"

trace_buf_header_size=128
dd if=${coredump_file} of=${trace_buf_header_file} bs=1 count=${trace_buf_header_size} skip=${buf_offset_in_file}

default_writer_info_size=64
writer_info_size=$(hexdump -v -e '1/4 "%u"' -n 4 -s 0x48 ${trace_buf_header_file})
if [[ ${writer_info_size} -ne ${default_writer_info_size} ]]
then
	echo "Invalid writer info size: ${writer_info_size}, should be ${default_writer_info_size}!"
	rm -f ${trace_buf_header_file}
	exit 4
fi

echo -e "\nExtract trace buffer payload:"
payload_size=$(hexdump -v -e '1/4 "%u"' -n 4 -s 0x4C ${trace_buf_header_file})
payload_offset_in_file=$[ ${buf_offset_in_file} + ${trace_buf_header_size} ]
dd if=${coredump_file} of=${trace_buf_payload_file} bs=1 count=${payload_size} skip=${payload_offset_in_file}

echo ""
trace_log_buf_size=$[ ${trace_buf_header_size} + ${payload_size} ]
echo "trace log buffer size: ${trace_log_buf_size}"
echo "header size: ${trace_buf_header_size}"
echo "payload size: ${payload_size}"

# format the trace log
write_offset=$(hexdump -v -e '1/4 "%u"' -n 4 -s 0x40 ${trace_buf_header_file})
is_overrun=$(hexdump -v -e '1/4 "%u"' -n 4 -s 0x50 ${trace_buf_header_file})

echo -e "\nExtract trace log:"

if [[ ${is_overrun} -ne 0 ]]
then
	read_skip_byte_cnt=$[ ${write_offset} - ${payload_size} ]
	first_read_byte_cnt=$[ ${payload_size} - ${read_skip_byte_cnt} ]
	dd if=${trace_buf_payload_file} of=${trace_log_file} bs=1 count=${first_read_byte_cnt} skip=${read_skip_byte_cnt}

	second_read_byte_cnt=${read_skip_byte_cnt}
	dd if=${trace_buf_payload_file} of=${trace_log_file} bs=1 count=${second_read_byte_cnt} seek=${first_read_byte_cnt}
else
	read_byte_cnt=${write_offset}
	dd if=${trace_buf_payload_file} of=${trace_log_file} bs=1 count=${read_byte_cnt}
fi

rm -f ${trace_buf_header_file}
rm -f ${trace_buf_payload_file}

echo -e "\ntrace log file: ${trace_log_file}"
exit 0
