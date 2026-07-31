#include "ds1307.h"
#include "i2c.h"

#define DS1307_I2C_PORT &hi2c2

// Chuyển BCD ↔ Decimal
static uint8_t bcd_to_dec(uint8_t val) {
    return (val / 16 * 10) + (val % 16);
}

static uint8_t dec_to_bcd(uint8_t val) {
    return (val / 10 * 16) + (val % 10);
}

void DS1307_Init(void) {
    // Hardware I2C is initialized in MX_I2C2_Init() which is called in main.c
}

void DS1307_SetTime(DS1307_Time *time) {
    uint8_t data[7];
    data[0] = dec_to_bcd(time->sec);
    data[1] = dec_to_bcd(time->min);
    data[2] = dec_to_bcd(time->hour);
    data[3] = dec_to_bcd(time->day);
    data[4] = dec_to_bcd(time->date);
    data[5] = dec_to_bcd(time->month);
    data[6] = dec_to_bcd(time->year);
    
    HAL_I2C_Mem_Write(DS1307_I2C_PORT, DS1307_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, data, 7, 1000);
}

void DS3231_SetTime(DS1307_Time *time) {
    DS1307_SetTime(time);
}

void DS1307_GetTime(DS1307_Time *time) {
    uint8_t data[7] = {0};
    HAL_I2C_Mem_Read(DS1307_I2C_PORT, DS1307_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, data, 7, 1000);
    
    time->sec   = bcd_to_dec(data[0] & 0x7F);
    time->min   = bcd_to_dec(data[1]);
    time->hour  = bcd_to_dec(data[2] & 0x3F);
    time->day   = bcd_to_dec(data[3]);
    time->date  = bcd_to_dec(data[4]);
    time->month = bcd_to_dec(data[5]);
    time->year  = bcd_to_dec(data[6]);
}

void DS3231_GetTime(DS1307_Time *time) {
    DS1307_GetTime(time);
}

void DS1307_Start(void) {
    uint8_t sec = 0;
    HAL_I2C_Mem_Read(DS1307_I2C_PORT, DS1307_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &sec, 1, 100);
    sec &= 0x7F; // Clear CH bit
    HAL_I2C_Mem_Write(DS1307_I2C_PORT, DS1307_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &sec, 1, 100);
}

void DS1307_Stop(void) {
    uint8_t sec = 0;
    HAL_I2C_Mem_Read(DS1307_I2C_PORT, DS1307_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &sec, 1, 100);
    sec |= 0x80; // Set CH bit
    HAL_I2C_Mem_Write(DS1307_I2C_PORT, DS1307_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &sec, 1, 100);
}
