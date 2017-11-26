#include "soil.h"
#include "const.h"
#include "mywire.h"

#define DEVICE_ADDR 0x20

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
#define SOIL_MEASURE_CAP        0x11 // (r)	    1 bytes

uint16_t ICACHE_FLASH_ATTR soilGetCap()
{
    readI2CRegister16bit(DEVICE_ADDR, SOIL_GET_CAPACITANCE);//throw away the data
        
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
        DBG("BusyC:%d", counter);
    }

    return readI2CRegister16bit(DEVICE_ADDR, SOIL_GET_CAPACITANCE);
}

uint16_t ICACHE_FLASH_ATTR soilGetCap_CUSTOM()
{
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

void ICACHE_FLASH_ATTR soilMeasure()
{
    writeI2CRegister8bit(DEVICE_ADDR, SOIL_MEASURE_CAP);
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

uint8_t ICACHE_FLASH_ATTR soilReset()
{
    writeI2CRegister8bit(DEVICE_ADDR, SOIL_RESET);
    int i = 0;
    for(;i < 100;i++)
    {
        os_delay_us(10000);//wait 1s
    }
}