#ifndef EXTI_COUNTER_H
#define EXTI_COUNTER_H

#include <stdint.h>

/* Reset đếm xung dầu (thường gọi khi đã lưu xong 1 giờ) */
void ExtiCounter_Reset(void);

/* Lấy tổng số xung dầu đi (OIL_1) từ lần reset cuối */
uint32_t ExtiCounter_GetOil1(void);

/* Lấy tổng số xung dầu về (OIL_2) từ lần reset cuối */
uint32_t ExtiCounter_GetOil2(void);

/* Trả về chu kỳ (ms) giữa 2 xung dầu đi. Nếu bơm dừng (> 3 giây), trả về 0 */
uint32_t ExtiCounter_GetOil1_Period(void);

/* Trả về chu kỳ (ms) giữa 2 xung dầu về. Nếu bơm dừng (> 3 giây), trả về 0 */
uint32_t ExtiCounter_GetOil2_Period(void);

#endif
