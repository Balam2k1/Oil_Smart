#include "scheduler.h"
#include "stm32f0xx_hal.h"
#include "app.h"
#include "spi.h"
#include "esp32.h"
#include "log.h"
#include "ds1307.h"

static uint32_t LastSensorTick = 0;
static uint32_t LastDisplayTick = 0;
static uint32_t LastAlarmTick = 0;

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
    
    LOG_Printf("Init: OK. Entering Main Loop.\r\n");
    
    /* Đồng bộ tick khởi tạo */
    uint32_t now = HAL_GetTick();
    LastSensorTick = now;
    LastDisplayTick = now;
    LastAlarmTick = now;
}

void Scheduler_Run(void)
{
    uint32_t now = HAL_GetTick();
    SystemTime_Update();

    /* 1. Tác vụ đọc cảm biến (chu kỳ 200ms) */
    if ((now - LastSensorTick) >= TASK_SENSOR_PERIOD_MS)
    {
        LastSensorTick = now;
        SensorManager_Task();
    }

    /* 2. Tác vụ báo động (chu kỳ 100ms) */
    if ((now - LastAlarmTick) >= TASK_ALARM_PERIOD_MS)
    {
        LastAlarmTick = now;
        AlarmManager_Task();
    }

    /* 2.5. Tác vụ điều khiển LED Fan (chu kỳ 20ms) */
    static uint32_t LastFanTick = 0;
    if ((now - LastFanTick) >= 20)
    {
        LastFanTick = now;
        HC595_UpdateFan(gSensorData.RPM/100);
    }

    /* 3. Tác vụ hiển thị (chu kỳ 50ms) */
    if ((now - LastDisplayTick) >= TASK_DISPLAY_PERIOD_MS)
    {
        LastDisplayTick = now;
        DisplayManager_Task();
    }

    /* 4. Các tác vụ chạy liên tục (Non-blocking background tasks) */
    ModbusMaster_Task();
    RS485_Task();
    Buzzer_Task();
    ESP32_Task();
    LOG_Task();
}
