################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include emlib/subdir.mk
-include emdrv/BRD4160A/subdir.mk
-include device/EFR32_M_1_1_P/gcc/subdir.mk
-include bgapi/subdir.mk
-include .metadata/.plugins/org.eclipse.cdt.make.core/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: base_station_code.axf

# Tool invocations
base_station_code.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GNU ARM C Linker'
	arm-none-eabi-gcc -g -gdwarf-2 -mcpu=cortex-m4 -mthumb -T "D:\SiliconLabs\SimplicityStudio\v4_workspace\base_station_code\linker\GCC\efr32mg1p132f256gm48.ld" -Xlinker -no-enum-size-warning -Xlinker -no-wchar-size-warning -Xlinker --gc-sections -Xlinker -Map="base_station_code.map" -mfpu=fpv4-sp-d16 -mfloat-abi=hard -o base_station_code.axf "./.metadata/.plugins/org.eclipse.cdt.make.core/specs.o" "./application_properties.o" "./gatt_db.o" "./main.o" "./bgapi/em_leuart.o" "./device/EFR32_M_1_1_P/gcc/startup_efr32mg1p.o" "./emdrv/BRD4160A/dmadrv.o" "./emdrv/BRD4160A/gpiointerrupt.o" "./emlib/em_cmu.o" "./emlib/em_cryotimer.o" "./emlib/em_gpio.o" "./emlib/em_i2c.o" "./emlib/em_msc.o" "./emlib/em_rtcc.o" "./emlib/em_system.o" "./emlib/em_usart.o" "./src/InitDevice.o" "D:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v1.1//protocol/bluetooth_2.4/lib/EFR32MG1P/GCC/bgapi.a" "D:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v1.1//protocol/bluetooth_2.4/lib/EFR32MG1P/EABI/stack.a" "D:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v1.1//protocol/bluetooth_2.4/lib/EFR32MG1P/EABI/binstack.o" "D:/SiliconLabs/SimplicityStudio/v4/developer/sdks/gecko_sdk_suite/v1.1//protocol/bluetooth_2.4/lib/EFR32MG1P/EABI/binbootloader.o" -lm -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

	@echo 'Building hex file: base_station_code.hex'
	arm-none-eabi-objcopy -O ihex "base_station_code.axf" "base_station_code.hex"
	@echo ' '

	@echo 'Building bin file: base_station_code.bin'
	arm-none-eabi-objcopy -O binary "base_station_code.axf" "base_station_code.bin"
	@echo ' '

	@echo 'Building s37 file: base_station_code.s37'
	arm-none-eabi-objcopy -O srec "base_station_code.axf" "base_station_code.s37"
	@echo ' '

	@echo 'Running size tool'
	arm-none-eabi-size "base_station_code.axf"
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) base_station_code.axf
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
