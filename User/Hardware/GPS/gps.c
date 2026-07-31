
#include "stm32f0xx.h"
#include "main.h"
#include <math.h>
#include <stdlib.h>
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include "gpio.h"
#include "time.h"
#include "usart.h"
#include "adc.h"
#include "TinyGPS.h"


#define TWO_PI M_PI*2
#define sq(n) (n*n)
#define radians(angleDegrees) (angleDegrees * M_PI / 180.0)
#define degrees(angleRadians) (angleRadians * 180.0 / M_PI)
#ifndef GPS_UART
#define GPS_UART (&huart3)
#endif

volatile bool flag_gps_Neo_7M = false;
static uint8_t GpsRxByte;

uint32_t millis(void)
{
    return HAL_GetTick();
}


const float GPS_INVALID_F_ANGLE = 1000.0;
const float GPS_INVALID_F_ALTITUDE = 1000000.0;
const float GPS_INVALID_F_SPEED = -1.0;

uint8_t setGGA[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x24, 0x0D};
uint8_t setGLL[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B, 0x0D};
uint8_t setGSA[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32, 0x0D};
uint8_t setGSV[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39, 0x0D};
uint8_t setRMC[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x40, 0x0D};
uint8_t setVTG[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x46, 0x0D};

uint8_t Off_GPS[] = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4D, 0x3B};
uint8_t Max_Run[]   = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x00, 0x00, 0x19, 0x81, 0x0D};
uint8_t Eco_Run[]   = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x00, 0x04, 0x1D, 0x85, 0x0D};
uint8_t Reset_GPS[] = {0xB5, 0x62, 0x06, 0x04, 0x00, 0x00, 0x0A, 0x21};

uint8_t Rate_GPS[]  = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x01, 0x00};


enum {_GPS_SENTENCE_GPGGA, _GPS_SENTENCE_GPRMC, _GPS_SENTENCE_OTHER};

uint32_t _time, _new_time;
uint32_t _date, _new_date;
long _latitude, _new_latitude;
long _longitude, _new_longitude;
long _altitude, _new_altitude;
uint32_t  _speed, _new_speed;
uint32_t  _course, _new_course;
uint32_t  _hdop, _new_hdop;
unsigned short _numsats, _new_numsats;

uint32_t _last_time_fix, _new_time_fix;
uint32_t _last_position_fix, _new_position_fix;

// parsing state variables
unsigned char _parity;
bool _is_checksum_term;
char _term[15];
unsigned char _sentence_type;
unsigned char _term_number;
unsigned char _term_offset;
bool _gps_data_good;

#ifndef _GPS_NO_STATS
  // statistics
  uint32_t _encoded_characters;
  unsigned short _good_sentences;
  unsigned short _failed_checksum;
  unsigned short _passed_checksum;
#endif

// internal utilities
int from_hex(char a);
uint32_t parse_decimal(void);
uint32_t parse_degrees(void);
bool term_complete(void);
bool gpsisdigit(char c) { return c >= '0' && c <= '9'; }
long gpsatol(const char *str);
int gpsstrcmp(const char *str1, const char *str2);

#define _GPRMC_TERM   "GPRMC"
#define _GPGGA_TERM   "GPGGA"

void GPS_Init(void)
{
    // Start interrupt reception on GPS_UART
    HAL_UART_Receive_IT(GPS_UART, &GpsRxByte, 1);

    // Turn off GGA
    HAL_UART_Transmit_IT(GPS_UART, setGGA, sizeof(setGGA));
    HAL_Delay(300);
    
    // Turn off GLL
    HAL_UART_Transmit_IT(GPS_UART, setGLL, sizeof(setGLL));
    HAL_Delay(300);
    
    // Turn off GSA
    HAL_UART_Transmit_IT(GPS_UART, setGSA, sizeof(setGSA));
    HAL_Delay(300);
    
    // Turn off GSV
    HAL_UART_Transmit_IT(GPS_UART, setGSV, sizeof(setGSV));
    HAL_Delay(300);
    
    // Turn off VTG
    HAL_UART_Transmit_IT(GPS_UART, setVTG, sizeof(setVTG));
    HAL_Delay(300);
}

void GPS_On(void)
{
    HAL_UART_Transmit_IT(GPS_UART, Max_Run, sizeof(Max_Run));
    HAL_Delay(300);
}

void GPS_Off(void)
{
    HAL_UART_Transmit_IT(GPS_UART, Off_GPS, sizeof(Off_GPS));
}

void GPS_Reset(void)
{
#if defined(GPS_INT_Pin) && defined(GPS_INT_GPIO_Port)
    HAL_GPIO_WritePin(GPS_INT_GPIO_Port, GPS_INT_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPS_INT_GPIO_Port, GPS_INT_Pin, GPIO_PIN_SET);
    HAL_Delay(250);
#endif
}

void GPS_TinyGPS(void)
{
    _time = GPS_INVALID_TIME;
    _date = GPS_INVALID_DATE;
    _latitude = GPS_INVALID_ANGLE;
    _longitude = GPS_INVALID_ANGLE;
    _altitude = GPS_INVALID_ALTITUDE;
    _speed = GPS_INVALID_SPEED;
    _course = GPS_INVALID_ANGLE;
    _hdop = GPS_INVALID_HDOP;
    _numsats = GPS_INVALID_SATELLITES;
    _last_time_fix = GPS_INVALID_FIX_TIME;
    _last_position_fix = GPS_INVALID_FIX_TIME;
    _parity = 0;
    _is_checksum_term = false;
    _sentence_type = _GPS_SENTENCE_OTHER;
    _term_number = 0;
    _term_offset = 0;
    _gps_data_good = false;
    _term[0] = '\0';
}

void GPS_UART_Callback(void)
{
    encode((char)GpsRxByte);
    HAL_UART_Receive_IT(GPS_UART, &GpsRxByte, 1);
}

void GPS_RestartReceive(void)
{
    HAL_UART_Receive_IT(GPS_UART, &GpsRxByte, 1);
}


//-------------------------------------------------------

bool encode(char c)
{
	 bool valid_sentence = false;

	 #ifndef _GPS_NO_STATS
	     ++_encoded_characters;
	 #endif
     switch(c)
     {
		  case ',': // term terminators
			  	_parity ^= c;
		  case '\r':
		  case '\n':
		  case '*':
				if (_term_offset < sizeof(_term))
				{
				  _term[_term_offset] = 0;
				  valid_sentence = term_complete();
				}
				++_term_number;
				_term_offset = 0;
				_is_checksum_term = c == '*';
				return valid_sentence;

		  case '$': // sentence begin
				_term_number = _term_offset = 0;
				_parity = 0;
				_sentence_type = _GPS_SENTENCE_OTHER;
				_is_checksum_term = false;
				_gps_data_good = false;
				return valid_sentence;
      }

	  // ordinary characters
	  if (_term_offset < sizeof(_term) - 1)
		 _term[_term_offset++] = c;
	  if (!_is_checksum_term)
		 _parity ^= c;

	  return valid_sentence;
}

//-------------------------------------------------------
int from_hex(char a) 
{
	  if (a >= 'A' && a <= 'F')
		  return a - 'A' + 10;
	  else if (a >= 'a' && a <= 'f')
		  return a - 'a' + 10;
	  else
      return a - '0';
}
//-------------------------------------------------------
uint32_t parse_decimal(void)
{
  char *p = _term;
  bool isneg = *p == '-';
  if (isneg) ++p;
  uint32_t ret = 100UL * gpsatol(p);
  while (gpsisdigit(*p)) ++p;
  if (*p == '.')
  {
		if (gpsisdigit(p[1]))
		{
		    ret += 10 * (p[1] - '0');
		    if (gpsisdigit(p[2]))
			ret += p[2] - '0';
		}
  }
  return isneg ? -ret : ret;
}

//-------------------------------------------------------
uint32_t parse_degrees(void)
{
  char *p;
  uint32_t left_of_decimal = gpsatol(_term);
  uint32_t hundred1000ths_of_minute = (left_of_decimal % 100UL) * 100000UL;
  for (p=_term; gpsisdigit(*p); ++p);
  if (*p == '.')
  {
		uint32_t mult = 10000;
		while (gpsisdigit(*++p))
		{
			  hundred1000ths_of_minute += mult * (*p - '0');
			  mult /= 10;
		}
  }
  return (left_of_decimal / 100) * 1000000 + (hundred1000ths_of_minute + 3) / 6;
}

#define COMBINE(sentence_type, term_number) (((unsigned)(sentence_type) << 5) | term_number)

//-------------------------------------------------------
bool term_complete(void)
{
  if (_is_checksum_term)
  {
    unsigned char checksum = 16 * from_hex(_term[0]) + from_hex(_term[1]);
    if (checksum == _parity)
    {
      if (_gps_data_good)
      {
#ifndef _GPS_NO_STATS
        ++_good_sentences;
#endif
        _last_time_fix = _new_time_fix;
        _last_position_fix = _new_position_fix;

        switch(_sentence_type)
        {
        case _GPS_SENTENCE_GPRMC:
          _time      = _new_time;
          _date      = _new_date;
          _latitude  = _new_latitude;
          _longitude = _new_longitude;
          _speed     = _new_speed;
          _course    = _new_course;
          break;
        case _GPS_SENTENCE_GPGGA:
          _altitude  = _new_altitude;
          _time      = _new_time;
          _latitude  = _new_latitude;
          _longitude = _new_longitude;
          _numsats   = _new_numsats;
          _hdop      = _new_hdop;
          break;
        }

        return true;
      }
    }

#ifndef _GPS_NO_STATS
    else
      ++_failed_checksum;
#endif
    return false;
  }

  // the first term determines the sentence type
  if (_term_number == 0)
  {
    if (!gpsstrcmp(_term, _GPRMC_TERM))
      _sentence_type = _GPS_SENTENCE_GPRMC;
    else if (!gpsstrcmp(_term, _GPGGA_TERM))
      _sentence_type = _GPS_SENTENCE_GPGGA;
    else
      _sentence_type = _GPS_SENTENCE_OTHER;
    return false;
  }

  if (_sentence_type != _GPS_SENTENCE_OTHER && _term[0])
    switch(COMBINE(_sentence_type, _term_number))
  {
    case COMBINE(_GPS_SENTENCE_GPRMC, 1): // Time in both sentences
    case COMBINE(_GPS_SENTENCE_GPGGA, 1):
      _new_time = parse_decimal();
      _new_time_fix = millis();
      break;
    case COMBINE(_GPS_SENTENCE_GPRMC, 2): // GPRMC validity
      _gps_data_good = _term[0] == 'A';
      break;
    case COMBINE(_GPS_SENTENCE_GPRMC, 3): // Latitude
    case COMBINE(_GPS_SENTENCE_GPGGA, 2):
      _new_latitude = parse_degrees();
      _new_position_fix = millis();
      break;
    case COMBINE(_GPS_SENTENCE_GPRMC, 4): // N/S
    case COMBINE(_GPS_SENTENCE_GPGGA, 3):
      if (_term[0] == 'S')
        _new_latitude = -_new_latitude;
      break;
    case COMBINE(_GPS_SENTENCE_GPRMC, 5): // Longitude
    case COMBINE(_GPS_SENTENCE_GPGGA, 4):
      _new_longitude = parse_degrees();
      break;
    case COMBINE(_GPS_SENTENCE_GPRMC, 6): // E/W
    case COMBINE(_GPS_SENTENCE_GPGGA, 5):
      if (_term[0] == 'W')
        _new_longitude = -_new_longitude;
      break;
    case COMBINE(_GPS_SENTENCE_GPRMC, 7): // Speed (GPRMC)
      _new_speed = parse_decimal();
      break;
    case COMBINE(_GPS_SENTENCE_GPRMC, 8): // Course (GPRMC)
      _new_course = parse_decimal();
      break;
    case COMBINE(_GPS_SENTENCE_GPRMC, 9): // Date (GPRMC)
      _new_date = gpsatol(_term);
      break;
    case COMBINE(_GPS_SENTENCE_GPGGA, 6): // Fix data (GPGGA)
      _gps_data_good = _term[0] > '0';
      break;
    case COMBINE(_GPS_SENTENCE_GPGGA, 7): // Satellites used (GPGGA)
      _new_numsats = (uint8_t)atoi(_term);
      break;
    case COMBINE(_GPS_SENTENCE_GPGGA, 8): // HDOP
      _new_hdop = parse_decimal();
      break;
    case COMBINE(_GPS_SENTENCE_GPGGA, 9): // Altitude (GPGGA)
      _new_altitude = parse_decimal();
      break;
  }

  return false;
}
//-------------------------------------------------------
long gpsatol(const char *str)
{
  long ret = 0;
  while (gpsisdigit(*str))
    ret = 10 * ret + *str++ - '0';
  return ret;
}
//-------------------------------------------------------
int gpsstrcmp(const char *str1, const char *str2)
{
  while (*str1 && *str1 == *str2)
    ++str1, ++str2;
  return *str1;
}

//-------------------------------------------------------
// lat/long in MILLIONTHs of a degree and age of fix in milliseconds
//ddd.dddddd  of degree.
void get_position(long *latitude, long *longitude, uint32_t *fix_age)
{
  if (latitude) *latitude = _latitude;
  if (longitude) *longitude = _longitude;
  if (fix_age) *fix_age = _last_position_fix == GPS_INVALID_FIX_TIME ? 
  GPS_INVALID_AGE : millis() - _last_position_fix;
}

// date as ddmmyy, time as hhmmsscc, and age in milliseconds
void get_datetime(uint32_t *date, uint32_t *time, uint32_t *age)
{
  if (date) *date = _date;
  if (time) *time = _time;
  if (age) *age = _last_time_fix == GPS_INVALID_FIX_TIME ? 
   GPS_INVALID_AGE : millis() - _last_time_fix;
}
//-------------------------------------------------------
void f_get_position(float *latitude, float *longitude, uint32_t *fix_age)
{
  long lat, lon;
  get_position(&lat, &lon, fix_age);
  *latitude = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lat / 1000000.0);
  *longitude = lat == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : (lon / 1000000.0);
}
//-------------------------------------------------------
void crack_datetime(int *year, unsigned char *month, unsigned char *day,
  unsigned char *hour, unsigned char *minute, unsigned char *second, unsigned char *hundredths, uint32_t *age)
{
  uint32_t date, time;
  get_datetime(&date, &time, age);
  if (year) 
  {
    *year = date % 100;
    *year += *year > 80 ? 1900 : 2000;
  }
  if (month) *month = (date / 100) % 100;
  if (day) *day = date / 10000;
  if (hour) *hour = time / 1000000;
  if (minute) *minute = (time / 10000) % 100;
  if (second) *second = (time / 100) % 100;
  if (hundredths) *hundredths = time % 100;
}

//---------------------------------------------------
float f_altitude(void)    
{
  return _altitude == GPS_INVALID_ALTITUDE ? GPS_INVALID_F_ALTITUDE : _altitude / 100.0;
}

float f_course(void)
{
  return _course == GPS_INVALID_ANGLE ? GPS_INVALID_F_ANGLE : _course / 100.0;
}

float f_speed_knots(void) 
{
  return _speed == GPS_INVALID_SPEED ? GPS_INVALID_F_SPEED : _speed / 100.0;
}

float f_speed_kmph(void)  
{ 
  float sk = f_speed_knots();
  return sk == GPS_INVALID_F_SPEED ? GPS_INVALID_F_SPEED : _GPS_KMPH_PER_KNOT * sk; 
}

// signed altitude in centimeters (from GPGGA sentence)
long altitude(void) { return _altitude; }

  // course in last full GPRMC sentence in 100th of a degree
uint32_t course(void) { return _course; }

  // speed in last full GPRMC sentence in 100ths of a knot
uint32_t speed(void) { return _speed; }

//-----------------------------------
uint8_t cardinal(double course)
{
  //directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  int direction = (int)((course + 11.25f) / 22.5f);
  return (uint8_t)direction;
}
//--------------------------------------------------------
//check GPS signal is ok or not?
bool check_gps_signal(void){ return _gps_data_good;}

//--------------------------------------------------------
// Function to convert a date and time to a Unix timestamp (timezone-free)
time_t dateTimeToTimestamp(Gps_date_time * date_time)
{
    int year = date_time->year;
    int month = date_time->month;
    int day = date_time->day;
    
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    
    long long days = (day + (153 * month - 457) / 5 + 365 * year + year / 4 - year / 100 + year / 400 - 719468);
    time_t seconds = days * 86400 + date_time->hour * 3600 + date_time->minute * 60 + date_time->second;
    return seconds;
}
//-------------------------------------------------------
//"$GPRMC,041855.00,A,2102.91058,N,10548.15397,E,0.301,,120623,,,A*7E\r\n";
void Get_Gps_now( Gps_record * record_now, Gps_date_time * date_time_now)
{

	if (check_gps_signal()==true)         //nếu có gps-->update data, nếu không ---> status updated only
	{
		flag_gps_Neo_7M=true;

		uint32_t date, time, age;
		long t_lat, t_long;
		Gps_date_time  t_date_time_2018 = {2018, 1, 1, 0, 0, 0};                //starting time of AS module

		get_datetime(&date, &time, &age);
		get_position(&t_lat, &t_long, &age);

		//get date and time
		date_time_now->year = 2000+ date % 100;				                     //fomat date: ddmmyy = 120623
		date_time_now->month=(date / 100) % 100;
		date_time_now->day= date / 10000;
		date_time_now->hour= time / 1000000;
		date_time_now->minute = (time / 10000) % 100;
		date_time_now->second = (time / 100) % 100;

		record_now->time = dateTimeToTimestamp (date_time_now) - dateTimeToTimestamp(&t_date_time_2018);
		record_now->lat = t_lat;
		record_now->lon = t_long;
		record_now->speed = (uint8_t)round(f_speed_knots());                    //max speed=255 ~ 25.5 knot.
		record_now->direct = cardinal((uint16_t)f_course());
		record_now->status = 1;


	}else
	{
		flag_gps_Neo_7M = false;
		record_now->status = 0;
	}
}

