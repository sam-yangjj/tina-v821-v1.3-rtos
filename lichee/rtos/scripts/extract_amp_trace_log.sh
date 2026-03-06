#!/bin/bash

function usage()
{
	echo "Usage: $0 <coredump_file> <firmware_elf_file>"
	echo -e "\nexample: $0 coredump.elf rt_system.elf"
}

if [[ $# -ne 2 ]]
then
	usage
	exit 1
fi

coredump_file=$1
fw_elf_file=$2

if [[ ! -f ${coredump_file} ]]
then
	echo "Error: cordump file '${coredump_file}' not exist!"
	exit 2
fi

if [[ ! -f ${fw_elf_file} ]]
then
	echo "Error: firmware ELF file '${fw_elf_file}' not exist!"
	exit 3
fi

current_script_dir=$(cd $(dirname $0) && pwd)

trace_log_buf_addr=$(objdump -t ${fw_elf_file} | grep -w amp_log_buffer | awk '{print $1}')

script_file="${current_script_dir}/__extract_amp_trace_log.sh"

if [[ ! -f ${script_file} ]]
then
	echo "Error: script file '${script_file}' not exist!"
	exit 4
fi

"${script_file}" "${coredump_file}" "0x${trace_log_buf_addr}"
