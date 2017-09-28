#include "twi.h"

static unsigned char twi_sda;
static unsigned char twi_scl;
static unsigned char twi_dcount = 18;
static uint32_t twi_clockStretchLimit;
uint8_t esp8266_gpioToFn[16] = {0x34, 0x18, 0x38, 0x14, 0x3C, 0x40, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x04, 0x08, 0x0C, 0x10};

uint8_t rxBuffer[BUFFER_LENGTH];
uint8_t rxBufferIndex = 0;
uint8_t rxBufferLength = 0;

uint8_t txAddress = 0;
uint8_t txBuffer[BUFFER_LENGTH];
uint8_t txBufferIndex = 0;
uint8_t txBufferLength = 0;

#define SDA_READ()  ((GPI & (1 << twi_sda)) != 0)
#define SCL_LOW()   (GPES = (1 << twi_scl))
#define SDA_LOW()   (GPES = (1 << twi_sda)) //Enable SDA (becomes output and since GPO is 0 for the pin, it will pull the line low)
#define SCL_READ()  ((GPI & (1 << twi_scl)) != 0)
#define SCL_HIGH()  (GPEC = (1 << twi_scl))
#define SDA_HIGH()  (GPEC = (1 << twi_sda)) //Disable SDA (becomes input and since it has pullup it will go high)

void ICACHE_FLASH_ATTR twi_init(uint8_t sda, uint8_t scl)
{
  ETS_GPIO_INTR_DISABLE();
  twi_sda = sda;
  twi_scl = scl;
  pinMode(twi_sda, INPUT_PULLUP);
  pinMode(twi_scl, INPUT_PULLUP);
  twi_setClock(100000);
  twi_setClockStretchLimit(230); // default value is 230 uS
  ETS_GPIO_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR pinMode(uint8_t pin, uint8_t mode)
{
    if(mode == INPUT || mode == INPUT_PULLUP){
      GPF(pin) = GPFFS(GPFFS_GPIO(pin));//Set mode to GPIO
      GPEC = (1 << pin); //Disable
      GPC(pin) = (GPC(pin) & (0xF << GPCI)) | (1 << GPCD); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
      if(mode == INPUT_PULLUP) {
          GPF(pin) |= (1 << GPFPU);  // Enable  Pullup
      }
    }
} 

void ICACHE_FLASH_ATTR twi_setClock(unsigned int freq)
{
    /*if(freq <= 100000)*/ twi_dcount = 32;//about 100KHz
}

void ICACHE_FLASH_ATTR twi_setClockStretchLimit(uint32_t limit)
{
     twi_clockStretchLimit = limit * TWI_CLOCK_STRETCH_MULTIPLIER;
}

void ICACHE_FLASH_ATTR  flush(){
  rxBufferIndex = 0;
  rxBufferLength = 0;
  txBufferIndex = 0;
  txBufferLength = 0;
}
