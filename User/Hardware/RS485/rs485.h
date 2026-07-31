/******************************************************************************
 * File    : rs485.h
 * Brief   : RS485 Physical Layer Driver
 ******************************************************************************/

#ifndef __RS485_H
#define __RS485_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"


/*============================= API ================================*/

/**
 * @brief Khởi tạo driver RS485
 */
void RS485_Init(void);

/**
 * @brief Task xử lý timeout frame
 *
 * Gọi liên tục trong Scheduler
 */
void RS485_Task(void);

/**
 * @brief Gửi dữ liệu lên bus RS485
 *
 * @param data    Buffer truyền
 * @param length  Số byte truyền
 *
 * @return 0 = OK
 */
uint8_t RS485_Send(
        const uint8_t *data,
        uint16_t length);

/**
 * @brief Xóa frame nhận hiện tại
 */
void RS485_ClearFrame(void);

/**
 * @brief Kiểm tra đã nhận đủ frame chưa
 *
 * @return
 *      1 : Có frame mới
 *      0 : Chưa có
 */
uint8_t RS485_IsFrameReady(void);

/**
 * @brief Lấy con trỏ tới buffer RX
 */
uint8_t* RS485_GetRxBuffer(void);

/**
 * @brief Lấy độ dài frame RX
 */
uint16_t RS485_GetRxLength(void);

/**
 * @brief Hàm xử lý byte nhận từ UART Interrupt
 *
 * Gọi từ HAL_UART_RxCpltCallback()
 */
void RS485_RxHandler(uint8_t byte);

/* Callback UART */
void RS485_UART_Callback(void);


void RS485_Test(void);
void RS485_EchoTest(void);


#ifdef __cplusplus
}
#endif

#endif
