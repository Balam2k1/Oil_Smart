
#include "w25q32.h"
#include "spi.h"
#include <gpio.h>

uint8_t Flash_ReadStatusReg(void) {
    uint8_t txData[1] = { FLASH_READ_STATUS_REG };
    uint8_t rxData[1];

    spi_cs_low();
    SPI_Software_Transmit_Receive(txData, rxData, 1);
    SPI_Software_Transmit_Receive(NULL, rxData, 1);
    spi_cs_high();
    return rxData[0];
}

void Flash_Write_Enable(void) {
    uint8_t txData[1] = { FLASH_WRITE_ENABLE };

    spi_cs_low();
    SPI_Software_Transmit_Receive(txData, NULL, 1);
    spi_cs_high();
    uint32_t timeout = 50000;
    while (!(Flash_ReadStatusReg() & 0x02) && --timeout);
}

void Flash_Write_Disable(void) {
    uint8_t txData[1] = { FLASH_WRITE_DISABLE };

    spi_cs_low();
    SPI_Software_Transmit_Receive(txData, NULL, 1);
    spi_cs_high();
}

void W_Flash_Write_Byte(uint32_t addr, uint8_t data) {
    uint8_t txData[5] = { FLASH_PAGE_PROGRAM, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF, data };

    Flash_Write_Enable();
    spi_cs_low();
    SPI_Software_Transmit_Receive(txData, NULL, 5);
    spi_cs_high();
    uint32_t timeout = 10; /* Max 3ms for Page Program, wait up to 10ms */
    while ((Flash_ReadStatusReg() & 0x01) && --timeout) {
        HAL_Delay(1);
    }
    Flash_Write_Disable();
}

uint8_t W_Flash_Read_Byte(uint32_t addr) {
    uint8_t txData[4] = { FLASH_READ_DATA, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF };
    uint8_t rxData[1];

    spi_cs_low();
    SPI_Software_Transmit_Receive(txData, NULL, 4);
    SPI_Software_Transmit_Receive(NULL, rxData, 1);
    spi_cs_high();
    return rxData[0];
}

void W_Flash_Sector_Erase(uint32_t sector_address) {
    uint8_t txData[4] = { FLASH_SECTOR_ERASE, (sector_address >> 16) & 0xFF, (sector_address >> 8) & 0xFF, sector_address & 0xFF };

    Flash_Write_Enable();
    spi_cs_low();
    SPI_Software_Transmit_Receive(txData, NULL, 4);
    spi_cs_high();
    uint32_t timeout = 500; /* Max 400ms for Sector Erase */
    while ((Flash_ReadStatusReg() & 0x01) && --timeout) {
        HAL_Delay(1);
    }
}
void W_Flash_Page_Erase(uint16_t page)
{
	W_Flash_Sector_Erase(4096 * page);
}
void W_Flash_Chip_Erase(void) {
    uint8_t txData[1] = { FLASH_CHIP_ERASE };
    Flash_Write_Enable();
    spi_cs_low();
    SPI_Software_Transmit_Receive(txData, NULL, 1);
    spi_cs_high();
    uint32_t timeout = 20000; /* Max 20s for Chip Erase */
    while ((Flash_ReadStatusReg() & 0x01) && --timeout) {
        HAL_Delay(1);
    }
}

void W_Flash_Write_Arr(uint32_t address, const uint8_t *data, uint16_t length) {
	 uint16_t i;
	 for (i=0; i<length; i++)
	 {
         /* Tối ưu: Bỏ qua không ghi nếu data là 0xFF (vì flash sau khi xóa mặc định là 0xFF) */
         if (data[i] != 0xFF) {
		     W_Flash_Write_Byte(address+i, data[i]);
         }
	 }
}

void W_Flash_Write_Int(uint32_t address, uint16_t value)
{
	 uint8_t temp;
	 temp=(uint8_t)(value&0x00FF);
	 W_Flash_Write_Byte(address+1,temp);
	 value=value>>8;
	 temp=(uint8_t)(value&0x00FF);
	 W_Flash_Write_Byte(address,temp);
}

void W_Flash_Write_Long(uint32_t address, uint32_t value)
{
	 uint8_t temp;
	 temp=(uint8_t)(value&0x000000FF);
	 W_Flash_Write_Byte(address+3,temp);
	 value=value>>8;
	 temp=(uint8_t)(value&0x000000FF);
	 W_Flash_Write_Byte(address+2,temp);
	 value=value>>8;
	 temp=(uint8_t)(value&0x000000FF);
	 W_Flash_Write_Byte(address+1,temp);
	 value=value>>8;
	 temp=(uint8_t)(value&0x000000FF);
	 W_Flash_Write_Byte(address,temp);
}

void W_Flash_Read_Arr(uint32_t address, uint8_t *buffer, uint16_t length)
{
	 uint16_t i;
	 for (i=0; i<length; i++)
	 {
		 buffer[i] = W_Flash_Read_Byte(address+i);
	 }
}

uint16_t W_Flash_Read_Int(uint32_t address)
{
	uint8_t buf[2]={0};
	uint16_t value =0;
	buf[0] = W_Flash_Read_Byte(address);
	buf[1] = W_Flash_Read_Byte(address+1);
	value+=buf[0];value=value<<8;
	value+=buf[1];;
	return value;
}

uint32_t W_Flash_Read_Long(uint32_t address)
{
	uint8_t buf[4]={0};
	uint32_t value =0;
	buf[0] = W_Flash_Read_Byte(address);
	buf[1] = W_Flash_Read_Byte(address+1);
	buf[2] = W_Flash_Read_Byte(address+2);
	buf[3] = W_Flash_Read_Byte(address+3);

	value+=buf[0];value=value<<8;
	value+=buf[1];value=value<<8;
	value+=buf[2];value=value<<8;
	value+=buf[3];
	return value;
}

uint8_t W25Q32_Sector_Buf[4096];

void W_Flash_Safe_Write_Int(uint32_t address, uint16_t value)
{
    uint32_t sector_addr = address & 0xFFFFF000;
    uint32_t offset = address & 0x0FFF;
    
    W_Flash_Read_Arr(sector_addr, W25Q32_Sector_Buf, 4096);
    
    W25Q32_Sector_Buf[offset] = (value >> 8) & 0xFF;
    W25Q32_Sector_Buf[offset+1] = value & 0xFF;
    
    W_Flash_Sector_Erase(sector_addr);
    W_Flash_Write_Arr(sector_addr, W25Q32_Sector_Buf, 4096);
}

void W_Flash_Safe_Write_Long(uint32_t address, uint32_t value)
{
    uint32_t sector_addr = address & 0xFFFFF000;
    uint32_t offset = address & 0x0FFF;
    
    W_Flash_Read_Arr(sector_addr, W25Q32_Sector_Buf, 4096);
    
    W25Q32_Sector_Buf[offset] = (value >> 24) & 0xFF;
    W25Q32_Sector_Buf[offset+1] = (value >> 16) & 0xFF;
    W25Q32_Sector_Buf[offset+2] = (value >> 8) & 0xFF;
    W25Q32_Sector_Buf[offset+3] = value & 0xFF;
    
    W_Flash_Sector_Erase(sector_addr);
    W_Flash_Write_Arr(sector_addr, W25Q32_Sector_Buf, 4096);
}
