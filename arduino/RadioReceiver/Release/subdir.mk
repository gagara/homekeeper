################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../RadioReceiver.cpp 

CPP_DEPS += \
./RadioReceiver.cpp.d 

LINK_OBJ += \
./RadioReceiver.cpp.o 


# Each subdirectory must supply rules for building sources it contributes
RadioReceiver.cpp.o: ../RadioReceiver.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"/usr/bin/avr-g++" -c -g -Os -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=164 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR     -I"/usr/share/arduino/hardware/arduino/avr/cores/arduino" -I"/usr/share/arduino/hardware/arduino/avr/variants/standard" -I"/home/slg/Arduino/libraries/RF24" -I"/usr/share/arduino/hardware/arduino/avr/libraries/SPI" -I"/home/slg/Arduino/libraries/RF24Mesh" -I"/home/slg/Arduino/libraries/RF24Network" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '


