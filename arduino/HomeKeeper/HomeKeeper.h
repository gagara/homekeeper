// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _HomeKeeper_H_
#define _HomeKeeper_H_
#include "Arduino.h"
//add your includes for the project HomeKeeper here

#include <aJSON.h>

//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

//add your function definitions for the project HomeKeeper here

void processSupplyCircuit();
void processHeatingCircuit();
void processFloorCircuit();
void processHotWaterCircuit();
void processCirculationCircuit();
void processBoilerHeater();
void loadSensorsCalibrationFactors();
double readSensorCalibrationFactor(int offset);
void writeSensorCalibrationFactor(int offset, double value);
char* rfRead();
void readSensors();
void readSensor(uint8_t id, uint8_t* const &values, uint8_t &idx);
void refreshSensorValues();
uint8_t getTemp(uint8_t sensor);
unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);
unsigned long getBoilerPowersavePeriod(unsigned long currActivePeriod);
bool isInForcedMode(uint8_t bit, unsigned long ts);
void switchNodeState(uint8_t id, uint8_t sensId[], uint8_t sensVal[], uint8_t sensCnt);
void restoreNodesState();
void forceNodeState(uint8_t id, uint8_t state, unsigned long ts);
void forceNodeState(uint8_t id, uint8_t bit, uint8_t state, unsigned long &nodeTs, unsigned long ts);
void unForceNodeState(uint8_t id);
void reportStatus();
void reportNodeStatus(uint8_t nodeId, uint8_t nodeBit, unsigned long ts, unsigned long tsf);
void reportSensorsStatus();
void reportConfiguration();
void syncClocks();
void processRfMsg();
void processSerialMsg();
void parseCommand(char* command);

//Do not add code below this line
#endif /* _HomeKeeper_H_ */
