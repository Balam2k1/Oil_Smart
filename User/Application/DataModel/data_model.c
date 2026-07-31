#include "data_model.h"
#include "string.h"

#include "app_config.h"
#include "Flash_definition.h"
#include "w25q32.h"
#include "log.h"


DataModel_Sensor_t  gSensorData;
DataModel_Alarm_t   gAlarmData;
DataModel_Runtime_t gRuntimeData;

uint32_t gSystemUnixTime = 0;
uint32_t gFirmwareVersion = 0;
uint32_t gRecordCount = 0;
uint32_t gLogInterval = 3600; /* default 60s, changed to 32-bit */
uint16_t gEngineRunSeconds = 0;
uint32_t gEngineRunSinceLastLog = 0;

int32_t gLastLatitude = 0;
int32_t gLastLongitude = 0;

uint8_t gHardwareUID[12];
uint8_t gFlashUID[12];
bool gUidMatched = false;

uint16_t gSensorMaintLimits[5] = {8760, 8760, 8760, 8760, 8760}; // Default 1 year
uint32_t gSensorRunTimes[5] = {0, 0, 0, 0, 0};
uint8_t gSystemControlMode = 0;
uint8_t gSensorEnableMask = 0xFF;
uint8_t gLedIntensity = 2;


void DataModel_Init(void)
{
    memset(&gSensorData , 0 , sizeof(gSensorData));
    memset(&gAlarmData  , 0 , sizeof(gAlarmData));
    memset(&gRuntimeData, 0 , sizeof(gRuntimeData));

    /* Kiem tra va Ghi Version vao Flash */
    uint32_t ver = W_Flash_Read_Long(AD_VERSION);
    if (ver != FIRMWARE_VERSION_VALUE) {
        W_Flash_Safe_Write_Long(AD_VERSION, FIRMWARE_VERSION_VALUE);
        gFirmwareVersion = FIRMWARE_VERSION_VALUE;
    } else {
        gFirmwareVersion = ver;
    }
    LOG_Printf("Firmware Version: %u\r\n", gFirmwareVersion);

    /* Tự động quét Flash bằng Binary Search để tìm gRecordCount cực kỳ nhanh (< 1 mili-giây) */
    uint32_t low = 0;
    uint32_t high = (AD_MAX_FLASH - AD_START_OIL_RECORD) / sizeof(DataRecord_t);
    uint32_t mid;
    gRecordCount = 0;

    while (low <= high) {
        mid = low + (high - low) / 2;
        uint32_t addr = AD_START_OIL_RECORD + (mid * sizeof(DataRecord_t));
        
        // Đọc Timestamp (offset 8) để kiểm tra xem đã ghi hay chưa
        uint32_t ts = W_Flash_Read_Long(addr + 8);
        
        if (ts != 0xFFFFFFFF) {
            // Có dữ liệu, vùng chưa ghi phải nằm ở phía sau
            gRecordCount = mid + 1;
            low = mid + 1;
        } else {
            // Trống, vùng kết thúc nằm ở phía trước
            if (mid == 0) break; // Tránh underflow
            high = mid - 1;
        }
    }
    
    LOG_Printf("Total Records Scanned in Flash: %u\r\n", gRecordCount);

    uint32_t intv = W_Flash_Read_Long(AD_LOG_INTERVAL);
    if (intv != 0xFFFFFFFF && intv > 0) gLogInterval = intv;
    else W_Flash_Safe_Write_Long(AD_LOG_INTERVAL, gLogInterval);
    
    LOG_Printf("Log Interval: %u seconds\r\n", gLogInterval);

    /* Load RunTime from Flash */
    uint32_t rt = W_Flash_Read_Long(AD_TIME_RUN);
    if (rt != 0xFFFFFFFF) gRuntimeData.RunTime = rt;
    else W_Flash_Safe_Write_Long(AD_TIME_RUN, gRuntimeData.RunTime);

    /* Load TotalTime from Flash */
    uint32_t tt = W_Flash_Read_Long(AD_TIME_SUM);
    if (tt != 0xFFFFFFFF) gRuntimeData.TotalTime = tt;
    else W_Flash_Safe_Write_Long(AD_TIME_SUM, gRuntimeData.TotalTime);

    LOG_Printf("Engine Run Time: %u hours\r\n", gRuntimeData.RunTime);

    /* --- Read UID from Hardware and Flash --- */
    uint32_t *uid = (uint32_t*)0x1FFFF7AC; // STM32F030 UID register
    memcpy(gHardwareUID, uid, 12);
    
    W_Flash_Read_Arr(AD_DEVICE_ID, gFlashUID, 12);
    bool flash_uid_empty = true;
    for(int i=0; i<12; i++) {
        if(gFlashUID[i] != 0xFF) { flash_uid_empty = false; break; }
    }
    
    if (flash_uid_empty) {
        W_Flash_Write_Arr(AD_DEVICE_ID, gHardwareUID, 12);
        memcpy(gFlashUID, gHardwareUID, 12);
    }
    
    gUidMatched = (memcmp(gHardwareUID, gFlashUID, 12) == 0);
    if (!gUidMatched) {
        LOG_Printf("CRITICAL WARNING: UID MISMATCH!\r\n");
    } else {
        LOG_Printf("UID MATCHED.\r\n");
    }

    /* --- Load Sensor Enable Mask --- */
    uint16_t mask = W_Flash_Read_Int(AD_SENSOR_ENABLE_MASK);
    if (mask == 0xFFFF) {
        gSensorEnableMask = 0xFF; // All enabled by default
        W_Flash_Safe_Write_Int(AD_SENSOR_ENABLE_MASK, 0x00FF);
    } else {
        gSensorEnableMask = (uint8_t)(mask & 0xFF);
    }

    /* --- Load System Control Mode --- */
    uint16_t mode = W_Flash_Read_Int(AD_SYSTEM_CONTROL_MODE);
    if (mode == 0xFFFF) {
        gSystemControlMode = 0; // Default to 0 (Local control)
        W_Flash_Safe_Write_Int(AD_SYSTEM_CONTROL_MODE, 0);
    } else {
        gSystemControlMode = (uint8_t)(mode & 0xFF);
    }

    /* --- Load Sensor Maintenance Limits --- */
    uint8_t maint_buf[10];
    W_Flash_Read_Arr(AD_SENSOR_MAINT_LIMIT_BASE, maint_buf, 10);
    bool maint_empty = true;
    for(int i=0; i<10; i++) {
        if(maint_buf[i] != 0xFF) { maint_empty = false; break; }
    }
    if (maint_empty) {
        for(int i=0; i<5; i++) {
            maint_buf[i*2] = (gSensorMaintLimits[i] >> 8) & 0xFF;
            maint_buf[i*2+1] = gSensorMaintLimits[i] & 0xFF;
        }
        W_Flash_Write_Arr(AD_SENSOR_MAINT_LIMIT_BASE, maint_buf, 10);
    } else {
        for(int i=0; i<5; i++) {
            gSensorMaintLimits[i] = (maint_buf[i*2] << 8) | maint_buf[i*2+1];
        }
    }
    
    /* --- Load Sensor Run Times --- */
    for (int i = 0; i < 5; i++) {
        uint32_t runtime = W_Flash_Read_Long(AD_SENSOR_RUNTIME_BASE + (i * 4));
        if (runtime != 0xFFFFFFFF) gSensorRunTimes[i] = runtime;
        else W_Flash_Safe_Write_Long(AD_SENSOR_RUNTIME_BASE + (i * 4), gSensorRunTimes[i]);
    }

    /* --- Load LED Intensity --- */
    uint16_t led_val = W_Flash_Read_Int(AD_LED_INTENSITY);
    if (led_val == 0xFFFF) {
        gLedIntensity = 2;
        W_Flash_Safe_Write_Int(AD_LED_INTENSITY, gLedIntensity);
    } else {
        gLedIntensity = (uint8_t)(led_val & 0xFF);
    }
}

#include <time.h>

/* Tính số giây từ 00:00:00 01/01/1970 (Chuẩn Unix) */
uint32_t DateTimeToUnix(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
    if (year < 100) year += 2000; // Xử lý năm 2 số (vd: 26 -> 2026)
    
    const uint16_t days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    uint32_t days = 0;
    
    for (uint16_t y = 1970; y < year; y++) {
        days += 365 + ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0) ? 1 : 0);
    }
    

    for (uint8_t m = 1; m < month; m++) {
        days += days_in_month[m];
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
            days++;
        }
    }
    
    days += (day - 1);
    
    return (days * 86400) + (hour * 3600) + (min * 60) + sec;
}

void SystemTime_Update(void)
{
    static uint32_t last_tick = 0;
    uint32_t tick = HAL_GetTick();
    if (tick - last_tick >= 1000) {
        last_tick += 1000;
        /* gSystemUnixTime được đồng bộ từ GPS hoặc DS1307 bên trong SensorManager_Task, 
           tuy nhiên nếu mất sóng cả 2 thì tự cộng ở đây cũng được. Nhưng không tự cộng RunTime ở đây nữa! */
        gSystemUnixTime++;
    }
}
