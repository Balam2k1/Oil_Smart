#include "Exti_Counter.h"
#include "main.h"
#include "app_config.h"

extern uint16_t OIL_1_CONS;
extern uint16_t OIL_2_CONS;

// Tính thời gian lọc nhiễu linh hoạt theo hệ số xung của người dùng.
// Giả định lưu lượng lớn nhất là 2000 L/h (bơm Dầu Đi luôn lớn hơn nhiều so với lượng tiêu thụ).
static uint32_t get_debounce_ms(uint8_t is_return) {
    // Cố định thời gian chặn nhiễu ở mức 60ms cho cả 2 kênh để chặn sườn xuống.
    return 20;
}

static volatile uint32_t oil1_total_pulses = 0;
static volatile uint32_t oil2_total_pulses = 0;

static volatile uint32_t oil1_last_tick = 0;
static volatile uint32_t oil2_last_tick = 0;
static volatile uint32_t oil1_last_valid_tick = 0;
static volatile uint32_t oil2_last_valid_tick = 0;

#define PERIOD_MA_SIZE 64

static volatile uint32_t oil1_periods[PERIOD_MA_SIZE] = {0};
static volatile uint8_t  oil1_p_idx = 0;
static volatile uint32_t oil1_period_avg = 0;

static volatile uint32_t oil2_periods[PERIOD_MA_SIZE] = {0};
static volatile uint8_t  oil2_p_idx = 0;
static volatile uint32_t oil2_period_avg = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t now = HAL_GetTick();
    
    // Bắt ngắt cho OIL 1 (Cảm biến dầu đi)
    if (GPIO_Pin == OIL_1_Pin) 
    {
        uint32_t debounce_limit = get_debounce_ms(0);
        uint32_t time_since_last_valid = now - oil1_last_valid_tick;
        
        if (oil1_last_valid_tick == 0 || time_since_last_valid > debounce_limit) {
            if (oil1_last_valid_tick != 0 && time_since_last_valid > 0) {
                if (oil1_periods[0] == 0) {
                    for(int i=0; i<PERIOD_MA_SIZE; i++) oil1_periods[i] = time_since_last_valid;
                }
                oil1_periods[oil1_p_idx] = time_since_last_valid;
                oil1_p_idx = (oil1_p_idx + 1) % PERIOD_MA_SIZE;
                
                uint32_t sum = 0;
                for(int i=0; i<PERIOD_MA_SIZE; i++) sum += oil1_periods[i];
                oil1_period_avg = sum / PERIOD_MA_SIZE;
            }
            
            oil1_last_valid_tick = now;
            oil1_total_pulses++;
        }
    }
    
    // Bắt ngắt cho OIL 2 (Cảm biến dầu về)
    else if (GPIO_Pin == OIL_2_Pin)
    {
        uint32_t debounce_limit = get_debounce_ms(1);
        uint32_t time_since_last_valid = now - oil2_last_valid_tick;
        
        if (oil2_last_valid_tick == 0 || time_since_last_valid > debounce_limit) {
            if (oil2_last_valid_tick != 0 && time_since_last_valid > 0) {
                if (oil2_periods[0] == 0) {
                    for(int i=0; i<PERIOD_MA_SIZE; i++) oil2_periods[i] = time_since_last_valid;
                }
                oil2_periods[oil2_p_idx] = time_since_last_valid;
                oil2_p_idx = (oil2_p_idx + 1) % PERIOD_MA_SIZE;
                
                uint32_t sum = 0;
                for(int i=0; i<PERIOD_MA_SIZE; i++) sum += oil2_periods[i];
                oil2_period_avg = sum / PERIOD_MA_SIZE;
            }
            
            oil2_last_valid_tick = now;
            oil2_total_pulses++;
        }
    }
}

void ExtiCounter_Reset(void)
{
    __disable_irq();
    oil1_total_pulses = 0;
    oil2_total_pulses = 0;
    __enable_irq();
}

uint32_t ExtiCounter_GetOil1(void)
{
    return oil1_total_pulses;
}

uint32_t ExtiCounter_GetOil2(void)
{
    return oil2_total_pulses;
}

uint32_t ExtiCounter_GetOil1_Period(void)
{
    uint32_t now = HAL_GetTick();
    // Thay vì dùng oil1_last_tick (bị reset liên tục bởi nhiễu), dùng oil1_last_valid_tick!
    uint32_t elapsed = now - oil1_last_valid_tick;
    
    if (elapsed > 10000) { // Timeout 10s tuyệt đối -> Min flow rate ~3.6 L/h
        oil1_period_avg = 0;
        for(int i=0; i<PERIOD_MA_SIZE; i++) oil1_periods[i] = 0;
        return 0;
    }
    
    // Thuật toán Ép rớt dốc (Decay):
    if (elapsed > oil1_period_avg && oil1_period_avg != 0) {
        return elapsed;
    }
    
    return oil1_period_avg;
}

uint32_t ExtiCounter_GetOil2_Period(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - oil2_last_valid_tick;
    
    if (elapsed > 10000) { // Timeout 10s tuyệt đối
        oil2_period_avg = 0;
        for(int i=0; i<PERIOD_MA_SIZE; i++) oil2_periods[i] = 0;
        return 0;
    }
    
    if (elapsed > oil2_period_avg && oil2_period_avg != 0) {
        return elapsed;
    }
    
    return oil2_period_avg;
}
