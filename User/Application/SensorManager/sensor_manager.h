#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>
#include "log.h"
#include "stm32f0xx_hal.h"

void SensorManager_Init(void);
void SensorManager_Task(void);

uint16_t Sensor_GetRawData(uint8_t sensor);
uint32_t Sensor_GetOperatingHours(uint8_t sensor);
uint16_t Sensor_GetType(uint8_t sensor);
uint32_t Sensor_GetSerialNumber(uint8_t sensor);
uint8_t  Sensor_IsOnline(uint8_t sensor);

uint16_t Sensor_CalcVin(uint16_t adc);

uint16_t Sensor_CalcTemperature_Modbus(uint16_t raw);
uint16_t Sensor_CalcTemperature_Local(uint16_t adc);

uint16_t Sensor_CalcPressure_Modbus(uint16_t raw);
uint16_t Sensor_CalcPressure_Local(uint16_t adc);

uint16_t Sensor_CalcRPM_Modbus(uint16_t raw);
uint16_t Sensor_CalcRPM_Local(uint32_t period);

uint16_t Sensor_CalcOil_Modbus(uint16_t raw);
uint16_t Sensor_CalcOil_Local(uint32_t period_ms, uint8_t is_return);

void SensorManager_LogAll(void);
void SensorManager_Test(void);

void SensorManager_Test_PA_Vin(void);
void SensorManager_Update(void);

void SensorManager_Task(void);

extern bool SENSOR_LogEnable;

#endif
