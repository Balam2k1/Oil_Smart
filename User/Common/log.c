/******************************************************************************
 * File    : log.c
 * Brief   : Debug Log Module
 ******************************************************************************/

#include "log.h"
#include "app_config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "w25q32.h"

/* Buffer format */
static char LogBuffer[256];

static uint8_t log_rx_byte;
static char log_cmd_buf[64];
static uint8_t log_cmd_idx = 0;
static bool log_cmd_ready = false;
static uint32_t last_rx_tick = 0;

/******************************************************************************
 * Khởi tạo
 ******************************************************************************/
void LOG_Init(void)
{
#if LOG_ENABLE
    HAL_UART_Receive_IT(LOG_UART, &log_rx_byte, 1);
#endif
}

#if LOG_ENABLE
void LOG_UART_Callback(void)
{
    if (log_cmd_idx < sizeof(log_cmd_buf) - 1) {
        log_cmd_buf[log_cmd_idx++] = log_rx_byte;
        log_cmd_buf[log_cmd_idx] = '\0';
        
        // Nhận diện Enter (PuTTY/TeraTerm)
        if (log_rx_byte == '\r' || log_rx_byte == '\n') {
            log_cmd_ready = true;
        }
    } else {
        log_cmd_idx = 0; /* Chống tràn */
    }
    
    last_rx_tick = HAL_GetTick(); // Reset timer khi có data mới
    HAL_UART_Receive_IT(LOG_UART, &log_rx_byte, 1);
}

void LOG_Task(void)
{
    // Kích hoạt Ready nếu nhận xong chuỗi từ Hercules (ngưng nhận > 50ms)
    if (log_cmd_idx > 0 && (HAL_GetTick() - last_rx_tick > 50)) {
        log_cmd_ready = true;
    }

    if (log_cmd_ready) {
        log_cmd_ready = false;
        
        if (strstr(log_cmd_buf, "ESP_LOG_ON") != NULL) {
            extern bool ESP32_LogEnable;
            ESP32_LogEnable = true;
            LOG_Print("\r\nESP32 Log Enabled\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "ESP_LOG_OFF") != NULL) {
            extern bool ESP32_LogEnable;
            ESP32_LogEnable = false;
            LOG_Print("\r\nESP32 Log Disabled\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "SENSOR_LOG_ON") != NULL) {
            extern bool SENSOR_LogEnable;
            SENSOR_LogEnable = true;
            LOG_Print("\r\nSensor Log Enabled\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "SENSOR_LOG_OFF") != NULL) {
            extern bool SENSOR_LogEnable;
            SENSOR_LogEnable = false;
            LOG_Print("\r\nSensor Log Disabled\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "GPS_LOG_ON") != NULL) {
            extern bool GPS_LogEnable;
            GPS_LogEnable = true;
            LOG_Print("\r\nGPS & RTC Log Enabled\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "GPS_LOG_OFF") != NULL) {
            extern bool GPS_LogEnable;
            GPS_LogEnable = false;
            LOG_Print("\r\nGPS & RTC Log Disabled\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "FLASH_LOG_ON") != NULL) {
            extern bool FLASH_LogEnable;
            FLASH_LogEnable = true;
            LOG_Print("\r\nFlash Log Enabled\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "FLASH_LOG_OFF") != NULL) {
            extern bool FLASH_LogEnable;
            FLASH_LogEnable = false;
            LOG_Print("\r\nFlash Log Disabled\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "GET_LIMITS") != NULL) {
            extern uint16_t Limit_Temp, Limit_VinMin, Limit_VinMax, Limit_Pressure, Limit_RPM, Limit_Oil, Limit_MaintenanceTime;
            LOG_Printf("\r\n--- SYSTEM LIMITS & CONSTANTS ---\r\n");
            LOG_Printf("Limit Temp: %u.%u C\r\n", Limit_Temp / 10, Limit_Temp % 10);
            LOG_Printf("Limit Vin MIN: %u.%u V\r\n", Limit_VinMin / 10, Limit_VinMin % 10);
            LOG_Printf("Limit Vin MAX: %u.%u V\r\n", Limit_VinMax / 10, Limit_VinMax % 10);
            LOG_Printf("Limit PA: %u.%u Bar\r\n", Limit_Pressure / 10, Limit_Pressure % 10);
            LOG_Printf("Limit Tacho: %u RPM\r\n", Limit_RPM);
            LOG_Printf("Limit Oil: %u.%u L/h\r\n", Limit_Oil / 10, Limit_Oil % 10);
            LOG_Printf("Limit Maintenance: %u hours\r\n", Limit_MaintenanceTime);
            LOG_Printf("---------------------------------\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "GET_VERSION") != NULL) {
            #include "data_model.h"
            LOG_Printf("\r\n--- FIRMWARE INFO ---\r\n");
            LOG_Printf("Version: %u\r\n", gSystemData.FirmwareVersion);
            LOG_Printf("---------------------\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "ERASE_FLASH") != NULL) {
            LOG_Print("\r\nErasing Flash... Please wait. System will reboot after completion.\r\n");
            
            /* Gọi hàm xóa toàn bộ chip Flash */
            extern void W_Flash_Chip_Erase(void);
            W_Flash_Chip_Erase();
            
            LOG_Print("\r\nFlash Erased! Rebooting...\r\n");
            
            /* Khởi động lại MCU để load cấu hình mặc định vào Flash */
            NVIC_SystemReset();
        } else if (strstr(log_cmd_buf, "COUNT_FLASH") != NULL) {
            LOG_Printf("\r\nTotal Records currently in Flash: %u\r\n", gSystemData.RecordCount);
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "READ_FLASH") != NULL) {
            LOG_Printf("\r\n=== READING FLASH RECORDS (%u) ===\r\n", gSystemData.RecordCount);
            
            #include "data_model.h"
            #include "Flash_definition.h"
            #include "w25q32.h"
            
            for (uint32_t i = 0; i < gSystemData.RecordCount; i++) {
                DataRecord_t rec;
                uint32_t addr = AD_START_OIL_RECORD + (i * sizeof(DataRecord_t));
                W_Flash_Read_Arr(addr, (uint8_t*)&rec, sizeof(DataRecord_t));
                
                LOG_Printf("[%04u] Unix: %u | Lat: %ld, Lon: %ld | Oil: %u.%02u | Vin: %u.%01uV | Tacho: %u RPM | Temp: %u.%01uC | PA: %u.%01uBar\r\n", 
                           i+1, 
                           rec.Timestamp, 
                           rec.Latitude, rec.Longitude, 
                           (uint16_t)(rec.Total_Oil_Consumed / 100), (uint16_t)(rec.Total_Oil_Consumed % 100),
                           rec.Voltage / 10, rec.Voltage % 10, 
                           rec.RPM, 
                           rec.Temperature / 10, rec.Temperature % 10, 
                           rec.Pressure / 10, rec.Pressure % 10);
                
                HAL_Delay(10); // Tránh bị tràn buffer UART
            }
            LOG_Printf("=== END OF RECORDS ===\r\n\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "READ_REC_") != NULL) {
            /* Đợi 50ms để ngắt UART nhận nốt các chữ số còn lại (nếu có) */
            extern void HAL_Delay(uint32_t Delay);
            HAL_Delay(50);
            
            uint32_t idx = 0;
            if (sscanf(strstr(log_cmd_buf, "READ_REC_"), "READ_REC_%lu", &idx) == 1) {
                if (idx > 0 && idx <= gSystemData.RecordCount) {
                    #include "data_model.h"
                    #include "Flash_definition.h"
                    #include "w25q32.h"
                    
                    DataRecord_t rec;
                    uint32_t addr = AD_START_OIL_RECORD + ((idx - 1) * sizeof(DataRecord_t));
                    W_Flash_Read_Arr(addr, (uint8_t*)&rec, sizeof(DataRecord_t));
                    
                    LOG_Printf("\r\n[RECORD %04u] Unix: %u | Lat: %d, Lon: %d | Oil: %u.%02u | Vin: %u.%01uV | Tacho: %u RPM | Temp: %u.%01uC | PA: %u.%01uBar\r\n", 
                               idx, 
                               rec.Timestamp, 
                               rec.Latitude, rec.Longitude, 
                               (uint16_t)(rec.Total_Oil_Consumed / 100), (uint16_t)(rec.Total_Oil_Consumed % 100),
                               rec.Voltage / 10, rec.Voltage % 10, 
                               rec.RPM, 
                               rec.Temperature / 10, rec.Temperature % 10, 
                               rec.Pressure / 10, rec.Pressure % 10);
                } else {
                    LOG_Printf("\r\nError: Record %lu not found. Total records: %u\r\n", idx, gSystemData.RecordCount);
                }
            }
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "GET_ID") != NULL) {
            LOG_Printf("\r\n--- HARDWARE UID ---\r\n");
            for (int i=0; i<12; i++) {
                LOG_Printf("%02X", gSystemData.HardwareUID[i]);
            }
            LOG_Printf("\r\n--------------------\r\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "SET_MAINT_") != NULL) {
            extern void HAL_Delay(uint32_t Delay);
            HAL_Delay(50);
            
            uint32_t hours = 0;
            if (sscanf(strstr(log_cmd_buf, "SET_MAINT_"), "SET_MAINT_%lu", &hours) == 1) {
                extern uint16_t Limit_MaintenanceTime;
                Limit_MaintenanceTime = (uint16_t)hours;
                
                #include "Flash_definition.h"
                #include "w25q32.h"
                W_Flash_Safe_Write_Int(AD_TIME_BAODUONG, Limit_MaintenanceTime);
                LOG_Printf("\r\n--- MAINTENANCE TIME UPDATED: %u hours ---\r\n", Limit_MaintenanceTime);
            }
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "SET_SENS_MAINT_") != NULL) {
            extern void HAL_Delay(uint32_t Delay);
            HAL_Delay(50);
            
            uint32_t id = 0;
            uint32_t hours = 0;
            if (sscanf(strstr(log_cmd_buf, "SET_SENS_MAINT_"), "SET_SENS_MAINT_%lu_%lu", &id, &hours) == 2) {
                if (id >= 1 && id <= 5) {
                    #include "data_model.h"
                    #include "Flash_definition.h"
                    #include "w25q32.h"
                    
                    gConfigData.SensorMaintLimits[id - 1] = (uint16_t)hours;
                    
                    uint8_t maint_buf[10];
                    for(int i=0; i<5; i++) {
                        maint_buf[i*2] = (gConfigData.SensorMaintLimits[i] >> 8) & 0xFF;
                        maint_buf[i*2+1] = gConfigData.SensorMaintLimits[i] & 0xFF;
                    }
                    W_Flash_Write_Arr(AD_SENSOR_MAINT_LIMIT_BASE, maint_buf, 10);
                    
                    LOG_Printf("\r\n--- SENSOR %lu MAINTENANCE LIMIT UPDATED: %lu hours ---\r\n", id, hours);
                } else {
                    LOG_Printf("\r\nError: Sensor ID must be 1 to 5.\r\n");
                }
            }
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "SET_RUNTIME_") != NULL) {
            extern void HAL_Delay(uint32_t Delay);
            HAL_Delay(50);
            
            uint32_t hours = 0;
            if (sscanf(strstr(log_cmd_buf, "SET_RUNTIME_"), "SET_RUNTIME_%lu", &hours) == 1) {
                #include "data_model.h"
                #include "Flash_definition.h"
                #include "w25q32.h"
                
                gRuntimeData.RunTime = hours;
                gRuntimeData.EngineRunSeconds = 0; /* Reset luôn số giây chạy tạm thời */
                W_Flash_Safe_Write_Long(AD_TIME_RUN, gRuntimeData.RunTime);
                
                LOG_Printf("\r\n--- ENGINE RUNTIME OVERWRITTEN: %u hours ---\r\n", gRuntimeData.RunTime);
            }
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "SET_LED_INTENSITY_") != NULL) {
            extern void HAL_Delay(uint32_t Delay);
            HAL_Delay(50);
            
            uint32_t level = 0;
            if (sscanf(strstr(log_cmd_buf, "SET_LED_INTENSITY_"), "SET_LED_INTENSITY_%lu", &level) == 1) {
                #include "max7219.h"
                MAX7219_SetIntensity((uint8_t)level);
            }
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "SET_BUZZER_") != NULL) {
            extern void HAL_Delay(uint32_t Delay);
            HAL_Delay(50);
            
            uint32_t state = 0;
            if (sscanf(strstr(log_cmd_buf, "SET_BUZZER_"), "SET_BUZZER_%lu", &state) == 1) {
                #include "buzzer.h"
                // state = 1 -> ON (Mute = false), state = 0 -> OFF (Mute = true)
                Buzzer_SetMute((state == 0) ? true : false);
            }
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "GET_RUNTIME") != NULL) {
            #include "data_model.h"
            extern uint16_t Limit_MaintenanceTime;
            LOG_Printf("\r\n--- ENGINE RUNTIME INFO ---\r\n");
            LOG_Printf("Total Run Time: %u hours\r\n", gRuntimeData.RunTime);
            LOG_Printf("Current Session (Unsaved): %u seconds\r\n", gRuntimeData.EngineRunSeconds);
            LOG_Printf("Maintenance Limit: %u hours\r\n", Limit_MaintenanceTime);
            LOG_Printf("---------------------------\r\n\n");
            log_cmd_idx = 0;
        } else if (strstr(log_cmd_buf, "SET_TIME_") != NULL) {
            extern void HAL_Delay(uint32_t Delay);
            HAL_Delay(50);
            
            uint32_t d, m, y, h, min, s;
            if (sscanf(strstr(log_cmd_buf, "SET_TIME_"), "SET_TIME_%lu_%lu_%lu_%lu_%lu_%lu", &d, &m, &y, &h, &min, &s) == 6) {
                #include "ds1307.h"
                DS1307_Time t;
                t.date = (uint8_t)d;
                t.month = (uint8_t)m;
                t.year = (uint8_t)(y % 100);
                t.hour = (uint8_t)h;
                t.min = (uint8_t)min;
                t.sec = (uint8_t)s;
                t.day = 1; // Default
                
                if (DS1307_SetTime(&t)) {
                    LOG_Printf("\r\n--- RTC TIME SET SUCCESSFULLY: %02u/%02u/%02u %02u:%02u:%02u ---\r\n", d, m, y, h, min, s);
                } else {
                    LOG_Printf("\r\n--- ERROR: FAILED TO SET RTC TIME ---\r\n");
                }
            } else {
                LOG_Printf("\r\nError: Invalid format. Use SET_TIME_dd_MM_yy_hh_mm_ss\r\n");
            }
            log_cmd_idx = 0;
        }
        
        /* Dọn dẹp buffer sau khi xử lý xong */
        log_cmd_idx = 0;
        log_cmd_buf[0] = '\0';
    }
}

void LOG_RestartReceive(void)
{
    HAL_UART_Receive_IT(LOG_UART, &log_rx_byte, 1);
}
#else
void LOG_UART_Callback(void) {}
void LOG_Task(void) {}
void LOG_RestartReceive(void) {}
#endif

/******************************************************************************
 * In chuỗi
 ******************************************************************************/
void LOG_Print(const char *str)
{
#if LOG_ENABLE

    if(!debug_main || str == NULL)
    {
        return;
    }

    HAL_UART_Transmit(
            LOG_UART,
            (uint8_t*)str,
            strlen(str),
            100);

#endif
}

/******************************************************************************
 * In format
 ******************************************************************************/
void LOG_Printf(const char *fmt, ...)
{
#if LOG_ENABLE

    if(!debug_main)
    {
        return;
    }

    va_list args;

    va_start(args, fmt);

    vsnprintf(
            LogBuffer,
            sizeof(LogBuffer),
            fmt,
            args);

    va_end(args);

    HAL_UART_Transmit(
            LOG_UART,
            (uint8_t*)LogBuffer,
            strlen(LogBuffer),
            100);

#endif
}
