#include "sensor_manager.h"
#include "modbus_master.h"
#include "app_config.h"
#include "adc_driver.h"
#include "data_model.h"
#include "timer_counter.h"
#include "Exti_Counter.h"
#include "ds1307.h"
#include "tinyGPS.h"
#include "string.h"
#include "Flash_definition.h"
#include "w25q32.h"
#include "kalman_filter.h"

/* Hệ số hiệu chỉnh lưu lượng (từ Flash) */
extern uint16_t OIL_1_CONS;
extern uint16_t OIL_2_CONS;
extern uint16_t RETURN_BUBBLE_PCT;
extern uint16_t FORWARD_BUBBLE_PCT;

Kalman1D_t kalman_rpm;

static uint32_t LastTick = 0;

static uint16_t RawData[MODBUS_SENSOR_COUNT];
static uint16_t OperatingHours[MODBUS_SENSOR_COUNT];
static uint16_t SensorType[MODBUS_SENSOR_COUNT];
static uint32_t SerialNumber[MODBUS_SENSOR_COUNT];
static uint8_t  Online[MODBUS_SENSOR_COUNT];

bool SENSOR_LogEnable = false;
bool GPS_LogEnable = false;
bool FLASH_LogEnable = false;

void SensorManager_Init(void)
{
    ExtiCounter_DWT_Init(); /* Debounce init (no-op trên Cortex-M0) */
    ADC_Driver_Init();
    TimerCounter_Init();
    
    // Khởi tạo bộ lọc Kalman (Q, R, P, Giá trị ban đầu)
    // RPM cần phản hồi cực nhanh, nên dùng Kalman là chuẩn bài.
    Kalman_Init(&kalman_rpm, 10.0f, 50.0f, 1.0f, 0.0f);
}

static void SensorManager_UpdateModbus(void)
{
    uint8_t i;

    for(i=0;i<MODBUS_SENSOR_COUNT;i++)
    {
        ModbusSensor_t* sensor;

        sensor = ModbusMaster_GetSensor(i);

        Online[i] = sensor->Online;

        if(sensor->Online)
        {
            RawData[i] = sensor->RawData;
            OperatingHours[i] = sensor->OperatingHours;
            SensorType[i] = sensor->SensorType;
            SerialNumber[i] = ((uint32_t)sensor->SerialNumH << 16) | sensor->SerialNumL;
        }
    }
}

uint16_t Sensor_CalcRPM_Modbus(uint16_t raw)
{
    // Type 2: raw (vòng/phut)
    uint32_t current_rpm = (uint32_t)raw;
    return current_rpm;

}

uint16_t Sensor_CalcRPM_Local(uint32_t period)
{
    if (period == 0) return 0;
    uint32_t inst_rpm = (6000000 / period / RPM_PULSE_PER_REV);
    float smoothed_rpm = Kalman_Update(&kalman_rpm, (float)inst_rpm);
    return (uint16_t)smoothed_rpm;
}

uint16_t Sensor_CalcOil_Modbus(uint16_t raw)
{
    /* Stub: Modbus oil chưa chuyển được raw → L/h.
     * Hiển thị/app PHẢI dùng Sensor_CalcOil_Frequency (EXTI). Không gọi hàm này. */
    (void)raw;
    return 0;
}

/**
 * @brief  Tính lưu lượng dầu realtime (L/h x10) từ tần số xung EXTI.
 *
 * @param  is_return  0 = dầu đi (Oil 1),  1 = dầu về (Oil 2)
 * @return Lưu lượng x10 (ví dụ: 105 = 10.5 L/h)
 *
 * Công thức:
 *   coef    = xung/lít (hiệu chỉnh theo calibration table)
 *   L/h     = Hz * 3600 / coef
 *   L/h×10  = Hz * 36000 / coef
 *
 * Bubble correction được áp dụng riêng cho từng đường dầu.
 */
uint16_t Sensor_CalcOil_Frequency(uint8_t is_return)
{
    /* Ưu tiên Hz đã lọc; nếu EMA chưa kịp mà vừa có xung → dùng Hz thô */
    uint32_t hz_smooth = is_return ? ExtiCounter_GetOil2_Hz() : ExtiCounter_GetOil1_Hz();
    uint32_t hz_raw    = is_return ? ExtiCounter_GetOil2_Hz_Raw() : ExtiCounter_GetOil1_Hz_Raw();
    uint32_t hz = (hz_smooth > 0) ? hz_smooth : hz_raw;
    if (hz == 0) return 0;

    float base_coef;
    if (is_return)
        base_coef = (OIL_2_CONS > 0) ? ((float)OIL_2_CONS / 10.0f) : ((float)OIL_OUT_PULSE_PER_LITER / 10.0f);
    else
        base_coef = (OIL_1_CONS > 0) ? ((float)OIL_1_CONS / 10.0f) : ((float)OIL_IN_PULSE_PER_LITER / 10.0f);

    float actual_coef = Get_Interpolated_Coef(hz, base_coef);
    if (actual_coef < 0.1f) actual_coef = base_coef;
    if (actual_coef < 0.1f) return 0;

    /* L/h × 10 = Hz * 36000 / (xung/lít) */
    float flow = ((float)hz * 36000.0f) / actual_coef;

    if (is_return)
        flow *= (100.0f - (float)RETURN_BUBBLE_PCT)  / 100.0f;
    else
        flow *= (100.0f - (float)FORWARD_BUBBLE_PCT) / 100.0f;

    if (flow < 0.0f) flow = 0.0f;
    if (flow > (float)OIL_FLOW_MAX_CHANNEL_XH) flow = (float)OIL_FLOW_MAX_CHANNEL_XH;
    return (uint16_t)(flow + 0.5f);
}

/* Giới hạn tốc độ đổi L/h (slew) — tránh nhảy số trên app/LED khi bơm xung */
static uint16_t Oil_SlewLimit(uint16_t target, uint16_t *p_last, uint32_t dt_ms)
{
    if (dt_ms == 0) dt_ms = 1;
    uint32_t max_step = ((uint32_t)OIL_FLOW_SLEW_XH_PER_SEC * dt_ms) / 1000U;
    if (max_step < 1U) max_step = 1U;

    uint16_t last = *p_last;
    if (target > last) {
        uint32_t d = (uint32_t)target - last;
        if (d > max_step) target = (uint16_t)(last + max_step);
    } else {
        uint32_t d = (uint32_t)last - target;
        if (d > max_step) target = (uint16_t)(last - max_step);
    }
    *p_last = target;
    return target;
}

uint16_t Sensor_GetRawData(uint8_t sensor)
{
    return RawData[sensor];
}

uint32_t Sensor_GetOperatingHours(uint8_t sensor)
{
    if (Online[sensor]) {
        return (uint32_t)OperatingHours[sensor] * 60; // Cảm biến Modbus gửi lên là GIỜ, nhân 60 để quy về PHÚT
    }
    return gRuntimeData.SensorRunTimes[sensor]; // Biến nội bộ đã được lưu bằng PHÚT
}

uint16_t Sensor_GetType(uint8_t sensor)
{
    return SensorType[sensor];
}

uint32_t Sensor_GetSerialNumber(uint8_t sensor)
{
    return SerialNumber[sensor];
}

uint8_t Sensor_IsOnline(uint8_t sensor)
{
    return Online[sensor];
}

uint16_t Sensor_CalcVin(uint16_t adc)
{
    float vout;
    float vin;
    extern uint16_t VOLT_CONS;

    // Lấy hệ số chia áp từ Flash (VOLT_CONS), nếu bằng 0 thì dùng mặc định
    float num = (VOLT_CONS > 0) ? (float)VOLT_CONS : VIN_DIVIDER_NUMERATOR;

    vout = ((float)adc * 3.3f) / 4095.0f;

    vin = (vout * num) / VIN_DIVIDER_DENOMINATOR;
    
    // Bù 0.6V sụt áp qua Diode SS34 (chỉ bù khi có điện áp thực tế để tránh báo 0.6V khi rút nguồn)
    if (vin > 1.0f) {
        vin += 0.6f;
    }

    return (uint16_t)(vin * 10.0f);
}

uint16_t Sensor_CalcPressure_Local(uint16_t adc)
{
    PA_VALUE = adc; 
    uint32_t vadc_mv = (adc * 3300) / 4095;
    uint32_t vout_mv = (vadc_mv * 5) / 3;
    uint32_t zero_vout_mv = 500;

    if (vout_mv > zero_vout_mv)
    {
        uint32_t press = (vout_mv - zero_vout_mv) / 40;
        return (uint16_t)press;
    }
    return 0;
}

uint16_t Sensor_CalcPressure_Modbus(uint16_t raw)
{
    // TODO: Hiện tại trả về 0 cho Modbus Pressure theo yêu cầu
    return 0;
}

uint16_t Sensor_CalcTemperature_Modbus(uint16_t raw)
{
    if (raw <= 700) {
        return 0;
    }
    
    uint64_t raw2 = (uint64_t)raw * raw;
    int64_t mega_sum = (7101LL * raw2) + (2455000LL * raw) - 5198000000LL;
    
    if (mega_sum < 0) {
        return 0;
    }
    
    int32_t temp_x10 = (int32_t)(mega_sum / 100000000LL);
    
    extern uint16_t TEMP_CONS;
    if (TEMP_CONS > 0) {
        temp_x10 = (temp_x10 * TEMP_CONS) / 1000;
    }
    
    return (uint16_t)temp_x10;
}

uint16_t Sensor_CalcTemperature_Local(uint16_t adc)
{
    // TODO: Hiện tại trả về 0 cho Local Temperature theo yêu cầu
    return 0;
}


void SensorManager_LogAll(void)
{
    uint8_t i;

    LOG_Printf("--- SENSOR DATA ---\r\n");

    for(i = 0; i < MODBUS_SENSOR_COUNT; i++)
    {
        ModbusSensor_t* sensor;

        sensor = ModbusMaster_GetSensor(i);

        if(sensor->Online)
        {
            LOG_Printf(
                "ID=%d Type=%u Raw=%u Hours=%u SN=%08X\r\n",
                sensor->SlaveId,
                sensor->SensorType,
                sensor->RawData,
                sensor->OperatingHours,
                ((uint32_t)sensor->SerialNumH << 16) | sensor->SerialNumL);
        }
        else
        {
            LOG_Printf("ID=%d OFFLINE\r\n", sensor->SlaveId);
        }
    }
}

void SensorManager_Test(void)
{
    static uint32_t LastTick = 0;

    if(HAL_GetTick() - LastTick >= 1000)
    {
        LastTick = HAL_GetTick();

        SensorManager_LogAll();
    }
}


void SensorManager_Test_PA_Vin(void)
{
    static uint32_t LastTick = 0;

    if(HAL_GetTick() - LastTick >= 1000)
    {
        LastTick = HAL_GetTick();

        uint16_t raw_pressure = ADC_Read_Channel(ADC_PRESSURE_CHANNEL);
        uint16_t raw_vin = ADC_Read_Channel(ADC_VIN_CHANNEL);
        uint16_t raw_temp = ADC_Read_Channel(ADC_TEMP_CHANNEL);

        uint16_t pressure = Sensor_CalcPressure_Local(raw_pressure);
        uint16_t vin = Sensor_CalcVin(raw_vin);
        uint16_t temp = Sensor_CalcTemperature_Local(raw_temp);

        LOG_Printf(
            "RAW_P=%u RAW_V=%u RAW_T=%u | P=%u Bar VIN=%u V T=%u.%u C\r\n",
            raw_pressure, raw_vin, raw_temp,
            pressure, vin, temp / 10, temp % 10);
    }
}


void SensorManager_Update(void)
{

    uint16_t raw_pressure = ADC_Read_Channel(ADC_PRESSURE_CHANNEL);
    uint16_t raw_vin = ADC_Read_Channel(ADC_VIN_CHANNEL);
    uint16_t raw_temp = ADC_Read_Channel(ADC_TEMP_CHANNEL);

    /* Điện áp Vin */
    gSensorData.Vin =  Sensor_CalcVin(raw_vin);


    /* Áp suất */
    gSensorData.Pressure = Sensor_CalcPressure_Local(raw_pressure);


    /* Nhiệt độ */
    gSensorData.Temperature = Sensor_CalcTemperature_Local(raw_temp);


    /* RPM */
    gSensorData.RPM = 100;

    /* Lưu lượng dầu đi — đọc từ EXTI Counter (dầu đi = Oil1, is_return=0) */
    gSensorData.OilForward = Sensor_CalcOil_Frequency(0);

    /* Lưu lượng dầu về — đọc từ EXTI Counter (dầu về = Oil2, is_return=1) */
    gSensorData.OilReturn = Sensor_CalcOil_Frequency(1);

    LOG_Printf(
	"VIN=%u.%uV P=%uBar T=%u RPM=%u\r\n",
	gSensorData.Vin / 10,
	gSensorData.Vin % 10,
	gSensorData.Pressure,
	gSensorData.Temperature,
	gSensorData.RPM);

}


void SensorManager_Task(void)
{
    uint16_t RawPressure;
    uint16_t RawVin;

    /* Cập nhật thông tin cảm biến Modbus */
    SensorManager_UpdateModbus();

    /*
     * Dầu đi/hồi: mỗi ~200 ms từ EXTI + slew (bộ lọc hiển thị tàu 350HP).
     */
    {
        static uint16_t last_fwd_xh = 0;
        static uint16_t last_ret_xh = 0;
        static uint32_t last_oil_tick = 0;
        uint32_t now = HAL_GetTick();
        uint32_t dt = (last_oil_tick == 0) ? TASK_SENSOR_PERIOD_MS : (now - last_oil_tick);
        last_oil_tick = now;

        uint16_t fwd = Sensor_CalcOil_Frequency(0);
        uint16_t ret = Sensor_CalcOil_Frequency(1);
        if (!(gConfigData.SensorEnableMask & (1 << 3))) fwd = 0;
        if (!(gConfigData.SensorEnableMask & (1 << 4))) ret = 0;

        gSensorData.OilForward = Oil_SlewLimit(fwd, &last_fwd_xh, dt);
        gSensorData.OilReturn  = Oil_SlewLimit(ret, &last_ret_xh, dt);
    }

    if((HAL_GetTick() - LastTick) < 1000U)
    {
        return;
    }

    LastTick = HAL_GetTick();

    /*================ ADC & Fallback Local Readings =================*/
    RawPressure = ADC_Read_Channel(ADC_PRESSURE_CHANNEL);
    RawVin = ADC_Read_Channel(ADC_VIN_CHANNEL);
    uint16_t RawTemp = ADC_Read_Channel(ADC_TEMP_CHANNEL);

    // Vin is always local ADC
    gSensorData.Vin = Sensor_CalcVin(RawVin);

    // Temperature (Modbus ID 101 -> Index 0)
    if (Online[0]) gSensorData.Temperature = Sensor_CalcTemperature_Modbus(RawData[0]);
    else           gSensorData.Temperature = Sensor_CalcTemperature_Local(RawTemp);
    if (!(gConfigData.SensorEnableMask & (1 << 0))) gSensorData.Temperature = 0;

    // RPM (Modbus ID 102 -> Index 1)
    if (Online[1]) gSensorData.RPM = Sensor_CalcRPM_Modbus(RawData[1]);
    else           gSensorData.RPM = Sensor_CalcRPM_Local(TimerCounter_GetTachoPeriod());
    if (!(gConfigData.SensorEnableMask & (1 << 1))) gSensorData.RPM = 0;

    // Pressure (Modbus ID 103 -> Index 2)
    if (Online[2]) gSensorData.Pressure = Sensor_CalcPressure_Modbus(RawData[2]);
    else           gSensorData.Pressure = Sensor_CalcPressure_Local(RawPressure);
    if (!(gConfigData.SensorEnableMask & (1 << 2))) gSensorData.Pressure = 0;

    /*================ MANUAL LOG =================*/
    uint32_t t1 = ExtiCounter_GetOil1();
    uint32_t t2 = ExtiCounter_GetOil2();
    uint32_t l1 = (uint32_t)(ExtiCounter_GetOil1Liters() * 100.0);
    uint32_t l2 = (uint32_t)(ExtiCounter_GetOil2Liters() * 100.0);

    if (SENSOR_LogEnable) {
        LOG_Printf("\r\n====== SENSOR DATA ======\r\n");
        LOG_Printf("[VOLT] Raw: %u -> Vin: %u.%u V\r\n", RawVin, gSensorData.Vin / 10, gSensorData.Vin % 10);
        LOG_Printf("[PRES] Raw: %u -> Pres: %u.%u Bar\r\n", RawPressure, gSensorData.Pressure / 10, gSensorData.Pressure % 10);
        LOG_Printf("[RPM ] Period: %lu us -> RPM: %u\r\n", TimerCounter_GetTachoPeriod(), gSensorData.RPM);
        LOG_Printf("[OIL+] Hz_raw=%lu Hz_smooth=%lu | Total: %lu pulses (%lu.%lu L) -> Flow: %u.%u L/h\r\n",
                   ExtiCounter_GetOil1_Hz_Raw(), ExtiCounter_GetOil1_Hz(),
                   t1, l1 / 10, l1 % 10,
                   gSensorData.OilForward / 10, gSensorData.OilForward % 10);
        LOG_Printf("[OIL-] Hz_raw=%lu Hz_smooth=%lu | Total: %lu pulses (%lu.%lu L) -> Flow: %u.%u L/h\r\n",
                   ExtiCounter_GetOil2_Hz_Raw(), ExtiCounter_GetOil2_Hz(),
                   t2, l2 / 10, l2 % 10,
                   gSensorData.OilReturn / 10, gSensorData.OilReturn % 10);
        LOG_Printf("=========================\r\n");
    }

    /*================ PROCESS GPS VALUES =================*/
    Gps_record gps_rec;
    Gps_date_time gps_dt;
    Get_Gps_now(&gps_rec, &gps_dt);

    if (gps_rec.status)
    {
        int32_t lat_val = gps_rec.lat;
        int32_t lon_val = gps_rec.lon;
        char lat_dir = (lat_val >= 0) ? 'N' : 'S';
        char lon_dir = (lon_val >= 0) ? 'E' : 'W';
        if (lat_val < 0) lat_val = -lat_val;
        if (lon_val < 0) lon_val = -lon_val;

        if (GPS_LogEnable) {
            LOG_Printf("--- GPS: Date=%02u/%02u/%04u Time=%02u:%02u:%02u Lat=%d.%06d%c Lon=%d.%06d%c Speed=%uKnots Dir=%u Satellites=%u ---\r\n",
                       gps_dt.day, gps_dt.month, gps_dt.year,
                       gps_dt.hour, gps_dt.minute, gps_dt.second,
                       (int)(lat_val / 1000000), (int)(lat_val % 1000000), lat_dir,
                       (int)(lon_val / 1000000), (int)(lon_val % 1000000), lon_dir,
                       gps_rec.speed, gps_rec.direct, (unsigned int)_numsats);
        }

        /* Đồng bộ RTC */
        gSystemData.UnixTime = DateTimeToUnix(gps_dt.year, gps_dt.month, gps_dt.day, gps_dt.hour, gps_dt.minute, gps_dt.second);
        
        /* Cập nhật DS1307 */
        DS1307_Time ds_time;
        ds_time.sec = gps_dt.second;
        ds_time.min = gps_dt.minute;
        ds_time.hour = gps_dt.hour;
        ds_time.day = 1;
        ds_time.date = gps_dt.day;
        ds_time.month = gps_dt.month;
        ds_time.year = gps_dt.year % 100; /* DS1307 requires 2-digit year (0-99) */
        DS1307_SetTime(&ds_time);
        
        if (GPS_LogEnable) {
            LOG_Printf("--- RTC UPDATED from GPS: %02u/%02u/%04u %02u:%02u:%02u ---\r\n", 
                       ds_time.date, ds_time.month, 2000 + ds_time.year,
                       ds_time.hour, ds_time.min, ds_time.sec);
        }
        
        /* Bật LED GPS khi có sóng */
        HAL_GPIO_WritePin(GPS_LED_PORT, GPS_LED_PIN, GPIO_PIN_SET);
    }
    else
    {
        if (GPS_LogEnable) {
            LOG_Printf("--- GPS: NO SIGNAL ---\r\n");
        }
        
        /* Đọc thời gian từ DS1307 */
        DS1307_Time ds_time = {0};
        if (DS1307_GetTime(&ds_time)) {
            uint32_t t = DateTimeToUnix(ds_time.year, ds_time.month, ds_time.date, ds_time.hour, ds_time.min, ds_time.sec);
            if (t >= 1782864000) { // Chỉ nhận thời gian hợp lệ (Từ 01/07/2026 trở lên)
                gSystemData.UnixTime = t;
            }
            
            if (GPS_LogEnable) {
                LOG_Printf("--- RTC: %02u:%02u:%02u %02u/%02u/%02u (Unix: %u) ---\r\n", 
                           ds_time.hour, ds_time.min, ds_time.sec, 
                           ds_time.date, ds_time.month, ds_time.year, gSystemData.UnixTime);
            }
        } else {
            if (GPS_LogEnable) {
                LOG_Printf("--- RTC: Failed to read from DS1307/DS3231 ---\r\n");
            }
        }
        
        /* Tắt LED GPS khi mất sóng */
        HAL_GPIO_WritePin(GPS_LED_PORT, GPS_LED_PIN, GPIO_PIN_RESET);
    }

    /* Ghi log dữ liệu vào Flash theo chu kỳ gConfigData.LogInterval */
    static uint32_t last_log_tick = 0;
    
    if (gps_rec.status) {
        gLocationData.Latitude = gps_rec.lat;
        gLocationData.Longitude = gps_rec.lon;
    }
    
    /* Cập nhật đếm số giờ chạy động cơ (1 giây / lần) */
    static uint32_t last_run_tick = 0;
    static uint16_t gDeviceRunSeconds = 0;
    if (HAL_GetTick() - last_run_tick >= 1000) {
        last_run_tick = HAL_GetTick();
        
        // 1. Đếm thời gian chạy mạch (Sum Time) - Bật điện là đếm (Đơn vị: GIỜ)
        gDeviceRunSeconds++;
        if (gDeviceRunSeconds >= 3600) {
            gDeviceRunSeconds = 0;
            gRuntimeData.TotalTime++;
            W_Flash_Safe_Write_Long(AD_TIME_SUM, gRuntimeData.TotalTime);
        }
        
        // 2. Đếm thời gian nổ máy (Run Time) - Có vòng tua mới đếm
        if (gSensorData.RPM > 0) {
            gRuntimeData.EngineRunSeconds++;
            gRuntimeData.EngineRunSinceLastLog++;
            if (gRuntimeData.EngineRunSeconds >= 60) {
                gRuntimeData.EngineRunSeconds = 0;
                gRuntimeData.RunTime++;
                
                for(int i = 0; i < 5; i++) {
                    gRuntimeData.SensorRunTimes[i]++;
                }
                
                // Chỉ lưu vào Flash mỗi 60 phút (1 tiếng) để bảo vệ tuổi thọ chip Flash
                if (gRuntimeData.RunTime % 60 == 0) {
                    W_Flash_Safe_Write_Long(AD_TIME_RUN, gRuntimeData.RunTime); // Ghi 1 biến vào Sector 0
                    
                    // Gom chung 5 biến Sensor_Time vào 1 lần mở kho Sector 3 duy nhất (Tránh mở 5 lần)
                    extern uint8_t W25Q32_Sector_Buf[4096];
                    
                    uint32_t sector_addr = AD_SENSOR_RUNTIME_BASE & 0xFFFFF000;
                    W_Flash_Read_Arr(sector_addr, W25Q32_Sector_Buf, 4096);
                    for(int i = 0; i < 5; i++) {
                        uint32_t offset = (AD_SENSOR_RUNTIME_BASE + (i * 4)) & 0x0FFF;
                        uint32_t val = gRuntimeData.SensorRunTimes[i];
                        W25Q32_Sector_Buf[offset]   = (val >> 24) & 0xFF;
                        W25Q32_Sector_Buf[offset+1] = (val >> 16) & 0xFF;
                        W25Q32_Sector_Buf[offset+2] = (val >> 8) & 0xFF;
                        W25Q32_Sector_Buf[offset+3] = val & 0xFF;
                    }
                    W_Flash_Sector_Erase(sector_addr);
                    W_Flash_Write_Arr(sector_addr, W25Q32_Sector_Buf, 4096);
                }

                if (FLASH_LogEnable) {
                    LOG_Printf("\r\n--- ENGINE RUN MINUTES UPDATED: %u minutes ---\r\n", gRuntimeData.RunTime);
                }
            }
        }
    }
    
    // --- Tính lượng dầu tiêu thụ Real-time (Tích lũy từ lần ghi log trước) ---
    
    double total_liters = ExtiCounter_GetTotalLiters();
    uint32_t liters_x100 = (uint32_t)(total_liters * 100.0);
    
    gSensorData.OilConsumption = (liters_x100 > 65535) ? 65535 : (uint16_t)liters_x100;

    /* Kiểm tra ghi bản tin khởi động */
    static uint32_t gBootTick = 0;
    static bool gStartupRecordLogged = false;
    if (gBootTick == 0) {
        gBootTick = HAL_GetTick();
        if (gBootTick == 0) gBootTick = 1; // Tránh lặp lại
    }

    if (!gStartupRecordLogged) {
        // Điều kiện 1: GPS ổn định (có sóng)
        // Điều kiện 2: Chờ quá 5 phút (300.000 ms) kể từ lúc bật nguồn
        bool gps_stable = gps_rec.status;
        bool timeout = (HAL_GetTick() - gBootTick >= 300000);

        if (gps_stable || timeout) {
            DataRecord_t record;
            memset(&record, 0, sizeof(record));
            record.Latitude = gLocationData.Latitude;
            record.Longitude = gLocationData.Longitude;
            record.Timestamp = gSystemData.UnixTime;
            
            double lit = ExtiCounter_GetTotalLiters();
            record.Total_Oil_Consumed = (uint32_t)(lit * 100.0); // Liters * 100
            record.Oil1_TotalPulses = ExtiCounter_GetOil1();
            record.Oil2_TotalPulses = ExtiCounter_GetOil2();
            record.Oil1_TotalLiters = (uint32_t)(ExtiCounter_GetOil1Liters() * 100.0);
            record.Oil2_TotalLiters = (uint32_t)(ExtiCounter_GetOil2Liters() * 100.0);
            
            record.Voltage = gSensorData.Vin;
            record.RPM = gSensorData.RPM;
            record.Temperature = gSensorData.Temperature;
            record.Pressure = gSensorData.Pressure;
            record.EngineRunTime = gRuntimeData.EngineRunSinceLastLog;
            record.RecordType = 1; // 1: Bản tin khởi động (Startup)
            
            gRuntimeData.EngineRunSinceLastLog = 0;
            // Xóa bộ đếm để bắt đầu lại từ đầu cho chu kỳ tiếp theo
            extern void ExtiCounter_Reset(void);
            extern void ExtiCounter_SetTotalLiters(double liters);
            ExtiCounter_Reset();
            ExtiCounter_SetTotalLiters(0.0);
            
            uint32_t addr = AD_START_OIL_RECORD + (gSystemData.RecordCount * sizeof(DataRecord_t));
            if (addr + sizeof(DataRecord_t) <= AD_MAX_FLASH) {
                if ((addr % 4096) == 0) {
                    extern void W_Flash_Sector_Erase(uint32_t sector_address);
                    W_Flash_Sector_Erase(addr);
                }
                W_Flash_Write_Arr(addr, (const uint8_t*)&record, sizeof(DataRecord_t));
                gSystemData.RecordCount++;
                
                if (FLASH_LogEnable) {
                    LOG_Printf("\r\n=======================================================\r\n");
                    LOG_Printf("--- FLASH LOG: Saved STARTUP Record #%u at Unix %u ---\r\n", gSystemData.RecordCount, gSystemData.UnixTime);
                    LOG_Printf("Data: Lat=%ld Lon=%ld Oil_Total=%u Vin=%u RPM=%u Temp=%u PA=%u\r\n",
                               record.Latitude, record.Longitude, record.Total_Oil_Consumed, record.Voltage, 
                               record.RPM, record.Temperature, record.Pressure);
                    LOG_Printf("=======================================================\r\n");
                }
            }
            
            gStartupRecordLogged = true;
            last_log_tick = HAL_GetTick(); // Bắt đầu chu kỳ ghi log định kỳ từ thời điểm này
        }
    } else {
        // Ghi log định kỳ hoàn toàn dựa vào timer phần cứng (HAL_GetTick) để miễn nhiễm với việc nhiễu/nhảy thời gian RTC
        if (HAL_GetTick() - last_log_tick >= (gConfigData.LogInterval * 1000) && gConfigData.LogInterval > 0) {
            last_log_tick = HAL_GetTick();
            
            DataRecord_t record;
            memset(&record, 0, sizeof(record));
            record.Latitude = gLocationData.Latitude;
            record.Longitude = gLocationData.Longitude;
            record.Timestamp = gSystemData.UnixTime;
            
            double lit = ExtiCounter_GetTotalLiters();
            record.Total_Oil_Consumed = (uint32_t)(lit * 100.0); // Liters * 100
            record.Oil1_TotalPulses = ExtiCounter_GetOil1();
            record.Oil2_TotalPulses = ExtiCounter_GetOil2();
            record.Oil1_TotalLiters = (uint32_t)(ExtiCounter_GetOil1Liters() * 100.0);
            record.Oil2_TotalLiters = (uint32_t)(ExtiCounter_GetOil2Liters() * 100.0);
            
            record.Voltage = gSensorData.Vin;
            record.RPM = gSensorData.RPM;
            record.Temperature = gSensorData.Temperature;
            record.Pressure = gSensorData.Pressure;
            record.EngineRunTime = gRuntimeData.EngineRunSinceLastLog;
            record.RecordType = 0; // 0: Bản tin định kỳ (Periodic)
            
            gRuntimeData.EngineRunSinceLastLog = 0;
            // Xóa bộ đếm để bắt đầu lại từ đầu cho chu kỳ tiếp theo
            extern void ExtiCounter_Reset(void);
            extern void ExtiCounter_SetTotalLiters(double liters);
            ExtiCounter_Reset();
            ExtiCounter_SetTotalLiters(0.0);
            
            uint32_t addr = AD_START_OIL_RECORD + (gSystemData.RecordCount * sizeof(DataRecord_t));
            if (addr + sizeof(DataRecord_t) <= AD_MAX_FLASH) {
                if ((addr % 4096) == 0) {
                    extern void W_Flash_Sector_Erase(uint32_t sector_address);
                    W_Flash_Sector_Erase(addr);
                }
                
                W_Flash_Write_Arr(addr, (const uint8_t*)&record, sizeof(DataRecord_t));
                gSystemData.RecordCount++;
                
                if (FLASH_LogEnable) {
                    LOG_Printf("\r\n=======================================================\r\n");
                    LOG_Printf("--- FLASH LOG: Saved Record #%u at Unix %u ---\r\n", gSystemData.RecordCount, gSystemData.UnixTime);
                    LOG_Printf("Data: Lat=%ld Lon=%ld Oil_Total=%u Vin=%u RPM=%u Temp=%u PA=%u\r\n",
                               record.Latitude, record.Longitude, record.Total_Oil_Consumed, record.Voltage, 
                               record.RPM, record.Temperature, record.Pressure);
                    LOG_Printf("=======================================================\r\n");
                }
            } else {
                if (FLASH_LogEnable) {
                    LOG_Printf("--- FLASH LOG: FLASH FULL! ---\r\n");
                }
            }
        }
    }

    if (SENSOR_LogEnable) {
        for (uint8_t i = 0; i < MODBUS_SENSOR_COUNT; i++)
        {
            if (Online[i])
            {
                LOG_Printf("Modbus ID %d: Raw=%u Hours=%u\r\n", 
                           101 + i, RawData[i], OperatingHours[i]);
            }
            else
            {
                LOG_Printf("Modbus ID %d: OFFLINE\r\n", 101 + i);
            }
        }
    }
}

