#include "scheduler.h"
#include "stm32f0xx_hal.h"
#include "app.h"
#include "spi.h"
#include "esp32.h"
#include "log.h"
#include "ds1307.h"

#define MAX_TASKS 15

typedef struct {
    TaskFunction_t pFunc;
    uint32_t PeriodMs;
    uint32_t LastTick;
} Task_t;

static Task_t Tasks[MAX_TASKS];
static uint8_t TaskCount = 0;

void Scheduler_AddTask(TaskFunction_t pFunc, uint32_t PeriodMs) {
    if (TaskCount < MAX_TASKS) {
        Tasks[TaskCount].pFunc = pFunc;
        Tasks[TaskCount].PeriodMs = PeriodMs;
        Tasks[TaskCount].LastTick = HAL_GetTick();
        TaskCount++;
    }
}

static void FanControl_Task(void) {
    HC595_UpdateFan(gSensorData.RPM/100);
}

extern void ExtiCounter_1s_Process(void);

void Scheduler_Init(void)
{
    /* Đưa chân CS của Flash lên mức cao (Idle) trước tiên */
    spi_cs_high();
    
    // Enable LOG output first so we can see the boot sequence
    LOG_Init();
    LOG_Printf("\r\n=== SYSTEM BOOTING ===\r\n");

    /* TEST SPI FLASH CONNECTION */
    spi_cs_low();
    uint8_t tx_id[1] = { 0x9F };
    uint8_t rx_id[3] = {0};
    extern void SPI_Software_Transmit_Receive(const uint8_t *txData, uint8_t *rxData, uint16_t length);
    SPI_Software_Transmit_Receive(tx_id, NULL, 1);
    SPI_Software_Transmit_Receive(NULL, rx_id, 3);
    spi_cs_high();
    LOG_Printf("Flash JEDEC ID: %02X %02X %02X\r\n", rx_id[0], rx_id[1], rx_id[2]);
    if (rx_id[0] == 0xFF || rx_id[0] == 0x00) {
        LOG_Printf("WARNING: SPI FLASH NOT RESPONDING PROPERLY!\r\n");
    }

    LOG_Printf("Init: DataModel...\r\n");
    DataModel_Init();

    LOG_Printf("Init: SensorManager...\r\n");
    SensorManager_Init();
    
    LOG_Printf("Init: DisplayManager...\r\n");
    DisplayManager_Init();
    
    LOG_Printf("Init: AlarmManager...\r\n");
    AlarmManager_Init();
    
    LOG_Printf("Init: ModbusMaster...\r\n");
    ModbusMaster_Init();
    
    LOG_Printf("Init: RS485...\r\n");
    RS485_Init();
    
    LOG_Printf("Init: Buzzer...\r\n");
    Buzzer_Init();
    
    LOG_Printf("Init: MAX7219...\r\n");
    MAX7219_InitAll();
    
    LOG_Printf("Init: HC595...\r\n");
    HC595_Init();
    
    LOG_Printf("Checking Sensor Maintenance Limits...\r\n");
    DisplayManager_StartupBlink();
    
    LOG_Printf("Init: GPS_TinyGPS...\r\n");
    GPS_TinyGPS();
    
    LOG_Printf("Init: GPS...\r\n");
    GPS_Init();
    
    LOG_Printf("Init: DS1307...\r\n");
    DS1307_Init();
    
    LOG_Printf("Init: ESP32...\r\n");
    ESP32_Init();
    
    LOG_Printf("Init: OK. Registering Tasks...\r\n");

    /* Đăng ký các tác vụ định kỳ */
    Scheduler_AddTask(SensorManager_Task, TASK_SENSOR_PERIOD_MS);
    Scheduler_AddTask(ExtiCounter_1s_Process, 1000);
    Scheduler_AddTask(AlarmManager_Task, TASK_ALARM_PERIOD_MS);
    Scheduler_AddTask(FanControl_Task, 20);
    Scheduler_AddTask(DisplayManager_Task, TASK_DISPLAY_PERIOD_MS);

    /* Đăng ký các tác vụ chạy liên tục (Non-blocking background tasks) */
    Scheduler_AddTask(SystemTime_Update, 0); // Vốn dùng để update system time, nhưng hàm tự check 1000ms bên trong
    Scheduler_AddTask(ModbusMaster_Task, 0);
    Scheduler_AddTask(RS485_Task, 0);
    Scheduler_AddTask(Buzzer_Task, 0);
    Scheduler_AddTask(ESP32_Task, 0);
    Scheduler_AddTask(LOG_Task, 0);
}

void Scheduler_Run(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < TaskCount; i++) {
        if (Tasks[i].PeriodMs == 0) {
            Tasks[i].pFunc();
        } else {
            if ((now - Tasks[i].LastTick) >= Tasks[i].PeriodMs) {
                Tasks[i].LastTick = now;
                Tasks[i].pFunc();
            }
        }
    }
}
