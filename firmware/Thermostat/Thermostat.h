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
void loadSensorsCalibrationFactors();
double readSensorCalibrationFactor(int offset);
void writeSensorCalibrationFactor(double value, int addr);
unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);
void readSensors();
void readSensor(uint8_t id, uint8_t* const &values, uint8_t &idx);
uint8_t getSensorValue(uint8_t sensor);
uint8_t getAnalogSensorValue(uint8_t sensor);
void refreshSensorValues();
void loadWifiConfig();
void wifiInit();
void wifiSetup();
void wifiCheckConnection();
bool wifiGetRemoteIP();
bool validIP(uint8_t ip[4]);
bool wifiSend(const char* msg);
bool wifiWrite(const char* msg, const char* rsp, const int wait = 0, const uint8_t maxRetry = 5);
void reportStatus();
void reportSensorStatus(const uint8_t id, const uint8_t value);
void reportConfiguration();
void reportSensorConfig(const uint8_t id, const double value);
void reportNumberConfig(const char* key, const int value);
void reportStringConfig(const char* key, const char* value);
void broadcastMsg(const char* msg);
void processSerialMsg();
void parseCommand(char* command);

//Do not add code below this line
#endif /* _Thermostat_H_ */
