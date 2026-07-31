#ifndef TIMER_COUNTER_H
#define TIMER_COUNTER_H

#include <stdint.h>

/* Khởi động Input Capture trên TIM3 */
void TimerCounter_Init(void);

/* Lấy chu kỳ (delta time) của TACHO bằng phần cứng. Trả về số Timer tick */
uint32_t TimerCounter_GetTachoPeriod(void);

#endif
