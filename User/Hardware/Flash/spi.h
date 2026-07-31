#ifndef SPI_H_
#define SPI_H_

#include "main.h"

void spi_cs_low(void);
void spi_cs_high(void);
void SPI_Software_Transmit_Receive(const uint8_t *txData, uint8_t *rxData, uint16_t length);

#endif /* SPI_H_ */
