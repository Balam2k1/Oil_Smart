#include "data_model.h"
#include "string.h"

#include "app_config.h"
#include "Flash_definition.h"
#include "w25q32.h"
#include "Exti_Counter.h"
#include "log.h"


DataModel_Sensor_t  gSensorData;
DataModel_Alarm_t   gAlarmData;
DataModel_Runtime_t gRuntimeData;

DataModel_System_t   gSystemData = {0};

DataModel_Config_t   gConfigData = {
    .LogInterval       = DEFAULT_LOG_INTERVAL,
    .ControlMode       = 0,
    .SensorEnableMask  = DEFAULT_SENSOR_ENABLE_MASK,
    .SensorMaintLimits = {DEFAULT_MAINT_HOURS, DEFAULT_MAINT_HOURS,
                          DEFAULT_MAINT_HOURS, DEFAULT_MAINT_HOURS,
                          DEFAULT_MAINT_HOURS},
    .LedIntensity      = DEFAULT_LED_INTENSITY,
};

DataModel_Location_t gLocationData = {0};


void DataModel_Init(void)
{
    memset(&gSensorData , 0 , sizeof(gSensorData));
    memset(&gAlarmData  , 0 , sizeof(gAlarmData));
    memset(&gRuntimeData, 0 , sizeof(gRuntimeData));

    /* Kiem tra va Ghi Version vao Flash */
    uint32_t ver = W_Flash_Read_Long(AD_VERSION);
    if (ver != FIRMWARE_VERSION_VALUE) {
        W_Flash_Safe_Write_Long(AD_VERSION, FIRMWARE_VERSION_VALUE);
        gSystemData.FirmwareVersion = FIRMWARE_VERSION_VALUE;
    } else {
        gSystemData.FirmwareVersion = ver;
    }
    LOG_Printf("Firmware Version: %u\r\n", gSystemData.FirmwareVersion);

    /* Tự động quét Flash bằng Binary Search để tìm gSystemData.RecordCount cực kỳ nhanh (< 1 mili-giây) */
    uint32_t low = 0;
    uint32_t high = (AD_MAX_FLASH - AD_START_OIL_RECORD) / sizeof(DataRecord_t);
    uint32_t mid;
    gSystemData.RecordCount = 0;

    while (low <= high) {
        mid = low + (high - low) / 2;
        uint32_t addr = AD_START_OIL_RECORD + (mid * sizeof(DataRecord_t));
        
        // Đọc Timestamp (offset 8) để kiểm tra xem đã ghi hay chưa
        uint32_t ts = W_Flash_Read_Long(addr + 8);
        
        if (ts != 0xFFFFFFFF) {
            // Có dữ liệu, vùng chưa ghi phải nằm ở phía sau
            gSystemData.RecordCount = mid + 1;
            low = mid + 1;
        } else {
            // Trống, vùng kết thúc nằm ở phía trước
            if (mid == 0) break; // Tránh underflow
            high = mid - 1;
        }
    }
    
    // Theo logic mới (Mỗi log là 1 Trip độc lập), khi khởi động lại máy sẽ LUÔN BẮT ĐẦU TỪ 0
    // Không khôi phục số xung và số Lít từ bản ghi cũ nữa.
    
    extern void ExtiCounter_SetTotalLiters(double liters);
    extern void ExtiCounter_SetOil1Liters(double liters);
    extern void ExtiCounter_SetOil2Liters(double liters);
    extern void ExtiCounter_SetOil1Pulses(uint32_t pulses);
    extern void ExtiCounter_SetOil2Pulses(uint32_t pulses);

    ExtiCounter_SetTotalLiters(0.0);
    ExtiCounter_SetOil1Liters(0.0);
    ExtiCounter_SetOil2Liters(0.0);
    ExtiCounter_SetOil1Pulses(0);
    ExtiCounter_SetOil2Pulses(0);

    LOG_Printf("New Session Started. All Oil counters initialized to 0.\r\n");

    uint32_t intv = W_Flash_Read_Long(AD_LOG_INTERVAL);
    if (intv != 0xFFFFFFFF && intv > 0) gConfigData.LogInterval = intv;
    else W_Flash_Safe_Write_Long(AD_LOG_INTERVAL, gConfigData.LogInterval);
    
    LOG_Printf("Log Interval: %u seconds\r\n", gConfigData.LogInterval);

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
    memcpy(gSystemData.HardwareUID, uid, 12);
    
    W_Flash_Read_Arr(AD_DEVICE_ID, gSystemData.FlashUID, 12);
    bool flash_uid_empty = true;
    for(int i=0; i<12; i++) {
        if(gSystemData.FlashUID[i] != 0xFF) { flash_uid_empty = false; break; }
    }
    
    if (flash_uid_empty) {
        W_Flash_Write_Arr(AD_DEVICE_ID, gSystemData.HardwareUID, 12);
        memcpy(gSystemData.FlashUID, gSystemData.HardwareUID, 12);
    }
    
    gSystemData.UidMatched = (memcmp(gSystemData.HardwareUID, gSystemData.FlashUID, 12) == 0);
    if (!gSystemData.UidMatched) {
        LOG_Printf("CRITICAL WARNING: UID MISMATCH!\r\n");
    } else {
        LOG_Printf("UID MATCHED.\r\n");
    }

    /* --- Load Sensor Enable Mask --- */
    uint16_t mask = W_Flash_Read_Int(AD_SENSOR_ENABLE_MASK);
    if (mask == 0xFFFF) {
        gConfigData.SensorEnableMask = 0xFF; // All enabled by default
        W_Flash_Safe_Write_Int(AD_SENSOR_ENABLE_MASK, 0x00FF);
    } else {
        gConfigData.SensorEnableMask = (uint8_t)(mask & 0xFF);
    }

    /* --- Load System Control Mode --- */
    uint16_t mode = W_Flash_Read_Int(AD_SYSTEM_CONTROL_MODE);
    if (mode == 0xFFFF) {
        gConfigData.ControlMode = 0; // Default to 0 (Local control)
        W_Flash_Safe_Write_Int(AD_SYSTEM_CONTROL_MODE, 0);
    } else {
        gConfigData.ControlMode = (uint8_t)(mode & 0xFF);
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
            maint_buf[i*2] = (gConfigData.SensorMaintLimits[i] >> 8) & 0xFF;
            maint_buf[i*2+1] = gConfigData.SensorMaintLimits[i] & 0xFF;
        }
        W_Flash_Write_Arr(AD_SENSOR_MAINT_LIMIT_BASE, maint_buf, 10);
    } else {
        for(int i=0; i<5; i++) {
            gConfigData.SensorMaintLimits[i] = (maint_buf[i*2] << 8) | maint_buf[i*2+1];
        }
    }
    
    /* --- Load Sensor Run Times --- */
    for (int i = 0; i < 5; i++) {
        uint32_t runtime = W_Flash_Read_Long(AD_SENSOR_RUNTIME_BASE + (i * 4));
        if (runtime != 0xFFFFFFFF) gRuntimeData.SensorRunTimes[i] = runtime;
        else W_Flash_Safe_Write_Long(AD_SENSOR_RUNTIME_BASE + (i * 4), gRuntimeData.SensorRunTimes[i]);
    }

    /* --- Load LED Intensity --- */
    uint16_t led_val = W_Flash_Read_Int(AD_LED_INTENSITY);
    if (led_val == 0xFFFF) {
        gConfigData.LedIntensity = 2;
        W_Flash_Safe_Write_Int(AD_LED_INTENSITY, gConfigData.LedIntensity);
    } else {
        gConfigData.LedIntensity = (uint8_t)(led_val & 0xFF);
    }
    
    /* --- Load Calibration Table --- */
    extern CalibrationPoint_t g_calib_table[10];
    W_Flash_Read_Arr(AD_OIL_CALIB_TABLE, (uint8_t*)g_calib_table, sizeof(g_calib_table));
    // Verify if flash is empty
    if (g_calib_table[0].freq_hz == 0xFFFFFFFF) {
        memset(g_calib_table, 0, sizeof(g_calib_table));
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
        /* gSystemData.UnixTime được đồng bộ từ GPS hoặc DS1307 bên trong SensorManager_Task, 
           tuy nhiên nếu mất sóng cả 2 thì tự cộng ở đây cũng được. Nhưng không tự cộng RunTime ở đây nữa! */
        gSystemData.UnixTime++;
    }
}
