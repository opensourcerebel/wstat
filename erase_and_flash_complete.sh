#!/bin/bash
set -x
binpth=../bin
usbpth=ttyACM0
speed=115200

esptool.py --port /dev/$usbpth --baud $speed erase_flash
esptool.py --port /dev/$usbpth --baud $speed write_flash -fs 4MB -ff 40m -fm qio 0 $binpth/eagle.flash.bin 0x10000 $binpth/eagle.irom0text.bin 0x3FE000 ./blank.bin 0x3fc000 ./esp8266_init_data_setting.bin
python -m serial.tools.miniterm /dev/$usbpth 74880
