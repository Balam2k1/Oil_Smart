/******************************************************************************
 * File    : rs485.c
 * Brief   : RS485 Physical Layer Driver
 ******************************************************************************/
#include "app_config.h"
#include "rs485.h"
#include "main.h"
#include "string.h"


/* Byte nhận từng lần */
static uint8_t RxByte;



/*======================== PRIVATE DATA ============================*/

/* Buffer nhận */
static uint8_t Rs485RxBuffer[RS485_RX_BUFFER_SIZE];

/* Độ dài frame hiện tại */
static volatile uint16_t Rs485RxLength;

/* Tick nhận byte cuối */
static volatile uint32_t LastRxTick;

/* Cờ frame hoàn chỉnh */
static volatile uint8_t FrameReady;

/*======================== PRIVATE FUNC ============================*/

/******************************************************************************
 * @brief Chuyển sang chế độ truyền
 ******************************************************************************/
static void RS485_SetTxMode(void)
{
	HAL_GPIO_WritePin(
	    RS485_DE_PORT,
	    RS485_DE_PIN,
	    GPIO_PIN_SET);
}

/******************************************************************************
 * @brief Chuyển sang chế độ nhận
 ******************************************************************************/
static void RS485_SetRxMode(void)
{
	HAL_GPIO_WritePin(
	    RS485_DE_PORT,
	    RS485_DE_PIN,
	    GPIO_PIN_RESET);
}

/*=========================== PUBLIC ===============================*/

/******************************************************************************
 * @brief Khởi tạo RS485
 ******************************************************************************/
void RS485_Init(void)
{
    Rs485RxLength = 0U;

    LastRxTick = 0U;

    FrameReady = 0U;

    memset(
        Rs485RxBuffer,
        0,
        sizeof(Rs485RxBuffer));


    RS485_SetRxMode();

    HAL_UART_Receive_IT(
        RS485_UART,
        &RxByte,
        1);

}

/******************************************************************************
 * @brief Task xử lý timeout frame
 ******************************************************************************/
void RS485_Task(void)
{
    if(Rs485RxLength == 0U)
    {
        return;
    }

    if(FrameReady == 1U)
    {
        return;
    }

    if((HAL_GetTick() - LastRxTick)
            >= RS485_FRAME_TIMEOUT_MS)
    {
        FrameReady = 1U;
    }
}

/******************************************************************************
 * @brief Gửi dữ liệu
 ******************************************************************************/
uint8_t RS485_Send(
        const uint8_t *data,
        uint16_t length)
{
    if(data == NULL)
    {
        return 1U;
    }

    RS485_SetTxMode();

    if(HAL_UART_Transmit(
            RS485_UART,
            (uint8_t*)data,
            length,
            100U) != HAL_OK)
    {
        RS485_SetRxMode();

        return 1U;
    }

    while(__HAL_UART_GET_FLAG(
              RS485_UART,
              UART_FLAG_TC) == RESET);

    /* Xóa echo sau khi truyền xong */
    RS485_ClearFrame();

    RS485_SetRxMode();

    return 0U;
}
/******************************************************************************
 * @brief Xóa frame hiện tại
 ******************************************************************************/
void RS485_ClearFrame(void)
{
    Rs485RxLength = 0U;

    FrameReady = 0U;

    memset(
        Rs485RxBuffer,
        0,
        sizeof(Rs485RxBuffer));
}

/******************************************************************************
 * @brief Có frame mới hay chưa
 ******************************************************************************/
uint8_t RS485_IsFrameReady(void)
{
    return FrameReady;
}

/******************************************************************************
 * @brief Trả về buffer RX
 ******************************************************************************/
uint8_t* RS485_GetRxBuffer(void)
{
    return Rs485RxBuffer;
}

/******************************************************************************
 * @brief Trả về độ dài frame RX
 ******************************************************************************/
uint16_t RS485_GetRxLength(void)
{
    return Rs485RxLength;
}

/******************************************************************************
 * @brief Nhận từng byte từ UART ISR
 ******************************************************************************/
void RS485_RxHandler(uint8_t byte)
{
    if(Rs485RxLength >= RS485_RX_BUFFER_SIZE)
    {
        return;
    }

    Rs485RxBuffer[Rs485RxLength++] = byte;

    LastRxTick = HAL_GetTick();

    FrameReady = 0U;
}


/* Callback UART */
void RS485_UART_Callback(void)
{
    RS485_RxHandler(RxByte);

    HAL_UART_Receive_IT(
        RS485_UART,
        &RxByte,
        1);
}


/******************************************************************************
 * Test gửi chuỗi
 ******************************************************************************/
void RS485_Test(void)
{
    static uint32_t LastTick = 0;

    if(HAL_GetTick() - LastTick >= 1000)
    {
        LastTick = HAL_GetTick();

        uint8_t msg[] = "HELLO\r\n";

        RS485_Send(
            msg,
            sizeof(msg)-1);
    }
}

/******************************************************************************
 * Test Echo
 ******************************************************************************/
void RS485_EchoTest(void)
{
    if(RS485_IsFrameReady())
    {
        RS485_Send(
            RS485_GetRxBuffer(),
            RS485_GetRxLength());

        RS485_ClearFrame();
    }
}

extern void GPS_UART_Callback(void);
extern void GPS_RestartReceive(void);
extern void ESP32_UART_Callback(void);
extern void LOG_UART_Callback(void);

/* HAL UART Receive Complete Callback */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == RS485_UART->Instance)
    {
        RS485_UART_Callback();
    }
    else if (huart->Instance == USART3)
    {
        GPS_UART_Callback();
    }
    else if (huart->Instance == ESP_UART->Instance)
    {
        ESP32_UART_Callback();
    }
    else if (huart->Instance == USART4)
    {
        LOG_UART_Callback();
    }
}

extern void ESP32_RestartReceive(void);
extern void LOG_RestartReceive(void);

/* HAL UART Error Callback (resets receiver on noise/overrun) */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == RS485_UART->Instance)
    {
        /* Clear error flags and restart receive interrupt */
        HAL_UART_Receive_IT(RS485_UART, &RxByte, 1);
    }
    else if (huart->Instance == USART3)
    {
        GPS_RestartReceive();
    }
    else if (huart->Instance == ESP_UART->Instance)
    {
        ESP32_RestartReceive();
    }
    else if (huart->Instance == USART4)
    {
        LOG_RestartReceive();
    }
}




