#include "CentralUnit.h"

#include <Arduino.h>
#include <EEPROMex.h>
#include <RF24.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <debug.h>
#include <ESP8266.h>

#define __DEBUG__

/*============================= Global configuration ========================*/

// Sensors IDs
const uint8_t SENSOR_SUPPLY = 54;
const uint8_t SENSOR_REVERSE = 55;
const uint8_t SENSOR_TANK = 56;
const uint8_t SENSOR_BOILER = 57;
const uint8_t SENSOR_MIX = 58;
const uint8_t SENSOR_SB_HEATER = 59;
const uint8_t SENSOR_BOILER_POWER = 60;
const uint8_t SENSOR_SOLAR_PRIMARY = 61; //A7
const uint8_t SENSOR_SOLAR_SECONDARY = 62;
const uint8_t SENSOR_TEMP_ROOM_1 = (54 + 16) + (4 * 1) + 0;
const uint8_t SENSOR_HUM_ROOM_1 = (54 + 16) + (4 * 1) + 1;

const uint8_t SENSORS_ADDR_CFG[] = { SENSOR_SUPPLY, SENSOR_REVERSE, SENSOR_TANK, SENSOR_BOILER, SENSOR_MIX,
        SENSOR_SB_HEATER, SENSOR_SOLAR_PRIMARY, SENSOR_SOLAR_SECONDARY, /*reserved*/0, 0 };

// Sensor thresholds
const uint8_t SENSOR_TH_ROOM1_SB_HEATER = 200 + 1;
const uint8_t SENSOR_TH_ROOM1_PRIMARY_HEATER = 200 + 2;

const uint8_t SENSORS_TH_ADDR_CFG[] = { SENSOR_TH_ROOM1_SB_HEATER, SENSOR_TH_ROOM1_PRIMARY_HEATER, /*reserved*/0, 0, 0,
        0, 0, 0, 0, 0 };

// Sensors Bus pin
const uint8_t SENSORS_BUS = 49;

// Nodes pins
const uint8_t NODE_SUPPLY = 22; //7
const uint8_t NODE_HEATING = 24; //8
const uint8_t NODE_FLOOR = 26; //9
const uint8_t NODE_HOTWATER = 28; //10
const uint8_t NODE_CIRCULATION = 30; //11
const uint8_t NODE_SB_HEATER = 34;
const uint8_t NODE_SOLAR_PRIMARY = 36;
const uint8_t NODE_SOLAR_SECONDARY = 38;
const uint8_t NODE_HEATING_VALVE = 40;
const uint8_t NODE_RESERVED = 42;

// WiFi pins
const uint8_t WIFI_RST_PIN = 12;

const uint8_t HEARTBEAT_LED = 13;

// Nodes State
const uint16_t NODE_SUPPLY_BIT = 1;
const uint16_t NODE_HEATING_BIT = 2;
const uint16_t NODE_FLOOR_BIT = 4;
const uint16_t NODE_HOTWATER_BIT = 8;
const uint16_t NODE_CIRCULATION_BIT = 16;
const uint16_t NODE_SB_HEATER_BIT = 64;
const uint16_t NODE_SOLAR_PRIMARY_BIT = 128;
const uint16_t NODE_SOLAR_SECONDARY_BIT = 256;
const uint16_t NODE_HEATING_VALVE_BIT = 512;
//const uint16_t NODE_RESERVED_BIT = 1024;

// etc
const unsigned long MAX_TIMESTAMP = -1;
const int16_t UNKNOWN_SENSOR_VALUE = -127;
const uint8_t SENSORS_PRECISION = 9;
const unsigned long NODE_SWITCH_SAFE_TIME_SEC = 60;

// Primary Heater
const uint8_t PRIMARY_HEATER_SUPPLY_REVERSE_HIST = 5;
const uint8_t PRIMARY_HEATER_SHORT_CIRCUIT_THRESHOLD_TEMP = 50;
const unsigned long PRIMARY_HEATER_SHORT_CIRCUIT_PERIOD_SEC = 1200; // 20 minutes

// heating
const uint8_t HEATING_ON_TEMP_THRESHOLD = 40;
const uint8_t HEATING_OFF_TEMP_THRESHOLD = 36;
const uint8_t FLOOR_ON_TEMP_THRESHOLD = 35;
const uint8_t FLOOR_OFF_TEMP_THRESHOLD = 26;
const uint8_t PRIMARY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD = 20;
const unsigned long HEATING_ROOM_1_MAX_VALIDITY_PERIOD = 1800; // 30m

// Boiler heating
const uint8_t TANK_BOILER_HEATING_ON_HIST = 3;
const uint8_t TANK_BOILER_HEATING_OFF_HIST = 2;
const uint8_t BOILER_SOLAR_MAX_TEMP_THRESHOLD = 46;
const uint8_t BOILER_SOLAR_MAX_TEMP_HIST = 2;

// Circulation
const uint8_t CIRCULATION_MIN_TEMP_THRESHOLD = 50;
const uint8_t CIRCULATION_COOLING_TEMP_THRESHOLD = 61;
const unsigned long CIRCULATION_ACTIVE_PERIOD_SEC = 180; // 3m
const unsigned long CIRCULATION_PASSIVE_PERIOD_SEC = 3420; // 57m

// Standby Heater
const uint8_t STANDBY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD = 10;

// Tank
const uint8_t TANK_MIN_TEMP_THRESHOLD = 3;
const uint8_t TANK_MIN_TEMP_HIST = 2;

// Solar
const unsigned long SOLAR_PRIMARY_COLDSTART_PERIOD_SEC = 600; // 10m
const uint8_t SOLAR_PRIMARY_CRITICAL_TEMP_THRESHOLD = 110; // stagnation
const uint8_t SOLAR_PRIMARY_CRITICAL_TEMP_HIST = 10;
const uint8_t SOLAR_PRIMARY_BOILER_ON_HIST = 9;
const uint8_t SOLAR_PRIMARY_BOILER_OFF_HIST = 0;
const uint8_t SOLAR_SECONDARY_BOILER_ON_HIST = 0;

// sensor BoilerPower
const uint8_t SENSOR_BOILER_POWER_THERSHOLD = 100;

// reporting
const unsigned long STATUS_REPORTING_PERIOD_SEC = 5; // 5s
const unsigned long SENSORS_READ_INTERVAL_SEC = 5; // 5s
const uint16_t WIFI_FAILURE_GRACE_PERIOD_SEC = 300; // 5 minutes

// JSON
const char MSG_TYPE_KEY[] = "m";
const char ID_KEY[] = "id";
const char STATE_KEY[] = "ns";
const char FORCE_FLAG_KEY[] = "ff";
const char TIMESTAMP_KEY[] = "ts";
const char FORCE_TIMESTAMP_KEY[] = "ft";
const char CALIBRATION_FACTOR_KEY[] = "cf";
const char NODES_KEY[] = "n";
const char SENSORS_KEY[] = "s";
const char VALUE_KEY[] = "v";
const char UID_KEY[] = "uid";

const char MSG_CURRENT_STATUS_REPORT[] = "csr";
const char MSG_NODE_STATE_CHANGED[] = "nsc";
const char MSG_CLOCK_SYNC[] = "cls";
const char MSG_CONFIGURATION[] = "cfg";

const char WIFI_REMOTE_AP_KEY[] = "rap";
const char WIFI_REMOTE_PASSWORD_KEY[] = "rpw";
const char WIFI_LOCAL_AP_KEY[] = "lap";
const char WIFI_LOCAL_PASSWORD_KEY[] = "lpw";
const char SERVER_IP_KEY[] = "sip";
const char SERVER_PORT_KEY[] = "sp";
const char LOCAL_IP_KEY[] = "lip";
const char DEBUG_SERIAL_PORT_KEY[] = "dsp";

const uint8_t JSON_MAX_SIZE = 128;
const uint8_t JSON_MAX_BUFFER_SIZE = 255;

/*============================= Global variables ============================*/

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

unsigned long tsPrev = 0;
unsigned long tsPrevRaw = 0;
unsigned long tsCurr = 0;

uint8_t overflowCount = 0;

unsigned long boilerPowersaveCurrentPassivePeriod = 0;

// WiFi
esp_config_t WIFI_REMOTE_AP;
esp_config_t WIFI_REMOTE_PW;
esp_config_t WIFI_LOCAL_AP;
esp_config_t WIFI_LOCAL_PW;
esp_ip_t SERVER_IP = { 0, 0, 0, 0 };
uint16_t SERVER_PORT = 80;
esp_ip_t WIFI_STA_IP = { 0, 0, 0, 0 };
uint16_t TCP_SERVER_PORT = 80;

// EEPROM addresses
const int NODE_STATE_FLAGS_EEPROM_ADDR = 0;
const int NODE_FORCED_MODE_FLAGS_EEPROM_ADDR = NODE_STATE_FLAGS_EEPROM_ADDR + sizeof(NODE_STATE_FLAGS);
const int SENSORS_UIDS_EEPROM_ADDR = NODE_FORCED_MODE_FLAGS_EEPROM_ADDR + sizeof(NODE_FORCED_MODE_FLAGS);
const int SENSORS_FACTORS_EEPROM_ADDR = SENSORS_UIDS_EEPROM_ADDR
        + sizeof(DeviceAddress) * sizeof(SENSORS_ADDR_CFG) / sizeof(SENSORS_ADDR_CFG[0]);
const int SENSORS_TH_EEPROM_ADDR = SENSORS_FACTORS_EEPROM_ADDR
        + sizeof(double) * sizeof(SENSORS_ADDR_CFG) / sizeof(SENSORS_ADDR_CFG[0]);
const int WIFI_REMOTE_AP_EEPROM_ADDR = SENSORS_TH_EEPROM_ADDR
        + sizeof(int16_t) * sizeof(SENSORS_TH_ADDR_CFG) / sizeof(SENSORS_TH_ADDR_CFG[0]);
const int WIFI_REMOTE_PW_EEPROM_ADDR = WIFI_REMOTE_AP_EEPROM_ADDR + sizeof(WIFI_REMOTE_AP);
const int SERVER_IP_EEPROM_ADDR = WIFI_REMOTE_PW_EEPROM_ADDR + sizeof(WIFI_REMOTE_PW);
const int SERVER_PORT_EEPROM_ADDR = SERVER_IP_EEPROM_ADDR + sizeof(SERVER_IP);
const int WIFI_LOCAL_AP_EEPROM_ADDR = SERVER_PORT_EEPROM_ADDR + sizeof(SERVER_PORT);
const int WIFI_LOCAL_PW_EEPROM_ADDR = WIFI_LOCAL_AP_EEPROM_ADDR + sizeof(WIFI_LOCAL_AP);

int eepromWriteCount = 0;

/*============================= Connectivity ================================*/

// Sensors
OneWire oneWire(SENSORS_BUS);
DallasTemperature sensors(&oneWire);

// Serial port
HardwareSerial *serial = &Serial;
#ifdef __DEBUG__
HardwareSerial *debug = serial;
#else
HardwareSerial *debug = NULL;
#endif

// BT
HardwareSerial *bt = &Serial1;

//WiFi
HardwareSerial *wifi = &Serial2;
ESP8266 esp8266;

/*============================= Arduino entry point =========================*/

void setup() {
    // Setup serial ports
    serial->begin(115200);
    bt->begin(115200);
    wifi->begin(115200);

    dbg(debug, F(":STARTING\n"));

    // init heartbeat led
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // init esp8266 hw reset pin
    pinMode(WIFI_RST_PIN, OUTPUT);

    // init boilerPower sensor
    pinMode(SENSOR_BOILER_POWER, INPUT);

    // init solar primary sensor
    pinMode(SENSOR_SOLAR_PRIMARY, INPUT);

    // restore forced node state flags from EEPROM
    // default node state -- OFF
    restoreNodesState();

    // init nodes pins
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

    // init sensors
    sensors.begin();
    sensors.setResolution(SENSORS_PRECISION);
    searchSensors();

    // setup WiFi
    loadWifiConfig();
    dbgf(debug, F(":setup wifi:R_AP:%s:L_AP:%s:L_PORT:%d\n"), &WIFI_REMOTE_AP, &WIFI_LOCAL_AP, TCP_SERVER_PORT);
    esp8266.init(wifi, MODE_STA_AP, WIFI_RST_PIN, WIFI_FAILURE_GRACE_PERIOD_SEC);
    esp8266.startAP(&WIFI_LOCAL_AP, &WIFI_LOCAL_PW);
    esp8266.connect(&WIFI_REMOTE_AP, &WIFI_REMOTE_PW);
    esp8266.startTcpServer(TCP_SERVER_PORT);
    esp8266.getStaIP(WIFI_STA_IP);
    dbgf(debug, F(":STA_IP: %d.%d.%d.%d\n"), WIFI_STA_IP[0], WIFI_STA_IP[1], WIFI_STA_IP[2], WIFI_STA_IP[3]);
}

void loop() {
    tsCurr = getTimestamp();
    if (diffTimestamps(tsCurr, tsLastSensorsRead) >= SENSORS_READ_INTERVAL_SEC) {
        readSensors();
        tsLastSensorsRead = tsCurr;

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
    if (serial->available() > 0) {
        processSerialMsg();
    }
    if (bt->available() > 0) {
        processBtMsg();
    }
    if (wifi->available() > 0) {
        processWifiMsg();
    }

    if (diffTimestamps(tsCurr, tsLastStatusReport) >= STATUS_REPORTING_PERIOD_SEC) {
        reportStatus();
        tsLastStatusReport = tsCurr;
    }

    tsPrev = tsCurr;
}

/*========================= Node processing methods =========================*/

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
                    if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
                        // standby heater is on
                        if (tempSbHeater <= tempMix && tempSbHeater <= tempTank) {
                            // temp in standby heater <= (temp in Mix && Tank)
                            // turn pump OFF
                            switchNodeState(NODE_HEATING, sensIds, sensVals, sensCnt);
                        } else {
                            // temp in standby heater > (temp in Mix || Tank)
                            // do nothing
                        }
                    } else {
                        // standby heater is off
                        // turn pump OFF
                        switchNodeState(NODE_HEATING, sensIds, sensVals, sensCnt);
                    }
                }
            } else {
                // temp is high enough at least in one source
                if (!room1TempSatisfyMaxThreshold() || (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT)) {
                    // temp in Room1 is low OR standby heater is on
                    // do nothing
                } else {
                    // temp in Room1 is high enough AND standby heater is off
                    // turn pump OFF
                    switchNodeState(NODE_HEATING, sensIds, sensVals, sensCnt);
                }
            }
        } else {
            // pump is OFF
            int16_t maxTemp = max(tempTank, max(tempMix, tempSbHeater));
            if (NODE_STATE_FLAGS & NODE_HEATING_VALVE_BIT) {
                maxTemp = tempSbHeater;
            }
            if (maxTemp >= HEATING_ON_TEMP_THRESHOLD) {
                // temp in one of applicable sources is high enough
                if (!room1TempSatisfyMaxThreshold() || (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT)) {
                    // temp in Room1 is low OR standby heater is on
                    // turn pump ON
                    switchNodeState(NODE_HEATING, sensIds, sensVals, sensCnt);
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
            int16_t maxTemp = max(tempTank, max(tempMix, tempSbHeater));
            if (NODE_STATE_FLAGS & NODE_HEATING_VALVE_BIT) {
                maxTemp = tempSbHeater;
            }
            if (maxTemp >= FLOOR_ON_TEMP_THRESHOLD) {
                // temp in one of applicable sources is high enough
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
                        if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_PASSIVE_PERIOD_SEC) {
                            // passive period is over
                            // turn pump ON
                            switchNodeState(NODE_CIRCULATION, sensIds, sensVals, sensCnt);
                        } else {
                            // passive period is going on
                            // do nothing
                        }
                    }
                } else {
                    // solar secondary node is OFF
                    if (sensorBoilerPowerState == 1) {
                        // boilePower is ON
                        if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_PASSIVE_PERIOD_SEC) {
                            // passive period is over
                            // turn pump ON
                            switchNodeState(NODE_CIRCULATION, sensIds, sensVals, sensCnt);
                        } else {
                            // passive period is going on
                            // do nothing
                        }
                    } else {
                        // boilerPower is OFF
                        // do nothing
                    }
                }
            } else {
                // temp in boiler is normal
                if (tempBoiler >= CIRCULATION_MIN_TEMP_THRESHOLD) {
                    // temp in boiler is high enough
                    if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_PASSIVE_PERIOD_SEC) {
                        // passive period is over
                        if ((NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) || sensorBoilerPowerState == 1) {
                            // (solar secondary || boilderPower) is ON
                            // turn pump ON
                            switchNodeState(NODE_CIRCULATION, sensIds, sensVals, sensCnt);
                        } else {
                            // (solar secondary && boilderPower) is OFF
                            // do nothing
                        }
                    } else {
                        // passive period is going on
                        // do nothing
                    }
                } else {
                    // temp in boiler is too low
                    // do nothing
                }
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
                if (tempTank > readSensorTH(SENSOR_TH_ROOM1_SB_HEATER) && room1TempReachedMinThreshold()) {
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
        reportNodeStatus(NODE_HEATING_VALVE, NODE_HEATING_VALVE_BIT, tsNodeHeatingValve, tsForcedNodeHeatingValve);
    }
    // update node modes in EEPROM if forced permanently
    if (ts == 0) {
        eepromWriteCount += EEPROM.updateInt(NODE_STATE_FLAGS_EEPROM_ADDR, NODE_STATE_FLAGS);
        eepromWriteCount += EEPROM.updateInt(NODE_FORCED_MODE_FLAGS_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
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
        reportNodeStatus(NODE_HEATING_VALVE, NODE_HEATING_VALVE_BIT, tsNodeHeatingValve, tsForcedNodeHeatingValve);
    }
    // update node modes in EEPROM if unforced from permanent
    if (prevPermanentlyForcedModeFlags != NODE_PERMANENTLY_FORCED_MODE_FLAGS) {
        eepromWriteCount += EEPROM.updateInt(NODE_FORCED_MODE_FLAGS_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
    }
}

bool room1TempReachedMinThreshold() {
    return (tsLastSensorTempRoom1 != 0
            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
            && tempRoom1 > readSensorTH(SENSOR_TH_ROOM1_SB_HEATER))
            || (tsLastSensorTempRoom1 != 0
                    && diffTimestamps(tsCurr, tsLastSensorTempRoom1) >= HEATING_ROOM_1_MAX_VALIDITY_PERIOD)
            || (tsLastSensorTempRoom1 == 0);
}

bool room1TempFailedMinThreshold() {
    return (tsLastSensorTempRoom1 != 0
            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
            && tempRoom1 < readSensorTH(SENSOR_TH_ROOM1_SB_HEATER));
}

bool room1TempSatisfyMaxThreshold() {
    return tsLastSensorTempRoom1 != 0
            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
            && tempRoom1 >= readSensorTH(SENSOR_TH_ROOM1_PRIMARY_HEATER);
}

/*====================== Load/Save configuration in EEPROM ==================*/

double readSensorCF(uint8_t sensor) {
    double f = 0;
    if (sensorCfOffset(sensor) >= 0) {
        f = EEPROM.readDouble(SENSORS_FACTORS_EEPROM_ADDR + sensorCfOffset(sensor));
        if (isnan(f) || f <= 0) {
            f = 1;
        }
    }
    return f;
}

void saveSensorCF(uint8_t sensor, double value) {
    if (sensorCfOffset(sensor) >= 0) {
        eepromWriteCount += EEPROM.updateDouble(SENSORS_FACTORS_EEPROM_ADDR + sensorCfOffset(sensor), value);
    }
}

void readSensorUID(uint8_t sensor, DeviceAddress uid) {
    uid[0] = uid[1] = uid[2] = uid[3] = uid[4] = uid[5] = uid[6] = uid[7] = 0;
    if (sensorUidOffset(sensor) >= 0) {
        EEPROM.readBlock(SENSORS_UIDS_EEPROM_ADDR + sensorUidOffset(sensor), uid, sizeof(DeviceAddress));
    }
}

void saveSensorUID(uint8_t sensor, DeviceAddress uid) {
    if (sensorUidOffset(sensor) >= 0) {
        eepromWriteCount += EEPROM.updateBlock(SENSORS_UIDS_EEPROM_ADDR + sensorUidOffset(sensor), uid,
                sizeof(DeviceAddress));
    }
}

int16_t readSensorTH(uint8_t sensor) {
    int16_t v = UNKNOWN_SENSOR_VALUE;
    if (sensorThOffset(sensor) >= 0) {
        v = EEPROM.readInt(SENSORS_TH_EEPROM_ADDR + sensorThOffset(sensor));
    }
    return v;
}

void saveSensorTH(uint8_t sensor, int16_t value) {
    if (sensorThOffset(sensor) >= 0) {
        eepromWriteCount += EEPROM.updateInt(SENSORS_TH_EEPROM_ADDR + sensorThOffset(sensor), value);
    }
}

int sensorCfOffset(uint8_t sensor) {
    int offset = 0;
    for (uint8_t i = 0; i < sizeof(SENSORS_ADDR_CFG) / sizeof(SENSORS_ADDR_CFG[0]); i++) {
        if (SENSORS_ADDR_CFG[i] == sensor) {
            return offset;
        }
        offset += sizeof(double);
    }
    return -1;
}

int sensorUidOffset(uint8_t sensor) {
    int offset = 0;
    for (uint8_t i = 0; i < sizeof(SENSORS_ADDR_CFG) / sizeof(SENSORS_ADDR_CFG[0]); i++) {
        if (SENSORS_ADDR_CFG[i] == sensor) {
            return offset;
        }
        offset += sizeof(DeviceAddress);
    }
    return -1;
}

int sensorThOffset(uint8_t sensor) {
    int offset = 0;
    for (uint8_t i = 0; i < sizeof(SENSORS_TH_ADDR_CFG) / sizeof(SENSORS_TH_ADDR_CFG[0]); i++) {
        if (SENSORS_TH_ADDR_CFG[i] == sensor) {
            return offset;
        }
        offset += sizeof(int16_t);
    }
    return -1;
}

void restoreNodesState() {
    NODE_STATE_FLAGS = EEPROM.readInt(NODE_STATE_FLAGS_EEPROM_ADDR);
    NODE_FORCED_MODE_FLAGS = EEPROM.readInt(NODE_FORCED_MODE_FLAGS_EEPROM_ADDR);
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

void validateStringParam(char* str, int maxSize) {
    char* ptr = str;
    while (*ptr && (ptr - str) < maxSize) {
        ptr++;
    }
    if (*ptr) {
        str[0] = '\0';
    }
}

void str2uid(const char *str, DeviceAddress uid) {
    char buf[2];
    for (uint8_t i = 0; i < 16; i = i + 2) {
        buf[0] = str[i];
        buf[1] = str[i + 1];
        sscanf(buf, "%X", (unsigned int*) &uid[i / 2]);
    }
}

void uid2str(const DeviceAddress uid, char *str) {
    for (uint8_t i = 0; i < 8; i++) {
        sprintf(&str[i * 2], "%02X", uid[i]);
    }
}

/*====================== Sensors processing methods =========================*/

void searchSensors() {
    DeviceAddress sensAddr;
    dbg(debug, F(":sensors:found:\n"));
    oneWire.reset_search();
    while (oneWire.search(sensAddr)) {
        char buf[16];
        uid2str(sensAddr, buf);
        dbgf(debug, F("%s\n"), buf);
    }
    dbg(debug, F(":sensors:end\n"));
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
    if (SENSOR_SOLAR_PRIMARY == sensor) { // analog sensor
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

        dbgf(debug, F(":SolarPrimary:%d/%dOhm/%dC\n"), v, r1, (r1 - 100) / 0.39);

        if (v > 0) {
            result = (r1 - 100) / 0.39;
        }
    } else { // all other digital
        DeviceAddress uid;
        readSensorUID(sensor, uid);
        if (sensors.requestTemperaturesByAddress(uid)) {
            uint8_t retry = 0;
            while ((result = sensors.getTempC(uid)) == DEVICE_DISCONNECTED_C && retry < 10) {
                delay(10);
                retry++;
            }
            if (result != DEVICE_DISCONNECTED_C) {
                result *= readSensorCF(sensor);
            } else {
                result = UNKNOWN_SENSOR_VALUE;
            }
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
    uint8_t state = (max > SENSOR_BOILER_POWER_THERSHOLD) ? 1 : 0;
    dbgf(debug, F(":BoilerPower:%d/%d\n"), max, state);
    return state;
}

bool validSensorValues(const int16_t values[], const uint8_t size) {
    for (int i = 0; i < size; i++) {
        if (values[i] == UNKNOWN_SENSOR_VALUE) {
            return false;
        }
    }
    return true;
}
/*============================ Reporting ====================================*/

void reportStatus() {
    if (nextEntryReport == 0) {
        // start reporting
        nextEntryReport = SENSOR_SUPPLY;
    }
    //////////////////////////
    char buf[JSON_MAX_BUFFER_SIZE] = "{\"m\":\"nsc\",\"id\":36,\"ns\":0,\"ts\":68086,\"ff\":0,\"s\":[{\"id\":61,\"v\":42},{\"id\":57,\"v\":45}]}";
    //////////////////////////
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
        reportSensorThStatus(SENSOR_TH_ROOM1_PRIMARY_HEATER);
        nextEntryReport = NODE_FLOOR;
        break;
    case NODE_FLOOR:
        reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
        nextEntryReport = NODE_SB_HEATER;
        break;
    case NODE_SB_HEATER:
        reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);
        reportSensorThStatus(SENSOR_TH_ROOM1_SB_HEATER);
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
        reportNodeStatus(NODE_HEATING_VALVE, NODE_HEATING_VALVE_BIT, tsNodeHeatingValve, tsForcedNodeHeatingValve);
        nextEntryReport = 0;
        ///////////////////
        broadcastMsg(buf);
        ///////////////////
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

    broadcastMsg(json);
}

void reportSensorThStatus(const uint8_t id) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[VALUE_KEY] = readSensorTH(id);

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    broadcastMsg(json);
}

void reportTimestamp() {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CLOCK_SYNC;
    root[TIMESTAMP_KEY] = getTimestamp();

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    broadcastMsg(json);
}

void reportConfiguration() {
    char ip[16];
    reportSensorCfConfig(SENSOR_SUPPLY);
    reportSensorCfConfig(SENSOR_REVERSE);
    reportSensorCfConfig(SENSOR_TANK);
    reportSensorCfConfig(SENSOR_BOILER);
    reportSensorCfConfig(SENSOR_MIX);
    reportSensorCfConfig(SENSOR_SB_HEATER);
    reportSensorCfConfig(SENSOR_SOLAR_PRIMARY);
    reportSensorCfConfig(SENSOR_SOLAR_SECONDARY);

    reportSensorUidConfig(SENSOR_SUPPLY);
    reportSensorUidConfig(SENSOR_REVERSE);
    reportSensorUidConfig(SENSOR_TANK);
    reportSensorUidConfig(SENSOR_BOILER);
    reportSensorUidConfig(SENSOR_MIX);
    reportSensorUidConfig(SENSOR_SB_HEATER);
    reportSensorUidConfig(SENSOR_SOLAR_SECONDARY);

    reportSensorThConfig(SENSOR_TH_ROOM1_SB_HEATER);
    reportSensorThConfig(SENSOR_TH_ROOM1_PRIMARY_HEATER);

    reportStringConfig(WIFI_REMOTE_AP_KEY, WIFI_REMOTE_AP);
    reportStringConfig(WIFI_REMOTE_PASSWORD_KEY, WIFI_REMOTE_PW);
    reportStringConfig(WIFI_LOCAL_AP_KEY, WIFI_LOCAL_AP);
    reportStringConfig(WIFI_LOCAL_PASSWORD_KEY, WIFI_LOCAL_PW);
    sprintf(ip, "%d.%d.%d.%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2], SERVER_IP[3]);
    reportStringConfig(SERVER_IP_KEY, ip);
    reportNumberConfig(SERVER_PORT_KEY, SERVER_PORT);
    sprintf(ip, "%d.%d.%d.%d", WIFI_STA_IP[0], WIFI_STA_IP[1], WIFI_STA_IP[2], WIFI_STA_IP[3]);
    reportStringConfig(LOCAL_IP_KEY, ip);
    dbgf(debug, F(":EEPROM:written:%d bytes\n"), eepromWriteCount);
}

void reportSensorCfConfig(const uint8_t id) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[CALIBRATION_FACTOR_KEY] = readSensorCF(id);

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    serial->println(json);
    bt->println(json);
}

void reportSensorUidConfig(const uint8_t id) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    DeviceAddress uid;
    readSensorUID(id, uid);
    char uidStr[16];
    uid2str(uid, uidStr);
    sens[UID_KEY] = uidStr;

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    serial->println(json);
    bt->println(json);
}

void reportSensorThConfig(const uint8_t id) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[VALUE_KEY] = readSensorTH(id);

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    serial->println(json);
    bt->println(json);
}

void reportStringConfig(const char* key, const char* value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    serial->println(json);
    bt->println(json);
}

void reportNumberConfig(const char* key, const int value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    serial->println(json);
    bt->println(json);
}

/*========================= Communication ===================================*/

void processSerialMsg() {
    char buff[JSON_MAX_SIZE + 1];
    while (serial->available()) {
        uint16_t l = serial->readBytesUntil('\0', buff, JSON_MAX_SIZE);
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

void processWifiMsg() {
    char buff[JSON_MAX_SIZE + 1];
    unsigned long start = millis();
    uint8_t l = esp8266.receive(buff, JSON_MAX_SIZE);
    if (l > 0) {
        dbgf(debug, F(":HTTP:receive:%d bytes:[%d msec]\n"), l, millis() - start);
        parseCommand(buff);
    }
}

void broadcastMsg(const char* msg) {
    serial->println(msg);
    bt->println(msg);
    unsigned long start = millis();
    int httpRsp = esp8266.send(SERVER_IP, SERVER_PORT, msg);
    dbgf(debug, F(":HTTP:send:%d:[%d msec]\n"), httpRsp, millis() - start);
}

bool parseCommand(char* command) {
    dbgf(debug, F(":parse cmd:%s\n"), command);

    if (strstr(command, "AT") == command) {
        // this is AT command for ESP8266
        dbgf(debug, F(":ESP8266:%s\n"), command);
        esp8266.write(command);
        return true;
    }

    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(command);

    if (root.success()) {
        const char* msgType = root[MSG_TYPE_KEY];
        if (strcmp(msgType, MSG_CLOCK_SYNC) == 0) {
            // SYNC
            reportTimestamp();
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
                    saveSensorCF(id, cf);
                } else if (sensor.containsKey(ID_KEY) && sensor.containsKey(UID_KEY)) {
                    uint8_t id = sensor[ID_KEY].as<uint8_t>();
                    DeviceAddress uid;
                    str2uid(sensor[UID_KEY].asString(), uid);
                    saveSensorUID(id, uid);
                } else if (sensor.containsKey(ID_KEY) && sensor.containsKey(VALUE_KEY)) {
                    uint8_t id = sensor[ID_KEY].as<uint8_t>();
                    int16_t val = sensor[VALUE_KEY].as<int16_t>();
                    saveSensorTH(id, val);
                    reportSensorStatus(id, readSensorTH(id));
                }
            } else if (root.containsKey(WIFI_REMOTE_AP_KEY)) {
                sprintf(WIFI_REMOTE_AP, "%s", root[WIFI_REMOTE_AP_KEY].asString());
                eepromWriteCount += EEPROM.updateBlock(WIFI_REMOTE_AP_EEPROM_ADDR, WIFI_REMOTE_AP,
                        sizeof(WIFI_REMOTE_AP));
            } else if (root.containsKey(WIFI_REMOTE_PASSWORD_KEY)) {
                sprintf(WIFI_REMOTE_PW, "%s", root[WIFI_REMOTE_PASSWORD_KEY].asString());
                eepromWriteCount += EEPROM.updateBlock(WIFI_REMOTE_PW_EEPROM_ADDR, WIFI_REMOTE_PW,
                        sizeof(WIFI_REMOTE_PW));
            } else if (root.containsKey(WIFI_LOCAL_AP_KEY)) {
                sprintf(WIFI_LOCAL_AP, "%s", root[WIFI_LOCAL_AP_KEY].asString());
                eepromWriteCount += EEPROM.updateBlock(WIFI_LOCAL_AP_EEPROM_ADDR, WIFI_LOCAL_AP, sizeof(WIFI_LOCAL_AP));
            } else if (root.containsKey(WIFI_LOCAL_PASSWORD_KEY)) {
                sprintf(WIFI_LOCAL_PW, "%s", root[WIFI_LOCAL_PASSWORD_KEY].asString());
                eepromWriteCount += EEPROM.updateBlock(WIFI_LOCAL_PW_EEPROM_ADDR, WIFI_LOCAL_PW, sizeof(WIFI_LOCAL_PW));
            } else if (root.containsKey(SERVER_IP_KEY)) {
                sscanf(root[SERVER_IP_KEY], "%d.%d.%d.%d", (int*) &SERVER_IP[0], (int*) &SERVER_IP[1],
                        (int*) &SERVER_IP[2], (int*) &SERVER_IP[3]);
                eepromWriteCount += EEPROM.updateBlock(SERVER_IP_EEPROM_ADDR, SERVER_IP, sizeof(SERVER_IP));
            } else if (root.containsKey(SERVER_PORT_KEY)) {
                SERVER_PORT = root[SERVER_PORT_KEY].as<int>();
                eepromWriteCount += EEPROM.updateInt(SERVER_PORT_EEPROM_ADDR, SERVER_PORT);
            } else if (root.containsKey(DEBUG_SERIAL_PORT_KEY)) {
                int sp = root[DEBUG_SERIAL_PORT_KEY].as<int>();
                if (sp == -1) {
                    debug = NULL;
                } else if (sp == 0) {
                    debug = serial;
                } else if (sp == 1) {
                    debug = bt;
                }
                esp8266.setDebug(debug);
            } else {
                reportConfiguration();
            }
        }
    } else {
        return false;
    }
    return true;
}

/*========================= Helper methods ==================================*/

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

