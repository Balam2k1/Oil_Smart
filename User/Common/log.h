/******************************************************************************
 * File    : log.h
 * Brief   : Debug Log Module
 ******************************************************************************/

#ifndef LOG_H
#define LOG_H

#include <app.h>
#include <stdint.h>

/******************************************************************************
 * API
 ******************************************************************************/

/* Khởi tạo */
void LOG_Init(void);

/* In chuỗi */
void LOG_Print(const char *str);

/* In format giống printf */
void LOG_Printf(const char *fmt, ...);

void LOG_UART_Callback(void);
void LOG_Task(void);
void LOG_RestartReceive(void);

#endif /* LOG_H */
