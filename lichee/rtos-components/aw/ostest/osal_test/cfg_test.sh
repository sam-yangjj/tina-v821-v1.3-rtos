#!/bin/bash

if [ "x$(echo ${RTOS_TARGET_BOARD_PATH} | grep "r128\|xr875")" != "x" ]; then
	file=$RTOS_BUILD_TOP/board/${RTOS_TARGET_BOARD_PATH%_*}/configs/sys_config.fex
else
	file=$RTOS_BUILD_TOP/board/${RTOS_TARGET_BOARD_PATH}/configs/sys_config.fex
fi

content="
[osal_cfg_test]
test_key_value = 1
test_gpio1 = port:PA16<5><1><default><default>
test_gpio2 = port:PA17<5><1><default><default>"

# 检查文件是否存在
if [ -f "$file" ]; then
if grep -qF "[osal_cfg_test]" "$file"; then
    echo "Found [osal_cfg_test] in file, skipping operation."
else
    sed -i -e '$a[osal_cfg_test]' $file
    sed -i -e '$atest_key_value = 1' $file
    sed -i -e '$atest_gpio1 = port:PA16<5><1><default><default>' $file
    sed -i -e '$atest_gpio2 = port:PA17<5><1><default><default>' $file
    echo "sys_config.fex add cfg_test content ok"

fi
else
    echo "file $file not exit"
fi
