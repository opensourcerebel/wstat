#!/bin/bash
make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE_MAP=$spi_size_map
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
