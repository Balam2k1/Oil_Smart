#ifndef DS1307_H
#define DS1307_H

#include <stdint.h>

#define DS1307_ADDRESS 0xD0

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} DS1307_Time;

void DS1307_Init(void);
void DS1307_SetTime(DS1307_Time *time);
void DS1307_GetTime(DS1307_Time *time);

void DS3231_SetTime(DS1307_Time *time);
void DS3231_GetTime(DS1307_Time *time);

void DS1307_Start(void);
void DS1307_Stop(void);

#endif /* DS1307_H */