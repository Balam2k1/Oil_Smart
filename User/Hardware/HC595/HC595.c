#include "hc595.h"
#include "main.h"

uint8_t HC595_State = 0x02;

// Function to send one byte to the shift register
void HC595_SendByte(uint8_t data)
{
    for (int i = 0; i < 8; i++)
    {
        // Write the data bit to the data pin
        HAL_GPIO_WritePin(HC595_DATA_PORT, HC595_DATA_PIN, (data & 0x80) ? GPIO_PIN_SET : GPIO_PIN_RESET);     

        // Pulse the clock pin
        HAL_GPIO_WritePin(HC595_CLOCK_PORT, HC595_CLOCK_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(HC595_CLOCK_PORT, HC595_CLOCK_PIN, GPIO_PIN_RESET);

        // Shift the data to the next bit
        data <<= 1;
    }
}

// Function to send multiple bytes to chained 74HC595Ds
void HC595_SendBytes(uint8_t *data, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++) {
        HC595_SendByte(data[i]);
    }
}

// Function to latch the data to output pins
void HC595_Latch(void)
{
    // Pulse the latch pin
    HAL_GPIO_WritePin(HC595_LATCH_PORT, HC595_LATCH_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HC595_LATCH_PORT, HC595_LATCH_PIN, GPIO_PIN_RESET);
}

void HC595_Display(uint8_t data)
{
    HC595_SendByte(data); // Send data to the shift register
    HC595_Latch();        // Latch the data to the output pins
}

void HC595_Init(void) {
    HC595_Display(0x00);
}

void Fan(bool cmd, uint8_t speed)
{
    uint8_t fanBits = 0x1f;
    if(cmd==true)
    {
        uint32_t cycle = HAL_GetTick() % (4 * speed);
        if (cycle < speed)
        {
            HC595_State = (HC595_State & ~fanBits) | 0x04;
        }else if(cycle < 2 * speed)
        {
            HC595_State = (HC595_State & ~fanBits) | 0x10;
        }else if(cycle < 3 * speed)
        {
            HC595_State = (HC595_State & ~fanBits) | 0x01;
        }else
        {
            HC595_State = (HC595_State & ~fanBits) | 0x08;
        }
    }else{
        HC595_State = HC595_State& (~fanBits|0x02);
    }
    HC595_Display(HC595_State);
}

extern uint16_t Limit_RPM;

void HC595_UpdateFan(uint16_t rpm)
{
    if (rpm > 0)
    {
        uint32_t limit = Limit_RPM;
        if (limit == 0) limit = 3000; // Fallback limit to prevent division by zero
        
        uint32_t level = (uint32_t)rpm * 10 / limit;
        if (level == 0) level = 1;
        if (level > 10) level = 10;
        
        // Map 10 levels to step delays: Level 1 -> 300ms, Level 10 -> 30ms
        uint8_t fan_delay = 330 - (level * 30);
        
        Fan(true, fan_delay);
    }
    else
    {
        Fan(false, 0);
    }
}

void Led_wifi(bool on)
{
    uint8_t wifiMask = 0x20;
    if( on ==true) {
        HC595_State |= wifiMask;
    }
    else {
        HC595_State &= ~wifiMask;
    }
    HC595_Display(HC595_State);
}

void Led_warning(bool on)
{
    uint8_t w = 0x40;
    if( on ==true) {
        HC595_State |= w;
    }
    else {
        HC595_State &= ~w;
    }
    HC595_Display(HC595_State);
}

void Led_maintain(bool on)
{
    uint8_t m = 0x80;
    if( on ==true) {
        HC595_State |= m;
    }
    else {
        HC595_State &= ~m;
    }
    HC595_Display(HC595_State);
}

void Test_Full_Led(void)
{

        Fan(1,200);
        Led_wifi(1);
        Led_maintain(1);
        Led_warning(1);


}
