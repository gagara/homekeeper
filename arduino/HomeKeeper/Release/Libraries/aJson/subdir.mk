################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/slg/Arduino/libraries/aJson/aJSON.cpp 

LINK_OBJ += \
./Libraries/aJson/aJSON.cpp.o 

CPP_DEPS += \
./Libraries/aJson/aJSON.cpp.d 


# Each subdirectory must supply rules for building sources it contributes
Libraries/aJson/aJSON.cpp.o: /home/slg/Arduino/libraries/aJson/aJSON.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"/usr/bin/avr-g++" -c -g -Os -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=164 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR     -I"/usr/share/arduino/hardware/arduino/avr/cores/arduino" -I"/usr/share/arduino/hardware/arduino/avr/variants/mega" -I"/home/slg/Arduino/libraries/aJson" -I"/home/slg/Arduino/libraries/aJson/utility" -I"/home/slg/Arduino/libraries/EEPROMEx" -I"/home/slg/Arduino/libraries/RF24" -I"/usr/share/arduino/hardware/arduino/avr/libraries/SPI" -I"/home/slg/Arduino/libraries/MemoryFree" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '


