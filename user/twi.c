#include "twi.h"

static unsigned char twi_sda;
static unsigned char twi_scl;
static unsigned char twi_dcount = 18;
static uint32_t twi_clockStretchLimit;
uint8_t esp8266_gpioToFn[16] = {0x34, 0x18, 0x38, 0x14, 0x3C, 0x40, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x04, 0x08, 0x0C, 0x10};

uint8_t transmitting = 0;
uint8_t rxBuffer[BUFFER_LENGTH];
uint8_t rxBufferIndex = 0;
uint8_t rxBufferLength = 0;

uint8_t txAddress = 0;
uint8_t txBuffer[BUFFER_LENGTH];
uint8_t txBufferIndex = 0;
uint8_t txBufferLength = 0;

#define SDA_READ()  ((GPI & (1 << twi_sda)) != 0)
#define SCL_READ()  ((GPI & (1 << twi_scl)) != 0)
#define SCL_LOW()   (GPES = (1 << twi_scl))
#define SDA_LOW()   (GPES = (1 << twi_sda)) //Enable SDA (becomes output and since GPO is 0 for the pin, it will pull the line low)

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
  twi_setClockStretchLimit(9000); // default value is 230 uS
  ETS_GPIO_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR pinMode(uint8_t pin, uint8_t mode)
{
    //pwm_stop_pin(pin);
    if(pin < 16){
        if(mode == SPECIAL){
        GPC(pin) = (GPC(pin) & (0xF << GPCI)); //SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
        GPEC = (1 << pin); //Disable
        GPF(pin) = GPFFS(GPFFS_BUS(pin));//Set mode to BUS (RX0, TX0, TX1, SPI, HSPI or CLK depending in the pin)
        if(pin == 3) GPF(pin) |= (1 << GPFPU);//enable pullup on RX
        } else if(mode & FUNCTION_0){
        GPC(pin) = (GPC(pin) & (0xF << GPCI)); //SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
        GPEC = (1 << pin); //Disable
        GPF(pin) = GPFFS((mode >> 4) & 0x07);
        if(pin == 13 && mode == FUNCTION_4) GPF(pin) |= (1 << GPFPU);//enable pullup on RX
        }  else if(mode == OUTPUT || mode == OUTPUT_OPEN_DRAIN){
        GPF(pin) = GPFFS(GPFFS_GPIO(pin));//Set mode to GPIO
        GPC(pin) = (GPC(pin) & (0xF << GPCI)); //SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
        if(mode == OUTPUT_OPEN_DRAIN) GPC(pin) |= (1 << GPCD);
        GPES = (1 << pin); //Enable
        } else if(mode == INPUT || mode == INPUT_PULLUP){
        GPF(pin) = GPFFS(GPFFS_GPIO(pin));//Set mode to GPIO
        GPEC = (1 << pin); //Disable
        GPC(pin) = (GPC(pin) & (0xF << GPCI)) | (1 << GPCD); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
        if(mode == INPUT_PULLUP) {
            GPF(pin) |= (1 << GPFPU);  // Enable  Pullup
        }
        } else if(mode == WAKEUP_PULLUP || mode == WAKEUP_PULLDOWN){
        GPF(pin) = GPFFS(GPFFS_GPIO(pin));//Set mode to GPIO
        GPEC = (1 << pin); //Disable
        if(mode == WAKEUP_PULLUP) {
            GPF(pin) |= (1 << GPFPU);  // Enable  Pullup
            GPC(pin) = (1 << GPCD) | (4 << GPCI) | (1 << GPCWE); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(LOW) | WAKEUP_ENABLE(ENABLED)
        } else {
            GPF(pin) |= (1 << GPFPD);  // Enable  Pulldown
            GPC(pin) = (1 << GPCD) | (5 << GPCI) | (1 << GPCWE); //SOURCE(GPIO) | DRIVER(OPEN_DRAIN) | INT_TYPE(HIGH) | WAKEUP_ENABLE(ENABLED)
        }
        }
    } else if(pin == 16){
        GPF16 = GP16FFS(GPFFS_GPIO(pin));//Set mode to GPIO
        GPC16 = 0;
        if(mode == INPUT || mode == INPUT_PULLDOWN_16){
        if(mode == INPUT_PULLDOWN_16){
            GPF16 |= (1 << GP16FPD);//Enable Pulldown
        }
        GP16E &= ~1;
        } else if(mode == OUTPUT){
        GP16E |= 1;
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

void ICACHE_FLASH_ATTR beginTransmission(uint8_t address)
{
  transmitting = 1;
  txAddress = address;
  txBufferIndex = 0;
  txBufferLength = 0;
}

size_t ICACHE_FLASH_ATTR writeT(uint8_t data)
{
  if(transmitting){
    if(txBufferLength >= BUFFER_LENGTH){
      //setWriteError();
      return 0;
    }
    txBuffer[txBufferIndex] = data;
    ++txBufferIndex;
    txBufferLength = txBufferIndex;
  } else {
    // i2c_slave_transmit(&data, 1);
  }
  return 1;
}

uint8_t ICACHE_FLASH_ATTR endTransmission(void){
  return endTransmission1(true);
}

uint8_t ICACHE_FLASH_ATTR endTransmission1(uint8_t sendStop)
{
  int8_t ret = twi_writeTo(txAddress, txBuffer, txBufferLength, sendStop);
  txBufferIndex = 0;
  txBufferLength = 0;
  transmitting = 0;
  return ret;
}

unsigned char ICACHE_FLASH_ATTR twi_writeTo(unsigned char address, unsigned char * buf, unsigned int len, unsigned char sendStop)
{
  unsigned int i;
  if(!twi_write_start()) return 4;//line busy
  if(!twi_write_byte(((address << 1) | 0) & 0xFF)) {
    if (sendStop) twi_write_stop();
    return 2; //received NACK on transmit of address
  }
  for(i=0; i<len; i++) {
    if(!twi_write_byte(buf[i])) {
      if (sendStop) twi_write_stop();
      return 3;//received NACK on transmit of data
    }
  }
  if(sendStop) twi_write_stop();
  i = 0;
  while(SDA_READ() == 0 && (i++) < 10){
    SCL_LOW();
    twi_delay(twi_dcount);
    SCL_HIGH();
    twi_delay(twi_dcount);
  }
  return 0;
}

void ICACHE_FLASH_ATTR twi_delay(unsigned char v){
  unsigned int i;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  unsigned int reg;
  for(i=0;i<v;i++) reg = GPI;
#pragma GCC diagnostic pop
}

bool ICACHE_FLASH_ATTR twi_write_start(void) {
  SCL_HIGH();
  SDA_HIGH();
  if (SDA_READ() == 0) return false;
  twi_delay(twi_dcount);
  SDA_LOW();
  twi_delay(twi_dcount);
  return true;
}

bool ICACHE_FLASH_ATTR twi_write_stop(void){
  uint32_t i = 0;
  SCL_LOW();
  SDA_LOW();
  twi_delay(twi_dcount);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit); // Clock stretching
  twi_delay(twi_dcount);
  SDA_HIGH();
  twi_delay(twi_dcount);

  return true;
}

bool ICACHE_FLASH_ATTR twi_write_bit(bool bit) {
  uint32_t i = 0;
  SCL_LOW();
  if (bit) SDA_HIGH();
  else SDA_LOW();
  twi_delay(twi_dcount+1);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);// Clock stretching
  twi_delay(twi_dcount);
  return true;
}

bool ICACHE_FLASH_ATTR twi_read_bit(void) {
  uint32_t i = 0;
  SCL_LOW();
  SDA_HIGH();
  twi_delay(twi_dcount+2);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);// Clock stretching
  bool bit = SDA_READ();
  twi_delay(twi_dcount);
  return bit;
}

bool ICACHE_FLASH_ATTR twi_write_byte(unsigned char byte) {
  unsigned char bit;
  for (bit = 0; bit < 8; bit++) {
    twi_write_bit(byte & 0x80);
    byte <<= 1;
  }
  return !twi_read_bit();//NACK/ACK
}

unsigned char ICACHE_FLASH_ATTR twi_read_byte(bool nack) {
  unsigned char byte = 0;
  unsigned char bit;
  for (bit = 0; bit < 8; bit++) byte = (byte << 1) | twi_read_bit();
  twi_write_bit(nack);
  return byte;
}

size_t ICACHE_FLASH_ATTR requestFrom1(uint8_t address, size_t size, bool sendStop){
  if(size > BUFFER_LENGTH){
    size = BUFFER_LENGTH;
  }
  size_t read = (twi_readFrom(address, rxBuffer, size, sendStop) == 0)?size:0;
  rxBufferIndex = 0;
  rxBufferLength = read;
  return read;
}

uint8_t ICACHE_FLASH_ATTR requestFrom(uint8_t address, uint8_t quantity){
  return requestFrom1(address, quantity, true);
}

unsigned char ICACHE_FLASH_ATTR twi_readFrom(unsigned char address, unsigned char* buf, unsigned int len, unsigned char sendStop){
  unsigned int i;
  if(!twi_write_start()) return 4;//line busy
  if(!twi_write_byte(((address << 1) | 1) & 0xFF)) {
    if (sendStop) twi_write_stop();
    return 2;//received NACK on transmit of address
  }
  for(i=0; i<(len-1); i++) buf[i] = twi_read_byte(false);
  buf[len-1] = twi_read_byte(true);
  if(sendStop) twi_write_stop();
  i = 0;
  while(SDA_READ() == 0 && (i++) < 10){
    SCL_LOW();
    twi_delay(twi_dcount);
    SCL_HIGH();
    twi_delay(twi_dcount);
  }
  return 0;
}

int ICACHE_FLASH_ATTR readT(void){
  int value = -1;
  if(rxBufferIndex < rxBufferLength){
    value = rxBuffer[rxBufferIndex];
    ++rxBufferIndex;
  }
  return value;
}
