#include "timer_counter.h"
#include "tim.h"

static volatile uint32_t tacho_capture_val1 = 0;
static volatile uint32_t tacho_capture_val2 = 0;
static volatile uint32_t tacho_period = 0;
static volatile uint8_t  is_first_capture = 0;

static volatile uint32_t last_capture_tick = 0;

#define TACHO_MA_SIZE 8
static volatile uint32_t tacho_periods[TACHO_MA_SIZE] = {0};
static volatile uint8_t  tacho_p_idx = 0;

void TimerCounter_Init(void)
{
    // Kích hoạt ngắt Input Capture cho TIM3 Channel 3 (chân PB0)
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_3);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
    {
        last_capture_tick = HAL_GetTick(); // Ghi nhận thời điểm có xung (để check timeout)

        if (is_first_capture == 0)
        {
            tacho_capture_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
            is_first_capture = 1;
        }
        else
        {
            tacho_capture_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
            uint32_t p = 0;
            if (tacho_capture_val2 > tacho_capture_val1)
            {
                p = tacho_capture_val2 - tacho_capture_val1;
            }
            else // Xảy ra khi Timer bị tràn (overflow qua 65535 về 0)
            {
                p = (0xFFFF - tacho_capture_val1) + tacho_capture_val2 + 1;
            }
            
            // Khởi tạo mảng lọc nhiễu nếu là lần đầu tiên
            if (tacho_periods[0] == 0) {
                 for(int i=0; i<TACHO_MA_SIZE; i++) tacho_periods[i] = p;
            }
            
            // Đưa vào mảng lọc trung bình (Moving Average)
            tacho_periods[tacho_p_idx] = p;
            tacho_p_idx = (tacho_p_idx + 1) % TACHO_MA_SIZE;
            
            uint32_t sum = 0;
            for(int i=0; i<TACHO_MA_SIZE; i++) sum += tacho_periods[i];
            tacho_period = sum / TACHO_MA_SIZE;
            
            tacho_capture_val1 = tacho_capture_val2;
        }
    }
}

uint32_t TimerCounter_GetTachoPeriod(void)
{
    // Nếu quá 1 giây (1000ms) không có xung nào đến (động cơ dừng), trả về chu kỳ = 0
    if ((HAL_GetTick() - last_capture_tick) > 1000)
    {
        is_first_capture = 0;
        tacho_periods[0] = 0; // Xóa dữ liệu lọc nhiễu cũ
        return 0;
    }
    
    return tacho_period;
}
