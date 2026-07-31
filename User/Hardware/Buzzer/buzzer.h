#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

void Buzzer_Init(void);
void Buzzer_Task(void);

void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_SetMute(bool mute);
bool Buzzer_IsMuted(void);

void Buzzer_StartPattern(
        uint16_t OnTimeMs,
        uint16_t OffTimeMs);

void Buzzer_StopPattern(void);

#endif
