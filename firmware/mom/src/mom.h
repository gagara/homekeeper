// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _mom_H_
#define _mom_H_
#include "Arduino.h"
//add your includes for the project mom here

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
void stepMotor();

void restoreNodesState();
void loadWifiConfig();
void validateStringParam(char* str, int maxSize);

void readSensors();
bool validSensorValues(const int16_t values[], const uint8_t size);

void reportStatus();
void reportConfiguration();
void reportTimestamp();

void processSerialMsg();
void processWifiMsg();
void broadcastMsg(const char* msg);
bool parseCommand(char* command);

unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);

//Do not add code below this line
#endif /* _mom_H_ */
