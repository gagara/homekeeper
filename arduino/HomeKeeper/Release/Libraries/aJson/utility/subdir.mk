################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/slg/Arduino/libraries/aJson/utility/stringbuffer.c 

C_DEPS += \
./Libraries/aJson/utility/stringbuffer.c.d 

LINK_OBJ += \
./Libraries/aJson/utility/stringbuffer.c.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/aJson/utility/stringbuffer.c.o: /home/slg/Arduino/libraries/aJson/utility/stringbuffer.c
	@echo 'Building file: $<'
	@echo 'Starting C compile'
	"/usr/bin/avr-gcc" -c -g -Os -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=164 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR     -I"/usr/share/arduino/hardware/arduino/avr/cores/arduino" -I"/usr/share/arduino/hardware/arduino/avr/variants/standard" -I"/home/slg/Arduino/libraries/aJson" -I"/home/slg/Arduino/libraries/aJson/utility" -I"/home/slg/Arduino/libraries/EEPROMEx" -I"/home/slg/Arduino/libraries/RF24" -I"/usr/share/arduino/hardware/arduino/avr/libraries/SPI" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '


