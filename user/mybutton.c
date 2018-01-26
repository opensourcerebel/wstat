#include "mybutton.h"
#include "const.h"
#include "mywire.h"

#define DEVICE_ADDR 0x41

#define GET_BUTTON 	0x01
#define ARM_AND_SLEEP 0x02
#define SLEEP 0x03

uint16_t ICACHE_FLASH_ATTR buttonGet()
{
    return readI2CRegister8bit(DEVICE_ADDR, GET_BUTTON);
}

void ICACHE_FLASH_ATTR buttonSleep()
{
    writeI2CRegister8bit(DEVICE_ADDR, SLEEP);
}

void ICACHE_FLASH_ATTR buttonArmAndSleep()
{
	writeI2CRegister8bit(DEVICE_ADDR, ARM_AND_SLEEP);
}
