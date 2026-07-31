################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/Controller/scheduler.c 

OBJS += \
./User/Controller/scheduler.o 

C_DEPS += \
./User/Controller/scheduler.d 


# Each subdirectory must supply rules for building sources it contributes
User/Controller/%.o User/Controller/%.su User/Controller/%.cyclo: ../User/Controller/%.c User/Controller/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F030xC -c -I../Core/Inc -I"D:/TASK/TACHO_V1_3/User/Hardware/EXTI_Counter" -I"D:/TASK/TACHO_V1_3/User/Hardware/DS1307" -I"D:/TASK/TACHO_V1_3/User/Hardware/ESP32" -I"D:/TASK/TACHO_V1_3/User/Hardware/GPS" -I"D:/TASK/TACHO_V1_3/User/Hardware/RS485" -I"D:/TASK/TACHO_V1_3/User/Hardware/ADC" -I"D:/TASK/TACHO_V1_3/User/Hardware/Buzzer" -I"D:/TASK/TACHO_V1_3/User/Hardware/Flash" -I"D:/TASK/TACHO_V1_3/User/Hardware/HC595" -I"D:/TASK/TACHO_V1_3/User/Hardware/MAX7219" -I"D:/TASK/TACHO_V1_3/User/Hardware/Timer_Counter" -I"D:/TASK/TACHO_V1_3/User/Controller" -I"D:/TASK/TACHO_V1_3/User/Common" -I"D:/TASK/TACHO_V1_3/User/Application/SensorManager" -I"D:/TASK/TACHO_V1_3/User/Application/ModbusMaster" -I"D:/TASK/TACHO_V1_3/User/Application/DisplayManager" -I"D:/TASK/TACHO_V1_3/User/Application/DataModel" -I"D:/TASK/TACHO_V1_3/User/Application/AlarmManager" -I../Drivers/STM32F0xx_HAL_Driver/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-User-2f-Controller

clean-User-2f-Controller:
	-$(RM) ./User/Controller/scheduler.cyclo ./User/Controller/scheduler.d ./User/Controller/scheduler.o ./User/Controller/scheduler.su

.PHONY: clean-User-2f-Controller

