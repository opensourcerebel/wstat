#include "twi.h"

void ICACHE_FLASH_ATTR setup()
{
    twi_init(I2C_SDA_PIN, I2C_SCK_PIN);
}

void writeI2CRegister8bit(int addr, int value) {
  beginTransmission(addr);
  writeT(value);
  int stat = endTransmission();
  if (stat != 0)
  {
    //Serial.printf("write:%d, %d, %d\r\n", addr, value, stat);
  }
}

unsigned int readI2CRegister16bit(int addr, int reg) {
  beginTransmission(addr);
  writeT(reg);
  int stat = endTransmission();

  if (stat != 0)
  {
    //Serial.printf("read2:%d, %d, %d\r\n", addr, reg, stat);
  }

  requestFrom(addr, 2);
  unsigned int t = readT() << 8;
  t = t | readT();
  return t;
}

 
