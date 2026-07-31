#ifndef __ESP32_H
#define __ESP32_H

#include "stdint.h"
#include "stdbool.h"

/* Khởi tạo UART1 cho ESP32 và các cấu hình liên quan */
void ESP32_Init(void);

/* Reset ESP32 thông qua chân PA15 */
void ESP32_Reset(void);

/* Task chạy trong vòng lặp chính (Scheduler) để xử lý dữ liệu nhận được */
void ESP32_Task(void);

/* Callback gọi trong ngắt UART RX khi có 1 byte đến */
void ESP32_UART_Callback(void);

/* Gửi bản tin tới ESP32 */
void ESP32_Send_Packet(uint8_t cmd, uint8_t len, uint8_t *data);

/* Restart Receive trong trường hợp có lỗi UART */
void ESP32_RestartReceive(void);

extern bool ESP32_LogEnable;

#endif /* __ESP32_H */
