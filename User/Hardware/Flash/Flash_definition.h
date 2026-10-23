#ifndef FLASH_FLASH_DEFINITION_H_
#define FLASH_FLASH_DEFINITION_H_
/********************************************************************************
KHAI BAO DIA CHI FLASH: 4MB (Winbond 25Q32)
********************************************************************************/
#define AD_START_PAGE_0                 0

#define AD_TIME_SUM                     (AD_START_PAGE_0 + 0)     // 4 bytes
#define AD_TIME_RUN                     (AD_START_PAGE_0 + 4)     // 4 bytes
#define AD_OIL_INDEX                    (AD_START_PAGE_0 + 8)     // 4 bytes (RecordCount - changed from 2 to 4 bytes for large capacity)
#define AD_LOG_INTERVAL                 (AD_START_PAGE_0 + 12)    // 2 bytes


#define AD_START_PAGE_1                 4096*1

#define AD_ADC_MIN                      (AD_START_PAGE_1 + 0)     // 2
#define AD_ADC_MAX                      (AD_START_PAGE_1 + 2)     // 2
#define AD_T_MAX                        (AD_START_PAGE_1 + 4)     // 2
#define AD_PA_MAX                       (AD_START_PAGE_1 + 6)     // 2
#define AD_OIL_MAX                      (AD_START_PAGE_1 + 8)     // 2
#define AD_TACHO_MAX                    (AD_START_PAGE_1 + 10)    // 2
#define AD_TIME_BAODUONG                (AD_START_PAGE_1 + 12)    // 2
#define AD_LED_INTENSITY                (AD_START_PAGE_1 + 14)    // 2
#define AD_BUZZER_MUTE                  (AD_START_PAGE_1 + 16)    // 2
#define AD_PA_MIN                       (AD_START_PAGE_1 + 18)    // 2


#define AD_START_PAGE_2                 4096*2
#define AD_KEY                          (AD_START_PAGE_2 + 0)     // 4
#define AD_VERSION                      (AD_START_PAGE_2 + 4)     // 4
#define AD_DEVICE_ID                    (AD_START_PAGE_2 + 8)     // 12 bytes (UID)

#define FIRMWARE_VERSION_VALUE          110   // Version 1.1.0 (110)

/* Constants in Page 3 */
#define AD_START_PAGE_3                 4096*3
#define AD_VOLT_CONS                    (AD_START_PAGE_3 + 0)     // 2
#define AD_TEMP_CONS                    (AD_START_PAGE_3 + 2)     // 2
#define AD_PA_CONS                      (AD_START_PAGE_3 + 4)     // 2
#define AD_OIL_1_CONS                   (AD_START_PAGE_3 + 6)     // 2
#define AD_OIL_2_CONS                   (AD_START_PAGE_3 + 8)     // 2
#define AD_SENSOR_MAINT_LIMIT_BASE      (AD_START_PAGE_3 + 10)    // 10 bytes (5 * 2)
#define AD_SENSOR_ENABLE_MASK           (AD_START_PAGE_3 + 20)    // 2 bytes
#define AD_SYSTEM_CONTROL_MODE          (AD_START_PAGE_3 + 22)    // 2 bytes
#define AD_SENSOR_RUNTIME_BASE          (AD_START_PAGE_3 + 30)    // 20 bytes (5 * 4 bytes)
#define AD_RETURN_BUBBLE_PCT            (AD_START_PAGE_3 + 50)    // 2 bytes
#define AD_FORWARD_BUBBLE_PCT           (AD_START_PAGE_3 + 52)    // 2 bytes

#define AD_START_PAGE_4                 4096*4
#define AD_OIL_MA_SIZE                  (AD_START_PAGE_4 + 0)     // 2 bytes
#define AD_OIL_DEBOUNCE                 (AD_START_PAGE_4 + 2)     // 2 bytes
#define AD_OIL_CALIB_TABLE              (AD_START_PAGE_4 + 10)    // 40 bytes (10 points * 4 bytes)

#define AD_START_PAGE_5                 4096*5
#define AD_START_OIL_RECORD             AD_START_PAGE_5           // Dữ liệu bắt đầu từ đây

#define AD_MAX_FLASH                    4194303                   // 4MB (W25Q32)

#endif

