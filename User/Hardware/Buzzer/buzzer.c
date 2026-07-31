#include "buzzer.h"
#include "app_config.h"
#include "main.h"
#include "Flash_definition.h"
#include "w25q32.h"
#include "log.h"

static bool     BuzzerEnable = false;
static bool     gBuzzerMuted = false;

static uint16_t BuzzerOnTime;
static uint16_t BuzzerOffTime;

static uint32_t BuzzerTick;

static bool     BuzzerState = false;



void Buzzer_On(void)
{
    if (gBuzzerMuted) return; // Muted

    HAL_GPIO_WritePin(
        BUZZER_PORT,
        BUZZER_PIN,
        GPIO_PIN_SET);
}

void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(
        BUZZER_PORT,
        BUZZER_PIN,
        GPIO_PIN_RESET);
}


void Buzzer_Init(void)
{
    BuzzerEnable = 0;

    // Đọc trạng thái Mute từ Flash
    uint16_t mute_val = W_Flash_Read_Int(AD_BUZZER_MUTE);
    if (mute_val == 1) {
        gBuzzerMuted = true;
    } else {
        gBuzzerMuted = false;
        if (mute_val != 0) {
            W_Flash_Safe_Write_Int(AD_BUZZER_MUTE, 0);
        }
    }

    Buzzer_Off();
}

bool Buzzer_IsMuted(void)
{
    return gBuzzerMuted;
}

void Buzzer_SetMute(bool mute)
{
    gBuzzerMuted = mute;
    W_Flash_Safe_Write_Int(AD_BUZZER_MUTE, mute ? 1 : 0);
    if (mute) {
        Buzzer_Off();
    }
    LOG_Printf("\r\n--- BUZZER %s ---\r\n", mute ? "MUTED (OFF)" : "UNMUTED (ON)");
}

void Buzzer_Toggle(void)
{
    if (gBuzzerMuted) return;

    HAL_GPIO_TogglePin(
        BUZZER_PORT,
        BUZZER_PIN);
}

void Buzzer_StartPattern(
        uint16_t OnTimeMs,
        uint16_t OffTimeMs)
{
    BuzzerEnable = true;

    BuzzerOnTime  = OnTimeMs;
    BuzzerOffTime = OffTimeMs;

    BuzzerState = true;

    BuzzerTick = HAL_GetTick();

    Buzzer_On();
}


void Buzzer_StopPattern(void)
{
    BuzzerEnable = false;

    BuzzerState = false;

    Buzzer_Off();
}


void Buzzer_Task(void)
{
    if(BuzzerEnable == false)
    {
        return;
    }

    uint32_t now = HAL_GetTick();

    if(BuzzerState == true)
    {
        if((now - BuzzerTick) >= BuzzerOnTime)
        {
            BuzzerState = false;

            BuzzerTick = now;

            Buzzer_Off();
        }
    }
    else
    {
        if((now - BuzzerTick) >= BuzzerOffTime)
        {
            BuzzerState = true;

            BuzzerTick = now;

            Buzzer_On();
        }
    }
}

