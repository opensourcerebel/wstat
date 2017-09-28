#ifndef __I2C_SOIL_H
#define	__I2C_SOIL_H

#include "osapi.h"

uint16_t ICACHE_FLASH_ATTR soilGetCap();
uint16_t ICACHE_FLASH_ATTR soilGetTemp();
void ICACHE_FLASH_ATTR soilSleep();
bool ICACHE_FLASH_ATTR soilCheck();
uint8_t ICACHE_FLASH_ATTR soilGetVersion();

#endif