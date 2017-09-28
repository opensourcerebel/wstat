#include "twi.h"

void ICACHE_FLASH_ATTR setup()
{
    twi_init(I2C_SDA_PIN, I2C_SCK_PIN);
}

 
