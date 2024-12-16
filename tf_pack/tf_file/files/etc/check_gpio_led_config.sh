#!/bin/bash

#network config for tfortis PSW ver 2.1
#instaling from ipk

echo "*********** Test GPIO and LED configuration *************"
echo "gpio reg. value = 0x8"
echo $(devmem 0x1b001000)

echo "LED_GLB_CTRL reg. value  = 0x2F391201"
echo $(devmem 0x1b00a000)

echo "LED_MODE_SEL reg. value  = 0xC"
echo $(devmem 0x1b001004)

echo "LED_MODE_CTRL reg. value = 0x0"
echo $(devmem 0x1b00a004)

echo "LED_P_EN_CTR reg. value  = 0x0500FF00"
echo $(devmem 0x1b00a008)

