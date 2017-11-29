#include "CentralUnit.h"

#include <Arduino.h>
#include <EEPROMex.h>
#include <MemoryFree.h>
#include <RF24.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* ========= Configuration ========= */

#define __DEBUG__

// Sensors IDs
static const uint8_t SENSOR_SUPPLY = 54;
static const uint8_t SENSOR_REVERSE = 55;
static const uint8_t SENSOR_TANK = 56;
static const uint8_t SENSOR_BOILER = 57;
static const uint8_t SENSOR_MIX = 58;
static const uint8_t SENSOR_SB_HEATER = 59;
static const uint8_t SENSOR_BOILER_POWER = 60;
static const uint8_t SENSOR_SOLAR_PRIMARY = 61; //A7
static const uint8_t SENSOR_SOLAR_SECONDARY = 62;
static const uint8_t SENSOR_TEMP_ROOM_1 = (54 + 16) + (4 * 1) + 0;
static const uint8_t SENSOR_HUM_ROOM_1 = (54 + 16) + (4 * 1) + 1;
// Sensor thresholds
static const uint8_t SENSOR_TH_ROOM1_SB_HEATER = 200 + 1;
static const uint8_t SENSOR_TH_ROOM1_PRIMARY_HEATER = 200 + 2;

// Sensors Addresses
static const DeviceAddress SENSOR_SUPPLY_ADDR = { 0x28, 0xFF, 0x70, 0x84, 0x23, 0x16, 0x04, 0xA5 };
static const DeviceAddress SENSOR_REVERSE_ADDR = { 0x28, 0xFF, 0xB7, 0x7C, 0x23, 0x16, 0x04, 0xBB };
static const DeviceAddress SENSOR_TANK_ADDR = { 0x28, 0xFF, 0x62, 0x82, 0x23, 0x16, 0x04, 0x53 };
static const DeviceAddress SENSOR_BOILER_ADDR = { 0x28, 0xFF, 0xD1, 0x84, 0x23, 0x16, 0x04, 0x5B };
static const DeviceAddress SENSOR_MIX_ADDR = { 0x28, 0xFF, 0x45, 0x90, 0x23, 0x16, 0x04, 0xC5 };
static const DeviceAddress SENSOR_SB_HEATER_ADDR = { 0x28, 0xFF, 0x28, 0x5B, 0x23, 0x16, 0x04, 0x95 };
//static const DeviceAddress SENSOR_SOLAR_PRIMAY_ADDR = { 0x28, 0xFF, 0xC3, 0x66, 0x23, 0x16, 0x04, 0xEA }; // analog sensor
static const DeviceAddress SENSOR_SOLAR_SECONDARY_ADDR = { 0x28, 0xFF, 0xCC, 0x84, 0x23, 0x16, 0x04, 0xCB };

// Sensors Bus pin
static const uint8_t SENSORS_BUS = 49;

// Nodes pins
static const uint8_t NODE_SUPPLY = 22; //7
static const uint8_t NODE_HEATING = 24; //8
static const uint8_t NODE_FLOOR = 26; //9
static const uint8_t NODE_HOTWATER = 28; //10
static const uint8_t NODE_CIRCULATION = 30; //11
static const uint8_t NODE_SB_HEATER = 34;
static const uint8_t NODE_SOLAR_PRIMARY = 36;
static const uint8_t NODE_SOLAR_SECONDARY = 38;
static const uint8_t NODE_HEATING_VALVE = 40;
static const uint8_t NODE_RESERVED = 42;

// WiFi pins
static const uint8_t WIFI_RST_PIN = 12;

static const uint8_t HEARTBEAT_LED = 13;

// Nodes State
static const uint16_t NODE_SUPPLY_BIT = 1;
static const uint16_t NODE_HEATING_BIT = 2;
static const uint16_t NODE_FLOOR_BIT = 4;
static const uint16_t NODE_HOTWATER_BIT = 8;
static const uint16_t NODE_CIRCULATION_BIT = 16;
static const uint16_t NODE_SB_HEATER_BIT = 64;
static const uint16_t NODE_SOLAR_PRIMARY_BIT = 128;
static const uint16_t NODE_SOLAR_SECONDARY_BIT = 256;
static const uint16_t NODE_HEATING_VALVE_BIT = 512;
static const uint16_t NODE_RESERVED_BIT = 1024;

// etc
static const unsigned long MAX_TIMESTAMP = -1;
static const int16_t UNKNOWN_SENSOR_VALUE = -127;
static const uint8_t SENSORS_PRECISION = 9;
static const unsigned long NODE_SWITCH_SAFE_TIME_SEC = 60;

// EEPROM
static const int NODE_STATE_EEPROM_ADDR = 0; // 2 bytes
static const int NODE_FORCED_MODE_EEPROM_ADDR = NODE_STATE_EEPROM_ADDR + 2; // 2 bytes
static const int SENSORS_FACTORS_EEPROM_ADDR = NODE_FORCED_MODE_EEPROM_ADDR + 2; // 8 x 4 bytes
static const int WIFI_REMOTE_AP_EEPROM_ADDR = SENSORS_FACTORS_EEPROM_ADDR + (8 * 4); // 32 bytes
static const int WIFI_REMOTE_PW_EEPROM_ADDR = WIFI_REMOTE_AP_EEPROM_ADDR + 32; // 32 bytes
static const int SERVER_IP_EEPROM_ADDR = WIFI_REMOTE_PW_EEPROM_ADDR + 32; // 4 bytes
static const int SERVER_PORT_EEPROM_ADDR = SERVER_IP_EEPROM_ADDR + 4; // 2 bytes
static const int WIFI_LOCAL_AP_EEPROM_ADDR = SERVER_PORT_EEPROM_ADDR + 2; // 32 bytes
static const int WIFI_LOCAL_PW_EEPROM_ADDR = WIFI_LOCAL_AP_EEPROM_ADDR + 32; // 32 bytes
static const int STANDBY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR = WIFI_LOCAL_PW_EEPROM_ADDR + 32; // 1 byte
static const int PRIMARY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR = STANDBY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR + 1; // 1 byte

// Primary Heater
static const uint8_t PRIMARY_HEATER_SUPPLY_REVERSE_HIST = 5;
static const uint8_t PRIMARY_HEATER_SHORT_CIRCUIT_THRESHOLD_TEMP = 50;
static const unsigned long PRIMARY_HEATER_SHORT_CIRCUIT_PERIOD_SEC = 1200; // 20 minutes

// heating
static const uint8_t HEATING_ON_TEMP_THRESHOLD = 40;
static const uint8_t HEATING_OFF_TEMP_THRESHOLD = 36;
static const uint8_t FLOOR_ON_TEMP_THRESHOLD = 35;
static const uint8_t FLOOR_OFF_TEMP_THRESHOLD = 26;
static const uint8_t PRIMARY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD = 20;
static const unsigned long HEATING_ROOM_1_MAX_VALIDITY_PERIOD = 1800; // 30m

// Boiler heating
static const uint8_t TANK_BOILER_HEATING_ON_HIST = 3;
static const uint8_t TANK_BOILER_HEATING_OFF_HIST = 1;
static const uint8_t BOILER_SOLAR_MAX_TEMP_THRESHOLD = 46;
static const uint8_t BOILER_SOLAR_MAX_TEMP_HIST = 2;

// Circulation
static const uint8_t CIRCULATION_MIN_TEMP_THRESHOLD = 35;
static const uint8_t CIRCULATION_COOLING_TEMP_THRESHOLD = 61;
static const unsigned long CIRCULATION_ACTIVE_PERIOD_SEC = 180; // 3m
static const unsigned long CIRCULATION_PASSIVE_PERIOD_SEC = 3420; // 57m

// Standby Heater
static const uint8_t STANDBY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD = 10;

// Tank
static const uint8_t TANK_MIN_TEMP_THRESHOLD = 3;
static const uint8_t TANK_MIN_TEMP_HIST = 2;

// Solar
static const unsigned long SOLAR_PRIMARY_COLDSTART_PERIOD_SEC = 600; // 10m
static const uint8_t SOLAR_PRIMARY_CRITICAL_TEMP_THRESHOLD = 110; // stagnation
static const uint8_t SOLAR_PRIMARY_CRITICAL_TEMP_HIST = 10;
static const uint8_t SOLAR_PRIMARY_BOILER_ON_HIST = 9;
static const uint8_t SOLAR_PRIMARY_BOILER_OFF_HIST = 0;
static const uint8_t SOLAR_SECONDARY_BOILER_ON_HIST = 0;

// sensor BoilerPower
static const uint8_t SENSOR_BOILER_POWER_THERSHOLD = 100;

// reporting
static const unsigned long STATUS_REPORTING_PERIOD_SEC = 5; // 5s
static const unsigned long SENSORS_READ_INTERVAL_SEC = 5; // 5s

// WiFi
static const unsigned long WIFI_MAX_FAILURE_PERIOD_SEC = 180; // 3m

// JSON
static const char MSG_TYPE_KEY[] = "m";
static const char ID_KEY[] = "id";
static const char STATE_KEY[] = "ns";
static const char FORCE_FLAG_KEY[] = "ff";
static const char TIMESTAMP_KEY[] = "ts";
static const char FORCE_TIMESTAMP_KEY[] = "ft";
static const char CALIBRATION_FACTOR_KEY[] = "cf";
static const char NODES_KEY[] = "n";
static const char SENSORS_KEY[] = "s";
static const char VALUE_KEY[] = "v";

static const char MSG_CURRENT_STATUS_REPORT[] = "csr";
static const char MSG_NODE_STATE_CHANGED[] = "nsc";
static const char MSG_CLOCK_SYNC[] = "cls";
static const char MSG_CONFIGURATION[] = "cfg";

static const char WIFI_REMOTE_AP_KEY[] = "rap";
static const char WIFI_REMOTE_PASSWORD_KEY[] = "rpw";
static const char WIFI_LOCAL_AP_KEY[] = "lap";
static const char WIFI_LOCAL_PASSWORD_KEY[] = "lpw";
static const char SERVER_IP_KEY[] = "sip";
static const char SERVER_PORT_KEY[] = "sp";
static const char LOCAL_IP_KEY[] = "lip";

static const uint8_t JSON_MAX_SIZE = 128;
static const uint8_t JSON_MAX_BUFFER_SIZE = 255;

static const uint8_t WIFI_MAX_AT_CMD_SIZE = 128;
static const uint16_t WIFI_MAX_BUFFER_SIZE = 512;

/* ===== End of Configuration ====== */

/* =========== Variables =========== */

//// Sensors
// Values
int16_t tempSupply = UNKNOWN_SENSOR_VALUE;
int16_t tempReverse = UNKNOWN_SENSOR_VALUE;
int16_t tempTank = UNKNOWN_SENSOR_VALUE;
int16_t tempBoiler = UNKNOWN_SENSOR_VALUE;
int16_t tempMix = UNKNOWN_SENSOR_VALUE;
int16_t tempSbHeater = UNKNOWN_SENSOR_VALUE;
int16_t tempRoom1 = UNKNOWN_SENSOR_VALUE;
int16_t humRoom1 = UNKNOWN_SENSOR_VALUE;
int16_t tempSolarPrimary = UNKNOWN_SENSOR_VALUE;
int16_t tempSolarSecondary = UNKNOWN_SENSOR_VALUE;
int16_t STANDBY_HEATER_ROOM_TEMP_THRESHOLD = STANDBY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD;
int16_t PRIMARY_HEATER_ROOM_TEMP_THRESHOLD = PRIMARY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD;
int16_t sensorBoilerPowerState = 0;

//// Nodes

// Nodes actual state
uint16_t NODE_STATE_FLAGS = 0;
// Nodes forced mode
uint16_t NODE_FORCED_MODE_FLAGS = 0;
// will be stored in EEPROM
uint16_t NODE_PERMANENTLY_FORCED_MODE_FLAGS = 0;

// Nodes switch timestamps
unsigned long tsNodeSupply = 0;
unsigned long tsNodeHeating = 0;
unsigned long tsNodeFloor = 0;
unsigned long tsNodeHotwater = 0;
unsigned long tsNodeCirculation = 0;
unsigned long tsNodeSbHeater = 0;
unsigned long tsNodeSolarPrimary = 0;
unsigned long tsNodeSolarSecondary = 0;
unsigned long tsNodeHeatingValve = 0;

// Nodes forced mode timestamps
unsigned long tsForcedNodeSupply = 0;
unsigned long tsForcedNodeHeating = 0;
unsigned long tsForcedNodeFloor = 0;
unsigned long tsForcedNodeHotwater = 0;
unsigned long tsForcedNodeCirculation = 0;
unsigned long tsForcedNodeSbHeater = 0;
unsigned long tsForcedNodeSolarPrimary = 0;
unsigned long tsForcedNodeSolarSecondary = 0;
unsigned long tsForcedNodeHeatingValve = 0;

// Sensors switch timestamps
unsigned long tsSensorBoilerPower = 0;

// reporting
uint8_t nextEntryReport = 0;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorsRead = 0;

unsigned long tsLastSensorTempRoom1 = 0;
unsigned long tsLastSensorHumRoom1 = 0;

unsigned long tsLastWifiSuccessTransmission = 0;

unsigned long tsPrev = 0;
unsigned long tsPrevRaw = 0;
unsigned long tsCurr = 0;

uint8_t overflowCount = 0;

// sensor calibration factors
double SENSOR_SUPPLY_FACTOR = 1;
double SENSOR_REVERSE_FACTOR = 1;
double SENSOR_TANK_FACTOR = 1;
double SENSOR_BOILER_FACTOR = 1;
double SENSOR_MIX_FACTOR = 1;
double SENSOR_SB_HEATER_FACTOR = 1;
double SENSOR_SOLAR_PRIMARY_FACTOR = 1;
double SENSOR_SOLAR_SECONDARY_FACTOR = 1;

unsigned long boilerPowersaveCurrentPassivePeriod = 0;

// WiFi
char WIFI_REMOTE_AP[32];
char WIFI_REMOTE_PW[32];
char WIFI_LOCAL_AP[32];
char WIFI_LOCAL_PW[32];
uint8_t SERVER_IP[4] = { 0, 0, 0, 0 };
uint16_t SERVER_PORT = 80;
uint8_t CLIENT_ID = 0;
uint8_t IP[4] = { 0, 0, 0, 0 };
char OK[] = "OK";

/* ======== End of Variables ======= */

//======== Connectivity ========//
// Sensors
OneWire oneWire(SENSORS_BUS);
DallasTemperature sensors(&oneWire);

// BT
HardwareSerial *bt = &Serial1;

//WiFi
HardwareSerial *wifi = &Serial2;

//======== ============ ========//

void setup() {
    // Setup serial ports
    // usb
    Serial.begin(9600);
#ifdef __DEBUG__
    Serial.println(F("STARTING"));
#endif
    // BT
    bt->begin(9600);
    // WiFi
    loadWifiConfig();
    pinMode(WIFI_RST_PIN, OUTPUT);
    wifiInit();
    wifiSetup();
    wifiGetRemoteIP();
#ifdef __DEBUG__
    logFreeMem();
#endif
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // Init sensors
    sensors.begin();
    sensors.setResolution(SENSORS_PRECISION);
#ifdef __DEBUG__
    DeviceAddress sensAddr;
    Serial.println(F("found sensors:"));
    oneWire.reset_search();
    while (oneWire.search(sensAddr)) {
        for (uint8_t i = 0; i < 8; i++) {
            // zero pad the address if necessary
            if (sensAddr[i] < 16)
                Serial.print(F("0"));
            Serial.print(sensAddr[i], HEX);
        }
        Serial.println("");
    }
#endif
    // init boilerPower sensor
    pinMode(SENSOR_BOILER_POWER, INPUT);
    // init solar primary sensor
    pinMode(SENSOR_SOLAR_PRIMARY, INPUT);

    // sync clock
    syncClocks();

    // read forced node state flags from EEPROM
    // default node state -- OFF
    restoreNodesState();

    // init nodes
    pinMode(NODE_SUPPLY, OUTPUT);
    pinMode(NODE_HEATING, OUTPUT);
    pinMode(NODE_FLOOR, OUTPUT);
    pinMode(NODE_HOTWATER, OUTPUT);
    pinMode(NODE_CIRCULATION, OUTPUT);
    pinMode(NODE_SB_HEATER, OUTPUT);
    pinMode(NODE_SOLAR_PRIMARY, OUTPUT);
    pinMode(NODE_SOLAR_SECONDARY, OUTPUT);
    pinMode(NODE_HEATING_VALVE, OUTPUT);
    pinMode(NODE_RESERVED, OUTPUT);

    // restore nodes state
    digitalWrite(NODE_SUPPLY, (~NODE_STATE_FLAGS & NODE_SUPPLY_BIT) ? HIGH : LOW);
    digitalWrite(NODE_HEATING, (~NODE_STATE_FLAGS & NODE_HEATING_BIT) ? HIGH : LOW);
    digitalWrite(NODE_FLOOR, (~NODE_STATE_FLAGS & NODE_FLOOR_BIT) ? HIGH : LOW);
    digitalWrite(NODE_HOTWATER, (~NODE_STATE_FLAGS & NODE_HOTWATER_BIT) ? HIGH : LOW);
    digitalWrite(NODE_CIRCULATION, (~NODE_STATE_FLAGS & NODE_CIRCULATION_BIT) ? HIGH : LOW);
    digitalWrite(NODE_SB_HEATER, (~NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) ? HIGH : LOW);
    digitalWrite(NODE_SOLAR_PRIMARY, (~NODE_STATE_FLAGS & NODE_SOLAR_PRIMARY_BIT) ? HIGH : LOW);
    digitalWrite(NODE_SOLAR_SECONDARY, (~NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) ? HIGH : LOW);
    // these two have inverted levels
    digitalWrite(NODE_HEATING_VALVE, (~NODE_STATE_FLAGS & NODE_HEATING_VALVE_BIT) ? LOW : HIGH);
    digitalWrite(NODE_RESERVED, LOW); // turn off reserved node

    // read sensors calibration factors from EEPROM
    loadSensorsCalibrationFactors();

    // restore sensor thresholds
    STANDBY_HEATER_ROOM_TEMP_THRESHOLD = EEPROM.readByte(STANDBY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR);
    PRIMARY_HEATER_ROOM_TEMP_THRESHOLD = EEPROM.readByte(PRIMARY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR);
}

void loop() {
    tsCurr = getTimestamp();
    if (diffTimestamps(tsCurr, tsLastSensorsRead) >= SENSORS_READ_INTERVAL_SEC) {
        readSensors();
        tsLastSensorsRead = tsCurr;
#ifdef __DEBUG__
        logFreeMem();
#endif
        // heater <--> tank
        processSupplyCircuit();
        // tank <--> heating system
        processHeatingCircuit();
        // floors
        processFloorCircuit();
        // heating valve
        processHeatingValve();
        // hot water
        processHotWaterCircuit();
        // circulation
        processCirculationCircuit();
        // solar primary
        processSolarPrimary();
        // solar secondary
        processSolarSecondary();
        // standby heater
        processStandbyHeater();

        digitalWrite(HEARTBEAT_LED, digitalRead(HEARTBEAT_LED) ^ 1);
    }
    if (Serial.available() > 0) {
        processSerialMsg();
    }
    if (bt->available() > 0) {
        processBtMsg();
    }
    if (wifi->available() > 0) {
        processWifiReq();
    }

    if (diffTimestamps(tsCurr, tsLastStatusReport) >= STATUS_REPORTING_PERIOD_SEC) {
        reportStatus();
        tsLastStatusReport = tsCurr;
    }

    tsPrev = tsCurr;
}

void processSupplyCircuit() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_SUPPLY_BIT;
    if (isInForcedMode(NODE_SUPPLY_BIT, tsForcedNodeSupply)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_SUPPLY, NODE_SUPPLY_BIT, tsNodeSupply, tsForcedNodeSupply);
    }
    if (diffTimestamps(tsCurr, tsNodeSupply) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_SUPPLY, SENSOR_REVERSE };
        int16_t sensVals[] = { tempSupply, tempReverse };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
            // pump is ON
            if (tempSupply <= tempReverse) {
                // temp is equal
                if (tempSupply <= PRIMARY_HEATER_SHORT_CIRCUIT_THRESHOLD_TEMP) {
                    // primary heater is on short circuit
                    if (diffTimestamps(tsCurr, tsNodeSupply) < PRIMARY_HEATER_SHORT_CIRCUIT_PERIOD_SEC) {
                        // pump active period is too short. give it some time
                        // do nothing
                    } else {
                        // pump active period is long enough
                        // turn pump OFF
                        switchNodeState(NODE_SUPPLY, sensIds, sensVals, sensCnt);
                    }
                } else {
                    // primary heater is in production mode
                    // turn pump OFF
                    switchNodeState(NODE_SUPPLY, sensIds, sensVals, sensCnt);
                }
            } else {
                // delta is big enough
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempSupply >= (tempReverse + PRIMARY_HEATER_SUPPLY_REVERSE_HIST)) {
                // delta is big enough
                // turn pump ON
                switchNodeState(NODE_SUPPLY, sensIds, sensVals, sensCnt);
            } else {
                // temp is equal
                // do nothing
            }
        }
    }
}

void processHeatingCircuit() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_HEATING_BIT;
    if (isInForcedMode(NODE_HEATING_BIT, tsForcedNodeHeating)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_HEATING, NODE_HEATING_BIT, tsNodeHeating, tsForcedNodeHeating);
    }
    if (diffTimestamps(tsCurr, tsNodeHeating) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_MIX, SENSOR_SB_HEATER };
        int16_t sensVals[] = { tempTank, tempMix, tempSbHeater };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_HEATING_BIT) {
            // pump is ON
            int16_t maxTemp = max(tempTank, max(tempMix, tempSbHeater));
            if (NODE_STATE_FLAGS & NODE_HEATING_VALVE_BIT) {
                maxTemp = tempSbHeater;
            }
            if (maxTemp < HEATING_OFF_TEMP_THRESHOLD) {
                // temp in all applicable sources is too low
                if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
                    // supply is on
                    // do nothing
                } else {
                    // supply is off
                    // turn pump OFF
                    switchNodeState(NODE_HEATING, sensIds, sensVals, sensCnt);
                }
            } else {
                // temp is high enough at least in one source
                if (room1TempSatisfyMaxThreshold() || (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT)) {
                    // temp in Room1 is high enough OR standby heater is on
                    // turn pump OFF
                    switchNodeState(NODE_HEATING, sensIds, sensVals, sensCnt);
                } else {
                    // temp in Room1 is low AND standby heater is off
                    // do nothing
                }
            }
        } else {
            // pump is OFF
            if (tempTank >= HEATING_ON_TEMP_THRESHOLD || tempMix >= HEATING_ON_TEMP_THRESHOLD
                    || tempSbHeater >= HEATING_ON_TEMP_THRESHOLD) {
                // temp in (tank || mix || sb_heater) is high enough
                if (room1TempSatisfyMaxThreshold() || (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT)) {
                    // temp in Room1 is high enough OR standby heater is on
                    // do nothing
                } else {
                    // temp in Room1 is low AND standby heater is off
                    // turn pump ON
                    switchNodeState(NODE_HEATING, sensIds, sensVals, sensCnt);
                }
            } else {
                // temp in (tank && mix && sb_heater) is too low
                // do nothing
            }
        }
    }
}

void processFloorCircuit() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_FLOOR_BIT;
    if (isInForcedMode(NODE_FLOOR_BIT, tsForcedNodeFloor)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
    }
    if (diffTimestamps(tsCurr, tsNodeFloor) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_MIX, SENSOR_SB_HEATER };
        int16_t sensVals[] = { tempTank, tempMix, tempSbHeater };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_FLOOR_BIT) {
            // pump is ON
            int16_t maxTemp = max(tempTank, max(tempMix, tempSbHeater));
            if (NODE_STATE_FLAGS & NODE_HEATING_VALVE_BIT) {
                maxTemp = tempSbHeater;
            }
            if (maxTemp < FLOOR_OFF_TEMP_THRESHOLD) {
                // temp in all applicable sources is too low
                if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
                    // standby heater is on
                    if (tempSbHeater <= tempMix && tempSbHeater <= tempTank) {
                        // temp in standby heater <= (temp in Mix && Tank)
                        // turn pump OFF
                        switchNodeState(NODE_FLOOR, sensIds, sensVals, sensCnt);
                    } else {
                        // temp in standby heater > (temp in Mix || Tank)
                        // do nothing
                    }
                } else {
                    // standby heater is off
                    // turn pump OFF
                    switchNodeState(NODE_FLOOR, sensIds, sensVals, sensCnt);
                }
            } else {
                // temp is high enough at least in one source
                if (!room1TempSatisfyMaxThreshold() || (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT)) {
                    // temp in Room1 is low OR standby heater is on
                    // do nothing
                } else {
                    // temp in Room1 is high enough AND standby heater is off
                    // turn pump OFF
                    switchNodeState(NODE_FLOOR, sensIds, sensVals, sensCnt);
                }
            }
        } else {
            // pump is OFF
            if (tempTank >= FLOOR_ON_TEMP_THRESHOLD || tempMix >= FLOOR_ON_TEMP_THRESHOLD
                    || tempSbHeater >= FLOOR_ON_TEMP_THRESHOLD) {
                // temp in (tank || mix || sb_heater) is high enough
                if (!room1TempSatisfyMaxThreshold() || (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT)) {
                    // temp in Room1 is low OR standby heater is on
                    // turn pump ON
                    switchNodeState(NODE_FLOOR, sensIds, sensVals, sensCnt);
                } else {
                    // temp in Room1 is high enough AND standby heater is off
                    // do nothing
                }
            } else {
                // temp in (tank && mix && sb_heater) is too low
                // do nothing
            }
        }
    }
}

void processHeatingValve() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_HEATING_VALVE_BIT;
    if (isInForcedMode(NODE_HEATING_VALVE_BIT, tsForcedNodeHeatingValve)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_HEATING_VALVE, NODE_HEATING_VALVE_BIT, tsNodeHeatingValve, tsForcedNodeHeatingValve);
    }
    if (diffTimestamps(tsCurr, tsNodeHeatingValve) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_TANK };
        int16_t sensVals[] = { tempTank };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_HEATING_VALVE_BIT) {
            // valve is OPEN (short circuit)
            if (tempTank < TANK_MIN_TEMP_THRESHOLD) {
                // temp in tank is critically low
                // CLOSE valve
                switchNodeState(NODE_HEATING_VALVE, sensIds, sensVals, sensCnt);
            } else {
                // temp in tank is OK
                if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
                    // stand by heater is on
                    // do nothing
                } else {
                    // stand by heater is off
                    if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
                        // primary heater is ON
                        // CLOSE valve
                        switchNodeState(NODE_HEATING_VALVE, sensIds, sensVals, sensCnt);
                    } else {
                        // primary heater is OFF
                        if ((NODE_STATE_FLAGS & NODE_HEATING_BIT) || (NODE_STATE_FLAGS & NODE_FLOOR_BIT)) {
                            // (heating || floor) node is ON
                            if (tempTank > tempSbHeater) {
                                // tempTank > tempSbHeater
                                // CLOSE valve
                                switchNodeState(NODE_HEATING_VALVE, sensIds, sensVals, sensCnt);
                            } else {
                                // tempTank <= tempSbHeater
                                // do nothing
                            }
                        } else {
                            // (heating && floor) node is OFF
                            // do nothing
                        }
                    }
                }
            }
        } else {
            // valve is CLOSED (full circuit)
            if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
                // stand by heater is on
                if ((NODE_STATE_FLAGS & NODE_HEATING_BIT) || (NODE_STATE_FLAGS & NODE_FLOOR_BIT)) {
                    // (heating || floor) node is ON
                    if (tempTank >= TANK_MIN_TEMP_THRESHOLD + TANK_MIN_TEMP_HIST) {
                        // temp in tank is OK
                        // OPEN valve
                        switchNodeState(NODE_HEATING_VALVE, sensIds, sensVals, sensCnt);
                    } else {
                        // temp in tank is critically low
                        // do nothing
                    }
                } else {
                    // (heating && floor) node is OFF
                    // do nothing
                }
            } else {
                // stand by heater is off
                // do nothing
            }
        }
    }
}

void processHotWaterCircuit() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_HOTWATER_BIT;
    if (isInForcedMode(NODE_HOTWATER_BIT, tsForcedNodeHotwater)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_HOTWATER, NODE_HOTWATER_BIT, tsNodeHotwater, tsForcedNodeHotwater);
    }
    if (diffTimestamps(tsCurr, tsNodeHotwater) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_BOILER };
        int16_t sensVals[] = { tempTank, tempBoiler };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_HOTWATER_BIT) {
            // pump is ON
            if (NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) {
                // solar secondary node is ON. cooling mode
                if (tempBoiler < (BOILER_SOLAR_MAX_TEMP_THRESHOLD - BOILER_SOLAR_MAX_TEMP_HIST)) {
                    // temp in boiler is normal
                    // turn pump OFF
                    switchNodeState(NODE_HOTWATER, sensIds, sensVals, sensCnt);
                } else {
                    // temp in boiler is high
                    if (tempTank < tempBoiler) {
                        // tank has capacity
                        // do nothing
                    } else {
                        // tank has no capacity
                        // turn pump OFF
                        switchNodeState(NODE_HOTWATER, sensIds, sensVals, sensCnt);
                    }
                }
            } else {
                // solar secondary node is OFF. heating mode
                if (tempTank <= (tempBoiler + TANK_BOILER_HEATING_OFF_HIST)) {
                    // temp in tank is low
                    // turn pump OFF
                    switchNodeState(NODE_HOTWATER, sensIds, sensVals, sensCnt);
                } else {
                    // temp in tank is high enough
                    // do nothing
                }
            }
        } else {
            // pump is OFF
            if (NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) {
                // solar secondary node is ON. cooling mode
                if (tempBoiler >= BOILER_SOLAR_MAX_TEMP_THRESHOLD) {
                    // temp in boiler is high
                    if (tempTank < tempBoiler) {
                        // tank has capacity
                        // turn pump ON
                        switchNodeState(NODE_HOTWATER, sensIds, sensVals, sensCnt);
                    } else {
                        // tank has no capacity
                        // do nothing
                    }
                } else {
                    // temp in boiler is normal
                    // do nothing
                }
            } else {
                // solar secondary node is OFF. heating mode
                if (tempTank > (tempBoiler + TANK_BOILER_HEATING_ON_HIST)) {
                    // temp in tank is high enough
                    // turn pump ON
                    switchNodeState(NODE_HOTWATER, sensIds, sensVals, sensCnt);
                } else {
                    // temp in tank is too low
                    // do nothing
                }
            }
        }
    }
}

void processCirculationCircuit() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_CIRCULATION_BIT;
    if (isInForcedMode(NODE_CIRCULATION_BIT, tsForcedNodeCirculation)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_CIRCULATION, NODE_CIRCULATION_BIT, tsNodeCirculation, tsForcedNodeCirculation);
    }
    if (diffTimestamps(tsCurr, tsNodeCirculation) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_BOILER, SENSOR_TANK };
        int16_t sensVals[] = { tempBoiler, tempTank };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_CIRCULATION_BIT) {
            // pump is ON
            if (tempBoiler < CIRCULATION_COOLING_TEMP_THRESHOLD) {
                // temp in boiler is normal
                if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_ACTIVE_PERIOD_SEC) {
                    // active period is over
                    // turn pump OFF
                    switchNodeState(NODE_CIRCULATION, sensIds, sensVals, sensCnt);
                } else {
                    // active period is going on
                    // do nothing
                }
            } else {
                // temp in boiler is high
                if (NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) {
                    // solar secondary node is ON
                    if (tempTank < CIRCULATION_COOLING_TEMP_THRESHOLD) {
                        // tank has capacity
                        if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_ACTIVE_PERIOD_SEC) {
                            // active period is over
                            // turn pump OFF
                            switchNodeState(NODE_CIRCULATION, sensIds, sensVals, sensCnt);
                        } else {
                            // active period is going on
                            // do nothing
                        }
                    } else {
                        // tank has no capacity
                        // do nothing
                    }
                } else {
                    // solar secondary node is OFF
                    if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_ACTIVE_PERIOD_SEC) {
                        // active period is over
                        // turn pump OFF
                        switchNodeState(NODE_CIRCULATION, sensIds, sensVals, sensCnt);
                    } else {
                        // active period is going on
                        // do nothing
                    }
                }
            }
        } else {
            // pump is OFF
            if (tempBoiler >= CIRCULATION_MIN_TEMP_THRESHOLD) {
                // temp in boiler is high enough
                if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_PASSIVE_PERIOD_SEC) {
                    // passive period is over
                    // turn pump ON
                    switchNodeState(NODE_CIRCULATION, sensIds, sensVals, sensCnt);
                } else {
                    // passive period is going on
                    if (tempBoiler >= CIRCULATION_COOLING_TEMP_THRESHOLD) {
                        // temp in boiler is high
                        if (NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) {
                            // solar secondary node is ON
                            if (tempTank >= CIRCULATION_COOLING_TEMP_THRESHOLD) {
                                // tank has no capacity. cooling
                                // turn pump ON
                                switchNodeState(NODE_CIRCULATION, sensIds, sensVals, sensCnt);
                            } else {
                                // tank has capacity
                                // do nothing
                            }
                        } else {
                            // solar secondary node is OFF
                            // do nothing
                        }
                    } else {
                        // temp in boiler is normal
                        // do nothing
                    }
                }
            } else {
                // temp in boiler is too low
                // do nothing
            }
        }
    }
}

void processSolarPrimary() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_SOLAR_PRIMARY_BIT;
    if (isInForcedMode(NODE_SOLAR_PRIMARY_BIT, tsForcedNodeSolarPrimary)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_SOLAR_PRIMARY, NODE_SOLAR_PRIMARY_BIT, tsNodeSolarPrimary, tsForcedNodeSolarPrimary);
    }
    if (diffTimestamps(tsCurr, tsNodeSolarPrimary) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_SOLAR_PRIMARY, SENSOR_BOILER };
        int16_t sensVals[] = { tempSolarPrimary, tempBoiler };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_SOLAR_PRIMARY_BIT) {
            // solar primary is ON
            if (tempSolarPrimary >= SOLAR_PRIMARY_CRITICAL_TEMP_THRESHOLD) {
                // temp in solar primary is critically high. stagnation
                // turn solar primary OFF
                switchNodeState(NODE_SOLAR_PRIMARY, sensIds, sensVals, sensCnt);
            } else {
                // temp in solar primary is normal
                if (diffTimestamps(tsCurr, tsNodeSolarPrimary) >= SOLAR_PRIMARY_COLDSTART_PERIOD_SEC) {
                    // cold start period over
                    if (tempSolarPrimary <= (tempBoiler + SOLAR_PRIMARY_BOILER_OFF_HIST)) {
                        // temp in solar primary is too low
                        if (NODE_STATE_FLAGS & NODE_HOTWATER_BIT) {
                            // hotwater node is ON (cooling mode)
                            // do nothing
                        } else {
                            // hotwater node is OFF
                            // turn solar primary OFF
                            switchNodeState(NODE_SOLAR_PRIMARY, sensIds, sensVals, sensCnt);
                        }
                    } else {
                        // temp in solar primary is high enough
                        // do nothing
                    }
                } else {
                    // in cold start period
                    // do nothing
                }
            }
        } else {
            // solar primary is OFF
            if (tempSolarPrimary < (SOLAR_PRIMARY_CRITICAL_TEMP_THRESHOLD - SOLAR_PRIMARY_CRITICAL_TEMP_HIST)) {
                // temp in solar primary is normal
                if (tempSolarPrimary > (tempBoiler + SOLAR_PRIMARY_BOILER_ON_HIST)) {
                    // temp in solar primary is high enough
                    // turn solar primary ON
                    switchNodeState(NODE_SOLAR_PRIMARY, sensIds, sensVals, sensCnt);
                } else {
                    // temp in solar primary is too low
                    // do nothing
                }
            } else {
                // temp in solar primary is critically high. stagnation
                // do nothing
            }
        }
    }
}

void processSolarSecondary() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_SOLAR_SECONDARY_BIT;
    if (isInForcedMode(NODE_SOLAR_SECONDARY_BIT, tsForcedNodeSolarSecondary)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_SOLAR_SECONDARY, NODE_SOLAR_SECONDARY_BIT, tsNodeSolarSecondary,
                tsForcedNodeSolarSecondary);
    }
    if (diffTimestamps(tsCurr, tsNodeSolarSecondary) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_SOLAR_SECONDARY, SENSOR_BOILER };
        int16_t sensVals[] = { tempSolarSecondary, tempBoiler };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) {
            // solar secondary is ON
            if (NODE_STATE_FLAGS & NODE_SOLAR_PRIMARY_BIT) {
                // solar primary ON
                // do nothing
            } else {
                // solar primary OFF
                // turn solar secondary OFF
                switchNodeState(NODE_SOLAR_SECONDARY, sensIds, sensVals, sensCnt);
            }
        } else {
            // solar secondary is OFF
            if (NODE_STATE_FLAGS & NODE_SOLAR_PRIMARY_BIT) {
                // solar primary ON
                if (tempSolarSecondary > (tempBoiler + SOLAR_SECONDARY_BOILER_ON_HIST)) {
                    // temp in solar secondary is high enough
                    // turn solar secondary ON
                    switchNodeState(NODE_SOLAR_SECONDARY, sensIds, sensVals, sensCnt);
                } else {
                    // temp in solar secondary is too low
                    // do nothing
                }
            } else {
                // solar primary OFF
                // do nothing
            }
        }
    }
}

void processStandbyHeater() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_SB_HEATER_BIT;
    if (isInForcedMode(NODE_SB_HEATER_BIT, tsForcedNodeSbHeater)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);
    }
    if (diffTimestamps(tsCurr, tsNodeSbHeater) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_TEMP_ROOM_1 };
        int16_t sensVals[] = { tempTank, tempRoom1 };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
            // heater is ON
            if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
                // primary heater is on
                // turn standby heater OFF
                switchNodeState(NODE_SB_HEATER, sensIds, sensVals, sensCnt);
            } else {
                // primary heater is off
                if (tempTank > STANDBY_HEATER_ROOM_TEMP_THRESHOLD && room1TempReachedMinThreshold()) {
                    // temp in (tank && room1) is high enough
                    // turn heater OFF
                    switchNodeState(NODE_SB_HEATER, sensIds, sensVals, sensCnt);
                } else {
                    // temp in (tank || room1) is too low
                    // do nothing
                }
            }
        } else {
            // heater is OFF
            if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
                // primary heater is on
                // do nothing
            } else {
                // primary heater is off
                if (tempTank < TANK_MIN_TEMP_THRESHOLD || room1TempFailedMinThreshold()) {
                    // temp in (tank || room1) is too low
                    // turn heater ON
                    switchNodeState(NODE_SB_HEATER, sensIds, sensVals, sensCnt);
                } else {
                    // temp in (tank && room1) is high enough
                    // do nothing
                }
            }

        }
    }
}

/* ============ Helper methods ============ */

void logFreeMem() {
    Serial.print(getTimestamp());
    Serial.print(F(": "));
    Serial.print(F("free memory: "));
    Serial.println(freeMemory());
}

bool room1TempReachedMinThreshold() {
    return (tsLastSensorTempRoom1 != 0
            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
            && tempRoom1 > STANDBY_HEATER_ROOM_TEMP_THRESHOLD)
            || (tsLastSensorTempRoom1 != 0
                    && diffTimestamps(tsCurr, tsLastSensorTempRoom1) >= HEATING_ROOM_1_MAX_VALIDITY_PERIOD)
            || (tsLastSensorTempRoom1 == 0);
}

bool room1TempFailedMinThreshold() {
    return (tsLastSensorTempRoom1 != 0
            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
            && tempRoom1 < STANDBY_HEATER_ROOM_TEMP_THRESHOLD);
}

bool room1TempSatisfyMaxThreshold() {
    return tsLastSensorTempRoom1 != 0
            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
            && tempRoom1 >= PRIMARY_HEATER_ROOM_TEMP_THRESHOLD;
}

void validateStringParam(char* str, int maxSize) {
    char* ptr = str;
    while (*ptr && (ptr - str) < maxSize) {
        ptr++;
    }
    if (*ptr) {
        str[0] = '\0';
    }
}

void loadSensorsCalibrationFactors() {
    SENSOR_SUPPLY_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 0);
    SENSOR_REVERSE_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 1);
    SENSOR_TANK_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 2);
    SENSOR_BOILER_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 3);
    SENSOR_MIX_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 4);
    SENSOR_SB_HEATER_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 5);
    SENSOR_SOLAR_PRIMARY_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 6);
    SENSOR_SOLAR_SECONDARY_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 7);
}

double readSensorCalibrationFactor(int offset) {
    double f = EEPROM.readDouble(offset);
    if (isnan(f) || f <= 0) {
        f = 1;
    }
    return f;
}

void writeSensorCalibrationFactor(int offset, double value) {
    EEPROM.writeDouble(offset, value);
}

void restoreNodesState() {
    NODE_STATE_FLAGS = EEPROM.readInt(NODE_STATE_EEPROM_ADDR);
    NODE_FORCED_MODE_FLAGS = EEPROM.readInt(NODE_FORCED_MODE_EEPROM_ADDR);
    NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS;
    NODE_STATE_FLAGS = NODE_STATE_FLAGS & NODE_PERMANENTLY_FORCED_MODE_FLAGS;
}

void loadWifiConfig() {
    EEPROM.readBlock(WIFI_REMOTE_AP_EEPROM_ADDR, WIFI_REMOTE_AP, sizeof(WIFI_REMOTE_AP));
    validateStringParam(WIFI_REMOTE_AP, sizeof(WIFI_REMOTE_AP) - 1);
    EEPROM.readBlock(WIFI_REMOTE_PW_EEPROM_ADDR, WIFI_REMOTE_PW, sizeof(WIFI_REMOTE_PW));
    validateStringParam(WIFI_REMOTE_PW, sizeof(WIFI_REMOTE_PW) - 1);
    EEPROM.readBlock(SERVER_IP_EEPROM_ADDR, SERVER_IP, sizeof(SERVER_IP));
    SERVER_PORT = EEPROM.readInt(SERVER_PORT_EEPROM_ADDR);
    EEPROM.readBlock(WIFI_LOCAL_AP_EEPROM_ADDR, WIFI_LOCAL_AP, sizeof(WIFI_LOCAL_AP));
    validateStringParam(WIFI_LOCAL_AP, sizeof(WIFI_LOCAL_AP) - 1);
    EEPROM.readBlock(WIFI_LOCAL_PW_EEPROM_ADDR, WIFI_LOCAL_PW, sizeof(WIFI_LOCAL_PW));
    validateStringParam(WIFI_LOCAL_PW, sizeof(WIFI_LOCAL_PW) - 1);
}

void readSensors() {
    tempSupply = getSensorValue(SENSOR_SUPPLY);
    tempReverse = getSensorValue(SENSOR_REVERSE);
    tempTank = getSensorValue(SENSOR_TANK);
    tempBoiler = getSensorValue(SENSOR_BOILER);
    tempMix = getSensorValue(SENSOR_MIX);
    tempSbHeater = getSensorValue(SENSOR_SB_HEATER);
    tempSolarPrimary = getSensorValue(SENSOR_SOLAR_PRIMARY);
    tempSolarSecondary = getSensorValue(SENSOR_SOLAR_SECONDARY);
    // read sensorBoilerPower value
    int8_t state = getSensorBoilerPowerState();
    if (state != sensorBoilerPowerState) {
        // state changed
        sensorBoilerPowerState = state;
        tsSensorBoilerPower = tsCurr;
    }
}

int16_t getSensorValue(const uint8_t sensor) {
    float result = UNKNOWN_SENSOR_VALUE;
    if (SENSOR_SUPPLY == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_SUPPLY_ADDR)) {
            result = sensors.getTempC(SENSOR_SUPPLY_ADDR) * SENSOR_SUPPLY_FACTOR;
        }
    } else if (SENSOR_REVERSE == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_REVERSE_ADDR)) {
            result = sensors.getTempC(SENSOR_REVERSE_ADDR) * SENSOR_REVERSE_FACTOR;
        }
    } else if (SENSOR_TANK == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_TANK_ADDR)) {
            result = sensors.getTempC(SENSOR_TANK_ADDR) * SENSOR_TANK_FACTOR;
        }
    } else if (SENSOR_BOILER == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_BOILER_ADDR)) {
            result = sensors.getTempC(SENSOR_BOILER_ADDR) * SENSOR_BOILER_FACTOR;
        }
    } else if (SENSOR_MIX == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_MIX_ADDR)) {
            result = sensors.getTempC(SENSOR_MIX_ADDR) * SENSOR_MIX_FACTOR;
        }
    } else if (SENSOR_SB_HEATER == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_SB_HEATER_ADDR)) {
            result = sensors.getTempC(SENSOR_SB_HEATER_ADDR) * SENSOR_SB_HEATER_FACTOR;
        }
    } else if (SENSOR_SOLAR_PRIMARY == sensor) { // analog sensor
        float vin = 5; // 5V
        float r2 = 91.335; // 100Ohm + calibration
        int v = 0;
        for (int i = 0; i < 10; i++) {
            v += analogRead(SENSOR_SOLAR_PRIMARY);
            delay(100);
        }
        v = v / 10;
        float vout = (vin / 1023.0) * v;
        float r1 = (vin / vout - 1) * r2;
#ifdef __DEBUG__
        Serial.print(F("sensor SolarPrimary raw value: "));
        Serial.print(v);
        Serial.print(F("/"));
        Serial.print(r1);
        Serial.print(F("Ohm/"));
        Serial.print((r1 - 100)/0.39);
        Serial.println(F("C"));
#endif
        if (v > 0) {
            result = (r1 - 100) / 0.39;
        }
    } else if (SENSOR_SOLAR_SECONDARY == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_SOLAR_SECONDARY_ADDR)) {
            result = sensors.getTempC(SENSOR_SOLAR_SECONDARY_ADDR) * SENSOR_SOLAR_SECONDARY_FACTOR;
        }
    }
    result = (result > 0) ? result + 0.5 : result - 0.5;
    return int16_t(result);
}

int8_t getSensorBoilerPowerState() {
    uint16_t max = 0;
    for (uint8_t i = 0; i < 100; i++) {
        uint16_t v = abs(analogRead(SENSOR_BOILER_POWER) - 512);
        if (v > max && v < 1000) { // filter out error values. correct value should be <1000
            max = v;
        }
        delay(1);
    }
#ifdef __DEBUG__
    Serial.print(F("sensor BoilerPower raw value: "));
    Serial.println(max);
#endif
    return (max > SENSOR_BOILER_POWER_THERSHOLD) ? 1 : 0;
}

bool validSensorValues(const int16_t values[], const uint8_t size) {
    for (int i = 0; i < size; i++) {
        if (values[i] == UNKNOWN_SENSOR_VALUE) {
            return false;
        }
    }
    return true;
}

unsigned long getTimestamp() {
    unsigned long ts = millis();
    if (ts < tsPrevRaw) {
        overflowCount++;
    }
    tsPrevRaw = ts;
    return (MAX_TIMESTAMP / 1000) * overflowCount + (ts / 1000);
}

unsigned long diffTimestamps(unsigned long hi, unsigned long lo) {
    if (hi < lo) {
        return lo - hi;
    } else {
        return hi - lo;
    }
}

bool isInForcedMode(uint16_t bit, unsigned long ts) {
    if (NODE_FORCED_MODE_FLAGS & bit) { // forced mode
        if (ts == 0) {
            // permanent forced mode
            return true;
        }
        if (tsCurr < ts) {
            // didn't reach target TS. continue in forced mode
            return true;
        }
        // forced mode over. clear forced state
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS ^ bit;
    }
    return false;
}

void switchNodeState(uint8_t id, uint8_t sensId[], int16_t sensVal[], uint8_t sensCnt) {
    uint16_t bit;
    unsigned long* ts = NULL;
    unsigned long* tsf = NULL;
    if (NODE_SUPPLY == id) {
        bit = NODE_SUPPLY_BIT;
        ts = &tsNodeSupply;
        tsf = &tsForcedNodeSupply;
    } else if (NODE_HEATING == id) {
        bit = NODE_HEATING_BIT;
        ts = &tsNodeHeating;
        tsf = &tsForcedNodeHeating;
    } else if (NODE_FLOOR == id) {
        bit = NODE_FLOOR_BIT;
        ts = &tsNodeFloor;
        tsf = &tsForcedNodeFloor;
    } else if (NODE_HOTWATER == id) {
        bit = NODE_HOTWATER_BIT;
        ts = &tsNodeHotwater;
        tsf = &tsForcedNodeHotwater;
    } else if (NODE_CIRCULATION == id) {
        bit = NODE_CIRCULATION_BIT;
        ts = &tsNodeCirculation;
        tsf = &tsForcedNodeCirculation;
    } else if (NODE_SB_HEATER == id) {
        bit = NODE_SB_HEATER_BIT;
        ts = &tsNodeSbHeater;
        tsf = &tsForcedNodeSbHeater;
    } else if (NODE_SOLAR_PRIMARY == id) {
        bit = NODE_SOLAR_PRIMARY_BIT;
        ts = &tsNodeSolarPrimary;
        tsf = &tsForcedNodeSolarPrimary;
    } else if (NODE_SOLAR_SECONDARY == id) {
        bit = NODE_SOLAR_SECONDARY_BIT;
        ts = &tsNodeSolarSecondary;
        tsf = &tsForcedNodeSolarSecondary;
    } else if (NODE_HEATING_VALVE == id) {
        bit = NODE_HEATING_VALVE_BIT;
        ts = &tsNodeHeatingValve;
        tsf = &tsForcedNodeHeatingValve;
    }

    if (ts != NULL) {
        digitalWrite(id, digitalRead(id) ^ 1);
        NODE_STATE_FLAGS = NODE_STATE_FLAGS ^ bit;

        *ts = getTimestamp();

        // report state change
        StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root[MSG_TYPE_KEY] = MSG_NODE_STATE_CHANGED;
        root[ID_KEY] = id;
        root[STATE_KEY] = NODE_STATE_FLAGS & bit ? 1 : 0;
        root[TIMESTAMP_KEY] = *ts;
        root[FORCE_FLAG_KEY] = NODE_FORCED_MODE_FLAGS & bit ? 1 : 0;
        if (NODE_FORCED_MODE_FLAGS & bit) {
            root[FORCE_TIMESTAMP_KEY] = *tsf;
        }

        JsonArray& sensors = root.createNestedArray(SENSORS_KEY);
        for (unsigned int i = 0; i < sensCnt; i++) {
            JsonObject& sens = jsonBuffer.createObject();
            sens[ID_KEY] = sensId[i];
            sens[VALUE_KEY] = sensVal[i];
            sensors.add(sens);
        }

        char json[JSON_MAX_SIZE];
        root.printTo(json, JSON_MAX_SIZE);

        broadcastMsg(json);
    }
}

void forceNodeState(uint8_t id, uint8_t state, unsigned long ts) {
    if (NODE_SUPPLY == id) {
        forceNodeState(NODE_SUPPLY, NODE_SUPPLY_BIT, state, tsForcedNodeSupply, ts);
        reportNodeStatus(NODE_SUPPLY, NODE_SUPPLY_BIT, tsNodeSupply, tsForcedNodeSupply);
    } else if (NODE_HEATING == id) {
        forceNodeState(NODE_HEATING, NODE_HEATING_BIT, state, tsForcedNodeHeating, ts);
        reportNodeStatus(NODE_HEATING, NODE_HEATING_BIT, tsNodeHeating, tsForcedNodeHeating);
    } else if (NODE_FLOOR == id) {
        forceNodeState(NODE_FLOOR, NODE_FLOOR_BIT, state, tsForcedNodeFloor, ts);
        reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
    } else if (NODE_HOTWATER == id) {
        forceNodeState(NODE_HOTWATER, NODE_HOTWATER_BIT, state, tsForcedNodeHotwater, ts);
        reportNodeStatus(NODE_HOTWATER, NODE_HOTWATER_BIT, tsNodeHotwater, tsForcedNodeHotwater);
    } else if (NODE_CIRCULATION == id) {
        forceNodeState(NODE_CIRCULATION, NODE_CIRCULATION_BIT, state, tsForcedNodeCirculation, ts);
        reportNodeStatus(NODE_CIRCULATION, NODE_CIRCULATION_BIT, tsNodeCirculation, tsForcedNodeCirculation);
    } else if (NODE_SB_HEATER == id) {
        forceNodeState(NODE_SB_HEATER, NODE_SB_HEATER_BIT, state, tsForcedNodeSbHeater, ts);
        reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);
    } else if (NODE_SOLAR_PRIMARY == id) {
        forceNodeState(NODE_SOLAR_PRIMARY, NODE_SOLAR_PRIMARY_BIT, state, tsForcedNodeSolarPrimary, ts);
        reportNodeStatus(NODE_SOLAR_PRIMARY, NODE_SOLAR_PRIMARY_BIT, tsNodeSolarPrimary, tsForcedNodeSolarPrimary);
    } else if (NODE_SOLAR_SECONDARY == id) {
        forceNodeState(NODE_SOLAR_SECONDARY, NODE_SOLAR_SECONDARY_BIT, state, tsForcedNodeSolarSecondary, ts);
        reportNodeStatus(NODE_SOLAR_SECONDARY, NODE_SOLAR_SECONDARY_BIT, tsNodeSolarSecondary,
                tsForcedNodeSolarSecondary);
    } else if (NODE_HEATING_VALVE == id) {
        forceNodeState(NODE_HEATING_VALVE, NODE_HEATING_VALVE_BIT, state, tsForcedNodeHeatingValve, ts);
        reportNodeStatus(NODE_HEATING_VALVE, NODE_HEATING_VALVE_BIT, tsNodeHeatingValve,
                tsForcedNodeHeatingValve);
    }
    // update node modes in EEPROM if forced permanently
    if (ts == 0) {
        EEPROM.writeInt(NODE_STATE_EEPROM_ADDR, NODE_STATE_FLAGS);
        EEPROM.writeInt(NODE_FORCED_MODE_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
    }
}

void forceNodeState(uint8_t id, uint16_t bit, uint8_t state, unsigned long &nodeTs, unsigned long ts) {
    NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS | bit;
    if (ts == 0) {
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS | bit;
    } else {
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~bit;
    }
    nodeTs = ts;
    if (NODE_STATE_FLAGS & bit) {
        if (state == 0) {
            switchNodeState(id, 0, 0, 0);
        }
    } else {
        if (state != 0) {
            switchNodeState(id, 0, 0, 0);
        }
    }
}

void unForceNodeState(uint8_t id) {
    uint16_t prevPermanentlyForcedModeFlags = NODE_PERMANENTLY_FORCED_MODE_FLAGS;
    if (NODE_SUPPLY == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SUPPLY_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SUPPLY_BIT;
        reportNodeStatus(NODE_SUPPLY, NODE_SUPPLY_BIT, tsNodeSupply, tsForcedNodeSupply);
    } else if (NODE_HEATING == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_HEATING_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_HEATING_BIT;
        reportNodeStatus(NODE_HEATING, NODE_HEATING_BIT, tsNodeHeating, tsForcedNodeHeating);
    } else if (NODE_FLOOR == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_FLOOR_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_FLOOR_BIT;
        reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
    } else if (NODE_HOTWATER == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_HOTWATER_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_HOTWATER_BIT;
        reportNodeStatus(NODE_HOTWATER, NODE_HOTWATER_BIT, tsNodeHotwater, tsForcedNodeHotwater);
    } else if (NODE_CIRCULATION == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_CIRCULATION_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_CIRCULATION_BIT;
        reportNodeStatus(NODE_CIRCULATION, NODE_CIRCULATION_BIT, tsNodeCirculation, tsForcedNodeCirculation);
    } else if (NODE_SB_HEATER == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SB_HEATER_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SB_HEATER_BIT;
        reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);
    } else if (NODE_SOLAR_PRIMARY == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SOLAR_PRIMARY_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SOLAR_PRIMARY_BIT;
        reportNodeStatus(NODE_SOLAR_PRIMARY, NODE_SOLAR_PRIMARY_BIT, tsNodeSolarPrimary, tsForcedNodeSolarPrimary);
    } else if (NODE_SOLAR_SECONDARY == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SOLAR_SECONDARY_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SOLAR_SECONDARY_BIT;
        reportNodeStatus(NODE_SOLAR_SECONDARY, NODE_SOLAR_SECONDARY_BIT, tsNodeSolarSecondary,
                tsForcedNodeSolarSecondary);
    } else if (NODE_HEATING_VALVE == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_HEATING_VALVE_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_HEATING_VALVE_BIT;
        reportNodeStatus(NODE_HEATING_VALVE, NODE_HEATING_VALVE_BIT, tsNodeHeatingValve,
                tsForcedNodeHeatingValve);
    }
    // update node modes in EEPROM if unforced from permanent
    if (prevPermanentlyForcedModeFlags != NODE_PERMANENTLY_FORCED_MODE_FLAGS) {
        EEPROM.writeInt(NODE_FORCED_MODE_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
    }
}

bool wifiStationMode() {
    return strlen(WIFI_REMOTE_AP) + strlen(WIFI_REMOTE_PW) > 0;
}

bool wifiAPMode() {
    return strlen(WIFI_LOCAL_AP) + strlen(WIFI_LOCAL_PW) > 0;
}

void wifiInit() {
    // hardware reset
    digitalWrite(WIFI_RST_PIN, LOW);
    delay(500);
    digitalWrite(WIFI_RST_PIN, HIGH);
    delay(1000);

    wifi->end();
    wifi->begin(115200);
    wifi->println(F("AT+CIOBAUD=9600"));
    wifi->end();
    wifi->begin(9600);
    IP[0] = 0;
    IP[1] = 0;
    IP[2] = 0;
    IP[3] = 0;
}

void wifiSetup() {
#ifdef __DEBUG__
    Serial.println(F("wifi setup"));
#endif
    char buff[WIFI_MAX_BUFFER_SIZE + 1];
    wifi->println(F("AT+RST"));
    delay(500);
    wifi->println(F("AT+CIPMODE=0"));
    delay(100);
    if (wifiAPMode() && wifiStationMode()) {
        wifi->println(F("AT+CWMODE_CUR=3"));
#ifdef __DEBUG__
        Serial.println(F("runing in STATION & SoftAP mode"));
#endif
    } else if (wifiAPMode()) {
        wifi->println(F("AT+CWMODE_CUR=2"));
#ifdef __DEBUG__
        Serial.println(F("runing in SoftAP mode"));
#endif
    } else {
        wifi->println(F("AT+CWMODE_CUR=1"));
#ifdef __DEBUG__
        Serial.println(F("runing in STATION mode"));
#endif
    }
    delay(100);
#ifdef __DEBUG__
    Serial.println(F("starting TCP server"));
#endif
    wifi->println(F("AT+CIPMUX=1"));
    delay(100);
    wifi->println(F("AT+CIPSERVER=1,80"));
    delay(100);
    wifi->println(F("AT+CIPSTO=5"));
    delay(100);
    if (wifiAPMode()) {
        // setup AP
        // note: only channel 5 supported. bug?
        sprintf(buff, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d", WIFI_LOCAL_AP, WIFI_LOCAL_PW, 5, 3);
#ifdef __DEBUG__
        Serial.print(F("wifi setup AP: "));
        Serial.println(buff);
#endif
        wifi->println(buff);
        delay(100);
    }
    if (wifiStationMode()) {
        // join AP
        sprintf(buff, "AT+CWJAP_CUR=\"%s\",\"%s\"", WIFI_REMOTE_AP, WIFI_REMOTE_PW);
#ifdef __DEBUG__
        Serial.print(F("wifi join AP: "));
        Serial.println(buff);
#endif
        wifi->println(buff);
    }
    delay(10000);
    uint16_t l = wifi->readBytesUntil('\0', buff, WIFI_MAX_BUFFER_SIZE);
    buff[l] = '\0';
#ifdef __DEBUG__
    Serial.print(F("wifi setup rsp: "));
    Serial.println(buff);
#endif
}

void wifiCheckConnection() {
    if (wifiStationMode()) {
        if (!wifiGetRemoteIP()
                || (validIP(SERVER_IP)
                        && diffTimestamps(tsCurr, tsLastWifiSuccessTransmission) >= WIFI_MAX_FAILURE_PERIOD_SEC)) {
            wifiInit();
            wifiSetup();
            wifiGetRemoteIP();
            tsLastWifiSuccessTransmission = tsCurr;
        }
    }
}

bool wifiGetRemoteIP() {
    wifi->println(F("AT+CIFSR"));
    char buff[WIFI_MAX_AT_CMD_SIZE + 1];
    uint16_t l = wifi->readBytesUntil('\0', buff, WIFI_MAX_AT_CMD_SIZE);
    buff[l] = '\0';
#ifdef __DEBUG__
    logFreeMem();
    Serial.print(F("wifi rq ip: "));
    Serial.println(buff);
#endif
    int n = 0;
    int tmp[4];
    char *substr;
    substr = strstr(buff, ":STAIP,");
    if (substr != NULL) {
        n += sscanf(substr, ":STAIP,\"%d.%d.%d.%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
    }
    if (n == 4) {
        IP[0] = tmp[0];
        IP[1] = tmp[1];
        IP[2] = tmp[2];
        IP[3] = tmp[3];
#ifdef __DEBUG__
        char ip[16];
        sprintf(ip, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
        Serial.print(F("wifi got ip: "));
        Serial.println(ip);
#endif
        return validIP(IP);
    }
    return false;
}

bool validIP(uint8_t ip[4]) {
    int sum = ip[0] + ip[1] + ip[2] + ip[3];
    return sum > 0 && sum < 255 * 4;
}

bool wifiRsp(const char* body) {
    char atcmd[WIFI_MAX_AT_CMD_SIZE + 1];

    sprintf(atcmd, "AT+CIPSEND=%d,%d", CLIENT_ID, strlen(body));
    if (!wifiWrite(atcmd, ">", 100, 3))
        return false;

    if (!wifiWrite(body, OK, 300, 1))
        return false;

    sprintf(atcmd, "AT+CIPCLOSE=%d", CLIENT_ID);
    if (!wifiWrite(atcmd, OK))
        return false;

    return true;
}

bool wifiRsp200() {
    return wifiRsp("HTTP/1.0 200 OK\r\n");
}

bool wifiRsp400() {
    return wifiRsp("HTTP/1.0 400 Bad Request\r\n");
}

bool wifiSend(const char* msg) {
    if (validIP(IP) && validIP(SERVER_IP)) {
        char buff[WIFI_MAX_BUFFER_SIZE + 1];
        char atcmd[WIFI_MAX_AT_CMD_SIZE];

        sprintf(buff,
                "POST / HTTP/1.1\r\nUser-Agent: ESP8266\r\nAccept: */*\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s\r\n",
                strlen(msg), msg);

        sprintf(atcmd, "AT+CIPSTART=3,\"TCP\",\"%d.%d.%d.%d\",%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2],
                SERVER_IP[3], SERVER_PORT);
        if (!wifiWrite(atcmd, "CONNECT", 300, 3))
            return false;

        sprintf(atcmd, "AT+CIPSEND=3,%d", strlen(buff));
        if (!wifiWrite(atcmd, ">", 300, 3))
            return false;

        if (!wifiWrite(buff, OK, 300, 1))
            return false;

        // do not wait for server response
        sprintf(atcmd, "AT+CIPCLOSE=3");
        return wifiWrite(atcmd, OK, 100, 3);

    } else {
        return false;
    }
}

bool wifiWrite(const char* msg, const char* rsp, const int wait, const uint8_t maxRetry) {
    char r[strlen(rsp) + 1];
    strcpy(r, rsp);
    r[strlen(rsp)] = '\0';
    byte a = 0;
    while (a < maxRetry) {
        wifi->println(msg);
        delay(wait);
        if (wifi->find(r)) {
            break;
        }
        a++;
    }
    return a < maxRetry;
}

void wifiRead(char* req) {
    char buff[WIFI_MAX_BUFFER_SIZE + 1];
    uint16_t l = wifi->readBytesUntil('\0', buff, WIFI_MAX_BUFFER_SIZE);
    buff[l] = '\0';
#ifdef __DEBUG__
    Serial.print(F("wifi req len: "));
    Serial.println(l);
    Serial.print(F("wifi req: "));
    Serial.println(buff);
#endif
    char *substr;
    int n = 0;
    int clientId;
    substr = strstr(buff, "+IPD");
    if (substr != NULL) {
        n += sscanf(substr, "+IPD,%d", &clientId);
    }
    substr = strstr(substr, "\r\n\r\n");
    if (substr) {
        substr = strstr(substr, "{");
        if (substr) {
            char* end = strrchr(substr, '}');
            if (end && (end - substr) < JSON_MAX_SIZE) {
                strncpy(req, substr, (end - substr + 1));
                req[(end - substr + 1)] = '\0';
                n++;
            }
        }
    }
    if (n == 2) {
        CLIENT_ID = clientId;
    } else {
        req[0] = '\0';
    }
#ifdef __DEBUG__
    Serial.print(F("wifi client: "));
    Serial.println(clientId);
    Serial.print(F("wifi body: "));
    Serial.println(req);
#endif
}

void reportStatus() {
    if (nextEntryReport == 0) {
        // check WiFi connectivity
        wifiCheckConnection();
        // start reporting
        nextEntryReport = SENSOR_SUPPLY;
    }
    switch (nextEntryReport) {
    case SENSOR_SUPPLY:
        reportSensorStatus(SENSOR_SUPPLY, tempSupply);
        nextEntryReport = SENSOR_REVERSE;
        break;
    case SENSOR_REVERSE:
        reportSensorStatus(SENSOR_REVERSE, tempReverse);
        nextEntryReport = SENSOR_TANK;
        break;
    case SENSOR_TANK:
        reportSensorStatus(SENSOR_TANK, tempTank);
        nextEntryReport = SENSOR_MIX;
        break;
    case SENSOR_MIX:
        reportSensorStatus(SENSOR_MIX, tempMix);
        nextEntryReport = SENSOR_SB_HEATER;
        break;
    case SENSOR_SB_HEATER:
        reportSensorStatus(SENSOR_SB_HEATER, tempSbHeater);
        nextEntryReport = SENSOR_BOILER;
        break;
    case SENSOR_BOILER:
        reportSensorStatus(SENSOR_BOILER, tempBoiler);
        nextEntryReport = SENSOR_TEMP_ROOM_1;
        break;
    case SENSOR_TEMP_ROOM_1:
        reportSensorStatus(SENSOR_TEMP_ROOM_1, tempRoom1, tsLastSensorTempRoom1);
        nextEntryReport = SENSOR_HUM_ROOM_1;
        break;
    case SENSOR_HUM_ROOM_1:
        reportSensorStatus(SENSOR_HUM_ROOM_1, humRoom1, tsLastSensorHumRoom1);
        nextEntryReport = SENSOR_BOILER_POWER;
        break;
    case SENSOR_BOILER_POWER:
        reportSensorStatus(SENSOR_BOILER_POWER, sensorBoilerPowerState, tsSensorBoilerPower);
        nextEntryReport = SENSOR_SOLAR_PRIMARY;
        break;
    case SENSOR_SOLAR_PRIMARY:
        reportSensorStatus(SENSOR_SOLAR_PRIMARY, tempSolarPrimary);
        nextEntryReport = SENSOR_SOLAR_SECONDARY;
        break;
    case SENSOR_SOLAR_SECONDARY:
        reportSensorStatus(SENSOR_SOLAR_SECONDARY, tempSolarSecondary);
        nextEntryReport = NODE_SUPPLY;
        break;
    case NODE_SUPPLY:
        reportNodeStatus(NODE_SUPPLY, NODE_SUPPLY_BIT, tsNodeSupply, tsForcedNodeSupply);
        nextEntryReport = NODE_HEATING;
        break;
    case NODE_HEATING:
        reportNodeStatus(NODE_HEATING, NODE_HEATING_BIT, tsNodeHeating, tsForcedNodeHeating);
        reportSensorConfigValue(SENSOR_TH_ROOM1_PRIMARY_HEATER, PRIMARY_HEATER_ROOM_TEMP_THRESHOLD);
        nextEntryReport = NODE_FLOOR;
        break;
    case NODE_FLOOR:
        reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
        nextEntryReport = NODE_SB_HEATER;
        break;
    case NODE_SB_HEATER:
        reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);
        reportSensorConfigValue(SENSOR_TH_ROOM1_SB_HEATER, STANDBY_HEATER_ROOM_TEMP_THRESHOLD);
        nextEntryReport = NODE_HOTWATER;
        break;
    case NODE_HOTWATER:
        reportNodeStatus(NODE_HOTWATER, NODE_HOTWATER_BIT, tsNodeHotwater, tsForcedNodeHotwater);
        nextEntryReport = NODE_CIRCULATION;
        break;
    case NODE_CIRCULATION:
        reportNodeStatus(NODE_CIRCULATION, NODE_CIRCULATION_BIT, tsNodeCirculation, tsForcedNodeCirculation);
        nextEntryReport = NODE_SOLAR_PRIMARY;
        break;
    case NODE_SOLAR_PRIMARY:
        reportNodeStatus(NODE_SOLAR_PRIMARY, NODE_SOLAR_PRIMARY_BIT, tsNodeSolarPrimary, tsForcedNodeSolarPrimary);
        nextEntryReport = NODE_SOLAR_SECONDARY;
        break;
    case NODE_SOLAR_SECONDARY:
        reportNodeStatus(NODE_SOLAR_SECONDARY, NODE_SOLAR_SECONDARY_BIT, tsNodeSolarSecondary,
                tsForcedNodeSolarSecondary);
        nextEntryReport = NODE_HEATING_VALVE;
        break;
    case NODE_HEATING_VALVE:
        reportNodeStatus(NODE_HEATING_VALVE, NODE_HEATING_VALVE_BIT, tsNodeHeatingValve,
                tsForcedNodeHeatingValve);
        nextEntryReport = 0;
        break;
    default:
        break;
    }
}

void reportNodeStatus(uint8_t id, uint16_t bit, unsigned long ts, unsigned long tsf) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CURRENT_STATUS_REPORT;

    JsonObject& node = jsonBuffer.createObject();
    node[ID_KEY] = id;
    node[STATE_KEY] = NODE_STATE_FLAGS & bit ? 1 : 0;
    node[TIMESTAMP_KEY] = ts;
    node[FORCE_FLAG_KEY] = NODE_FORCED_MODE_FLAGS & bit ? 1 : 0;
    if (NODE_FORCED_MODE_FLAGS & bit) {
        node[FORCE_TIMESTAMP_KEY] = tsf;
    }

    root[NODES_KEY] = node;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

#ifdef __DEBUG__
    logFreeMem();
#endif
    broadcastMsg(json);
}

void reportSensorStatus(const uint8_t id, const int16_t value, const unsigned long ts) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CURRENT_STATUS_REPORT;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[VALUE_KEY] = value;
    if (ts != 0) {
        sens[TIMESTAMP_KEY] = ts;
    }

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

#ifdef __DEBUG__
    logFreeMem();
#endif
    broadcastMsg(json);
}

void reportConfiguration() {
    // check WiFi connectivity
    wifiCheckConnection();
    char ip[16];

    reportSensorCalibrationFactor(SENSOR_SUPPLY, SENSOR_SUPPLY_FACTOR);
    reportSensorCalibrationFactor(SENSOR_REVERSE, SENSOR_REVERSE_FACTOR);
    reportSensorCalibrationFactor(SENSOR_TANK, SENSOR_TANK_FACTOR);
    reportSensorCalibrationFactor(SENSOR_BOILER, SENSOR_BOILER_FACTOR);
    reportSensorCalibrationFactor(SENSOR_MIX, SENSOR_MIX_FACTOR);
    reportSensorCalibrationFactor(SENSOR_SB_HEATER, SENSOR_SB_HEATER_FACTOR);
    reportSensorCalibrationFactor(SENSOR_SOLAR_PRIMARY, SENSOR_SOLAR_PRIMARY_FACTOR);
    reportSensorCalibrationFactor(SENSOR_SOLAR_SECONDARY, SENSOR_SOLAR_SECONDARY_FACTOR);
    reportSensorConfigValue(SENSOR_TH_ROOM1_SB_HEATER, STANDBY_HEATER_ROOM_TEMP_THRESHOLD);
    reportSensorConfigValue(SENSOR_TH_ROOM1_PRIMARY_HEATER, PRIMARY_HEATER_ROOM_TEMP_THRESHOLD);
    reportStringConfig(WIFI_REMOTE_AP_KEY, WIFI_REMOTE_AP);
    reportStringConfig(WIFI_REMOTE_PASSWORD_KEY, WIFI_REMOTE_PW);
    reportStringConfig(WIFI_LOCAL_AP_KEY, WIFI_LOCAL_AP);
    reportStringConfig(WIFI_LOCAL_PASSWORD_KEY, WIFI_LOCAL_PW);
    sprintf(ip, "%d.%d.%d.%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2], SERVER_IP[3]);
    reportStringConfig(SERVER_IP_KEY, ip);
    reportNumberConfig(SERVER_PORT_KEY, SERVER_PORT);
    sprintf(ip, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
    reportStringConfig(LOCAL_IP_KEY, ip);
}

void reportSensorCalibrationFactor(const uint8_t id, const double value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[CALIBRATION_FACTOR_KEY] = value;

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    broadcastMsg(json);
}

void reportSensorConfigValue(const uint8_t id, const int8_t value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[VALUE_KEY] = value;

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    broadcastMsg(json);
}

void reportStringConfig(const char* key, const char* value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    broadcastMsg(json);
}

void reportNumberConfig(const char* key, const int value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    broadcastMsg(json);
}

void syncClocks() {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CLOCK_SYNC;
    root[TIMESTAMP_KEY] = getTimestamp();

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    broadcastMsg(json);
}

void broadcastMsg(const char* msg) {
    Serial.println(msg);
    bt->println(msg);
    bool ok = wifiSend(msg);
    if (ok) {
        tsLastWifiSuccessTransmission = tsCurr;
    }
#ifdef __DEBUG__
    if (ok) {
        Serial.println(F("wifi send: OK"));
    } else {
        Serial.println(F("wifi send: FAILED"));
    }
#endif
}

void processSerialMsg() {
    char buff[JSON_MAX_SIZE + 1];
    while (Serial.available()) {
        uint16_t l = Serial.readBytesUntil('\0', buff, JSON_MAX_SIZE);
        buff[l] = '\0';
        parseCommand(buff);
    }
}

void processBtMsg() {
    char buff[JSON_MAX_SIZE + 1];
    while (bt->available()) {
        uint16_t l = bt->readBytesUntil('\0', buff, JSON_MAX_SIZE);
        buff[l] = '\0';
        parseCommand(buff);
    }
}

void processWifiReq() {
    char buff[JSON_MAX_SIZE + 1];
    while (wifi->available()) {
        wifiRead(buff);
        if (parseCommand(buff)) {
            wifiRsp200();
        } else {
            wifiRsp400();
        }
    }
}

bool parseCommand(char* command) {
#ifdef __DEBUG__
    Serial.print(F("parsing cmd: "));
    Serial.println(command);
    if (strstr(command, "AT") == command) {
        // this is AT command for ESP8266
        Serial.print(F("sending to wifi: "));
        Serial.println(command);
        wifi->println(command);
        return true;
    }
#endif
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(command);
#ifdef __DEBUG__
    Serial.println(F("parsed"));
    logFreeMem();
#endif
    if (root.success()) {
        const char* msgType = root[MSG_TYPE_KEY];
        if (strcmp(msgType, MSG_CLOCK_SYNC) == 0) {
            // SYNC
            syncClocks();
        } else if (strcmp(msgType, MSG_CURRENT_STATUS_REPORT) == 0) {
            // CSR
            if (root.containsKey(SENSORS_KEY)) {
                JsonObject& sensor = root[SENSORS_KEY];
                uint8_t id = sensor[ID_KEY].as<uint8_t>();
                uint8_t val = sensor[VALUE_KEY].as<uint8_t>();
                // find sensor
                if (id == SENSOR_TEMP_ROOM_1) {
                    tempRoom1 = val;
                    tsLastSensorTempRoom1 = tsCurr;
                } else if (id == SENSOR_HUM_ROOM_1) {
                    humRoom1 = val;
                    tsLastSensorHumRoom1 = tsCurr;
                } // else if(...)
            } else {
                tsLastStatusReport = tsCurr - STATUS_REPORTING_PERIOD_SEC;
            }
        } else if (strcmp(msgType, MSG_NODE_STATE_CHANGED) == 0) {
            // NSC
            if (root.containsKey(ID_KEY)) {
                uint8_t id = root[ID_KEY].as<uint8_t>();
                if (root.containsKey(STATE_KEY)) {
                    uint8_t state = root[STATE_KEY].as<uint8_t>();
                    if (state == 0 || state == 1) {
                        unsigned long ts;
                        if (root.containsKey(FORCE_TIMESTAMP_KEY)) {
                            ts = root[FORCE_TIMESTAMP_KEY].as<unsigned long>();
                            ts += tsCurr;
                        } else {
                            ts = 0;
                        }
                        forceNodeState(id, state, ts);
                    }
                } else {
                    unForceNodeState(id);
                }
            }
        } else if (strcmp(msgType, MSG_CONFIGURATION) == 0) {
            // CFG
            if (root.containsKey(SENSORS_KEY)) {
                JsonObject& sensor = root[SENSORS_KEY].asObject();
                if (sensor.containsKey(ID_KEY) && sensor.containsKey(CALIBRATION_FACTOR_KEY)) {
                    uint8_t id = sensor[ID_KEY].as<uint8_t>();
                    double cf = sensor[CALIBRATION_FACTOR_KEY].as<double>();
                    if (id == SENSOR_SUPPLY) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 0, cf);
                        SENSOR_SUPPLY_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 0);
                    } else if (id == SENSOR_REVERSE) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 1, cf);
                        SENSOR_REVERSE_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 1);
                    } else if (id == SENSOR_TANK) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 2, cf);
                        SENSOR_TANK_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 2);
                    } else if (id == SENSOR_BOILER) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 3, cf);
                        SENSOR_BOILER_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 3);
                    } else if (id == SENSOR_MIX) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 4, cf);
                        SENSOR_MIX_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 4);
                    } else if (id == SENSOR_SB_HEATER) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 5, cf);
                        SENSOR_SB_HEATER_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 5);
                    } else if (id == SENSOR_SOLAR_PRIMARY) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 6, cf);
                        SENSOR_SOLAR_PRIMARY_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 6);
                    } else if (id == SENSOR_SOLAR_SECONDARY) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4 * 7, cf);
                        SENSOR_SOLAR_SECONDARY_FACTOR = readSensorCalibrationFactor(
                                SENSORS_FACTORS_EEPROM_ADDR + 4 * 7);
                    }
                } else if (sensor.containsKey(ID_KEY) && sensor.containsKey(VALUE_KEY)) {
                    uint8_t id = sensor[ID_KEY].as<uint8_t>();
                    uint8_t val = sensor[VALUE_KEY].as<uint8_t>();
                    if (id == SENSOR_TH_ROOM1_SB_HEATER) {
                        EEPROM.writeByte(STANDBY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR, val);
                        STANDBY_HEATER_ROOM_TEMP_THRESHOLD = val;
                        reportSensorConfigValue(SENSOR_TH_ROOM1_SB_HEATER, STANDBY_HEATER_ROOM_TEMP_THRESHOLD);
                    } else if (id == SENSOR_TH_ROOM1_PRIMARY_HEATER) {
                        EEPROM.writeByte(PRIMARY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR, val);
                        PRIMARY_HEATER_ROOM_TEMP_THRESHOLD = val;
                        reportSensorConfigValue(SENSOR_TH_ROOM1_PRIMARY_HEATER, PRIMARY_HEATER_ROOM_TEMP_THRESHOLD);
                    }
                }
            } else if (root.containsKey(WIFI_REMOTE_AP_KEY)) {
                sprintf(WIFI_REMOTE_AP, "%s", root[WIFI_REMOTE_AP_KEY].asString());
                EEPROM.writeBlock(WIFI_REMOTE_AP_EEPROM_ADDR, WIFI_REMOTE_AP, sizeof(WIFI_REMOTE_AP));
            } else if (root.containsKey(WIFI_REMOTE_PASSWORD_KEY)) {
                sprintf(WIFI_REMOTE_PW, "%s", root[WIFI_REMOTE_PASSWORD_KEY].asString());
                EEPROM.writeBlock(WIFI_REMOTE_PW_EEPROM_ADDR, WIFI_REMOTE_PW, sizeof(WIFI_REMOTE_PW));
            } else if (root.containsKey(WIFI_LOCAL_AP_KEY)) {
                sprintf(WIFI_LOCAL_AP, "%s", root[WIFI_LOCAL_AP_KEY].asString());
                EEPROM.writeBlock(WIFI_LOCAL_AP_EEPROM_ADDR, WIFI_LOCAL_AP, sizeof(WIFI_LOCAL_AP));
            } else if (root.containsKey(WIFI_LOCAL_PASSWORD_KEY)) {
                sprintf(WIFI_LOCAL_PW, "%s", root[WIFI_LOCAL_PASSWORD_KEY].asString());
                EEPROM.writeBlock(WIFI_LOCAL_PW_EEPROM_ADDR, WIFI_LOCAL_PW, sizeof(WIFI_LOCAL_PW));
            } else if (root.containsKey(SERVER_IP_KEY)) {
                int tmpIP[4];
                sscanf(root[SERVER_IP_KEY], "%d.%d.%d.%d", &tmpIP[0], &tmpIP[1], &tmpIP[2], &tmpIP[3]);
                SERVER_IP[0] = tmpIP[0];
                SERVER_IP[1] = tmpIP[1];
                SERVER_IP[2] = tmpIP[2];
                SERVER_IP[3] = tmpIP[3];
                EEPROM.writeBlock(SERVER_IP_EEPROM_ADDR, SERVER_IP, sizeof(SERVER_IP));
            } else if (root.containsKey(SERVER_PORT_KEY)) {
                SERVER_PORT = root[SERVER_PORT_KEY].as<int>();
                EEPROM.writeInt(SERVER_PORT_EEPROM_ADDR, SERVER_PORT);
            } else {
                reportConfiguration();
            }
        }
    } else {
        return false;
    }
#ifdef __DEBUG__
    logFreeMem();
#endif
    return true;
}
