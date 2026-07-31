/******************************************************************************
 * File    : modbus_master.h
 * Brief   : Modbus RTU Master
 ******************************************************************************/

#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include <stdint.h>



/*========================== TYPE ==========================*/

typedef struct
{
    uint8_t  SlaveId;

    uint16_t RawData;        /* 0x0000 */

    uint16_t OperatingHours; /* 0x0001 */

    uint16_t ModbusID;       /* 0x0002 */

    uint16_t SensorType;     /* 0x0003 */

    uint16_t SerialNumH;     /* 0x0004 */

    uint16_t SerialNumL;     /* 0x0005 */

    uint8_t  Online;

} ModbusSensor_t;

/*========================== API ===========================*/

/* Khởi tạo */
void ModbusMaster_Init(void);

/* Task chính */
void ModbusMaster_Task(void);

/* Lấy dữ liệu cảm biến */
ModbusSensor_t* ModbusMaster_GetSensor(uint8_t index);

/* CRC16 */
uint16_t Modbus_CRC16(
        const uint8_t *data,
        uint16_t length);

#endif
