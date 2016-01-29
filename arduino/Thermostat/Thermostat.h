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
void loadSensorCalibrationFactor();
void writeSensorCalibrationFactor(double value);
void loadRfPipeAddr();
void writeRfPipeAddr(uint8_t value);
unsigned long getTimestamp();
unsigned long diffTimestamps(unsigned long hi, unsigned long lo);
void readSensors();
void readSensor(uint8_t id, uint8_t* const &values, uint8_t &idx);
uint8_t getTemp(uint8_t sensor);
void refreshSensorValues();
void rfWrite(char* msg);
void reportStatus();
void reportConfiguration();
void processSerialMsg();

//Do not add code below this line
#endif /* _Thermostat_H_ */
