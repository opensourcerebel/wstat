#!/bin/bash
set -x
binpth=../bin
usbpth=ttyUSB0
speed=230400
esptool.py --port /dev/$usbpth --baud $speed write_flash -fs 4MB -ff 40m -fm dio 0 $binpth/eagle.flash.bin 0x10000 $binpth/eagle.irom0text.bin 
python -m serial.tools.miniterm /dev/$usbpth 74880
