#ifndef _HC595_H_
#define _HC595_H_

#include "main.h"
#include "app.h"



/*=============================================================================
 * HC595 CONSTANTS & FUNCTIONS
 *============================================================================*/

#define LED_WIFI_ON       10
extern uint8_t HC595_State;

void HC595_Init(void);
void HC595_SendByte(uint8_t data);
void HC595_SendBytes(uint8_t *data, uint8_t length) ;
void HC595_Latch(void);
void HC595_Display(uint8_t data);
void Fan(bool cmd, uint8_t speed);
void HC595_UpdateFan(uint16_t rpm);
void Led_wifi(bool on);
void Led_warning(bool on);
void Led_maintain(bool on);

void Test_Full_Led(void);

#endif
