#!/bin/bash
set -x
binpth=../bin
usbpth=ttyACM0
speed=115200
esptool.py --port /dev/$usbpth --baud $speed write_flash -fs 4MB -ff 40m -fm qio 0 $binpth/eagle.flash.bin 0x10000 $binpth/eagle.irom0text.bin 
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
python -m serial.tools.miniterm /dev/$usbpth 74880
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
