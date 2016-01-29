################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include arduino/core/subdir.mk
-include Libraries/SPI/subdir.mk
-include Libraries/RF24/tests/pingpair_test/subdir.mk
-include Libraries/RF24/tests/pingpair_blocking/subdir.mk
-include Libraries/RF24/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
AR += \
arduino.ar \

ELF += \
rf_tx.elf \

EEP += \
rf_tx.eep \

FLASH_IMAGE += \
rf_tx.hex \


# All Target
all: rf_tx

# Tool invocations
rf_tx: $(FLASH_IMAGE)
	@echo 'Building target: $@'
	@echo 'Printing size:'
	"/usr/bin/avr-size" -A "/home/slg/projects/home_keeper/arduino/rf_tx/Release/rf_tx.elf"
	@echo 'Finished building target: $@'
	@echo ' '

arduino.ar: $(AR_OBJ)
	@echo 'Starting archiver'
	"/usr/bin/avr-ar" rcs  "/home/slg/projects/home_keeper/arduino/rf_tx/Release/arduino.ar"   $(AR_OBJ)
	@echo 'Finished building: $@'
	@echo ' '

rf_tx.elf: $(LINK_OBJ) $(AR)
	@echo 'Starting combiner'
	"/usr/bin/avr-gcc" -Os -Wl,--gc-sections -mmcu=atmega328p  -o "/home/slg/projects/home_keeper/arduino/rf_tx/Release/rf_tx.elf"   $(LINK_OBJ) $(AR)  "/home/slg/projects/home_keeper/arduino/rf_tx/Release/arduino.ar" "-L/home/slg/projects/home_keeper/arduino/rf_tx/Release" -lm
	@echo 'Finished building: $@'
	@echo ' '

rf_tx.eep: $(ELF)
	@echo 'Create eeprom image'
	"/usr/bin/avr-objcopy" -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0  "/home/slg/projects/home_keeper/arduino/rf_tx/Release/rf_tx.elf" "/home/slg/projects/home_keeper/arduino/rf_tx/Release/rf_tx.eep"
	@echo 'Finished building: $@'
	@echo ' '

rf_tx.hex: $(EEP)
	@echo 'Create Flash image (ihex format)'
	"/usr/bin/avr-objcopy" -O ihex -R .eeprom  "/home/slg/projects/home_keeper/arduino/rf_tx/Release/rf_tx.elf" "/home/slg/projects/home_keeper/arduino/rf_tx/Release/rf_tx.hex"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(AR)$(C_DEPS)$(FLASH_IMAGE)$(EEP)$(CXX_DEPS)$(S_UPPER_DEPS)$(ELF)$(C++_DEPS)$(ASM_DEPS)$(CC_DEPS)$(INO_DEPS)$(AR_OBJ)$(CPP_DEPS)$(PDE_DEPS)$(LINK_OBJ)$(C_UPPER_DEPS)$(SIZEDUMMY) rf_tx
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
