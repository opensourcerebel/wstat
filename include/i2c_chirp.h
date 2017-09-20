
#ifndef __I2C_CHIRP_H
#define	__I2C_CHIRP_H

#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"

//0x20 = 0b100000 address
//0x40 = 0b1000000 write
//0x41 = 0b1000001 read

#define CHIRP_W					0x40
#define CHIRP_R					0x41

#define CHIRP_DEBUG 1 //uncomment for debugging messages

uint16_t CHIRP_GetLightRaw(void);
uint16_t CHIRP_GetPressureRaw(void);
uint16_t CHIRP_GetHumidityRaw(void);
bool CHIRP_Sleep(void);
bool ICACHE_FLASH_ATTR isBusy(void);

#endif