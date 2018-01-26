#include "twi.h"
#include "const.h"

void ICACHE_FLASH_ATTR setupWire()
{
    twi_init(I2C_SDA_PIN, I2C_SCK_PIN);
}

void ICACHE_FLASH_ATTR writeI2CRegister8bit(int addr, int value) {
  beginTransmission(addr);
  writeT(value);
  int stat = endTransmission();
  if (stat != 0)
  {
    DBG("!!!writeStatE:%d, %d, %d\r\n", addr, value, stat);
  }
}

uint8_t readI2CRegister8bit(int addr, int reg) {
  beginTransmission(addr);
  writeT(reg);
  int stat = endTransmission();

  if (stat != 0)
  {
    DBG("!!!readE:%d, %d, %d\r\n", addr, reg, stat);
  }

  requestFrom(addr, 1);
  return readT();
}

unsigned int ICACHE_FLASH_ATTR readI2CRegister16bit(int addr, int reg) {
  beginTransmission(addr);
  writeT(reg);
  int stat = endTransmission();

  if (stat != 0)
  {
    DBG("!!!read16StatE:%d, %d, %d\r\n", addr, reg, stat);
  }

  requestFrom(addr, 2);
  unsigned int t = readT() << 8;
  t = t | readT();
  return t;
}

 
