################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/Common/kalman_filter.c \
../User/Common/log.c 

OBJS += \
./User/Common/kalman_filter.o \
./User/Common/log.o 

C_DEPS += \
./User/Common/kalman_filter.d \
./User/Common/log.d 


# Each subdirectory must supply rules for building sources it contributes
User/Common/%.o User/Common/%.su User/Common/%.cyclo: ../User/Common/%.c User/Common/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F030xC -c -I../Core/Inc -I"D:/TASK/TACHO_V1_3/User/Hardware/EXTI_Counter" -I"D:/TASK/TACHO_V1_3/User/Hardware/DS1307" -I"D:/TASK/TACHO_V1_3/User/Hardware/ESP32" -I"D:/TASK/TACHO_V1_3/User/Hardware/GPS" -I"D:/TASK/TACHO_V1_3/User/Hardware/RS485" -I"D:/TASK/TACHO_V1_3/User/Hardware/ADC" -I"D:/TASK/TACHO_V1_3/User/Hardware/Buzzer" -I"D:/TASK/TACHO_V1_3/User/Hardware/Flash" -I"D:/TASK/TACHO_V1_3/User/Hardware/HC595" -I"D:/TASK/TACHO_V1_3/User/Hardware/MAX7219" -I"D:/TASK/TACHO_V1_3/User/Hardware/Timer_Counter" -I"D:/TASK/TACHO_V1_3/User/Controller" -I"D:/TASK/TACHO_V1_3/User/Common" -I"D:/TASK/TACHO_V1_3/User/Application/SensorManager" -I"D:/TASK/TACHO_V1_3/User/Application/ModbusMaster" -I"D:/TASK/TACHO_V1_3/User/Application/DisplayManager" -I"D:/TASK/TACHO_V1_3/User/Application/DataModel" -I"D:/TASK/TACHO_V1_3/User/Application/AlarmManager" -I../Drivers/STM32F0xx_HAL_Driver/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-User-2f-Common

clean-User-2f-Common:
	-$(RM) ./User/Common/kalman_filter.cyclo ./User/Common/kalman_filter.d ./User/Common/kalman_filter.o ./User/Common/kalman_filter.su ./User/Common/log.cyclo ./User/Common/log.d ./User/Common/log.o ./User/Common/log.su

.PHONY: clean-User-2f-Common

