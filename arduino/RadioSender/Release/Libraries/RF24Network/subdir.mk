################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/slg/Arduino/libraries/RF24Network/RF24Network.cpp \
/home/slg/Arduino/libraries/RF24Network/Sync.cpp 

CPP_DEPS += \
./Libraries/RF24Network/RF24Network.cpp.d \
./Libraries/RF24Network/Sync.cpp.d 

LINK_OBJ += \
./Libraries/RF24Network/RF24Network.cpp.o \
./Libraries/RF24Network/Sync.cpp.o 


# Each subdirectory must supply rules for building sources it contributes
Libraries/RF24Network/RF24Network.cpp.o: /home/slg/Arduino/libraries/RF24Network/RF24Network.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"/usr/bin/avr-g++" -c -g -Os -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=164 -DARDUINO_AVR_NANO -DARDUINO_ARCH_AVR     -I"/usr/share/arduino/hardware/arduino/avr/cores/arduino" -I"/usr/share/arduino/hardware/arduino/avr/variants/eightanaloginputs" -I"/home/slg/Arduino/libraries/RF24" -I"/usr/share/arduino/hardware/arduino/avr/libraries/SPI" -I"/home/slg/Arduino/libraries/RF24Mesh" -I"/home/slg/Arduino/libraries/RF24Network" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '

Libraries/RF24Network/Sync.cpp.o: /home/slg/Arduino/libraries/RF24Network/Sync.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"/usr/bin/avr-g++" -c -g -Os -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=164 -DARDUINO_AVR_NANO -DARDUINO_ARCH_AVR     -I"/usr/share/arduino/hardware/arduino/avr/cores/arduino" -I"/usr/share/arduino/hardware/arduino/avr/variants/eightanaloginputs" -I"/home/slg/Arduino/libraries/RF24" -I"/usr/share/arduino/hardware/arduino/avr/libraries/SPI" -I"/home/slg/Arduino/libraries/RF24Mesh" -I"/home/slg/Arduino/libraries/RF24Network" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"   -Wall
	@echo 'Finished building: $<'
	@echo ' '


