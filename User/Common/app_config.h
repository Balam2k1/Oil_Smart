/******************************************************************************
 * File    : app_config.h
 * Brief   : Cấu hình hệ thống
 ******************************************************************************/

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "usart.h"
#include "stdbool.h"

/* ---------------------------------------------------------
 * Time
 */

#define Sec         (HAL_GetTick() / 1000)
#define mSec        HAL_GetTick()

/*----------------------------------------------------------
 * Sensor
 *---------------------------------------------------------*/
#define SENSOR_COUNT               5



/******************************************************************************
 * ADC CONFIG
 ******************************************************************************/

/* Tổng số kênh ADC sử dụng */
#define ADC_CHANNEL_COUNT      3

/* STM32 ADC Physical Channel */
#define ADC_PRESSURE_CHANNEL       ADC_CHANNEL_14
#define ADC_VIN_CHANNEL            ADC_CHANNEL_11
#define ADC_TEMP_CHANNEL           ADC_CHANNEL_7 // TODO: Channel tương ứng với chân cắm cảm biến nhiệt độ

/* Vin Divider Calculation:
 * R1 = 33K, R2 = 3.3K
 * vin = vout * (R1 + R2) / R2
 * Ratio = (33000 + 3300) / 3300 = 11
 */
#define VIN_DIVIDER_NUMERATOR      36300.0f
#define VIN_DIVIDER_DENOMINATOR      3300.0f



/*=========================================================
 * PRESSURE SENSOR (Linear 0.5 - 4.5V)
 *=========================================================*/

/* Global Variables */
uint16_t PA_VALUE __attribute__((weak)) = 0; // Raw ADC value
extern uint16_t PA_CONS;


/* Temperature */
#define TEMP_SENSOR_MIN_C            0.0f
#define TEMP_SENSOR_MAX_C          120.0f






/*=========================================================
 * RPM SENSOR
 *=========================================================*/

#define RPM_TIMER               (&htim3)
#define RPM_PULSE_PER_REV           1


/*=========================================================
 * OIL FLOW SENSOR
 * Dựa trên bảng đo thực tế: 21 Lít = 3453 xung.
 * Suy ra: 1 Lít = 3453 / 21 = 164.428 xung.
 * Để tính toán không cần số thập phân (float), ta nhân hệ số lên 10 lần.
 * -> Hệ số mặc định mới: 1644.
 *=========================================================*/

/* Dầu đi */
#define OIL_IN_PULSE_PER_LITER      1644
#define OIL_IN_TIMER            (&htim3)

/* Dầu về */
#define OIL_OUT_PULSE_PER_LITER     1644
#define OIL_OUT_TIMER           (&htim14)

/*
 * Đảo kênh logic dầu đi ↔ dầu hồi (không đổi chân GPIO).
 * Bật (=1) khi đo thực tế: xung/L/h hồi > đi trong lúc máy chạy
 * (ngược vật lý: dầu đi phải nhiều hơn dầu hồi).
 */
#define OIL_SWAP_CHANNELS           1


/*=========================================================
 * DEFAULT VALUES — Chỉnh tất cả giá trị mặc định TẠI ĐÂY
 *
 * Các giá trị này được dùng khi Flash chưa có dữ liệu
 * (lần đầu cài firmware hoặc sau khi xóa Flash).
 * Sau khi ghi vào Flash, giá trị Flash sẽ được ưu tiên.
 *=========================================================*/

/* --- Ngưỡng cảnh báo --- */
#define DEFAULT_LIMIT_TEMP          750     /* Nhiệt độ: 75.0 °C (x10) */
#define DEFAULT_LIMIT_PA_MAX        50      /* Áp suất max: 5.0 Bar (x10) */
#define DEFAULT_LIMIT_PA_MIN        25      /* Áp suất min: 2.5 Bar (x10) */
#define DEFAULT_LIMIT_VIN_MIN       110     /* Điện áp thấp: 11.0 V (x10) */
#define DEFAULT_LIMIT_VIN_MAX       150     /* Điện áp cao: 15.0 V (x10) */
#define DEFAULT_LIMIT_OIL           1000    /* Lưu lượng dầu min: 100.0 L/h (x10) */
#define DEFAULT_LIMIT_RPM           3000    /* Vòng tua: 3000 RPM */
#define DEFAULT_LIMIT_MAINT_TIME    5000    /* Giờ bảo dưỡng: 5000 giờ */

/* --- Hệ số cảm biến (Calibration Constants) ---
 * Lưu ý: Giá trị lưu trong Flash là x10 để tránh số thực.
 * Ví dụ: VOLT_CONS = 36300 tức là R1+R2 = 36300 Ohm
 *        OIL_CONS  = 1644  tức là 164.4 xung/lít            */
#define DEFAULT_VOLT_CONS           36300   /* Tổng trở chia áp (R1+R2) tính bằng Ohm */
#define DEFAULT_TEMP_CONS           750     /* Hệ số bù nhiệt (x1000) — 1000 = không bù */
#define DEFAULT_PA_CONS             372     /* Hệ số bù áp suất */
#define DEFAULT_OIL_1_CONS          1644    /* Xung/lít x10 — dầu đi  (164.4 xung/lít) */
#define DEFAULT_OIL_2_CONS          1644    /* Xung/lít x10 — dầu về  (164.4 xung/lít) */

/* --- Bù bọt khí (Bubble Compensation) --- */
#define DEFAULT_FORWARD_BUBBLE_PCT  0       /* % bọt khí dầu đi  (0-100) */
#define DEFAULT_RETURN_BUBBLE_PCT   0       /* % bọt khí dầu về  (0-100) */

/* --- Ghi log & hệ thống --- */
#define DEFAULT_LOG_INTERVAL        3600    /* Chu kỳ ghi log: 3600 giây (1 tiếng) */
#define DEFAULT_SENSOR_ENABLE_MASK  0xFF    /* Bật tất cả cảm biến */
#define DEFAULT_LED_INTENSITY       2       /* Độ sáng LED (0-15) */
#define DEFAULT_MAINT_HOURS         8760    /* Giờ bảo dưỡng cảm biến: 8760h = 1 năm */


/*----------------------------------------------------------
 * Alarm & LEDs
 *---------------------------------------------------------*/

/* Cấu hình chân LED báo trạng thái GPS */
#define GPS_LED_PORT               LED_GPS_GPIO_Port       // Port cho LED GPS (Bạn có thể sửa lại)
#define GPS_LED_PIN                LED_GPS_Pin 			// Chân cho LED GPS (Bạn có thể sửa lại)

/*----------------------------------------------------------
 * Scheduler
 *---------------------------------------------------------*/
#define TASK_SENSOR_PERIOD_MS      200

#define TASK_DISPLAY_PERIOD_MS     50

#define TASK_ALARM_PERIOD_MS       100

/*----------------------------------------------------------
 * Communication
 *---------------------------------------------------------*/

/*------------------------------------------------------------------
 * RS485 CONFIG
 *-----------------------------------------------------------------*/

/* UART dùng cho RS485 */
#define RS485_UART (&huart2)
#define RS485_TIMEOUT_MS           1000

/* UART dùng cho ESP32 */
#define ESP_UART (&huart1)

/*========================= CONFIG =========================*/

/* Số lượng cảm biến */
#define MODBUS_SENSOR_COUNT      5U

/* Chân DE */
#define RS485_DE_PORT            GPIOA
#define RS485_DE_PIN             GPIO_PIN_4

/* Buffer RX */
#define RS485_RX_BUFFER_SIZE     128U
/* Timeout xác định kết thúc frame Modbus */
#define RS485_FRAME_TIMEOUT_MS      5U

/******************************************************************************
 * LOG CONFIG
 ******************************************************************************/

#define LOG_UART            (&huart4)

#define LOG_ENABLE          1U

#define debug_main          true

/*=============================================================================
 * Max7219 DRIVER CONFIGURATION
 *============================================================================*/

//IC1
#define MAX7219_DIN1_PORT    GPIOA  		// Chan DIN PA0
#define MAX7219_DIN1_PIN     GPIO_PIN_0  	// PA0
#define MAX7219_CS1_PIN      GPIO_PIN_3  	// PC3
#define MAX7219_CLK1_PIN     GPIO_PIN_2  	// PC2
#define MAX7219_CLK1_PORT    GPIOC      	// Chân CLK ở PC2
#define MAX7219_CS1_PORT     GPIOC      	// Chân CS ở PC3

//IC2
#define MAX7219_DIN2_PORT    GPIOB  		// Chan DIN PB15
#define MAX7219_DIN2_PIN     GPIO_PIN_5  	// PB5
#define MAX7219_CS2_PIN      GPIO_PIN_4  	// PB4
#define MAX7219_CLK2_PIN     GPIO_PIN_3  	// PB3
#define MAX7219_CLK2_PORT    GPIOB      	// Chân CLK ở PB4
#define MAX7219_CS2_PORT     GPIOB      	// Chân CS ở PB3

//IC3
#define MAX7219_DIN3_PORT    GPIOB  		// Chan DIN PC6
#define MAX7219_DIN3_PIN     GPIO_PIN_15  	// PB15
#define MAX7219_CS3_PIN      GPIO_PIN_12  	// PC12
#define MAX7219_CLK3_PIN     GPIO_PIN_2  	// PD2
#define MAX7219_CLK3_PORT    GPIOD      	// Chân CLK ở PD2
#define MAX7219_CS3_PORT     GPIOC      	// Chân CS ở PC12

/*=============================================================================
 * HC595 DRIVER CONFIGURATION
 *============================================================================*/

/* You can change these defines to any GPIO port and pin */
#define HC595_DATA_PORT     GPIOC
#define HC595_DATA_PIN      GPIO_PIN_13

#define HC595_LATCH_PORT    GPIOC
#define HC595_LATCH_PIN     GPIO_PIN_14

#define HC595_CLOCK_PORT    GPIOC
#define HC595_CLOCK_PIN     GPIO_PIN_15

/*=============================================================================
 * SOFTWARE SPI (FLASH W25Q32) CONFIGURATION
 *============================================================================*/

#define FLASH_SPI_CS_PORT    GPIOC
#define FLASH_SPI_CS_PIN     GPIO_PIN_7

#define FLASH_SPI_SO_PORT    GPIOC
#define FLASH_SPI_SO_PIN     GPIO_PIN_8

#define FLASH_SPI_SI_PORT    GPIOC
#define FLASH_SPI_SI_PIN     GPIO_PIN_9

#define FLASH_SPI_SCK_PORT   GPIOA
#define FLASH_SPI_SCK_PIN    GPIO_PIN_8

/*=========================================================
 * BUZZER CONFIG
 *=========================================================*/

#define BUZZER_PORT     GPIOB
#define BUZZER_PIN      GPIO_PIN_9

#define BUZZER_ACTIVE_LEVEL   GPIO_PIN_SET

#define BUZZER_WARNING_ON_TIME      200U
#define BUZZER_WARNING_OFF_TIME     200U

#define BUZZER_ALARM_ON_TIME        500U
#define BUZZER_ALARM_OFF_TIME       100U






#endif
