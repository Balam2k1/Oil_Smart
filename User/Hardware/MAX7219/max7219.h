
#ifndef LED_MAX7219_H_
#define LED_MAX7219_H_
#include "stm32f0xx.h"
#include "main.h"



void MAX7219_SelectChip(uint8_t chip);
void MAX7219_SendByteToChip(uint8_t chip, uint8_t data);
void MAX7219_SendCommandToChip(uint8_t chip, uint8_t address, uint8_t data);
void MAX7219_InitAll(void);
void MAX7219_SetIntensity(uint8_t level);
void MAX7219_DisplayTest(void);
void MAX7219_ClearSensor(uint8_t sensor);
void MAX7219_DashesSensor(uint8_t sensor);
void show_temp(unsigned int temp);
void show_oil(unsigned int Lit);
void show_rpm(unsigned int rpm);
void show_pressure(unsigned int Pa);
void show_volt(unsigned int vol);
void show_volt_warning(unsigned int max,unsigned int min,  unsigned int vol);
void show_pressure_warning(unsigned int max, unsigned int min, unsigned int Pa);
void show_oil_warning(unsigned int max,  unsigned int Lit);
void show_temp_warning(unsigned int max,  unsigned int Temp);
void show_rpm_warning(unsigned int max,  unsigned int Rpm);

#endif
