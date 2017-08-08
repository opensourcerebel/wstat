#!/bin/bash
set -x
binpth=../bin
esptool.py --baud 230400 write_flash -fs 4MB -ff 40m -fm qio 0 $binpth/eagle.flash.bin 0x10000 $binpth/eagle.irom0text.bin 
python -m serial.tools.miniterm /dev/ttyUSB0 74880
