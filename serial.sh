#!/bin/bash
set -x
python -m serial.tools.miniterm /dev/ttyACM0 74880
