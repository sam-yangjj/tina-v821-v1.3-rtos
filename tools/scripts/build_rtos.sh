#!/bin/bash

RTOS_TOPDIR=${1}
cd ${RTOS_TOPDIR}

source ${RTOS_TOPDIR}/tools/scripts/envsetup.sh

function print_help()
{
	echo "build_rtos.sh - this script is used to build rots when terminal have not run lunch_rtos and copy rots bin to tina environment."
	echo "Usage examples: ./build_rtos.sh rtos_path rtos_project_name Tina_bin_path"
}

function build_rtos()
{
	if [ ! -d "$1" ]; then
		echo -e "ERROR: rtos path ${1} not exit"
		print_help
		return 1
	fi

	local PROJECT
	local TINA_RTOS_BIN
	local T=$(gettop)

	if [ -n "$2" ]; then
		PROJECT=$2
	else
		echo -e "ERROR: rtos project name is null"
		print_help
		return 1
	fi

	lunch_rtos $PROJECT

	if [ "x$4" = "x" ]; then
		mrtos clean
		mrtos pm
	fi
	mrtos $4

	build_results=$?

	if [ $build_results != 0 ]; then
		return $build_results
	fi

	if [ "x$4" = "xclean" ]; then
		return 0
	elif [ "x$4" = "xoldconfig" ]; then
		return 0
	elif [ "x$4" = "xmenuconfig" ]; then
		return 0
	elif [ "x$4" = "xpm" ]; then
		mrtos clean
		return 0
	elif [ "x$4" = "xmkconfig" ]; then
		return 0
	fi

	if [ -n "$3" ]; then
		TINA_RTOS_BIN=$3
	else
		echo -e "INFO: TINA_RTOS_BIN is null"
		print_help
		return 1
	fi

	#copy rtos bin to Tina environment
	local RISV64_STRIP=${RTOS_BUILD_TOOLCHAIN}strip
	local IMG_DIR=${T}/lichee/rtos/build/${PROJECT}/img
	rtos_config=${T}/lichee/rtos/.config
	if grep -q "CONFIG_KERNEL_COMPRESS=y" $rtos_config; then
		if [ -f ${IMG_DIR}/zImage ]; then
			if grep -q "CONFIG_GEN_DIGEST=y" $rtos_config; then
				if grep -q "CONFIG_VERIFY_MD5=y" $rtos_config; then
					${T}/lichee/rtos/scripts/digest/rv_digest -S ${IMG_DIR}/zImage -a md5
				elif grep -q "CONFIG_VERIFY_CRC32=y" $rtos_config; then
					${T}/lichee/rtos/scripts/digest/rv_digest -S ${IMG_DIR}/zImage -a crc32
				fi
			fi
			cp -v ${IMG_DIR}/zImage ${TINA_RTOS_BIN}
			${RISV64_STRIP} ${TINA_RTOS_BIN}
		else
			echo -e "ERROR: rtos zImage not exit"
			return 1
		fi
	else
		if [ -f ${IMG_DIR}/rt_system.elf ]; then
			if grep -q "CONFIG_GEN_DIGEST=y" $rtos_config; then
				if grep -q "CONFIG_VERIFY_MD5=y" $rtos_config; then
					${T}/lichee/rtos/scripts/digest/rv_digest -S ${IMG_DIR}/rt_system.elf -a md5
				elif grep -q "CONFIG_VERIFY_CRC32=y" $rtos_config; then
					${T}/lichee/rtos/scripts/digest/rv_digest -S ${IMG_DIR}/rt_system.elf -a crc32
				fi
			fi
			cp -v ${IMG_DIR}/rt_system.elf ${TINA_RTOS_BIN}
			${RISV64_STRIP} ${TINA_RTOS_BIN}
		else
			echo -e "ERROR: rt_system.elf not exit"
			return 1
		fi
	fi
	return 0
}

build_rtos $@
