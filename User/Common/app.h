/******************************************************************************
 * File    : app.h
 * Brief   : Global Include File
 ******************************************************************************/

#ifndef APP_H
#define APP_H

#include "stdbool.h"
#include "main.h"

/*===================== COMMON =====================*/

#include "app_config.h"
#include "log.h"


/*==================== HARDWARE ====================*/

#include "rs485.h"
#include "max7219.h"
#include "hc595.h"
#include "buzzer.h"
#include "adc_driver.h"
#include "tinyGPS.h"

/*=================== APPLICATION ==================*/

#include "modbus_master.h"
#include "sensor_manager.h"
#include "display_manager.h"
#include "alarm_manager.h"
#include "data_model.h"
/*=================== CONTROLLER ===================*/


#endif
