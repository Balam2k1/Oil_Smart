/******************************************************************************
 * File    : modbus_master.c
 * Brief   : Modbus RTU Master
 ******************************************************************************/

#include "modbus_master.h"
#include "rs485.h"
#include "string.h"
#include "main.h"
#include "app_config.h"

/*========================== CONFIG ============================*/

/* Đọc 2 thanh ghi */
#define MODBUS_FUNCTION_READ_HOLDING      0x03

/* Timeout phản hồi */
#define MODBUS_RESPONSE_TIMEOUT_MS        200U

/*=========================== TYPE ============================*/

typedef enum
{
    MODBUS_STATE_IDLE = 0,

    MODBUS_STATE_WAIT_RESPONSE

} ModbusState_t;

/*======================== PRIVATE DATA =======================*/

static ModbusSensor_t Sensors[MODBUS_SENSOR_COUNT];

static ModbusState_t ModbusState;

static uint8_t CurrentSensor;

static uint32_t RequestTick;

/*======================= PRIVATE FUNC ========================*/

/******************************************************************************
 * CRC16 Modbus
 ******************************************************************************/
uint16_t Modbus_CRC16(
        const uint8_t *data,
        uint16_t length)
{
    uint16_t crc = 0xFFFF;

    uint16_t pos;
    uint8_t  i;

    for(pos = 0; pos < length; pos++)
    {
        crc ^= data[pos];

        for(i = 0; i < 8; i++)
        {
            if(crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/******************************************************************************
 * Gửi lệnh đọc 6 thanh ghi từ address 0x0000
 ******************************************************************************/
static void Modbus_SendReadRequest(
        uint8_t slaveId)
{
    uint8_t frame[8];

    uint16_t crc;

    frame[0] = slaveId;

    frame[1] = 0x03;

    frame[2] = 0x00;
    frame[3] = 0x00;

    frame[4] = 0x00;
    frame[5] = 0x06;

    crc = Modbus_CRC16(frame, 6);

    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;

    RS485_Send(frame, sizeof(frame));
}

/******************************************************************************
 * Parse phản hồi
 ******************************************************************************/
static uint8_t Modbus_ParseResponse(
        uint8_t expectedId)
{
    uint8_t* rx;
    uint16_t len;

    uint16_t crc_calc;
    uint16_t crc_recv;

    rx = RS485_GetRxBuffer();

    len = RS485_GetRxLength();

    /* 6 registers: 3 (header) + 12 (data) + 2 (crc) = 17 bytes */
    if(len < 17)
    {
        return 0;
    }

    /* Kiểm tra ID và Function Code */
    if(rx[0] != expectedId || rx[1] != 0x03)
    {
        return 0;
    }

    crc_calc = Modbus_CRC16(rx, len - 2);

    crc_recv =
            rx[len - 2]
          | (rx[len - 1] << 8);

    if(crc_calc != crc_recv)
    {
        return 0;
    }

    Sensors[CurrentSensor].RawData =
            (rx[3] << 8)
          | rx[4];

    Sensors[CurrentSensor].OperatingHours =
            (rx[5] << 8)
          | rx[6];

    Sensors[CurrentSensor].ModbusID =
            (rx[7] << 8)
          | rx[8];

    Sensors[CurrentSensor].SensorType =
            (rx[9] << 8)
          | rx[10];

    Sensors[CurrentSensor].SerialNumH =
            (rx[11] << 8)
          | rx[12];

    Sensors[CurrentSensor].SerialNumL =
            (rx[13] << 8)
          | rx[14];

    Sensors[CurrentSensor].Online = 1;

    return 1;
}

/*========================== PUBLIC ==========================*/

/******************************************************************************
 * Khởi tạo
 ******************************************************************************/
void ModbusMaster_Init(void)
{
    uint8_t i;

    memset(
        Sensors,
        0,
        sizeof(Sensors));

    /* Default IDs: 101, 102, 103, 104, 105 */
    for(i = 0; i < MODBUS_SENSOR_COUNT; i++)
    {
        Sensors[i].SlaveId = 101 + i;
    }

    CurrentSensor = 0;

    ModbusState = MODBUS_STATE_IDLE;
}

/******************************************************************************
 * Task chính
 ******************************************************************************/
void ModbusMaster_Task(void)
{
    static uint32_t LastActionTick = 0;

    switch(ModbusState)
    {
        case MODBUS_STATE_IDLE:
        {
            /* Delay giữa các lần polling (Modbus RTU T3.5) */
            if(HAL_GetTick() - LastActionTick < 20)
            {
                return;
            }

            Modbus_SendReadRequest(
                    Sensors[CurrentSensor].SlaveId);

            RequestTick = HAL_GetTick();

            ModbusState =
                    MODBUS_STATE_WAIT_RESPONSE;

            break;
        }

        case MODBUS_STATE_WAIT_RESPONSE:
        {
            if(RS485_IsFrameReady())
            {
                if(Modbus_ParseResponse(Sensors[CurrentSensor].SlaveId))
                {
                    RS485_ClearFrame();

                    CurrentSensor++;

                    if(CurrentSensor >= MODBUS_SENSOR_COUNT)
                    {
                        CurrentSensor = 0;
                    }

                    LastActionTick = HAL_GetTick();

                    ModbusState =
                            MODBUS_STATE_IDLE;
                }
                else
                {
                    /* Nếu frame không hợp lệ (noise/echo), xóa để đợi tiếp hoặc timeout */
                    RS485_ClearFrame();
                }
            }

            if((HAL_GetTick() - RequestTick)
                    >= MODBUS_RESPONSE_TIMEOUT_MS)
            {
                Sensors[CurrentSensor].Online = 0;

                CurrentSensor++;

                if(CurrentSensor >= MODBUS_SENSOR_COUNT)
                {
                    CurrentSensor = 0;
                }

                LastActionTick = HAL_GetTick();

                ModbusState =
                        MODBUS_STATE_IDLE;
            }

            break;
        }

        default:
        {
            ModbusState =
                    MODBUS_STATE_IDLE;
            break;
        }
    }
}

/******************************************************************************
 * Lấy dữ liệu cảm biến
 ******************************************************************************/
ModbusSensor_t* ModbusMaster_GetSensor(
        uint8_t index)
{
    if(index >= MODBUS_SENSOR_COUNT)
    {
        return 0;
    }

    return &Sensors[index];
}
