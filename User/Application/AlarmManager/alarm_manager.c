#include "alarm_manager.h"
#include "data_model.h"
#include "buzzer.h"
#include "hc595.h"
#include "w25q32.h"
#include "Flash_definition.h"
#include "app_config.h"
#include "log.h"
#include "sensor_manager.h"

/* Ngưỡng cảnh báo — giá trị mặc định lấy từ app_config.h */
uint16_t Limit_Temp          = DEFAULT_LIMIT_TEMP;
uint16_t Limit_Pressure      = DEFAULT_LIMIT_PA_MAX;
uint16_t Limit_PressureMin   = DEFAULT_LIMIT_PA_MIN;
uint16_t Limit_VinMin        = DEFAULT_LIMIT_VIN_MIN;
uint16_t Limit_VinMax        = DEFAULT_LIMIT_VIN_MAX;
uint16_t Limit_Oil           = DEFAULT_LIMIT_OIL;
uint16_t Limit_RPM           = DEFAULT_LIMIT_RPM;
uint16_t Limit_MaintenanceTime = DEFAULT_LIMIT_MAINT_TIME;

/* Hệ số hiệu chỉnh cảm biến — giá trị mặc định lấy từ app_config.h */
uint16_t VOLT_CONS  = DEFAULT_VOLT_CONS;
uint16_t TEMP_CONS  = DEFAULT_TEMP_CONS;
uint16_t PA_CONS    = DEFAULT_PA_CONS;
uint16_t OIL_1_CONS = DEFAULT_OIL_1_CONS;
uint16_t OIL_2_CONS = DEFAULT_OIL_2_CONS;

/* Khai báo extern tập trung — không rải trong thân hàm */
extern uint16_t RETURN_BUBBLE_PCT;
extern uint16_t FORWARD_BUBBLE_PCT;

void AlarmManager_Init(void)
{
    // Load limits from Flash. If Flash is empty (0xFFFF), write the defaults.
    uint16_t val;
    
    LOG_Printf("--- FLASH RAW READS ---\r\n");

    val = W_Flash_Read_Int(AD_T_MAX);
    LOG_Printf("AD_T_MAX raw: 0x%04X (%u)\r\n", val, val);
    if (val != 0xFFFF) Limit_Temp = val;
    else W_Flash_Safe_Write_Int(AD_T_MAX, Limit_Temp);

    val = W_Flash_Read_Int(AD_PA_MAX);
    LOG_Printf("AD_PA_MAX raw: 0x%04X (%u)\r\n", val, val);
    if (val != 0xFFFF) Limit_Pressure = val;
    else W_Flash_Safe_Write_Int(AD_PA_MAX, Limit_Pressure);
    
    val = W_Flash_Read_Int(AD_PA_MIN);
    if (val != 0xFFFF) Limit_PressureMin = val;
    else W_Flash_Safe_Write_Int(AD_PA_MIN, Limit_PressureMin);

    val = W_Flash_Read_Int(AD_ADC_MIN);
    LOG_Printf("AD_ADC_MIN (VinMin) raw: 0x%04X (%u)\r\n", val, val);
    if (val != 0xFFFF) Limit_VinMin = val;
    else W_Flash_Safe_Write_Int(AD_ADC_MIN, Limit_VinMin);

    val = W_Flash_Read_Int(AD_ADC_MAX);
    LOG_Printf("AD_ADC_MAX (VinMax) raw: 0x%04X (%u)\r\n", val, val);
    if (val != 0xFFFF) Limit_VinMax = val;
    else W_Flash_Safe_Write_Int(AD_ADC_MAX, Limit_VinMax);

    val = W_Flash_Read_Int(AD_OIL_MAX);
    LOG_Printf("AD_OIL_MAX raw: 0x%04X (%u)\r\n", val, val);
    if (val != 0xFFFF) Limit_Oil = val;
    else W_Flash_Safe_Write_Int(AD_OIL_MAX, Limit_Oil);

    val = W_Flash_Read_Int(AD_TACHO_MAX);
    LOG_Printf("AD_TACHO_MAX raw: 0x%04X (%u)\r\n", val, val);
    if (val != 0xFFFF) Limit_RPM = val;
    else W_Flash_Safe_Write_Int(AD_TACHO_MAX, Limit_RPM);

    val = W_Flash_Read_Int(AD_TIME_BAODUONG);
    LOG_Printf("AD_TIME_BAODUONG raw: 0x%04X (%u)\r\n", val, val);
    if (val != 0xFFFF) Limit_MaintenanceTime = val;
    else W_Flash_Safe_Write_Int(AD_TIME_BAODUONG, Limit_MaintenanceTime);
    
    // Load Constants from Flash
    val = W_Flash_Read_Int(AD_VOLT_CONS);
    if (val != 0xFFFF) VOLT_CONS = val;
    else W_Flash_Safe_Write_Int(AD_VOLT_CONS, VOLT_CONS);

    val = W_Flash_Read_Int(AD_TEMP_CONS);
    if (val != 0xFFFF && val > 100) TEMP_CONS = val;
    else {
        TEMP_CONS = DEFAULT_TEMP_CONS;
        W_Flash_Safe_Write_Int(AD_TEMP_CONS, TEMP_CONS);
    }

    val = W_Flash_Read_Int(AD_PA_CONS);
    if (val != 0xFFFF) {
        if (val == 620) {
            PA_CONS = 372; // Tự động quy đổi mốc Zero cũ sang mạch chia áp mới
            W_Flash_Safe_Write_Int(AD_PA_CONS, PA_CONS);
        } else {
            PA_CONS = val;
        }
    } else {
        W_Flash_Safe_Write_Int(AD_PA_CONS, PA_CONS);
    }

    /* OIL_CONS: điều kiện val > 0 để chấp nhận mọi hệ số hợp lệ người dùng cài */
    val = W_Flash_Read_Int(AD_OIL_1_CONS);
    if (val != 0xFFFF && val > 0) OIL_1_CONS = val;
    else W_Flash_Safe_Write_Int(AD_OIL_1_CONS, OIL_1_CONS);

    val = W_Flash_Read_Int(AD_OIL_2_CONS);
    if (val != 0xFFFF && val > 0) OIL_2_CONS = val;
    else W_Flash_Safe_Write_Int(AD_OIL_2_CONS, OIL_2_CONS);

    val = W_Flash_Read_Int(AD_RETURN_BUBBLE_PCT);
    if (val != 0xFFFF && val <= 100) RETURN_BUBBLE_PCT = val;
    else W_Flash_Safe_Write_Int(AD_RETURN_BUBBLE_PCT, RETURN_BUBBLE_PCT);

    val = W_Flash_Read_Int(AD_FORWARD_BUBBLE_PCT);
    if (val != 0xFFFF && val <= 100) FORWARD_BUBBLE_PCT = val;
    else W_Flash_Safe_Write_Int(AD_FORWARD_BUBBLE_PCT, FORWARD_BUBBLE_PCT);

    LOG_Printf("-----------------------\r\n");

    // Log the configured alarm limits
    LOG_Printf("--- ALARM LIMITS CONFIGURATION ---\r\n");
    LOG_Printf("Limit Temp: %u.%u C\r\n", Limit_Temp / 10, Limit_Temp % 10);
    LOG_Printf("Limit Pressure MAX: %u.%u Bar\r\n", Limit_Pressure / 10, Limit_Pressure % 10);
    LOG_Printf("Limit Pressure MIN: %u.%u Bar\r\n", Limit_PressureMin / 10, Limit_PressureMin % 10);
    LOG_Printf("Limit Vin MIN: %u.%u V\r\n", Limit_VinMin / 10, Limit_VinMin % 10);
    LOG_Printf("Limit Vin MAX: %u.%u V\r\n", Limit_VinMax / 10, Limit_VinMax % 10);
    LOG_Printf("Limit Oil Flow: %u.%u L/h\r\n", Limit_Oil / 10, Limit_Oil % 10);
    LOG_Printf("Limit RPM: %u RPM\r\n", Limit_RPM);
    LOG_Printf("Limit Maintenance: %u hours\r\n", Limit_MaintenanceTime);
    LOG_Printf("----------------------------------\r\n");

    // Log the configured sensor constants
    LOG_Printf("--- SENSOR CONSTANTS ---\r\n");
    LOG_Printf("Volt  Cons : %u  (divisor Ohm)\r\n", VOLT_CONS);
    LOG_Printf("Temp  Cons : %u  (x1000)\r\n", TEMP_CONS);
    LOG_Printf("PA    Cons : %u\r\n", PA_CONS);
    LOG_Printf("Oil1  Cons : %u  (%.1f pulse/L)\r\n", OIL_1_CONS, (float)OIL_1_CONS / 10.0f);
    LOG_Printf("Oil2  Cons : %u  (%.1f pulse/L)\r\n", OIL_2_CONS, (float)OIL_2_CONS / 10.0f);
    LOG_Printf("Bubble FWD : %u%%\r\n", FORWARD_BUBBLE_PCT);
    LOG_Printf("Bubble RTN : %u%%\r\n", RETURN_BUBBLE_PCT);
    LOG_Printf("------------------------\r\n");
}

void AlarmManager_Task(void)
{
    // 1. Check alarm conditions
    gAlarmData.TempAlarm     = (gSensorData.Temperature >= Limit_Temp);
    // Cảnh báo nếu Áp suất vượt ngưỡng max HOẶC tụt dưới ngưỡng min
    gAlarmData.PressureAlarm = (gSensorData.Pressure >= Limit_Pressure) || (gSensorData.Pressure < Limit_PressureMin); 
    /* Cảnh báo điện áp: quá cao (>= VinMax) hoặc quá thấp (<= VinMin) */
    gAlarmData.VinAlarm      = (gSensorData.Vin >= Limit_VinMax) || (gSensorData.Vin <= Limit_VinMin);
    gAlarmData.RPMAlarm      = (gSensorData.RPM >= Limit_RPM);
    
    // For oil alarm, check if Net Flow (Forward - Return) exceeds limit
    uint16_t net_flow = (gSensorData.OilForward > gSensorData.OilReturn) ? (gSensorData.OilForward - gSensorData.OilReturn) : 0;
    gAlarmData.OilAlarm = (net_flow >= Limit_Oil);

    // Maintenance Alarm check (Limit is in hours, RunTime is in minutes)
    gAlarmData.MaintenanceAlarm = (gRuntimeData.RunTime >= ((uint32_t)Limit_MaintenanceTime * 60));
    
    // Check individual sensor maintenance limits (Limits are in hours, RunTimes are in minutes)
    bool sensorMaintAlarm = false;
    for(int i = 0; i < 5; i++) {
        if (Sensor_GetOperatingHours(i) >= ((uint32_t)gConfigData.SensorMaintLimits[i] * 60)) {
            sensorMaintAlarm = true;
            break;
        }
    }
    
    bool hasSevereAlarm = gAlarmData.TempAlarm || gAlarmData.PressureAlarm || gAlarmData.RPMAlarm;
    bool hasWarningAlarm = gAlarmData.VinAlarm || gAlarmData.OilAlarm;
    
    if (gConfigData.ControlMode == 1) {
        Led_maintain(true);
        Led_warning(true);
    } else {
        Led_maintain(gAlarmData.MaintenanceAlarm || sensorMaintAlarm);
    }

    static enum {
        BUZZER_STATE_OFF,
        BUZZER_STATE_WARNING,
        BUZZER_STATE_ALARM
    } currentBuzzerState = BUZZER_STATE_OFF;

    if (hasSevereAlarm)
    {
        if (currentBuzzerState != BUZZER_STATE_ALARM)
        {
            Buzzer_StartPattern(BUZZER_ALARM_ON_TIME, BUZZER_ALARM_OFF_TIME);
            currentBuzzerState = BUZZER_STATE_ALARM;
        }
        if (gConfigData.ControlMode != 1) Led_warning(true);
    }
    else if (hasWarningAlarm)
    {
        if (currentBuzzerState != BUZZER_STATE_WARNING)
        {
            Buzzer_StartPattern(BUZZER_WARNING_ON_TIME, BUZZER_WARNING_OFF_TIME);
            currentBuzzerState = BUZZER_STATE_WARNING;
        }
        if (gConfigData.ControlMode != 1) Led_warning(true);
    }
    else
    {
        if (currentBuzzerState != BUZZER_STATE_OFF)
        {
            Buzzer_StopPattern();
            currentBuzzerState = BUZZER_STATE_OFF;
        }
        if (gConfigData.ControlMode != 1) Led_warning(false);
    }
}
