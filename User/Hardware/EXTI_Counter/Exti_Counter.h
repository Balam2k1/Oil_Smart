#ifndef EXTI_COUNTER_H
#define EXTI_COUNTER_H

#include <stdint.h>

typedef struct {
    uint32_t freq_hz;
    float coef;
} CalibrationPoint_t;

/* Reset đếm xung dầu (thường gọi khi đã lưu xong 1 giờ) */
void ExtiCounter_Reset(void);

/* Init debounce (Cortex-M0: no-op; giữ API tương thích) */
void ExtiCounter_DWT_Init(void);

/* Lấy tổng số xung dầu đi (OIL_1) từ lần reset cuối */
uint32_t ExtiCounter_GetOil1(void);
uint32_t ExtiCounter_GetOil2(void);

uint32_t ExtiCounter_GetOil1_Hz(void);
uint32_t ExtiCounter_GetOil2_Hz(void);

float Get_Interpolated_Coef(uint32_t hz, float default_coef);

void ExtiCounter_1s_Process(void);
double ExtiCounter_GetTotalLiters(void);
void ExtiCounter_SetTotalLiters(double liters);

/* Get individual total liters */
double ExtiCounter_GetOil1Liters(void);
double ExtiCounter_GetOil2Liters(void);

/* Set individual total liters (used when restoring from Flash at startup) */
void ExtiCounter_SetOil1Liters(double liters);
void ExtiCounter_SetOil2Liters(double liters);

void ExtiCounter_SetCalibPoint(uint8_t index, uint32_t freq, float coef);
void ExtiCounter_GetCalibPoint(uint8_t index, uint32_t *freq, float *coef);

/* Bubble percentage in return/forward oil (0 to 100) */
extern uint16_t RETURN_BUBBLE_PCT;
extern uint16_t FORWARD_BUBBLE_PCT;

/* Set individual total pulses (used when restoring from Flash at startup) */
void ExtiCounter_SetOil1Pulses(uint32_t pulses);
void ExtiCounter_SetOil2Pulses(uint32_t pulses);

#endif
