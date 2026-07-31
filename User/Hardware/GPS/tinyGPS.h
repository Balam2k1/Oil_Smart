

#ifndef TINY_GPS_H
#define TINY_GPS_H

#include "main.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define _GPS_VERSION 13 // software version of this library
#define _GPS_MPH_PER_KNOT 1.15077945
#define _GPS_MPS_PER_KNOT 0.51444444
#define _GPS_KMPH_PER_KNOT 1.852
#define _GPS_MILES_PER_METER 0.00062137112
#define _GPS_KM_PER_METER 0.001

#define _GPS_NO_STATS

enum {
    GPS_INVALID_AGE = 0xFF,
    GPS_INVALID_ANGLE = 999999999, 
    GPS_INVALID_ALTITUDE = 999999999,
    GPS_INVALID_DATE = 0,
    GPS_INVALID_TIME = 0xFFFFFF,
    GPS_INVALID_SPEED = 999999999, 
    GPS_INVALID_FIX_TIME = 0xFFFFFF,
    GPS_INVALID_SATELLITES = 0xFF,
    GPS_INVALID_HDOP = 0xFFFFFF
};

/*=============================================================================
 * GPS Data Structures
 *============================================================================*/
typedef struct {
    uint32_t time;    // Time status (offset from 2018 base or absolute Unix timestamp)
    int32_t lat;      // Latitude in millionths of a degree
    int32_t lon;      // Longitude in millionths of a degree
    uint8_t speed;    // Speed in knots
    uint8_t direct;   // Heading direction (cardinal index)
    uint8_t status;   // 1 = Valid signal, 0 = No signal
} Gps_record;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} Gps_date_time;

extern volatile bool flag_gps_Neo_7M;

extern uint32_t _time, _new_time;
extern uint32_t _date, _new_date;
extern long _latitude, _new_latitude;
extern long _longitude, _new_longitude;
extern long _altitude, _new_altitude;
extern uint32_t  _speed, _new_speed;
extern uint32_t  _course, _new_course;
extern uint32_t  _hdop, _new_hdop;
extern unsigned short _numsats, _new_numsats;

/*=============================================================================
 * APIs
 *============================================================================*/
void GPS_Init(void);
void GPS_Off(void);
void GPS_On(void);
void GPS_Reset(void);
void GPS_TinyGPS(void);

/* Callback and Error Recovery Router */
void GPS_UART_Callback(void);
void GPS_RestartReceive(void);

bool encode(char c); // process one character received from GPS

// lat/long in MILLIONTHs of a degree and age of fix in milliseconds
void get_position(long *latitude, long *longitude, unsigned long *fix_age);

// date as ddmmyy, time as hhmmsscc, and age in milliseconds
void get_datetime(unsigned long *date, unsigned long *time, unsigned long *age);

// signed altitude in centimeters (from GPGGA sentence)
long altitude(void);

// course in last full GPRMC sentence in 100th of a degree
unsigned long course(void);

// speed in last full GPRMC sentence in 100ths of a knot
unsigned long speed(void);

// satellites used in last full GPGGA sentence
unsigned short satellites(void);

// horizontal dilution of precision in 100ths
unsigned long hdop(void);

void f_get_position(float *latitude, float *longitude, unsigned long *fix_age);
void crack_datetime(int *year, unsigned char *month, unsigned char *day,
                    unsigned char *hour, unsigned char *minute, unsigned char *second,
                    unsigned char *hundredths, unsigned long *fix_age);
float f_altitude(void);
float f_course(void);
float f_speed_knots(void);
float f_speed_kmph(void);

bool check_gps_signal(void);
void Get_Gps_now(Gps_record * record_now, Gps_date_time * date_time_now);

#ifndef _GPS_NO_STATS
void stats(unsigned long *chars, unsigned short *good_sentences, unsigned short *failed_cs);
#endif

#endif // TINY_GPS_H

	
