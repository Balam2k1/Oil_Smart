################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/Hardware/EXTI_Counter/Exti_Counter.c 

OBJS += \
./User/Hardware/EXTI_Counter/Exti_Counter.o 

C_DEPS += \
./User/Hardware/EXTI_Counter/Exti_Counter.d 


# Each subdirectory must supply rules for building sources it contributes
User/Hardware/EXTI_Counter/%.o User/Hardware/EXTI_Counter/%.su User/Hardware/EXTI_Counter/%.cyclo: ../User/Hardware/EXTI_Counter/%.c User/Hardware/EXTI_Counter/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F030xC -c -I../Core/Inc -I"D:/TASK/TACHO_V1_3/User/Hardware/EXTI_Counter" -I"D:/TASK/TACHO_V1_3/User/Hardware/DS1307" -I"D:/TASK/TACHO_V1_3/User/Hardware/ESP32" -I"D:/TASK/TACHO_V1_3/User/Hardware/GPS" -I"D:/TASK/TACHO_V1_3/User/Hardware/RS485" -I"D:/TASK/TACHO_V1_3/User/Hardware/ADC" -I"D:/TASK/TACHO_V1_3/User/Hardware/Buzzer" -I"D:/TASK/TACHO_V1_3/User/Hardware/Flash" -I"D:/TASK/TACHO_V1_3/User/Hardware/HC595" -I"D:/TASK/TACHO_V1_3/User/Hardware/MAX7219" -I"D:/TASK/TACHO_V1_3/User/Hardware/Timer_Counter" -I"D:/TASK/TACHO_V1_3/User/Controller" -I"D:/TASK/TACHO_V1_3/User/Common" -I"D:/TASK/TACHO_V1_3/User/Application/SensorManager" -I"D:/TASK/TACHO_V1_3/User/Application/ModbusMaster" -I"D:/TASK/TACHO_V1_3/User/Application/DisplayManager" -I"D:/TASK/TACHO_V1_3/User/Application/DataModel" -I"D:/TASK/TACHO_V1_3/User/Application/AlarmManager" -I../Drivers/STM32F0xx_HAL_Driver/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-User-2f-Hardware-2f-EXTI_Counter

clean-User-2f-Hardware-2f-EXTI_Counter:
	-$(RM) ./User/Hardware/EXTI_Counter/Exti_Counter.cyclo ./User/Hardware/EXTI_Counter/Exti_Counter.d ./User/Hardware/EXTI_Counter/Exti_Counter.o ./User/Hardware/EXTI_Counter/Exti_Counter.su

.PHONY: clean-User-2f-Hardware-2f-EXTI_Counter

