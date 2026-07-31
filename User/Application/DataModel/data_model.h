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

/*=========================================================
 * Runtime
 *=========================================================*/
typedef struct
{
    uint32_t RunTime;

    uint32_t TotalTime;

}DataModel_Runtime_t;

/*=========================================================
 * Data Record (Flash Logging)
 *=========================================================*/
typedef struct
{
    int32_t Latitude;
    int32_t Longitude;
    uint32_t Timestamp;
    uint16_t OilConsumed;
    uint16_t Voltage;
    uint16_t RPM;
    uint16_t Temperature;
    uint16_t Pressure;
    uint8_t RecordType;    /* 0: Periodic log, 1: Startup log */
    uint32_t EngineRunTime; /* Thoi gian dong co chay trong chu ky log */
    uint8_t Reserved[5];   /* Pad to 32 bytes */
} DataRecord_t;

/*=========================================================
 * Global Data
 *=========================================================*/
extern DataModel_Sensor_t  gSensorData;
extern DataModel_Alarm_t   gAlarmData;
extern DataModel_Runtime_t gRuntimeData;

extern uint32_t gSystemUnixTime;
extern uint32_t gFirmwareVersion;
extern uint32_t gRecordCount;
extern uint32_t gLogInterval; /* in seconds */
extern uint16_t gEngineRunSeconds;
extern uint32_t gEngineRunSinceLastLog;

extern int32_t gLastLatitude;
extern int32_t gLastLongitude;

extern uint8_t gHardwareUID[12];
extern uint8_t gFlashUID[12];
extern bool gUidMatched;

extern uint8_t gSystemControlMode;
extern uint8_t gSensorEnableMask;
extern uint16_t gSensorMaintLimits[5];
extern uint32_t gSensorRunTimes[5];
extern uint8_t gLedIntensity;

void DataModel_Init(void);
void SystemTime_Update(void);
uint32_t DateTimeToUnix(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);

#endif
