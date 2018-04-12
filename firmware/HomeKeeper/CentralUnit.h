// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _CentralUnit_H_
#define _CentralUnit_H_
#include "Arduino.h"
//add your includes for the project CentralUnit here

//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

//add your function definitions for the project CentralUnit here

void processSupplyCircuit();
void processHeatingCircuit();
void processFloorCircuit();
void processHeatingValve();
void processHotWaterCircuit();
void processCirculationCircuit();
void processStandbyHeater();
void processSolarPrimary();
void processSolarSecondary();
bool isInForcedMode(uint16_t bit, unsigned long ts);
void switchNodeState(uint8_t id, uint8_t sensId[], int16_t sensVal[], uint8_t sensCnt);
void forceNodeState(uint8_t id, uint8_t state, unsigned long ts);
void forceNodeState(uint8_t id, uint16_t bit, uint8_t state, unsigned long &nodeTs, unsigned long ts);
void unForceNodeState(uint8_t id);
bool room1TempReachedMinThreshold();
bool room1TempFailedMinThreshold();
bool room1TempSatisfyMaxThreshold();

void loadSensorsCalibrationFactors();
double readSensorCalibrationFactor(int offset);
void writeSensorCalibrationFactor(int offset, double value);
void restoreNodesState();
void loadWifiConfig();
void validateStringParam(char* str, int maxSize);

void readSensors();
int16_t getSensorValue(const uint8_t sensor);
int8_t getSensorBoilerPowerState();
bool validSensorValues(const int16_t values[], const uint8_t size);

void reportStatus();
void reportNodeStatus(uint8_t nodeId, uint16_t nodeBit, unsigned long ts, unsigned long tsf);
void reportSensorStatus(const uint8_t id, const int16_t value,const unsigned long ts = 0);
void reportConfiguration();
void reportSensorCalibrationFactor(const uint8_t id, const double value);
void reportSensorConfigValue(const uint8_t id, const int8_t value);
void reportStringConfig(const char* key, const char* value);
void reportNumberConfig(const char* key, const int value);
void syncClocks();

void processSerialMsg();
void processBtMsg();
void processWifiReq();
void broadcastMsg(const char* msg);
bool parseCommand(char* command);

unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);
void logFreeMem();

////////////////////
bool wifiStationMode();
bool wifiAPMode();
void wifiInit();
void wifiSetup();
void wifiCheckConnection();
bool wifiGetRemoteIP();
bool validIP(uint8_t ip[4]);
bool wifiRsp(const char* body);
bool wifiRsp200();
bool wifiRsp400();
bool wifiSend(const char* msg);
bool wifiWrite(const char* msg, const char* rsp, const int wait = 0, const uint8_t maxRetry = 5);
void wifiRead(char* req);
/////////////////////////

//Do not add code below this line
#endif /* _CentralUnit_H_ */
