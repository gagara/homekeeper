// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _Ventilation_H_
#define _Ventilation_H_
#include "Arduino.h"
//add your includes for the project Ventilation here

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

void processVentilationValve();
bool isInForcedMode(uint16_t bit, unsigned long ts);
void switchNodeState(uint8_t id, uint8_t sensId[], int16_t sensVal[], uint8_t sensCnt);
void forceNodeState(uint8_t id, uint8_t state, unsigned long ts);
void forceNodeState(uint8_t id, uint16_t bit, uint8_t state, unsigned long &nodeTs, unsigned long ts);
void unForceNodeState(uint8_t id);
void switchVentilationValve();

double readSensorCF(const uint8_t sensor);
void saveSensorCF(const uint8_t sensor, const double value);
int sensorCfOffset(const uint8_t sensor);
void loadWifiConfig();
void validateStringParam(char* str, int maxSize);

void readSensors();
int8_t getSensorValue(uint8_t sensor);
bool validSensorValues(const int16_t values[], const uint8_t size);

void reportStatus();
//void reportNodeStatus(uint8_t id, uint16_t bit, unsigned long ts, unsigned long tsf);
//void reportSensorStatus(const uint8_t id, const uint8_t value);
void reportConfiguration();
void reportTimestamp();
//void reportSensorCfConfig(const uint8_t id);
//void reportNumberConfig(const char* key, const int value);
//void reportStringConfig(const char* key, const char* value);

void processSerialMsg();
void processWifiMsg();
void broadcastMsg(const char* msg);
bool parseCommand(char* command);

unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);

//Do not add code below this line
#endif /* _Ventilation_H_ */
