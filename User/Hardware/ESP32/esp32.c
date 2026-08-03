#include "esp32.h"
#include "usart.h"
#include "gpio.h"
#include "data_model.h"
#include "alarm_manager.h"
#include "app_config.h"
#include "w25q32.h"
#include "Flash_definition.h"
#include "log.h"
#include "hc595.h"
#include <string.h>
#include "buzzer.h"
#include "Exti_Counter.h"
#include "ds1307.h"
#define ESP_RX_BUFFER_SIZE 256
static uint8_t esp_rx_buf[ESP_RX_BUFFER_SIZE];
static uint16_t esp_rx_head = 0;
static uint16_t esp_rx_tail = 0;

static uint8_t esp_rx_byte = 0;
bool ESP32_LogEnable = false;

static uint32_t last_esp_comm_ms = 0;

/* Relay Watchdog Logic */
static uint32_t relay1_last_cmd_ms = 0;
static bool relay1_active = false;
static uint32_t relay2_last_cmd_ms = 0;
static bool relay2_active = false;

typedef enum {
    STATE_WAIT_STX,
    STATE_WAIT_04,
    STATE_WAIT_CMD,
    STATE_WAIT_LEN,
    STATE_WAIT_DATA,
    STATE_WAIT_CS,
    STATE_WAIT_ETX
} ESP32_State_t;

static ESP32_State_t state = STATE_WAIT_STX;
static uint8_t current_cmd = 0;
static uint8_t current_len = 0;
static uint8_t data_buf[128];
static uint8_t data_idx = 0;

static uint16_t TIME_BAODUONG = 0; /* Mock */

void ESP32_Init(void)
{
    /* Bắt đầu nhận ngắt UART1 */
    HAL_UART_Receive_IT(ESP_UART, &esp_rx_byte, 1);
    
    /* Đọc thời gian bảo dưỡng từ Flash */
    uint16_t val = W_Flash_Read_Int(AD_TIME_BAODUONG);
    if (val != 0xFFFF) {
        TIME_BAODUONG = val;
    }
    
    /* Reset ESP32 khi khởi động STM32 */
    //ESP32_Reset();
    
    last_esp_comm_ms = HAL_GetTick(); // Khởi tạo mốc thời gian
}

void ESP32_Reset(void)
{
    HAL_GPIO_WritePin(RESET_ESSP_GPIO_Port, RESET_ESSP_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(RESET_ESSP_GPIO_Port, RESET_ESSP_Pin, GPIO_PIN_SET);
}

void ESP32_UART_Callback(void)
{
    esp_rx_buf[esp_rx_head] = esp_rx_byte;
    esp_rx_head = (esp_rx_head + 1) % ESP_RX_BUFFER_SIZE;
    last_esp_comm_ms = HAL_GetTick(); // Đánh dấu có giao tiếp nhận dữ liệu
    HAL_UART_Receive_IT(ESP_UART, &esp_rx_byte, 1);
}

void ESP32_Send_Packet(uint8_t cmd, uint8_t len, uint8_t *data)
{
    uint8_t tx_buf[128];
    uint8_t idx = 0;
    
    tx_buf[idx++] = 0x02; /* STX */
    tx_buf[idx++] = cmd;
    tx_buf[idx++] = len;
    
    for (uint8_t i = 0; i < len; i++) {
        tx_buf[idx++] = data[i];
    }
    
    tx_buf[idx++] = 0x03; /* ETX */
    
    HAL_UART_Transmit(ESP_UART, tx_buf, idx, 100);
}

void ESP32_RestartReceive(void)
{
    HAL_UART_Receive_IT(ESP_UART, &esp_rx_byte, 1);
}

static void ESP32_Process_Command(uint8_t cmd, uint8_t *data, uint8_t len)
{
    uint8_t tx_data[64];
    
    if (ESP32_LogEnable) {
        LOG_Printf("ESP32_RX: CMD=%02X LEN=%d DATA=[", cmd, len);
        for (int i=0; i<len; i++) LOG_Printf("%02X ", data[i]);
        LOG_Printf("]\r\n");
    }
    
    switch (cmd) {
        case 0xE0: /* Read Set Value */
        {
            /* Byte 0-1:  Vin MIN (ngưỡng điện áp dưới) */
            /* Byte 2-3:  Vin MAX (ngưỡng điện áp trên)  */
            /* Byte 4-5:  T_MAX                             */
            /* Byte 6-7:  PA_MAX                            */
            /* Byte 8-9:  OIL_MAX                           */
            /* Byte 10-11: TACHO_MAX                        */
            /* Byte 12-13: TIME_BAODUONG                    */
            /* Byte 14-15: PA_MIN                           */
            tx_data[0]  = (Limit_VinMin >> 8) & 0xFF; tx_data[1]  = Limit_VinMin & 0xFF;
            tx_data[2]  = (Limit_VinMax >> 8) & 0xFF; tx_data[3]  = Limit_VinMax & 0xFF;
            tx_data[4]  = (Limit_Temp >> 8) & 0xFF;   tx_data[5]  = Limit_Temp & 0xFF;
            tx_data[6]  = (Limit_Pressure >> 8) & 0xFF; tx_data[7] = Limit_Pressure & 0xFF;
            tx_data[8]  = (Limit_Oil >> 8) & 0xFF;    tx_data[9]  = Limit_Oil & 0xFF;
            tx_data[10] = (Limit_RPM >> 8) & 0xFF;    tx_data[11] = Limit_RPM & 0xFF;
            tx_data[12] = (TIME_BAODUONG >> 8) & 0xFF; tx_data[13] = TIME_BAODUONG & 0xFF;
            tx_data[14] = (Limit_PressureMin >> 8) & 0xFF; tx_data[15] = Limit_PressureMin & 0xFF;
            
            if (ESP32_LogEnable) LOG_Printf("ESP32: Responding Set Values VinMin=%u VinMax=%u\r\n", Limit_VinMin, Limit_VinMax);
            ESP32_Send_Packet(0xE0, 16, tx_data);
            break;
        }

        case 0xEA: /* Read Firmware Version */
        {
            tx_data[0] = (gFirmwareVersion >> 24) & 0xFF;
            tx_data[1] = (gFirmwareVersion >> 16) & 0xFF;
            tx_data[2] = (gFirmwareVersion >> 8) & 0xFF;
            tx_data[3] = gFirmwareVersion & 0xFF;
            
            if (ESP32_LogEnable) LOG_Printf("ESP32: Responding Firmware Version (%u)\r\n", gFirmwareVersion);
            ESP32_Send_Packet(0xEA, 4, tx_data);
            break;
        }
        
        case 0xD2: /* Get Realtime Data (Kèm GPS) */
        {
            uint16_t rpm = gSensorData.RPM;
            uint32_t time_run = gRuntimeData.RunTime;
            uint32_t time_sum = gRuntimeData.TotalTime;
            
            tx_data[0] = (gSensorData.Vin >> 8) & 0xFF; tx_data[1] = gSensorData.Vin & 0xFF;
            tx_data[2] = (gSensorData.Temperature >> 8) & 0xFF; tx_data[3] = gSensorData.Temperature & 0xFF;
            tx_data[4] = (gSensorData.Pressure >> 8) & 0xFF; tx_data[5] = gSensorData.Pressure & 0xFF;
            


            uint16_t lit_sent = (uint16_t)((uint32_t)gSensorData.OilConsumption);
            
            tx_data[6] = (lit_sent >> 8) & 0xFF; 
            tx_data[7] = lit_sent & 0xFF;
            tx_data[8] = (rpm >> 8) & 0xFF; tx_data[9] = rpm & 0xFF;
            
            tx_data[10] = (time_run >> 24) & 0xFF;
            tx_data[11] = (time_run >> 16) & 0xFF;
            tx_data[12] = (time_run >> 8) & 0xFF;
            tx_data[13] = time_run & 0xFF;
            
            tx_data[14] = (time_sum >> 24) & 0xFF;
            tx_data[15] = (time_sum >> 16) & 0xFF;
            tx_data[16] = (time_sum >> 8) & 0xFF;
            tx_data[17] = time_sum & 0xFF;
            
            tx_data[18] = (Limit_MaintenanceTime >> 8) & 0xFF; tx_data[19] = Limit_MaintenanceTime & 0xFF;
            
            tx_data[20] = (gLastLatitude >> 24) & 0xFF;
            tx_data[21] = (gLastLatitude >> 16) & 0xFF;
            tx_data[22] = (gLastLatitude >> 8) & 0xFF;
            tx_data[23] = gLastLatitude & 0xFF;
            
            tx_data[24] = (gLastLongitude >> 24) & 0xFF;
            tx_data[25] = (gLastLongitude >> 16) & 0xFF;
            tx_data[26] = (gLastLongitude >> 8) & 0xFF;
            tx_data[27] = gLastLongitude & 0xFF;
            
            tx_data[28] = (gSystemUnixTime >> 24) & 0xFF;
            tx_data[29] = (gSystemUnixTime >> 16) & 0xFF;
            tx_data[30] = (gSystemUnixTime >> 8) & 0xFF;
            tx_data[31] = gSystemUnixTime & 0xFF;
            
            tx_data[32] = (gSensorData.OilForward >> 8) & 0xFF;
            tx_data[33] = gSensorData.OilForward & 0xFF;
            tx_data[34] = (gSensorData.OilReturn >> 8) & 0xFF;
            tx_data[35] = gSensorData.OilReturn & 0xFF;
            
            ESP32_Send_Packet(0xD2, 36, tx_data);
            break;
        }
        
        case 0xE1: /* Set Volt Min */
        case 0xE2: /* Set Volt Max */
        case 0xE3: /* Set Temperature Max */
        case 0xE4: /* Set Pressure Max */
        case 0xE5: /* Set Tacho max */
        case 0xE6: /* Set Oil flow Max */
        case 0xE7: /* Set Time Bao duong */
        {
            if (len >= 2) {
                uint16_t val = (data[0] << 8) | data[1];
                if      (cmd == 0xE1) { Limit_VinMin = val; W_Flash_Safe_Write_Int(AD_ADC_MIN, val); }
                else if (cmd == 0xE2) { Limit_VinMax = val; W_Flash_Safe_Write_Int(AD_ADC_MAX, val); }
                else if (cmd == 0xE3) { Limit_Temp = val;     W_Flash_Safe_Write_Int(AD_T_MAX, val); }
                else if (cmd == 0xE4) { Limit_Pressure = val; W_Flash_Safe_Write_Int(AD_PA_MAX, val); }
                else if (cmd == 0xE5) { Limit_RPM = val;      W_Flash_Safe_Write_Int(AD_TACHO_MAX, val); }
                else if (cmd == 0xE6) { Limit_Oil = val;      W_Flash_Safe_Write_Int(AD_OIL_MAX, val); }
                else if (cmd == 0xE7) { TIME_BAODUONG = val;  W_Flash_Safe_Write_Int(AD_TIME_BAODUONG, val); }
                
                tx_data[0] = 0x01; /* Success */
                tx_data[1] = 0x01;
                if (ESP32_LogEnable) LOG_Printf("ESP32: Updated Limit via CMD %02X to %d\r\n", cmd, val);
                ESP32_Send_Packet(cmd, 2, tx_data);
            }
            break;
        }
        
        case 0xD3: /* Set Volt Constant */
        case 0xD4: /* Set Temp Constant */
        case 0xD5: /* Set Pressure Constant */
        case 0xD6: /* Set Oil_1 Constant */
        case 0xD7: /* Set Oil_2 Constant */
        {
            if (len >= 2) {
                uint16_t val = (data[0] << 8) | data[1];
                if (cmd == 0xD3) { VOLT_CONS = val; W_Flash_Safe_Write_Int(AD_VOLT_CONS, val); }
                else if (cmd == 0xD4) { TEMP_CONS = val; W_Flash_Safe_Write_Int(AD_TEMP_CONS, val); }
                else if (cmd == 0xD5) { PA_CONS = val; W_Flash_Safe_Write_Int(AD_PA_CONS, val); }
                else if (cmd == 0xD6) { OIL_1_CONS = val; W_Flash_Safe_Write_Int(AD_OIL_1_CONS, val); }
                else if (cmd == 0xD7) { OIL_2_CONS = val; W_Flash_Safe_Write_Int(AD_OIL_2_CONS, val); }
                
                tx_data[0] = 0x01;
                tx_data[1] = 0x01;
                ESP32_Send_Packet(cmd, 2, tx_data);
            }
            break;
        }

        case 0xE8: /* Set Log Interval */
        {
            if (len >= 4) {
                uint32_t val = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | data[3];
                gLogInterval = val; 
                W_Flash_Safe_Write_Long(AD_LOG_INTERVAL, val);
                
                tx_data[0] = 0x01;
                tx_data[1] = 0x01;
                ESP32_Send_Packet(cmd, 2, tx_data);
            }
            break;
        }

        case 0xE9: /* Read Log Interval */
        {
            tx_data[0] = (gLogInterval >> 24) & 0xFF;
            tx_data[1] = (gLogInterval >> 16) & 0xFF;
            tx_data[2] = (gLogInterval >> 8) & 0xFF;
            tx_data[3] = gLogInterval & 0xFF;
            ESP32_Send_Packet(0xE9, 4, tx_data);
            break;
        }

        case 0xD8: /* Get All Constants */
        {
            tx_data[0] = (VOLT_CONS >> 8) & 0xFF; tx_data[1] = VOLT_CONS & 0xFF;
            tx_data[2] = (TEMP_CONS >> 8) & 0xFF; tx_data[3] = TEMP_CONS & 0xFF;
            tx_data[4] = (PA_CONS >> 8) & 0xFF; tx_data[5] = PA_CONS & 0xFF;
            tx_data[6] = (OIL_1_CONS >> 8) & 0xFF; tx_data[7] = OIL_1_CONS & 0xFF;
            tx_data[8] = (OIL_2_CONS >> 8) & 0xFF; tx_data[9] = OIL_2_CONS & 0xFF;
            ESP32_Send_Packet(0xD8, 10, tx_data);
            break;
        }

        case 0xEF: /* Get Device ID (Hardware UID) */
        {
            extern uint8_t gHardwareUID[12];
            memcpy(tx_data, gHardwareUID, 12);
            ESP32_Send_Packet(0xEF, 12, tx_data);
            break;
        }

        case 0xD0: /* Get Time Sum, Time Run, Count Record */
        {
            uint32_t time_sum = gRuntimeData.TotalTime;
            uint32_t time_run = gRuntimeData.RunTime;
            uint32_t record_count = gRecordCount;
            
            tx_data[0] = (time_sum >> 24) & 0xFF;
            tx_data[1] = (time_sum >> 16) & 0xFF;
            tx_data[2] = (time_sum >> 8) & 0xFF;
            tx_data[3] = time_sum & 0xFF;
            
            tx_data[4] = (time_run >> 24) & 0xFF;
            tx_data[5] = (time_run >> 16) & 0xFF;
            tx_data[6] = (time_run >> 8) & 0xFF;
            tx_data[7] = time_run & 0xFF;
            
            tx_data[8] = (record_count >> 24) & 0xFF;
            tx_data[9] = (record_count >> 16) & 0xFF;
            tx_data[10] = (record_count >> 8) & 0xFF;
            tx_data[11] = record_count & 0xFF;
            
            ESP32_Send_Packet(0xD0, 12, tx_data);
            break;
        }

        case 0xD1: /* Get Data Record */
        {
            if (len >= 2) {
                uint16_t index = (data[0] << 8) | data[1];
                DataRecord_t rec;
                memset(&rec, 0, sizeof(rec));
                if (index < gRecordCount) {
                    uint32_t addr = AD_START_OIL_RECORD + (index * sizeof(DataRecord_t));
                    W_Flash_Read_Arr(addr, (uint8_t*)&rec, sizeof(DataRecord_t));
                }
                
                tx_data[0] = (rec.OilConsumed >> 8) & 0xFF; tx_data[1] = rec.OilConsumed & 0xFF;
                tx_data[2] = (rec.Voltage >> 8) & 0xFF; tx_data[3] = rec.Voltage & 0xFF;
                tx_data[4] = (rec.Temperature >> 8) & 0xFF; tx_data[5] = rec.Temperature & 0xFF;
                tx_data[6] = (rec.Pressure >> 8) & 0xFF; tx_data[7] = rec.Pressure & 0xFF;
                tx_data[8] = (rec.RPM >> 8) & 0xFF; tx_data[9] = rec.RPM & 0xFF;
                tx_data[10] = (rec.Timestamp >> 24) & 0xFF;
                tx_data[11] = (rec.Timestamp >> 16) & 0xFF;
                tx_data[12] = (rec.Timestamp >> 8) & 0xFF;
                tx_data[13] = rec.Timestamp & 0xFF;
                tx_data[14] = rec.RecordType; /* 0: Periodic, 1: Startup */
                
                tx_data[15] = (rec.Latitude >> 24) & 0xFF;
                tx_data[16] = (rec.Latitude >> 16) & 0xFF;
                tx_data[17] = (rec.Latitude >> 8) & 0xFF;
                tx_data[18] = rec.Latitude & 0xFF;
                
                tx_data[19] = (rec.Longitude >> 24) & 0xFF;
                tx_data[20] = (rec.Longitude >> 16) & 0xFF;
                tx_data[21] = (rec.Longitude >> 8) & 0xFF;
                tx_data[22] = rec.Longitude & 0xFF;
                
                tx_data[23] = (rec.EngineRunTime >> 24) & 0xFF;
                tx_data[24] = (rec.EngineRunTime >> 16) & 0xFF;
                tx_data[25] = (rec.EngineRunTime >> 8) & 0xFF;
                tx_data[26] = rec.EngineRunTime & 0xFF;
                
                ESP32_Send_Packet(0xD1, 27, tx_data);
            }
            break;
        }

        case 0xB1: /* Set Time RTC */
        {
            if (len >= 7) {
                uint8_t h = data[0], m = data[1], s = data[2];
                uint8_t day = data[3];
                uint8_t dt = data[4], mo = data[5], y = data[6];
                
                DS1307_Time t;
                t.hour = h; t.min = m; t.sec = s;
                t.day = (day == 0) ? 1 : day; // Prevent day 0
                t.date = dt; t.month = mo; t.year = y % 100;
                if (DS1307_SetTime(&t)) {
                    gSystemUnixTime = DateTimeToUnix(y, mo, dt, h, m, s);
                }
                
                tx_data[0] = 0x01; tx_data[1] = 0x01;
                ESP32_Send_Packet(0xB1, 2, tx_data);
            }
            break;
        }

        case 0xB2: /* Get Time RTC */
        {
            DS1307_Time t = {0};
            DS1307_GetTime(&t); // Lấy giờ thực tế từ phần cứng
            
            tx_data[0] = t.hour;
            tx_data[1] = t.min;
            tx_data[2] = t.sec;
            tx_data[3] = t.day;
            tx_data[4] = t.date;
            tx_data[5] = t.month;
            tx_data[6] = t.year; // Trả về dạng 2 số (0-99)
            
            ESP32_Send_Packet(0xB2, 7, tx_data);
            break;
        }

        case 0xDA: /* Reboot System */
        {
            tx_data[0] = 0x01; tx_data[1] = 0x01;
            ESP32_Send_Packet(0xDA, 2, tx_data);
            NVIC_SystemReset();
            break;
        }

        case 0xDB: /* Set Relay State (ID: 1=Buzzer) */
        {
            if (len >= 2) {
                uint8_t relay_id = data[0];
                uint8_t state = data[1];
                if (relay_id == 1) {
                    // Turn on/off Buzzer directly
                    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
                }
                else if (relay_id == 2) {
                    if (state == 1) {
                        HAL_GPIO_WritePin(RELAY_1_GPIO_Port, RELAY_1_Pin, GPIO_PIN_SET);
                        relay1_last_cmd_ms = HAL_GetTick();
                        relay1_active = true;
                    } else {
                        HAL_GPIO_WritePin(RELAY_1_GPIO_Port, RELAY_1_Pin, GPIO_PIN_RESET);
                        relay1_active = false;
                    }
                }
                else if (relay_id == 3) {
                    if (state == 1) {
                        HAL_GPIO_WritePin(RELAY_2_GPIO_Port, RELAY_2_Pin, GPIO_PIN_SET);
                        relay2_last_cmd_ms = HAL_GetTick();
                        relay2_active = true;
                    } else {
                        HAL_GPIO_WritePin(RELAY_2_GPIO_Port, RELAY_2_Pin, GPIO_PIN_RESET);
                        relay2_active = false;
                    }
                }
                tx_data[0] = 0x01; tx_data[1] = 0x01;
                ESP32_Send_Packet(0xDB, 2, tx_data);
            }
            break;
        }

        case 0xCB: /* Debug Raw Oil Pulses */
        {
        	//esp32 raw
            uint32_t t1 = ExtiCounter_GetOil1();
            uint32_t t2 = ExtiCounter_GetOil2();
            uint32_t net_pulses = (t1 > t2) ? (t1 - t2) : 0;
            uint32_t liters_x100 = gSensorData.OilConsumption; // Lít * 100
            
            extern uint16_t OIL_1_CONS;
            extern uint16_t OIL_2_CONS;
            uint32_t c1 = (OIL_1_CONS > 0) ? OIL_1_CONS : 1000;
            uint32_t c2 = (OIL_2_CONS > 0) ? OIL_2_CONS : 1000;
            uint32_t liters_fwd_x100 = (t1 * 1000) / c1;
            uint32_t liters_ret_x100 = (t2 * 1000) / c2;
            
            tx_data[0] = (t1 >> 24) & 0xFF;
            tx_data[1] = (t1 >> 16) & 0xFF;
            tx_data[2] = (t1 >> 8) & 0xFF;
            tx_data[3] = t1 & 0xFF;
            
            tx_data[4] = (t2 >> 24) & 0xFF;
            tx_data[5] = (t2 >> 16) & 0xFF;
            tx_data[6] = (t2 >> 8) & 0xFF;
            tx_data[7] = t2 & 0xFF;
            
            tx_data[8] = (net_pulses >> 24) & 0xFF;
            tx_data[9] = (net_pulses >> 16) & 0xFF;
            tx_data[10] = (net_pulses >> 8) & 0xFF;
            tx_data[11] = net_pulses & 0xFF;
            
            tx_data[12] = (liters_x100 >> 24) & 0xFF;
            tx_data[13] = (liters_x100 >> 16) & 0xFF;
            tx_data[14] = (liters_x100 >> 8) & 0xFF;
            tx_data[15] = liters_x100 & 0xFF;
            
            tx_data[16] = (liters_fwd_x100 >> 24) & 0xFF;
            tx_data[17] = (liters_fwd_x100 >> 16) & 0xFF;
            tx_data[18] = (liters_fwd_x100 >> 8) & 0xFF;
            tx_data[19] = liters_fwd_x100 & 0xFF;
            
            tx_data[20] = (liters_ret_x100 >> 24) & 0xFF;
            tx_data[21] = (liters_ret_x100 >> 16) & 0xFF;
            tx_data[22] = (liters_ret_x100 >> 8) & 0xFF;
            tx_data[23] = liters_ret_x100 & 0xFF;
            
            ESP32_Send_Packet(0xCB, 24, tx_data);
            break;
        }

        case 0xDC: /* Get Raw Data */
        {
            extern uint16_t PA_VALUE;
            uint16_t raw_vin = ADC_Read_Channel(ADC_VIN_CHANNEL);
            uint16_t raw_temp = ADC_Read_Channel(ADC_TEMP_CHANNEL);
            
            tx_data[0] = (raw_vin >> 8) & 0xFF; tx_data[1] = raw_vin & 0xFF;
            tx_data[2] = (raw_temp >> 8) & 0xFF; tx_data[3] = raw_temp & 0xFF;
            tx_data[4] = (PA_VALUE >> 8) & 0xFF; tx_data[5] = PA_VALUE & 0xFF;
            tx_data[6] = 0; tx_data[7] = 0; // Mock raw oil pulse
            tx_data[8] = 0; tx_data[9] = 0; // Mock raw rpm pulse
            ESP32_Send_Packet(0xDC, 10, tx_data);
            break;
        }

        case 0xDD: /* Reset Runtime */
        {
            gRuntimeData.TotalTime = 0;
            gRuntimeData.RunTime = 0;
            W_Flash_Safe_Write_Long(AD_TIME_SUM, 0);
            W_Flash_Safe_Write_Long(AD_TIME_RUN, 0);
            
            tx_data[0] = 0x01; tx_data[1] = 0x01;
            ESP32_Send_Packet(0xDD, 2, tx_data);
            break;
        }

        case 0xDE: /* Set Buzzer Mute State */
        {
            if (len >= 1) {
                uint8_t mute_state = data[0];
                extern void Buzzer_SetMute(bool mute);
                Buzzer_SetMute(mute_state ? true : false);
                
                tx_data[0] = 0x01; tx_data[1] = 0x01;
                ESP32_Send_Packet(0xDE, 2, tx_data);
            }
            break;
        }

        case 0xCC: /* SET Sensor RunTimes (20 Bytes) */
        {
            if (len >= 20) {
                for (int i = 0; i < 5; i++) {
                    uint32_t rt = ((uint32_t)data[i*4] << 24) | ((uint32_t)data[i*4+1] << 16) | 
                                  ((uint32_t)data[i*4+2] << 8) | data[i*4+3];
                    gSensorRunTimes[i] = rt;
                    W_Flash_Safe_Write_Long(AD_SENSOR_RUNTIME_BASE + (i * 4), rt);
                }
                tx_data[0] = 0x01; tx_data[1] = 0x01;
                ESP32_Send_Packet(0xCC, 2, tx_data);
            }
            break;
        }

        case 0xCD: /* GET Sensor RunTimes */
        {
            extern uint32_t Sensor_GetOperatingHours(uint8_t sensor);
            for (int i = 0; i < 5; i++) {
                uint32_t rt = Sensor_GetOperatingHours(i);
                tx_data[i*4] = (rt >> 24) & 0xFF;
                tx_data[i*4+1] = (rt >> 16) & 0xFF;
                tx_data[i*4+2] = (rt >> 8) & 0xFF;
                tx_data[i*4+3] = rt & 0xFF;
            }
            ESP32_Send_Packet(0xCD, 20, tx_data);
            break;
        }

        case 0xDF: /* Delete Flash */
        {
            W_Flash_Chip_Erase();
            gRecordCount = 0;
            W_Flash_Write_Long(AD_OIL_INDEX, 0);
            
            tx_data[0] = 0x01; tx_data[1] = 0x01;
            ESP32_Send_Packet(0xDF, 2, tx_data);
            
            // Should reboot system to reload defaults gracefully
            NVIC_SystemReset();
            break;
        }
        
        case 0xD9: /* Get System Information */
        {
            memcpy(tx_data, gHardwareUID, 12);
            tx_data[12] = (gFirmwareVersion >> 24) & 0xFF;
            tx_data[13] = (gFirmwareVersion >> 16) & 0xFF;
            tx_data[14] = (gFirmwareVersion >> 8) & 0xFF;
            tx_data[15] = gFirmwareVersion & 0xFF;
            tx_data[16] = gLedIntensity;
            extern bool Buzzer_IsMuted(void);
            tx_data[17] = Buzzer_IsMuted() ? 1 : 0; // Buzzer state (1 or 0)
            tx_data[18] = gSystemControlMode;
            ESP32_Send_Packet(0xD9, 19, tx_data);
            break;
        }

        case 0xEB: /* Set Sensor Maint Limits */
        {
            if (len >= 10) {
                for(int i=0; i<5; i++) {
                    gSensorMaintLimits[i] = (data[i*2] << 8) | data[i*2+1];
                }
                W_Flash_Write_Arr(AD_SENSOR_MAINT_LIMIT_BASE, data, 10);
                
                tx_data[0] = 0x01; tx_data[1] = 0x01;
                ESP32_Send_Packet(0xEB, 2, tx_data);
            }
            break;
        }

        case 0xEC: /* Get Sensor Maint Limits */
        {
            for(int i=0; i<5; i++) {
                tx_data[i*2] = (gSensorMaintLimits[i] >> 8) & 0xFF;
                tx_data[i*2+1] = gSensorMaintLimits[i] & 0xFF;
            }
            ESP32_Send_Packet(0xEC, 10, tx_data);
            break;
        }

        case 0xED: /* Set Sensor Enable Mask */
        {
            if (len >= 1) {
                gSensorEnableMask = data[0];
                W_Flash_Safe_Write_Int(AD_SENSOR_ENABLE_MASK, gSensorEnableMask);
                
                tx_data[0] = 0x01; tx_data[1] = 0x01;
                ESP32_Send_Packet(0xED, 2, tx_data);
            }
            break;
        }

        case 0xEE: /* Set System Control Mode */
        {
            if (len >= 1) {
                gSystemControlMode = data[0];
                W_Flash_Safe_Write_Int(AD_SYSTEM_CONTROL_MODE, gSystemControlMode);
                
                tx_data[0] = 0x01; tx_data[1] = 0x01;
            }
            break;
        }

        case 0xF0: /* Set LED Brightness */
        {
            if (len >= 1) {
                uint8_t brightness = data[0];
                if (brightness > 15) brightness = 15; // MAX7219 max limit
                
                extern uint8_t gLedIntensity;
                gLedIntensity = brightness;
                
                #include "Flash_definition.h"
                extern void W_Flash_Safe_Write_Int(uint32_t addr, uint16_t data);
                W_Flash_Safe_Write_Int(AD_LED_INTENSITY, gLedIntensity);
                
                #include "max7219.h"
                MAX7219_SetIntensity(gLedIntensity);
                
                tx_data[0] = 0x01; tx_data[1] = gLedIntensity;
                ESP32_Send_Packet(0xF0, 2, tx_data);
            }
            break;
        }

        case 0xF1: /* Set Pressure Min */
        {
            if (len >= 2) {
                uint16_t val = (data[0] << 8) | data[1];
                extern uint16_t Limit_PressureMin;
                Limit_PressureMin = val;
                
                #include "Flash_definition.h"
                extern void W_Flash_Safe_Write_Int(uint32_t addr, uint16_t data);
                W_Flash_Safe_Write_Int(AD_PA_MIN, val);
                
                tx_data[0] = 0x01; /* Success */
                tx_data[1] = 0x01;
                ESP32_Send_Packet(0xF1, 2, tx_data);
            }
            break;
        }

        default:
            break;
    }
}

void ESP32_Task(void)
{
    /* Kiểm tra trạng thái giao tiếp ESP32 để bật/tắt LED WIFI */
    if (HAL_GetTick() - last_esp_comm_ms < 30000) {
        Led_wifi(true);
    } else {
        Led_wifi(false);
    }

    /* Kiểm tra Watchdog cho Relay 1 và Relay 2 (200ms) */
    if (relay1_active && (HAL_GetTick() - relay1_last_cmd_ms >= 200)) {
        HAL_GPIO_WritePin(RELAY_1_GPIO_Port, RELAY_1_Pin, GPIO_PIN_RESET);
        relay1_active = false;
    }
    
    if (relay2_active && (HAL_GetTick() - relay2_last_cmd_ms >= 200)) {
        HAL_GPIO_WritePin(RELAY_2_GPIO_Port, RELAY_2_Pin, GPIO_PIN_RESET);
        relay2_active = false;
    }

    while (esp_rx_head != esp_rx_tail) {
        uint8_t b = esp_rx_buf[esp_rx_tail];
        esp_rx_tail = (esp_rx_tail + 1) % ESP_RX_BUFFER_SIZE;
        
        switch (state) {
            case STATE_WAIT_STX:
                if (b == 0x02) {
                    state = STATE_WAIT_04;
                }
                break;
                
            case STATE_WAIT_04:
                if (b == 0x04) {
                    state = STATE_WAIT_CMD;
                } else if (b == 0x02) {
                    state = STATE_WAIT_04;
                } else {
                    state = STATE_WAIT_STX;
                }
                break;
                
            case STATE_WAIT_CMD:
                current_cmd = b;
                // Kiểm tra xem lệnh có trong danh sách hợp lệ không
                if (current_cmd == 0xE0 || current_cmd == 0xD2 || current_cmd == 0xD0 || current_cmd == 0xD8 || current_cmd == 0xB2 || current_cmd == 0xDF || current_cmd == 0xDA || current_cmd == 0xDC || current_cmd == 0xDD || current_cmd == 0xE9 || current_cmd == 0xEA || current_cmd == 0xD9 || current_cmd == 0xEC || current_cmd == 0xEF || current_cmd == 0xCB) {
                    /* Những lệnh này không có byte DATA, ta coi như LEN=0 */
                    current_len = 0;
                    state = STATE_WAIT_CS;
                } else {
                    state = STATE_WAIT_LEN;
                }
                break;
                
            case STATE_WAIT_LEN:
                current_len = b;
                data_idx = 0;
                if (current_len > 0) {
                    state = STATE_WAIT_DATA;
                } else {
                    state = STATE_WAIT_CS;
                }
                break;
                
            case STATE_WAIT_DATA:
                if (data_idx < sizeof(data_buf)) {
                    data_buf[data_idx++] = b;
                }
                if (data_idx >= current_len) {
                    state = STATE_WAIT_CS;
                }
                break;
                
            case STATE_WAIT_CS:
                /* Bỏ qua kiểm tra byte 0x05, chờ ETX */
                if (b == 0x05 || b == 0x03) {
                    if (b == 0x03) {
                        ESP32_Process_Command(current_cmd, data_buf, current_len);
                        state = STATE_WAIT_STX;
                    } else {
                        state = STATE_WAIT_ETX;
                    }
                } else {
                    state = STATE_WAIT_STX;
                }
                break;
                
            case STATE_WAIT_ETX:
                if (b == 0x03) {
                    ESP32_Process_Command(current_cmd, data_buf, current_len);
                }
                state = STATE_WAIT_STX;
                break;
        }
    }
}
