/******************************************************************************
 * File    : app_types.h
 * Brief   : Kiểu dữ liệu dùng chung
 ******************************************************************************/

#ifndef APP_TYPES_H
#define APP_TYPES_H

#include "stdint.h"

typedef enum
{
    DISPLAY_MODE_SENSOR = 0,

    DISPLAY_MODE_ALARM,

    DISPLAY_MODE_TIME

}DisplayMode_t;

#endif