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
    // Modbus trả tổng xung tích lũy (Flow Meter), không phải L/h trực tiếp.
    // Tạm thời để trống (TODO: Implement difference calculation)
    return 0;
}

uint16_t Sensor_CalcOil_Frequency(uint32_t current_pulses, uint8_t is_return)
{
    extern uint16_t OIL_1_CONS;
    extern uint16_t OIL_2_CONS;
    uint32_t coef = is_return ? 
                    ((OIL_2_CONS > 0) ? OIL_2_CONS : OIL_OUT_PULSE_PER_LITER) : 
                    ((OIL_1_CONS > 0) ? OIL_1_CONS : OIL_IN_PULSE_PER_LITER);
                    
    uint32_t period = is_return ? ExtiCounter_GetOil2_Period() : ExtiCounter_GetOil1_Period();
    if (period == 0) return 0;
    
    // Thuật toán đếm chu kỳ (Period) TỐI GIẢN & CHÍNH XÁC NHẤT:
    // - Tần số (Hz) = 1000 / period
    // - L/h = Hz * 3600 / (coef / 10) = (1000 / period) * 36000 / coef
    // - Do màn hình LED cần giá trị (L/h * 10), ta nhân thêm 10
    // => Flow_Value = 360,000,000 / (period * coef)
    
    uint64_t flow = 360000000ULL / ((uint64_t)period * coef);
    
    if (flow > 65535) return 65535;
    return (uint16_t)flow;
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
    return gSensorRunTimes[sensor]; // Biến nội bộ đã được lưu bằng PHÚT
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

    /* Lưu lượng dầu đi */
    gSensorData.OilForward = 10;

    /* Lưu lượng dầu về */
    gSensorData.OilReturn = 10;

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
    if (!(gSensorEnableMask & (1 << 0))) gSensorData.Temperature = 0;

    // RPM (Modbus ID 102 -> Index 1)
    if (Online[1]) gSensorData.RPM = Sensor_CalcRPM_Modbus(RawData[1]);
    else           gSensorData.RPM = Sensor_CalcRPM_Local(TimerCounter_GetTachoPeriod());
    if (!(gSensorEnableMask & (1 << 1))) gSensorData.RPM = 0;

    // Pressure (Modbus ID 103 -> Index 2)
    if (Online[2]) gSensorData.Pressure = Sensor_CalcPressure_Modbus(RawData[2]);
    else           gSensorData.Pressure = Sensor_CalcPressure_Local(RawPressure);
    if (!(gSensorEnableMask & (1 << 2))) gSensorData.Pressure = 0;

    // Oil Forward (Modbus ID 104 -> Index 3)
    if (Online[3]) gSensorData.OilForward = Sensor_CalcOil_Modbus(RawData[3]);
    else           gSensorData.OilForward = Sensor_CalcOil_Frequency(ExtiCounter_GetOil1(), 0);
    if (!(gSensorEnableMask & (1 << 3))) gSensorData.OilForward = 0;

    // Oil Return (Modbus ID 105 -> Index 4)
    if (Online[4]) gSensorData.OilReturn = Sensor_CalcOil_Modbus(RawData[4]);
    else           gSensorData.OilReturn = Sensor_CalcOil_Frequency(ExtiCounter_GetOil2(), 1);
    if (!(gSensorEnableMask & (1 << 4))) gSensorData.OilReturn = 0;

    /*================ MANUAL LOG =================*/
    extern uint16_t OIL_1_CONS, OIL_2_CONS;
    uint32_t c1 = (OIL_1_CONS > 0) ? OIL_1_CONS : OIL_IN_PULSE_PER_LITER;
    uint32_t c2 = (OIL_2_CONS > 0) ? OIL_2_CONS : OIL_OUT_PULSE_PER_LITER;
    uint32_t t1 = ExtiCounter_GetOil1();
    uint32_t t2 = ExtiCounter_GetOil2();
    uint32_t l1 = (t1 * 100) / c1;
    uint32_t l2 = (t2 * 100) / c2;

    if (SENSOR_LogEnable) {
        LOG_Printf("\r\n====== SENSOR DATA ======\r\n");
        LOG_Printf("[VOLT] Raw: %u -> Vin: %u.%u V\r\n", RawVin, gSensorData.Vin / 10, gSensorData.Vin % 10);
        LOG_Printf("[PRES] Raw: %u -> Pres: %u.%u Bar\r\n", RawPressure, gSensorData.Pressure / 10, gSensorData.Pressure % 10);
        LOG_Printf("[RPM ] Period: %lu us -> RPM: %u\r\n", TimerCounter_GetTachoPeriod(), gSensorData.RPM);
        LOG_Printf("[OIL+] Period: %lu ms | Total: %lu pulses (%lu.%lu L) -> Flow: %u.%u L/h\r\n", ExtiCounter_GetOil1_Period(), t1, l1 / 10, l1 % 10, gSensorData.OilForward / 10, gSensorData.OilForward % 10);
        LOG_Printf("[OIL-] Period: %lu ms | Total: %lu pulses (%lu.%lu L) -> Flow: %u.%u L/h\r\n", ExtiCounter_GetOil2_Period(), t2, l2 / 10, l2 % 10, gSensorData.OilReturn / 10, gSensorData.OilReturn % 10);
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
        gSystemUnixTime = DateTimeToUnix(gps_dt.year, gps_dt.month, gps_dt.day, gps_dt.hour, gps_dt.minute, gps_dt.second);
        
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
                gSystemUnixTime = t;
            }
            
            if (GPS_LogEnable) {
                LOG_Printf("--- RTC: %02u:%02u:%02u %02u/%02u/%02u (Unix: %u) ---\r\n", 
                           ds_time.hour, ds_time.min, ds_time.sec, 
                           ds_time.date, ds_time.month, ds_time.year, gSystemUnixTime);
            }
        } else {
            if (GPS_LogEnable) {
                LOG_Printf("--- RTC: Failed to read from DS1307/DS3231 ---\r\n");
            }
        }
        
        /* Tắt LED GPS khi mất sóng */
        HAL_GPIO_WritePin(GPS_LED_PORT, GPS_LED_PIN, GPIO_PIN_RESET);
    }

    /* Ghi log dữ liệu vào Flash theo chu kỳ gLogInterval */
    static uint32_t last_log_tick = 0;
    
    if (gps_rec.status) {
        gLastLatitude = gps_rec.lat;
        gLastLongitude = gps_rec.lon;
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
            gEngineRunSeconds++;
            gEngineRunSinceLastLog++;
            if (gEngineRunSeconds >= 60) {
                gEngineRunSeconds = 0;
                gRuntimeData.RunTime++;
                
                for(int i = 0; i < 5; i++) {
                    gSensorRunTimes[i]++;
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
                        uint32_t val = gSensorRunTimes[i];
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
    uint32_t rt_o1 = ExtiCounter_GetOil1();
    uint32_t rt_o2 = ExtiCounter_GetOil2();
    
    extern uint16_t OIL_1_CONS;
    extern uint16_t OIL_2_CONS;
    uint32_t coef1 = (OIL_1_CONS > 0) ? OIL_1_CONS : OIL_IN_PULSE_PER_LITER;
    uint32_t coef2 = (OIL_2_CONS > 0) ? OIL_2_CONS : OIL_OUT_PULSE_PER_LITER;
    
    // Quy đổi số Xung ra Số Lít (Nhân 10000 để giữ độ chính xác thập phân)
    // Lưu ý: coef1 đang là (Xung_Trên_Lít * 10). 
    // Lít = Xung / (coef / 10) = (Xung * 10) / coef
    // Lít * 10000 = (Xung * 100000) / coef
    uint64_t lit1_10k = ((uint64_t)rt_o1 * 100000) / coef1;
    uint64_t lit2_10k = ((uint64_t)rt_o2 * 100000) / coef2;
    
    // Trừ Số Lít (Lít Đi - Lít Về)
    uint64_t lit_consumed_10k = (lit1_10k > lit2_10k) ? (lit1_10k - lit2_10k) : 0;
    
    // Lưu thẳng bằng Lít (Nhân 100 để lưu dạng số nguyên, ví dụ 15.52 Lít -> 1552)
    uint32_t rt_consumed = lit_consumed_10k / 100;
    
    gSensorData.OilConsumption = (rt_consumed > 65535) ? 65535 : (uint16_t)rt_consumed;

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
            record.Latitude = gLastLatitude;
            record.Longitude = gLastLongitude;
            record.Timestamp = gSystemUnixTime;
            record.OilConsumed = gSensorData.OilConsumption;
            record.Voltage = gSensorData.Vin;
            record.RPM = gSensorData.RPM;
            record.Temperature = gSensorData.Temperature;
            record.Pressure = gSensorData.Pressure;
            record.EngineRunTime = gEngineRunSinceLastLog;
            record.RecordType = 1; // 1: Bản tin khởi động (Startup)
            
            gEngineRunSinceLastLog = 0;
            ExtiCounter_Reset();
            
            uint32_t addr = AD_START_OIL_RECORD + (gRecordCount * sizeof(DataRecord_t));
            if (addr + sizeof(DataRecord_t) <= AD_MAX_FLASH) {
                if ((addr % 4096) == 0) {
                    extern void W_Flash_Sector_Erase(uint32_t sector_address);
                    W_Flash_Sector_Erase(addr);
                }
                W_Flash_Write_Arr(addr, (const uint8_t*)&record, sizeof(DataRecord_t));
                gRecordCount++;
                
                if (FLASH_LogEnable) {
                    LOG_Printf("\r\n=======================================================\r\n");
                    LOG_Printf("--- FLASH LOG: Saved STARTUP Record #%u at Unix %u ---\r\n", gRecordCount, gSystemUnixTime);
                    LOG_Printf("Data: Lat=%ld Lon=%ld Oil=%u Vin=%u RPM=%u Temp=%u PA=%u\r\n",
                               record.Latitude, record.Longitude, record.OilConsumed, record.Voltage, 
                               record.RPM, record.Temperature, record.Pressure);
                    LOG_Printf("=======================================================\r\n");
                }
            }
            
            gStartupRecordLogged = true;
            last_log_tick = HAL_GetTick(); // Bắt đầu chu kỳ ghi log định kỳ từ thời điểm này
        }
    } else {
        // Ghi log định kỳ hoàn toàn dựa vào timer phần cứng (HAL_GetTick) để miễn nhiễm với việc nhiễu/nhảy thời gian RTC
        if (HAL_GetTick() - last_log_tick >= (gLogInterval * 1000) && gLogInterval > 0) {
            last_log_tick = HAL_GetTick();
            
            DataRecord_t record;
            memset(&record, 0, sizeof(record));
            record.Latitude = gLastLatitude;
            record.Longitude = gLastLongitude;
            record.Timestamp = gSystemUnixTime;
            record.OilConsumed = gSensorData.OilConsumption;
            record.Voltage = gSensorData.Vin;
            record.RPM = gSensorData.RPM;
            record.Temperature = gSensorData.Temperature;
            record.Pressure = gSensorData.Pressure;
            record.EngineRunTime = gEngineRunSinceLastLog;
            record.RecordType = 0; // 0: Bản tin định kỳ (Periodic)
            
            gEngineRunSinceLastLog = 0;
            ExtiCounter_Reset();
            
            uint32_t addr = AD_START_OIL_RECORD + (gRecordCount * sizeof(DataRecord_t));
            if (addr + sizeof(DataRecord_t) <= AD_MAX_FLASH) {
                if ((addr % 4096) == 0) {
                    extern void W_Flash_Sector_Erase(uint32_t sector_address);
                    W_Flash_Sector_Erase(addr);
                }
                
                W_Flash_Write_Arr(addr, (const uint8_t*)&record, sizeof(DataRecord_t));
                gRecordCount++;
                
                if (FLASH_LogEnable) {
                    LOG_Printf("\r\n=======================================================\r\n");
                    LOG_Printf("--- FLASH LOG: Saved Record #%u at Unix %u ---\r\n", gRecordCount, gSystemUnixTime);
                    LOG_Printf("Data: Lat=%ld Lon=%ld Oil=%u Vin=%u RPM=%u Temp=%u PA=%u\r\n",
                               record.Latitude, record.Longitude, record.OilConsumed, record.Voltage, 
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

