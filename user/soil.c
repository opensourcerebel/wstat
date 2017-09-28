#include "soil.h"


#define DEVICE_ADDR 0x01

 //Soil Moisture Sensor Register Addresses
#define SOIL_GET_CAPACITANCE 	0x00 // (r) 	2 bytes
#define SOIL_SET_ADDRESS 	0x01 //	(w) 	1 byte
#define SOIL_GET_ADDRESS 	0x02 // (r) 	1 byte
#define SOIL_MEASURE_LIGHT 	0x03 //	(w) 	n/a
#define SOIL_GET_LIGHT 		0x04 //	(r) 	2 bytes
#define SOIL_GET_TEMPERATURE	0x05 //	(r) 	2 bytes
#define SOIL_RESET 		0x06 //	(w) 	n/a
#define SOIL_GET_VERSION 	0x07 //	(r) 	1 bytes
#define SOIL_SLEEP	        0x08 // (w)     n/a
#define SOIL_GET_BUSY	        0x09 // (r)	    1 bytes

uint16_t ICACHE_FLASH_ATTR soilGetCap()
{
    readI2CRegister16bit(DEVICE_ADDR, SOIL_GET_CAPACITANCE);
        
    int status = 1;
    int counter = 0;
    do
    {
        status = readI2CRegister8bit(DEVICE_ADDR, SOIL_GET_BUSY);
        os_delay_us(1000);
        counter++;
    } while (status != 0);

    if (counter > 1)
    {
        //DBG("BusyC:%d", counter);
    }

    return readI2CRegister16bit(DEVICE_ADDR, SOIL_GET_CAPACITANCE);
}

uint16_t ICACHE_FLASH_ATTR soilGetTemp()
{
   return readI2CRegister16bit(DEVICE_ADDR, SOIL_GET_TEMPERATURE);
}

void ICACHE_FLASH_ATTR soilSleep()
{
    writeI2CRegister8bit(DEVICE_ADDR, SOIL_SLEEP); //sleep
}

bool ICACHE_FLASH_ATTR soilCheck()
{
    uint8_t version = soilGetVersion();
    if(version == 0x23)
    {
        return 1;
    }
    
    return 0;
}

uint8_t ICACHE_FLASH_ATTR soilGetVersion()
{
    return readI2CRegister8bit(DEVICE_ADDR, SOIL_GET_VERSION);
}