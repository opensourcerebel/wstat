#!/bin/bash
set -x
binpth=../bin
usbpth=ttyUSB0
speed=921600

esptool.py --port /dev/$usbpth --baud $speed erase_flash
