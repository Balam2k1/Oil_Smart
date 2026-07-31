################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include User/Hardware/Timer_Counter/subdir.mk
-include User/Hardware/RS485/subdir.mk
-include User/Hardware/MAX7219/subdir.mk
-include User/Hardware/HC595/subdir.mk
-include User/Hardware/GPS/subdir.mk
-include User/Hardware/Flash/subdir.mk
-include User/Hardware/EXTI_Counter/subdir.mk
-include User/Hardware/ESP32/subdir.mk
-include User/Hardware/DS1307/subdir.mk
-include User/Hardware/Buzzer/subdir.mk
-include User/Hardware/ADC/subdir.mk
-include User/Controller/subdir.mk
-include User/Common/subdir.mk
-include User/Application/SensorManager/subdir.mk
-include User/Application/ModbusMaster/subdir.mk
-include User/Application/DisplayManager/subdir.mk
-include User/Application/DataModel/subdir.mk
-include User/Application/AlarmManager/subdir.mk
-include Drivers/STM32F0xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := TACHO_V1_3
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
TACHO_V1_3.elf \

MAP_FILES += \
TACHO_V1_3.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
TACHO_V1_3.list \

OBJCOPY_HEX += \
TACHO_V1_3.hex \

OBJCOPY_BIN += \
TACHO_V1_3.bin \


# All Target
all: main-build

# Main-build Target
main-build: TACHO_V1_3.elf secondary-outputs

# Tool invocations
TACHO_V1_3.elf TACHO_V1_3.map: $(OBJS) $(USER_OBJS) D:\TASK\TACHO_V1_3\STM32F030RCTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "TACHO_V1_3.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m0 -T"D:\TASK\TACHO_V1_3\STM32F030RCTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="TACHO_V1_3.map" -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

TACHO_V1_3.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "TACHO_V1_3.list"
	@echo 'Finished building: $@'
	@echo ' '

TACHO_V1_3.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "TACHO_V1_3.hex"
	@echo 'Finished building: $@'
	@echo ' '

TACHO_V1_3.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "TACHO_V1_3.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) TACHO_V1_3.bin TACHO_V1_3.elf TACHO_V1_3.hex TACHO_V1_3.list TACHO_V1_3.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
