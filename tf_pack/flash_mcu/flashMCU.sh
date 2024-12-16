#!/bin/bash

verNumber=0
i2cStatus=""
dir="/etc/mcu_bin"
currentVersion=0
files=$(ls $dir)
filename="mcu_"i
filenameType=".bin"
pathMCU=""
echo "files in /etc/mcu_bin":  "${files}"

if [[ "$files" =~ \.bin ]]; then
    regex="mcu_([0-9]+)\.bin"
    for file in $files; do
        if [[ $file =~ $regex ]]; then
            verNumber="${BASH_REMATCH[1]}"
            logger -t FlashMCU -p INFO "Find mcu prog with version: $verNumber"
        fi
    done
fi

i2cStatus=$(/etc/init.d/i2c_daemon status)

if [[ "${i2cStatus}" != "running" ]]; then
    /etc/init.d/i2c_deamon start
fi

currentVersion=$(request_ctrl SW_VERS get)

pathMCU+="${dir}"
pathMCU+="/"
pathMCU+="${filename}"
pathMCU+="${verNumber}"
pathMCU+="${filenameType}"
echo "path MCU:" "${pathMCU}"

if [[ "${currentVersion}" -eq "${verNumber}" ]]; then
  echo "installed Actual MCU version"
  logger -t FlashMCU -p INFO "installed Actual MCU version" "${currentVersion}"
else
  /etc/init.d/i2c_daemon stop
  gpio_ctrl BOOT0 SET 1
  gpio_ctrl IOMCU_RESET RESET
  logger -t FlashMCU -p INFO "BOOT0 restarted to 1"
  echo "FLASHING MCU version" "${verNumber}"
  stm32flash -w "${pathMCU}" -a 0x3b /dev/i2c-0
  gpio_ctrl BOOT0 SET 0
  gpio_ctrl IOMCU_RESET RESET
  logger -t FlashMCU -p INFO "BOOT0 restarted to 0"
  sleep 1
  /etc/init.d/i2c_daemon start
  i2cStatus=$(/etc/init.d/i2c_daemon status)
  currentVersion=$(request_ctrl SW_VERS get)
fi

if [[ "${i2cStatus}" != "running" ]]; then
  logger -t FlashMCU -p ERROR "I2C can not run"
fi

if [[ "${currentVersion}" -ne "${verNumber}" ]]; then
 logger -t FlashMCU -p ERROR "Flashing failed, MCU Version not corrected"
 logger -t FlashMCU -p ERROR "MCU version installed:   " "${currentVersion}"
 logger -t FlashMCU -p ERROR "MCU version required:    " "${verNumber}"
else
  logger -t FlashMCU -p INFO "MCU installed version:" "${currentVersion}"
fi

