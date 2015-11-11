################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
PDE_SRCS += \
/home/slg/Arduino/libraries/RF24/tests/pingpair_blocking/pingpair_blocking.pde 

PDE_DEPS += \
./Libraries/RF24/tests/pingpair_blocking/pingpair_blocking.pde.d 


# Each subdirectory must supply rules for building sources it contributes
Libraries/RF24/tests/pingpair_blocking/pingpair_blocking.o: /home/slg/Arduino/libraries/RF24/tests/pingpair_blocking/pingpair_blocking.pde
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"/usr/bin/avr-g++" -c -g -Os -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=164 -DARDUINO_AVR_UNO -DARDUINO_ARCH_AVR     -I"/usr/share/arduino/hardware/arduino/avr/cores/arduino" -I"/usr/share/arduino/hardware/arduino/avr/variants/standard" -I"/home/slg/Arduino/libraries/RF24" -I"/usr/share/arduino/hardware/arduino/avr/libraries/SPI" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '


