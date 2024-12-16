#!/bin/bash

verNumber=0
i2cStatus=""
dir="/etc/mcu_bin"
currentVersion=0
files=$(ls $dir)
filename="mcu_"
filenameType=".bin"
pathMCU=""
echo "files in /etc/mcu_bin":  "${files}"

i2cStatus=$(/etc/init.d/i2c_daemon status)

if [[ "${i2cStatus}" != "running" ]]; then
    /etc/init.d/i2c_deamon start
fi

currentVersion=$(request_ctrl SW_VERS get)

pathMCU+="${dir}"
pathMCU+="/"
pathMCU+="${filename}"
pathMCU+="${currentVersion}"
pathMCU+="${filenameType}"
echo "path MCU:" "${pathMCU}"

/etc/init.d/i2c_daemon stop
i2cdetect -y 0
gpio_ctrl BOOT0 SET 1
gpio_ctrl IOMCU_RESET RESET
i2cdetect -y 0
echo "FLASHING MCU version" "${verNumber}"
stm32flash -r "${pathMCU}" -a 0x3b /dev/i2c-0
gpio_ctrl BOOT0 SET 0
gpio_ctrl IOMCU_RESET RESET
i2cdetect -y 0
sleep 1
/etc/init.d/i2c_daemon start
i2cStatus=$(/etc/init.d/i2c_daemon status)
currentVersion=$(request_ctrl SW_VERS get)

if [[ "${i2cStatus}" != "running" ]]; then
  echo "I2C can not run"
fi

if [[ "$files" =~ \.bin ]]; then
    regex="mcu_([0-9]+)\.bin"
    for file in $files; do
        if [[ $file =~ $regex ]]; then
            verNumber="${BASH_REMATCH[1]}"
            echo "Find mcu prog with version: $verNumber"
            else
              verNumber=0
              echo "mcu file not found"
        fi
    done
fi

if [[ "${currentVersion}" -ne "${verNumber}" ]]; then
 echo "Reading MCU Version not corrected"
else
  echo "MCU read version:" "${verNumber}"
fi


