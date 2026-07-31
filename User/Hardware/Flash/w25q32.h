
#ifndef W25Q32_H_
#define W25Q32_H_

#include <stdint.h>

#define FLASH_READ_STATUS_REG  0x05
#define FLASH_WRITE_ENABLE     0x06
#define FLASH_WRITE_DISABLE    0x04
#define FLASH_PAGE_PROGRAM     0x02
#define FLASH_READ_DATA        0x03
#define FLASH_SECTOR_ERASE     0x20
#define FLASH_CHIP_ERASE       0xC7

uint8_t Flash_ReadStatusReg(void);
void Flash_Write_Enable(void);
void Flash_Write_Disable(void);
void W_Flash_Write_Byte(uint32_t addr, uint8_t data);
uint8_t W_Flash_Read_Byte(uint32_t addr);
void W_Flash_Sector_Erase(uint32_t sector_address);
void W_Flash_Page_Erase(uint16_t page);
void W_Flash_Chip_Erase(void);
void W_Flash_Write_Arr(uint32_t address, const uint8_t *data, uint16_t length);
void W_Flash_Write_Int(uint32_t address, uint16_t value);
void W_Flash_Write_Long(uint32_t address, uint32_t value);
void W_Flash_Read_Arr(uint32_t address, uint8_t *buffer, uint16_t length);
uint16_t W_Flash_Read_Int(uint32_t address);
uint32_t W_Flash_Read_Long(uint32_t address);

void W_Flash_Safe_Write_Int(uint32_t address, uint16_t value);
void W_Flash_Safe_Write_Long(uint32_t address, uint32_t value);

#endif
