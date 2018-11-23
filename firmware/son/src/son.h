// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _Thermostat_H_
#define _Thermostat_H_
#include "Arduino.h"
//add your includes for the project Thermostat here

//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

//add your function definitions for the project Thermostat here
void setup();
void loop();

double readSensorCF(const uint8_t sensor);
void saveSensorCF(const uint8_t sensor, const double value);
int sensorCfOffset(const uint8_t sensor);
void loadWifiConfig();
void validateStringParam(char* str, int maxSize);

void readSensors();
int8_t getSensorValue(uint8_t sensor);

void reportStatus();
void reportConfiguration();

void processSerialMsg();
void processWifiMsg();
void broadcastMsg(const char* msg);
bool parseCommand(char* command);

unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);

//Do not add code below this line
#endif /* _Thermostat_H_ */
