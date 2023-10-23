// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _CentralUnit_H_
#define _CentralUnit_H_
#include "Arduino.h"

#include <DallasTemperature.h>

//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif


void processHighSupplyCircuit();
void processHeatingCircuit();
void processFloorCircuit();
void processHeatingValve();
void processHotWaterCircuit();
void processCirculationCircuit();
void processStandbyHeater();
void processSolarPrimary();
void processSolarSecondary();
uint16_t getMaxCurrentHeatingTemp();

bool isInForcedMode(uint16_t bit, unsigned long ts);
void switchNodeState(uint8_t id, uint8_t sensId[], int16_t sensVal[], uint8_t sensCnt);
void forceNodeState(uint8_t id, uint8_t state, unsigned long ts);
void forceNodeState(uint8_t id, uint16_t bit, uint8_t state, unsigned long &nodeTs, unsigned long ts);
void unForceNodeState(uint8_t id);

double readSensorCF(const uint8_t sensor);
void saveSensorCF(const uint8_t sensor, const double value);
void readSensorUID(const uint8_t sensor, DeviceAddress uid);
void saveSensorUID(const uint8_t sensor, const DeviceAddress uid);
int16_t readSetting(const uint8_t sensor);
void saveSetting(const uint8_t sensor, const int16_t value);
int sensorCfOffset(const uint8_t sensor);
int sensorUidOffset(const uint8_t sensor);
int settingIdOffset(const uint8_t sensor);
void restoreNodesState();
void loadWifiConfig();
void validateStringParam(char *str, int maxSize);
void str2uid(const char *str, DeviceAddress uid);
void uid2str(const DeviceAddress uid, char *str);

void searchSensors();
void readSensors();
int8_t getSensorValue(const uint8_t sensor);
int8_t getSensorBoilerPowerState();
bool validSensorValues(const int16_t values[], const uint8_t size);

void reportStatus();
void reportSettings();
void reportConfiguration();
void reportTimestamp();

void processSerialMsg();
void processBtMsg();
void processWifiMsg();
void broadcastMsg(const char* msg);
bool parseCommand(char* command);

unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);
bool validSettingValue(uint8_t id, int16_t value);

//Do not add code below this line
#endif /* _CentralUnit_H_ */
