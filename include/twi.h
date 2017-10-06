
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

#define I2C_SCK_PIN 5
#define I2C_SDA_PIN 4

#define TWI_CLOCK_STRETCH_MULTIPLIER 6
#define BUFFER_LENGTH 32

#define SPECIAL           0xF8 //defaults to the usable BUSes uart0rx/tx uart1tx and hspi
#define OUTPUT            0x01
#define INPUT             0x00
#define INPUT_PULLUP      0x02
#define OUTPUT_OPEN_DRAIN 0x03
#define FUNCTION_0        0x08
#define FUNCTION_4        0x48
#define WAKEUP_PULLDOWN   0x07
#define WAKEUP_PULLUP     0x05
#define INPUT_PULLDOWN_16 0x04 // PULLDOWN only possible for pin16

#define ESP8266_REG(addr) *((volatile uint32_t *)(0x60000000+(addr)))
extern uint8_t esp8266_gpioToFn[16];

#define GPF(p) ESP8266_REG(0x800 + esp8266_gpioToFn[(p & 0xF)])

#define GPFFS0 4 //Function Select bit 0
#define GPFFS1 5 //Function Select bit 1
#define GPFFS2 8 //Function Select bit 2
#define GPFFS(f) (((((f) & 4) != 0) << GPFFS2) | ((((f) & 2) != 0) << GPFFS1) | ((((f) & 1) != 0) << GPFFS0))
#define GPFFS_BUS(p) (((p)==1||(p)==3)?0:((p)==2||(p)==12||(p)==13||(p)==14||(p)==15)?2:((p)==0)?4:1)
#define GP16FFS(f) (((f) & 0x03) | (((f) & 0x04) << 4))

#define GPFFS_GPIO(p) (((p)==0||(p)==2||(p)==4||(p)==5)?0:((p)==16)?1:3)
#define GPEC   ESP8266_REG(0x314) //GPIO_ENABLE_CLR WO
#define GPC(p) ESP8266_REG(0x328 + ((p & 0xF) * 4))
#define GPCI   7  //INT_TYPE (3bits) 0:disable,1:rising,2:falling,3:change,4:low,5:high
#define GPCD   2  //DRIVER 0:normal,1:open drain

#define GPFPU  7 //Pullup
#define GPFPD  6 //Pulldown

//GPIO 16 PIN Function Register
#define GP16F  ESP8266_REG(0x7A0)
#define GPF16  GP16F

//GPIO 16 PIN Control Register
#define GP16C  ESP8266_REG(0x790)
#define GPC16  GP16C

#define GP16FPD  3 //Pulldown

//GPIO 16 Control Registers
#define GP16O  ESP8266_REG(0x768)
#define GP16E  ESP8266_REG(0x774)
#define GP16I  ESP8266_REG(0x78C)

#define GPCWE  10 //WAKEUP_ENABLE (can be 1 only when INT_TYPE is high or low)

#define GPEC   ESP8266_REG(0x314) //GPIO_ENABLE_CLR WO

#define GPI    ESP8266_REG(0x318) //GPIO_IN RO (Read Input Level)

#define GPES   ESP8266_REG(0x310) //GPIO_ENABLE_SET WO

void ICACHE_FLASH_ATTR twi_init(uint8_t sda, uint8_t scl);
void ICACHE_FLASH_ATTR pinMode(uint8_t pin, uint8_t mode);
void ICACHE_FLASH_ATTR twi_setClock(unsigned int freq);
void ICACHE_FLASH_ATTR twi_setClockStretchLimit(uint32_t limit);
void ICACHE_FLASH_ATTR beginTransmission(uint8_t address);
size_t ICACHE_FLASH_ATTR write(uint8_t data);
uint8_t ICACHE_FLASH_ATTR endTransmission();
uint8_t ICACHE_FLASH_ATTR endTransmission1(uint8_t sendStop);
unsigned char ICACHE_FLASH_ATTR twi_writeTo(unsigned char address, unsigned char * buf, unsigned int len, unsigned char sendStop);
void ICACHE_FLASH_ATTR twi_delay(unsigned char v);
bool ICACHE_FLASH_ATTR twi_write_start(void);
bool ICACHE_FLASH_ATTR twi_write_stop(void);
bool ICACHE_FLASH_ATTR twi_write_byte(unsigned char byte);
unsigned char ICACHE_FLASH_ATTR twi_readFrom(unsigned char address, unsigned char* buf, unsigned int len, unsigned char sendStop);
uint8_t ICACHE_FLASH_ATTR requestFrom(uint8_t address, uint8_t quantity);
size_t ICACHE_FLASH_ATTR requestFrom1(uint8_t address, size_t size, bool sendStop);
int ICACHE_FLASH_ATTR readT(void);