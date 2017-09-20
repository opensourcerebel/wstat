/*
    I2C driver for the ESP8266 
    Copyright (C) 2014 Rudy Hardeman (zarya) 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __I2C_H__
#define __I2C_H__

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

#define I2C_SLEEP_TIME 10 

// SDA on GPIO2
#define I2C_SDA_MUX PERIPHS_IO_MUX_GPIO4_U
#define I2C_SDA_FUNC FUNC_GPIO4
#define I2C_SDA_PIN 4

// SCK on GPIO14
//#define I2C_SCK_MUX PERIPHS_IO_MUX_MTMS_U
//#define I2C_SCK_FUNC FUNC_GPIO14
//#define I2C_SCK_PIN 14

//SCK on GPIO0 (untested)
#define I2C_SCK_MUX PERIPHS_IO_MUX_GPIO5_U
#define I2C_SCK_PIN 5
#define I2C_SCK_FUNC FUNC_GPIO5

#define i2c_read()            GPIO_INPUT_GET(GPIO_ID_PIN(I2C_SDA_PIN)) 
//#define i2c_clock_line_read() GPIO_INPUT_GET(GPIO_ID_PIN(I2C_SCK_PIN))

#define INPUT             0x00
#define INPUT_PULLUP      0x02

#define ESP8266_REG(addr) *((volatile uint32_t *)(0x60000000+(addr)))
extern uint8_t esp8266_gpioToFn[16];

#define GPF(p) ESP8266_REG(0x800 + esp8266_gpioToFn[(p & 0xF)])

#define GPFFS0 4 //Function Select bit 0
#define GPFFS1 5 //Function Select bit 1
#define GPFFS2 8 //Function Select bit 2
#define GPFFS(f) (((((f) & 4) != 0) << GPFFS2) | ((((f) & 2) != 0) << GPFFS1) | ((((f) & 1) != 0) << GPFFS0))

#define GPFFS_GPIO(p) (((p)==0||(p)==2||(p)==4||(p)==5)?0:((p)==16)?1:3)
#define GPEC   ESP8266_REG(0x314) //GPIO_ENABLE_CLR WO
#define GPC(p) ESP8266_REG(0x328 + ((p & 0xF) * 4))
#define GPCI   7  //INT_TYPE (3bits) 0:disable,1:rising,2:falling,3:change,4:low,5:high
#define GPCD   2  //DRIVER 0:normal,1:open drain

#define GPFPU  7 //Pullup

#define GPEC   ESP8266_REG(0x314) //GPIO_ENABLE_CLR WO
#define SCL_HIGH()  (GPEC = (1 << twi_scl))
#define SDA_HIGH()  (GPEC = (1 << twi_sda)) //Disable SDA (becomes input and since it has pullup it will go high)


#define GPI    ESP8266_REG(0x318) //GPIO_IN RO (Read Input Level)
#define SDA_READ()  ((GPI & (1 << twi_sda)) != 0)

#define GPES   ESP8266_REG(0x310) //GPIO_ENABLE_SET WO
#define SCL_LOW()   (GPES = (1 << twi_scl))

#define SDA_LOW()   (GPES = (1 << twi_sda)) //Enable SDA (becomes output and since GPO is 0 for the pin, it will pull the line low)
#define SCL_READ()  ((GPI & (1 << twi_scl)) != 0)

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_send_ack(uint8 state);
uint8 i2c_check_ack(void);
uint8 i2c_readByte(void);
void i2c_writeByte(uint8 data);

#endif