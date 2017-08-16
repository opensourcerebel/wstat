#!/bin/bash
set -x
./compile.sh
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
./flash.sh
