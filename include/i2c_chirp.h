
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
#define CHIRP_CHIP_ID_REG			0xD0
#define CHIRP_CHIP_ID				0x60

#define CHIRP_REG_CTRL_HUM			0xF2
#define CHIRP_REG_CTRL_MEAS		0xF4
#define CHIRP_REG_CONFIG			0xF5

#define CHIRP_MODE_NORMAL			0x03 //reads sensors at set interval
#define CHIRP_MODE_FORCED			0x01 //reads sensors once when you write this register

#define CHIRP_DEBUG 1 //uncomment for debugging messages

bool CHIRP_Init(uint8_t operationMode);
bool ICACHE_FLASH_ATTR CHIRP_startI2cWrite(void);
bool ICACHE_FLASH_ATTR CHIRP_sendI2cWriteData(uint8_t writeReg, uint8_t regData);
bool ICACHE_FLASH_ATTR CHIRP_sendI2cWriteData1(uint8_t writeReg);
bool ICACHE_FLASH_ATTR CHIRP_sendI2cRead(uint8_t readReg);
bool ICACHE_FLASH_ATTR CHIRP_sendI2cReadSensorData();
bool CHIRP_verifyChipId(void);
void CHIRP_writeConfigRegisters(void);
void CHIRP_readCalibrationRegisters(void);

signed long int CHIRP_calibration_T(signed long int adc_T);
unsigned long int CHIRP_calibration_P(signed long int adc_P);
unsigned long int CHIRP_calibration_H(signed long int adc_H);

void CHIRP_readSensorData(void);

unsigned long int CHIRP_GetTemperatureRaw(void);
unsigned long int CHIRP_GetPressureRaw(void);
unsigned long int CHIRP_GetHumidityRaw(void);

signed long int CHIRP_GetTemperature(void);
unsigned long int CHIRP_GetPressure(void);
unsigned long int CHIRP_GetHumidity(void);

signed long int CHIRP_calibration_Temp(signed long int adc_T);
unsigned long int CHIRP_calibration_Press(signed long int adc_P);
unsigned long int CHIRP_calibration_Hum(signed long int adc_H);

#endif