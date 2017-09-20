#include <math.h>

#include "i2c_chirp.h"
#include "i2c.h"
 
#define SOILMOISTURESENSOR_DEFAULT_ADDR 0x20
 
 //Soil Moisture Sensor Register Addresses
#define SOILMOISTURESENSOR_GET_CAPACITANCE 	0x00 // (r) 	2 bytes
#define SOILMOISTURESENSOR_SET_ADDRESS 		0x01 //	(w) 	1 byte
#define SOILMOISTURESENSOR_GET_ADDRESS 		0x02 // (r) 	1 byte
#define SOILMOISTURESENSOR_MEASURE_LIGHT 	0x03 //	(w) 	n/a
#define SOILMOISTURESENSOR_GET_LIGHT 		0x04 //	(r) 	2 bytes
#define SOILMOISTURESENSOR_GET_TEMPERATURE	0x05 //	(r) 	2 bytes
#define SOILMOISTURESENSOR_RESET 		0x06 //	(w) 	n/a
#define SOILMOISTURESENSOR_GET_VERSION 		0x07 //	(r) 	1 bytes
#define SOILMOISTURESENSOR_SLEEP	        0x08 // (w)     n/a
#define SOILMOISTURESENSOR_GET_BUSY	        0x09 // (r)	    1 bytes

bool ICACHE_FLASH_ATTR CHIRP_startI2cWrite(){
	i2c_start();
	i2c_writeByte(CHIRP_W);
	if(!i2c_check_ack()){
		#ifdef CHIRP_DEBUG
		os_printf("++5WriteACK!\r\n");
		#endif
		i2c_stop();
		return 0;
	}
	return 1;
}

bool ICACHE_FLASH_ATTR CHIRP_sendI2cWriteData(uint8_t writeReg, uint8_t regData){
    if(!CHIRP_startI2cWrite() ){
    	return 0;
    }
	i2c_writeByte(writeReg);
	if(!i2c_check_ack()){
		#ifdef CHIRP_DEBUG
		os_printf("1CHIRP_sendI2cWriteData: i2c_writeByte(%X) slave not ack..\r\n", writeReg);
		#endif
		i2c_stop();
		return 0;
	}
	i2c_writeByte(regData);
	if(!i2c_check_ack()){
		#ifdef CHIRP_DEBUG
		os_printf("2CHIRP_sendI2cWriteData: i2c_writeByte(%X) slave not ack..\r\n", regData);
		#endif
		i2c_stop();
		return 0;
	}
	i2c_stop();
	return 1;
}

bool ICACHE_FLASH_ATTR CHIRP_sendI2cWriteData1(uint8_t writeReg){
    if(!CHIRP_startI2cWrite() ){
    	return 0;
    }
	i2c_writeByte(writeReg);
	if(!i2c_check_ack()){
		#ifdef CHIRP_DEBUG
		os_printf("++2Content NACK\r\n", writeReg);
		#endif
		i2c_stop();
		return 0;
	}
	
	i2c_stop();
	return 1;
}

bool ICACHE_FLASH_ATTR CHIRP_sendI2cRead(uint8_t readReg){

	i2c_start();
	i2c_writeByte(CHIRP_W);//will be writing to this i2c address
	if(!i2c_check_ack()){
		#ifdef CHIRP_DEBUG
		os_printf("++1!\r\n");
		#endif
		i2c_stop();
		return 0;
	}

	i2c_writeByte(readReg);//say the address
	if(!i2c_check_ack()){
		#ifdef CHIRP_DEBUG
		os_printf("++2!\r\n");
		#endif
		i2c_stop();
		return 0;
	}
	i2c_start();//restart the bus
	i2c_writeByte(CHIRP_R);//will be reading from this i2c address
	if(!i2c_check_ack()){
		#ifdef CHIRP_DEBUG
		os_printf("++3!\r\n");
		#endif
		i2c_stop();
		return 1;
	}

}

bool ICACHE_FLASH_ATTR printVersion()
{
    #ifdef CHIRP_DEBUG
    os_printf("+++CHRIP_RESET\r\n");
    #endif    
    CHIRP_sendI2cWriteData1(SOILMOISTURESENSOR_RESET);
    int i = 0;
    for(;i < 100;i++)
    {
        os_delay_us(10000);//wait 1s
    }
    while (isBusy()) os_delay_us(1000);
  
    #ifdef CHIRP_DEBUG
    os_printf("+++CHIRP_GETVERSION\r\n");
    #endif
    CHIRP_sendI2cRead(SOILMOISTURESENSOR_GET_VERSION);
    uint8_t version = i2c_readByte();
    i2c_send_ack(0);
    i2c_stop();
    #ifdef CHIRP_DEBUG
    os_printf("CHIRP: Version 0x%X\r\n", version);
    #endif
    if(version == 0xFF)
    {
        return 0;
    }
    
    return 1;
}

bool ICACHE_FLASH_ATTR isBusy()
{
    #ifdef CHIRP_DEBUG
    os_printf("+++CHIRP_isBusy\r\n");
    #endif    
    CHIRP_sendI2cRead(SOILMOISTURESENSOR_GET_BUSY);
    uint8_t busy = i2c_readByte();
    i2c_send_ack(0);
    i2c_stop();
    return busy;
}

bool ICACHE_FLASH_ATTR CHIRP_Init()
{
    i2c_init();
    return printVersion();
}

void ICACHE_FLASH_ATTR CHIRP_InitFromSleep()
{
    i2c_init();
    printVersion();
}


bool ICACHE_FLASH_ATTR CHIRP_Sleep()
{
#ifdef CHIRP_DEBUG    
        os_printf("CHIRP_Sleep\r\n");
#endif    
    CHIRP_sendI2cWriteData1(SOILMOISTURESENSOR_SLEEP);
}

uint16_t ICACHE_FLASH_ATTR CHIRP_GetTemperatureRaw(){
#ifdef CHIRP_DEBUG    
        os_printf("CHIRP_GetTemperatureRaw\r\n");
#endif
        CHIRP_sendI2cRead(SOILMOISTURESENSOR_GET_TEMPERATURE);
	uint8_t b1 = i2c_readByte();
        uint16_t t = b1 << 8;
        b1 = i2c_readByte();
        t = t | b1;
        
	i2c_send_ack(0);
	i2c_stop();
        
	return t;
}

uint16_t ICACHE_FLASH_ATTR CHIRP_GetLightRaw(){
#ifdef CHIRP_DEBUG    
        os_printf("CHIRP_GetLightRaw\r\n");
#endif     
        CHIRP_sendI2cWriteData1(SOILMOISTURESENSOR_MEASURE_LIGHT);
        while (isBusy()) os_delay_us(1000);
        
	CHIRP_sendI2cRead(SOILMOISTURESENSOR_MEASURE_LIGHT);

	uint16_t t = (i2c_readByte() << 8) | i2c_readByte();
        
	i2c_send_ack(0);
	i2c_stop();

	return t;    
	
}

uint16_t ICACHE_FLASH_ATTR CHIRP_GetHumidityRaw(){
#ifdef CHIRP_DEBUG    
        os_printf("CHIRP_GetHumidityRaw\r\n");
#endif        
        while (isBusy()) os_delay_us(1000);
	CHIRP_sendI2cRead(SOILMOISTURESENSOR_GET_CAPACITANCE);

        uint16_t t = (i2c_readByte() << 8) | i2c_readByte();
        
	i2c_send_ack(0);
	i2c_stop();

	return t;
}