/******************************************************************************
 * File    : adc_driver.c
 * Brief   : ADC Driver
 ******************************************************************************/

#include "adc.h"
#include "adc_driver.h"

/******************************************************************************
 * Init
 ******************************************************************************/
void ADC_Driver_Init(void)
{
    /* ADC Calibration for STM32F0 */
    HAL_ADCEx_Calibration_Start(&hadc);
}

/******************************************************************************
 * Read ADC Channel
 ******************************************************************************/
uint32_t ADC_Read_Channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    HAL_StatusTypeDef status;

    /* Clear previous channel selection */
    hadc.Instance->CHSELR = 0;

    sConfig.Channel = channel;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

    status = HAL_ADC_ConfigChannel(&hadc, &sConfig);
    if(status != HAL_OK)
    {
        LOG_Printf("ADC Config Error: status=%d\r\n", status);
        return 0;
    }

    status = HAL_ADC_Start(&hadc);
    if (status != HAL_OK)
    {
        LOG_Printf("ADC Start Error: status=%d\r\n", status);
        return 0;
    }

    status = HAL_ADC_PollForConversion(&hadc, 10);
    if (status == HAL_OK)
    {
        uint32_t value = HAL_ADC_GetValue(&hadc);
        return value;
    }
    else
    {
        LOG_Printf("ADC Poll Error: status=%d ISR=0x%X CR=0x%X CHSELR=0x%X\r\n", 
                   status, (unsigned int)hadc.Instance->ISR, (unsigned int)hadc.Instance->CR, (unsigned int)hadc.Instance->CHSELR);
    }

    return 0;
}


