#include "Exti_Counter.h"
#include "main.h"
#include "app_config.h"
#include "Flash_definition.h"

extern uint16_t OIL_1_CONS;
extern uint16_t OIL_2_CONS;

/*===========================================================================
 * Internal state
 *===========================================================================*/

/* Tổng số xung tích lũy (volatile vì được ghi trong EXTI ISR) */
static volatile uint32_t oil1_total_pulses = 0;
static volatile uint32_t oil2_total_pulses = 0;

/* Snapshot xung của giây trước — dùng để tính Hz */
static uint32_t last_oil1_total = 0;
static uint32_t last_oil2_total = 0;

/* Hz thô trong giây vừa qua (trước khi lọc) */
static uint32_t last_diff1 = 0;
static uint32_t last_diff2 = 0;

/*===========================================================================
 * Bộ lọc EMA (Exponential Moving Average) — làm mượt Hz hiển thị
 *
 * smooth = alpha * new_raw + (1 - alpha) * prev_smooth
 *
 * EMA_ALPHA_x10 = 3  →  alpha = 0.3  (phản hồi nhanh, mượt vừa)
 * Giảm xuống 2 nếu muốn mượt hơn; tăng lên 4-5 nếu muốn phản hồi nhanh hơn.
 *
 * ZERO_TIMEOUT_SEC: Số giây diff=0 liên tiếp trước khi báo flow = 0.
 * Giá trị 3 giây phù hợp cho cảm biến Hall/Turbine thường dùng.
 *===========================================================================*/
#define EMA_ALPHA_x10        3U   /* alpha = 0.3 */
#define ZERO_TIMEOUT_SEC     3U   /* giây */

static float    smooth_hz1 = 0.0f;
static float    smooth_hz2 = 0.0f;
static uint8_t  zero_cnt1  = 0;
static uint8_t  zero_cnt2  = 0;

/* Tổng lít tích lũy — nhất quán với L/h realtime (dùng calibration table) */
static double oil1_total_liters = 0.0;
static double oil2_total_liters = 0.0;

/* Bảng hiệu chỉnh và hệ số bọt khí */
CalibrationPoint_t g_calib_table[10] = {0};
uint16_t RETURN_BUBBLE_PCT  = 0;
uint16_t FORWARD_BUBBLE_PCT = 0;

/*===========================================================================
 * EXTI Callback — đếm xung rising edge với software debounce
 *
 * STM32F030 (Cortex-M0) không có DWT/CYCCNT → dùng HAL_GetTick() (1 ms).
 * Nếu 2 rising edge cách nhau < DEBOUNCE_MS → bỏ qua (nhiễu ringing).
 *
 * Cảm biến Hall/Turbine thường >= 5 ms giữa 2 xung thật ở lưu lượng cao.
 *===========================================================================*/

#define DEBOUNCE_MS     2U   /* ms — tăng nếu cảm biến bị nhiễu nhiều hơn */

static uint32_t oil1_last_tick = 0;
static uint32_t oil2_last_tick = 0;

/* Giữ API cũ; Cortex-M0 không cần init phần cứng cho debounce */
void ExtiCounter_DWT_Init(void)
{
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == OIL_1_Pin)
    {
        if (HAL_GPIO_ReadPin(OIL_1_GPIO_Port, OIL_1_Pin) == GPIO_PIN_RESET) return;

        uint32_t now = HAL_GetTick();
        if ((now - oil1_last_tick) >= DEBOUNCE_MS)
        {
            oil1_last_tick = now;
            oil1_total_pulses++;
        }
        /* else: bỏ qua — nhiễu ringing */
    }
    else if (GPIO_Pin == OIL_2_Pin)
    {
        if (HAL_GPIO_ReadPin(OIL_2_GPIO_Port, OIL_2_Pin) == GPIO_PIN_RESET) return;

        uint32_t now = HAL_GetTick();
        if ((now - oil2_last_tick) >= DEBOUNCE_MS)
        {
            oil2_last_tick = now;
            oil2_total_pulses++;
        }
    }
}

/*===========================================================================
 * Reset — xóa sạch TẤT CẢ biến để tránh underflow sau khi ghi log
 *===========================================================================*/

void ExtiCounter_Reset(void)
{
    __disable_irq();
    oil1_total_pulses = 0;
    oil2_total_pulses = 0;
    last_oil1_total   = 0;   /* FIX: đồng bộ snapshot tránh underflow uint32 */
    last_oil2_total   = 0;
    last_diff1        = 0;
    last_diff2        = 0;
    smooth_hz1        = 0.0f;
    smooth_hz2        = 0.0f;
    zero_cnt1         = 0;
    zero_cnt2         = 0;
    oil1_last_tick    = 0;
    oil2_last_tick    = 0;
    oil1_total_liters = 0.0;
    oil2_total_liters = 0.0;
    __enable_irq();
}

/*===========================================================================
 * Getters — xung và Hz
 *===========================================================================*/

uint32_t ExtiCounter_GetOil1(void) { return oil1_total_pulses; }
uint32_t ExtiCounter_GetOil2(void) { return oil2_total_pulses; }

/* Trả về Hz đã lọc EMA — dùng để hiển thị và tính L/h */
uint32_t ExtiCounter_GetOil1_Hz(void) { return (uint32_t)(smooth_hz1 + 0.5f); }
uint32_t ExtiCounter_GetOil2_Hz(void) { return (uint32_t)(smooth_hz2 + 0.5f); }

/* Trả về Hz thô (chưa lọc) — dùng cho calibration table nội suy */
uint32_t ExtiCounter_GetOil1_Hz_Raw(void) { return last_diff1; }
uint32_t ExtiCounter_GetOil2_Hz_Raw(void) { return last_diff2; }

/*===========================================================================
 * Calibration Table — nội suy tuyến tính
 *===========================================================================*/

float Get_Interpolated_Coef(uint32_t hz, float default_coef)
{
    /* Bảng rỗng → dùng hệ số mặc định */
    if (g_calib_table[0].freq_hz == 0 && g_calib_table[0].coef < 1.0f)
        return default_coef;

    for (int i = 0; i < 9; i++)
    {
        uint32_t f1 = g_calib_table[i].freq_hz;
        float    c1 = g_calib_table[i].coef;
        uint32_t f2 = g_calib_table[i + 1].freq_hz;
        float    c2 = g_calib_table[i + 1].coef;

        if (f1 == 0 && c1 < 1.0f) break;               /* Hết dữ liệu */
        if (i == 0 && hz <= f1)   return c1;            /* Nhỏ hơn Min */
        if (f2 == 0 && c2 < 1.0f && hz >= f1) return c1; /* Lớn hơn Max */

        if (hz >= f1 && hz <= f2)
        {
            if (f2 == f1) return c1;
            float ratio = (float)(hz - f1) / (float)(f2 - f1);
            return c1 + ratio * (c2 - c1);
        }
    }

    /* Lớn hơn điểm cuối cùng */
    if (g_calib_table[9].freq_hz > 0 || g_calib_table[9].coef >= 1.0f)
        return g_calib_table[9].coef;

    return default_coef;
}

/*===========================================================================
 * ExtiCounter_1s_Process — gọi mỗi 1 giây bởi Scheduler
 *
 * Tính Hz và tích lũy lít bằng calibration table (nhất quán với L/h realtime).
 *===========================================================================*/

void ExtiCounter_1s_Process(void)
{
    uint32_t current_oil1 = oil1_total_pulses;
    uint32_t current_oil2 = oil2_total_pulses;

    uint32_t diff1 = current_oil1 - last_oil1_total;
    uint32_t diff2 = current_oil2 - last_oil2_total;

    last_oil1_total = current_oil1;
    last_oil2_total = current_oil2;
    last_diff1      = diff1;
    last_diff2      = diff2;

    /*-----------------------------------------------------------------------
     * Bộ lọc EMA + Zero Suppression Timeout
     *
     * - Khi có xung (diff > 0): cập nhật EMA ngay lập tức → phản hồi nhanh
     *   smooth = 0.3 * diff + 0.7 * smooth
     *
     * - Khi không có xung (diff == 0): tăng bộ đếm zero_cnt.
     *   Chỉ đưa smooth về 0 sau ZERO_TIMEOUT_SEC giây liên tục không có xung.
     *   → Tránh nhảy về 0 khi xung thưa hoặc rơi lệch cửa sổ 1s.
     *-----------------------------------------------------------------------*/
    /* --- OIL 1 (dầu đi) --- */
    if (diff1 > 0)
    {
        smooth_hz1 = (EMA_ALPHA_x10 * (float)diff1 + (10U - EMA_ALPHA_x10) * smooth_hz1)
                     / 10.0f;
        zero_cnt1  = 0;
    }
    else
    {
        zero_cnt1++;
        if (zero_cnt1 >= ZERO_TIMEOUT_SEC)
        {
            smooth_hz1 = 0.0f;  /* Dừng hẳn — không còn lưu lượng */
            zero_cnt1  = ZERO_TIMEOUT_SEC; /* Giữ nguyên, không tràn */
        }
        /* else: giữ nguyên smooth_hz1 (không cập nhật) */
    }

    /* --- OIL 2 (dầu hồi) --- */
    if (diff2 > 0)
    {
        smooth_hz2 = (EMA_ALPHA_x10 * (float)diff2 + (10U - EMA_ALPHA_x10) * smooth_hz2)
                     / 10.0f;
        zero_cnt2  = 0;
    }
    else
    {
        zero_cnt2++;
        if (zero_cnt2 >= ZERO_TIMEOUT_SEC)
        {
            smooth_hz2 = 0.0f;
            zero_cnt2  = ZERO_TIMEOUT_SEC;
        }
    }

    /*-----------------------------------------------------------------------
     * Tích lũy lít — dùng diff1/diff2 thô (không dùng EMA) để đảm bảo
     * tổng lít chính xác, không bị biến dạng do lọc.
     *-----------------------------------------------------------------------*/
    if (diff1 > 0)
    {
        float base1  = (OIL_1_CONS > 0) ? ((float)OIL_1_CONS / 10.0f)
                                         : ((float)OIL_IN_PULSE_PER_LITER / 10.0f);
        float coef1  = Get_Interpolated_Coef(diff1, base1);
        double delta = (double)diff1 / (double)coef1;
        delta *= (100.0 - (double)FORWARD_BUBBLE_PCT) / 100.0;
        oil1_total_liters += delta;
    }

    if (diff2 > 0)
    {
        float base2  = (OIL_2_CONS > 0) ? ((float)OIL_2_CONS / 10.0f)
                                         : ((float)OIL_OUT_PULSE_PER_LITER / 10.0f);
        float coef2  = Get_Interpolated_Coef(diff2, base2);
        double delta = (double)diff2 / (double)coef2;
        delta *= (100.0 - (double)RETURN_BUBBLE_PCT) / 100.0;
        oil2_total_liters += delta;
    }
}

/*===========================================================================
 * Liters — getters / setters
 *===========================================================================*/

double ExtiCounter_GetOil1Liters(void) { return oil1_total_liters; }
double ExtiCounter_GetOil2Liters(void) { return oil2_total_liters; }

void ExtiCounter_SetOil1Liters(double liters) { oil1_total_liters = liters; }
void ExtiCounter_SetOil2Liters(double liters) { oil2_total_liters = liters; }

double ExtiCounter_GetTotalLiters(void)
{
    double net = oil1_total_liters - oil2_total_liters;
    return (net > 0.0) ? net : 0.0;
}

/* FIX: Hàm này không còn rỗng — reset bộ đếm lít về 0 sau mỗi lần ghi log */
void ExtiCounter_SetTotalLiters(double liters)
{
    oil1_total_liters = liters;
    oil2_total_liters = 0.0;
}

/*===========================================================================
 * Pulses — setters (dùng khi khôi phục trạng thái từ Flash khi khởi động)
 *===========================================================================*/

void ExtiCounter_SetOil1Pulses(uint32_t pulses)
{
    __disable_irq();
    oil1_total_pulses = pulses;
    last_oil1_total   = pulses; /* FIX: đồng bộ snapshot tránh underflow */
    smooth_hz1        = 0.0f;  /* Reset bộ lọc khi restore từ Flash */
    zero_cnt1         = 0;
    __enable_irq();
}

void ExtiCounter_SetOil2Pulses(uint32_t pulses)
{
    __disable_irq();
    oil2_total_pulses = pulses;
    last_oil2_total   = pulses;
    smooth_hz2        = 0.0f;
    zero_cnt2         = 0;
    __enable_irq();
}

/*===========================================================================
 * Calibration Table — lưu / đọc Flash
 *===========================================================================*/

void ExtiCounter_SetCalibPoint(uint8_t index, uint32_t freq, float coef)
{
    if (index >= 10) return;

    g_calib_table[index].freq_hz = freq;
    g_calib_table[index].coef    = coef;

    extern void W_Flash_Sector_Erase(uint32_t sector_address);
    extern void W_Flash_Write_Arr(uint32_t WriteAddr, const uint8_t *pBuffer, uint16_t NumByteToWrite);

    uint32_t flash_addr  = AD_OIL_CALIB_TABLE;
    uint32_t sector_base = flash_addr & 0xFFFFF000;
    W_Flash_Sector_Erase(sector_base);
    W_Flash_Write_Arr(flash_addr, (const uint8_t *)g_calib_table, sizeof(g_calib_table));
}

void ExtiCounter_GetCalibPoint(uint8_t index, uint32_t *freq, float *coef)
{
    if (index >= 10) return;
    *freq = g_calib_table[index].freq_hz;
    *coef = g_calib_table[index].coef;
}

