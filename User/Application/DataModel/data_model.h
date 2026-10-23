#ifndef DATA_MODEL_H
#define DATA_MODEL_H

#include <stdint.h>
#include <stdbool.h>

/*=========================================================
 * Sensor Data
 *=========================================================*/
typedef struct
{
    /* Nhiệt độ */
    uint16_t Temperature;

    /* Điện áp nguồn */
    uint16_t Vin;

    /* Áp suất dầu */
    uint16_t Pressure;

    /* Lưu lượng dầu đi */
    uint16_t OilForward;

    /* Lưu lượng dầu về */
    uint16_t OilReturn;

    /* Tiêu hao dầu */
    uint16_t OilConsumption;

    /* Vòng tua */
    uint16_t RPM;

}DataModel_Sensor_t;

/*=========================================================
 * Alarm Status
 *=========================================================*/
typedef struct
{
    bool TempAlarm;

    bool PressureAlarm;

    bool VinAlarm;

    bool OilAlarm;

    bool RPMAlarm;

    bool MaintenanceAlarm;

}DataModel_Alarm_t;

typedef struct
{
    uint32_t UnixTime;
    uint32_t FirmwareVersion;
    uint32_t RecordCount;
    uint8_t  HardwareUID[12];
    uint8_t  FlashUID[12];
    bool     UidMatched;
} DataModel_System_t;

typedef struct
{
    uint32_t LogInterval; /* in seconds */
    uint8_t  ControlMode;
    uint8_t  SensorEnableMask;
    uint16_t SensorMaintLimits[5];
    uint8_t  LedIntensity;
} DataModel_Config_t;

typedef struct
{
    int32_t Latitude;
    int32_t Longitude;
} DataModel_Location_t;

typedef struct
{
    uint32_t RunTime;
    uint32_t TotalTime;
    uint16_t EngineRunSeconds;
    uint32_t EngineRunSinceLastLog;
    uint32_t SensorRunTimes[5];
} DataModel_Runtime_t;

typedef struct
{
    int32_t Latitude;           // 4
    int32_t Longitude;          // 4
    uint32_t Timestamp;         // 4
    uint32_t Oil1_TotalPulses;  // 4
    uint32_t Oil2_TotalPulses;  // 4
    uint32_t Total_Oil_Consumed;// 4
    uint16_t Voltage;           // 2
    uint16_t RPM;               // 2
    uint16_t Temperature;       // 2
    uint16_t Pressure;          // 2
    uint16_t EngineRunTime;     // 2
    uint8_t RecordType;         // 1
    uint8_t Padding;            // 1 (explicit alignment)
    uint32_t Oil1_TotalLiters;  // 4 (Liters * 100)
    uint32_t Oil2_TotalLiters;  // 4 (Liters * 100)
    uint8_t Reserved[20];       // 20
} DataRecord_t;

/*=========================================================
 * Global Data
 *=========================================================*/
extern DataModel_Sensor_t   gSensorData;
extern DataModel_Alarm_t    gAlarmData;
extern DataModel_Runtime_t  gRuntimeData;
extern DataModel_System_t   gSystemData;
extern DataModel_Config_t   gConfigData;
extern DataModel_Location_t gLocationData;

void DataModel_Init(void);
void SystemTime_Update(void);
uint32_t DateTimeToUnix(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);

#endif
