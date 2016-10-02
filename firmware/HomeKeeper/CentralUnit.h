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
void processHotWaterCircuit();
void processCirculationCircuit();
void processBoilerHeater();
void processStandbyHeater();
void loadSensorsCalibrationFactors();
double readSensorCalibrationFactor(int offset);
void writeSensorCalibrationFactor(int offset, double value);
void rfRead(char* msg);
void readSensors();
void readSensor(const uint8_t id, int8_t* const &values, uint8_t &idx);
void refreshSensorValues();
int8_t getSensorValue(const uint8_t sensor);
unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);
bool isInForcedMode(uint8_t bit, unsigned long ts);
void switchNodeState(uint8_t id, uint8_t sensId[], int8_t sensVal[], uint8_t sensCnt);
void restoreNodesState();
void forceNodeState(uint8_t id, uint8_t state, unsigned long ts);
void forceNodeState(uint8_t id, uint8_t bit, uint8_t state, unsigned long &nodeTs, unsigned long ts);
void unForceNodeState(uint8_t id);
void loadWifiConfig();
bool wifiStationMode();
bool wifiAPMode();
void wifiInit();
void wifiSetup();
void wifiCheckConnection();
bool wifiGetRemoteIP();
bool validIP(uint8_t ip[4]);
void wifiStartServer();
bool wifiRsp(const char* body);
bool wifiRsp200();
bool wifiRsp400();
bool wifiSend(const char* msg);
bool wifiWrite(const char* msg, const char* rsp, const int wait = 0, const uint8_t maxRetry = 5);
void wifiRead(char* req);
void reportStatus();
void reportNodeStatus(uint8_t nodeId, uint8_t nodeBit, unsigned long ts, unsigned long tsf);
void reportSensorStatus(const uint8_t id, const int8_t value,const unsigned long ts = 0);
void reportConfiguration();
void reportSensorConfig(const uint8_t id, const double value);
void reportStringConfig(const char* key, const char* value);
void reportNumberConfig(const char* key, const int value);
void syncClocks();
void broadcastMsg(const char* msg);
void processRfMsg();
void processSerialMsg();
void processBtMsg();
void processWifiReq();
void parseCommand(char* command);

//Do not add code below this line
#endif /* _CentralUnit_H_ */
