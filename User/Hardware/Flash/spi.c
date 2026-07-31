#include "spi.h"
#include "app_config.h"

void spi_cs_low(void) {
    HAL_GPIO_WritePin(FLASH_SPI_CS_PORT, FLASH_SPI_CS_PIN, GPIO_PIN_RESET);
}

void spi_cs_high(void) {
    HAL_GPIO_WritePin(FLASH_SPI_CS_PORT, FLASH_SPI_CS_PIN, GPIO_PIN_SET);
}

void SPI_Software_Transmit_Receive(const uint8_t *txData, uint8_t *rxData, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        uint8_t txByte = txData ? txData[i] : 0xFF;
        uint8_t rxByte = 0;
        for (uint8_t bit = 0; bit < 8; bit++) {
            // Setup MOSI (FLASH_SPI_SI)
            HAL_GPIO_WritePin(FLASH_SPI_SI_PORT, FLASH_SPI_SI_PIN, (txByte & 0x80) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            txByte <<= 1;
            
            // SCK rising edge (FLASH_SPI_SCK)
            HAL_GPIO_WritePin(FLASH_SPI_SCK_PORT, FLASH_SPI_SCK_PIN, GPIO_PIN_SET);
            
            // Sample MISO (FLASH_SPI_SO)
            rxByte <<= 1;
            if (HAL_GPIO_ReadPin(FLASH_SPI_SO_PORT, FLASH_SPI_SO_PIN) == GPIO_PIN_SET) {
                rxByte |= 0x01;
            }
            
            // SCK falling edge (FLASH_SPI_SCK)
            HAL_GPIO_WritePin(FLASH_SPI_SCK_PORT, FLASH_SPI_SCK_PIN, GPIO_PIN_RESET);
        }
        if (rxData) {
            rxData[i] = rxByte;
        }
    }
}
