#include "CentralUnit.h"

#include <Arduino.h>
#include <EEPROMex.h>
#include <MemoryFree.h>
#include <RF24.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* ========= Configuration ========= */

#undef __DEBUG__

// Sensors IDs
static const uint8_t SENSOR_SUPPLY = 54;
static const uint8_t SENSOR_REVERSE = 55;
static const uint8_t SENSOR_TANK = 56;
static const uint8_t SENSOR_BOILER = 57;
static const uint8_t SENSOR_MIX = 58;
static const uint8_t SENSOR_SB_HEATER = 59;
static const uint8_t SENSOR_TEMP_ROOM_1 = (54 + 16) + (4 * 1) + 0;
static const uint8_t SENSOR_HUM_ROOM_1 = (54 + 16) + (4 * 1) + 1;

// Sensors Addresses
static const DeviceAddress SENSOR_SUPPLY_ADDR = { 0x28, 0xFF, 0x70, 0x84, 0x23, 0x16, 0x04, 0xA5 };
static const DeviceAddress SENSOR_REVERSE_ADDR = { 0x28, 0xFF, 0xB7, 0x7C, 0x23, 0x16, 0x04, 0xBB };
static const DeviceAddress SENSOR_TANK_ADDR = { 0x28, 0xFF, 0x62, 0x82, 0x23, 0x16, 0x04, 0x53 };
static const DeviceAddress SENSOR_BOILER_ADDR = { 0x28, 0xFF, 0xD1, 0x84, 0x23, 0x16, 0x04, 0x5B };
static const DeviceAddress SENSOR_MIX_ADDR = { 0x28, 0xFF, 0x45, 0x90, 0x23, 0x16, 0x04, 0xC5 };
static const DeviceAddress SENSOR_SB_HEATER_ADDR = { 0x28, 0xFF, 0x28, 0x5B, 0x23, 0x16, 0x04, 0x95 };

// Sensors Bus pin
static const uint8_t SENSORS_BUS = 49;

// Nodes pins
static const uint8_t NODE_SUPPLY = 22; //7
static const uint8_t NODE_HEATING = 24; //8
static const uint8_t NODE_FLOOR = 26; //9
static const uint8_t NODE_HOTWATER = 28; //10
static const uint8_t NODE_CIRCULATION = 30; //11
static const uint8_t NODE_BOILER = 32; //12
static const uint8_t NODE_SB_HEATER = 34;

// RF pins
static const uint8_t RF_CE_PIN = 9; //5
static const uint8_t RF_CSN_PIN = 10; //6

static const uint8_t HEARTBEAT_LED = 13;

// Nodes State
static const uint8_t NODE_SUPPLY_BIT = 1;
static const uint8_t NODE_HEATING_BIT = 2;
static const uint8_t NODE_FLOOR_BIT = 4;
static const uint8_t NODE_HOTWATER_BIT = 8;
static const uint8_t NODE_CIRCULATION_BIT = 16;
static const uint8_t NODE_BOILER_BIT = 32;
static const uint8_t NODE_SB_HEATER_BIT = 64;

// etc
static const unsigned long MAX_TIMESTAMP = -1;
static const int8_t UNKNOWN_SENSOR_VALUE = -127;
static const uint8_t SENSORS_PRECISION = 9;
static const unsigned long NODE_SWITCH_SAFE_TIME_MSEC = 60000;

// EEPROM
static const int NODE_STATE_EEPROM_ADDR = 0;
static const int NODE_FORCED_MODE_EEPROM_ADDR = 1;
static const int SENSORS_FACTORS_EEPROM_ADDR = 2;
static const int WIFI_AP_EEPROM_ADDR = SENSORS_FACTORS_EEPROM_ADDR + (6 * 4);
static const int WIFI_PW_EEPROM_ADDR = WIFI_AP_EEPROM_ADDR + 32;
static const int SERVER_IP_EEPROM_ADDR = WIFI_PW_EEPROM_ADDR + 32;
static const int SERVER_PORT_EEPROM_ADDR = SERVER_IP_EEPROM_ADDR + 4;

// Primary Heater
static const uint8_t PRIMARY_HEATER_SUPPLY_REVERSE_HIST = 5;

// heating
static const uint8_t HEATING_TEMP_THRESHOLD = 40;
static const uint8_t HEATING_PRIMARY_HIST = 3;
static const uint8_t HEATING_STANDBY_HIST = 22;
static const uint8_t HEATING_ROOM_1_THRESHOLD = 22;
static const unsigned long HEATING_ROOM_1_MAX_VALIDITY_PERIOD = 300000; // 5m

// Boiler heating
static const uint8_t TANK_BOILER_HIST = 3;

// Circulation
static const uint8_t CIRCULATION_TEMP_THRESHOLD = 37;
static const unsigned long CIRCULATION_ACTIVE_PERIOD_MSEC = 300000; // 5m
static const unsigned long CIRCULATION_PASSIVE_PERIOD_MSEC = 1800000; // 30m

// Standby Heater
static const uint8_t STANDBY_HEATER_WATER_MIN_TEMP_THRESHOLD = 12;
static const uint8_t STANDBY_HEATER_WATER_MAX_TEMP_THRESHOLD = 16;
static const uint8_t STANDBY_HEATER_ROOM_MIN_TEMP_THRESHOLD = 12;
static const uint8_t STANDBY_HEATER_ROOM_MAX_TEMP_THRESHOLD = 16;

// reporting
static const unsigned long STATUS_REPORTING_PERIOD_MSEC = 60000;
static const unsigned long SENSORS_REFRESH_INTERVAL_MSEC = 9000;
static const unsigned long SENSORS_READ_INTERVAL_MSEC = 3000;

// RF
static const uint64_t RF_PIPE_BASE = 0xE8E8F0F0A2LL;
static const uint8_t RF_PACKET_LENGTH = 32;

// sensors stats
static const uint8_t SENSORS_RAW_VALUES_MAX_COUNT = 3;
static const uint8_t SENSOR_NOISE_THRESHOLD = 255; // disable

// JSON
static const char MSG_TYPE_KEY[] = "m";
static const char ID_KEY[] = "id";
static const char STATE_KEY[] = "ns";
static const char FORCE_FLAG_KEY[] = "ff";
static const char TIMESTAMP_KEY[] = "ts";
static const char FORCE_TIMESTAMP_KEY[] = "ft";
static const char OVERFLOW_COUNT_KEY[] = "oc";
static const char CALIBRATION_FACTOR_KEY[] = "cf";
static const char NODES_KEY[] = "n";
static const char SENSORS_KEY[] = "s";
static const char VALUE_KEY[] = "v";

static const char MSG_CURRENT_STATUS_REPORT[] = "csr";
static const char MSG_NODE_STATE_CHANGED[] = "nsc";
static const char MSG_CLOCK_SYNC[] = "cls";
static const char MSG_CONFIGURATION[] = "cfg";

static const char WIFI_AP_KEY[] = "ap";
static const char WIFI_PASSWORD_KEY[] = "pw";
static const char SERVER_IP_KEY[] = "sip";
static const char SERVER_PORT_KEY[] = "sp";
static const char LOCAL_IP_KEY[] = "lip";

static const uint8_t JSON_MAX_WRITE_SIZE = 128;
static const uint8_t JSON_MAX_READ_SIZE = 128;
static const uint8_t JSON_MAX_BUFFER_SIZE = 255;
static const uint8_t RF_MAX_BODY_SIZE = 65;
static const uint8_t HTTP_MAX_BODY_SIZE = 255;
static const uint8_t HTTP_MAX_GET_SIZE = 32;

/* ===== End of Configuration ====== */

/* =========== Variables =========== */

//// Sensors
// Values
int8_t tempSupply = UNKNOWN_SENSOR_VALUE;
int8_t tempReverse = UNKNOWN_SENSOR_VALUE;
int8_t tempTank = UNKNOWN_SENSOR_VALUE;
int8_t tempBoiler = UNKNOWN_SENSOR_VALUE;
int8_t tempMix = UNKNOWN_SENSOR_VALUE;
int8_t tempSbHeater = UNKNOWN_SENSOR_VALUE;
int8_t tempRoom1 = UNKNOWN_SENSOR_VALUE;
int8_t humRoom1 = UNKNOWN_SENSOR_VALUE;

// stats
int8_t rawSupplyValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawReverseValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawTankValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawBoilerValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawMixValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawSbHeaterValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawSupplyIdx = 0;
uint8_t rawReverseIdx = 0;
uint8_t rawTankIdx = 0;
uint8_t rawBoilerIdx = 0;
uint8_t rawMixIdx = 0;
uint8_t rawSbHeaterIdx = 0;

//// Nodes

// Nodes actual state
uint8_t NODE_STATE_FLAGS = 255;
// Nodes forced mode
uint8_t NODE_FORCED_MODE_FLAGS = 0;
uint8_t FORCED_MODE_OVERFLOW_TS_FLAGS = 0;
// will be stored in EEPROM
uint8_t NODE_PERMANENTLY_FORCED_MODE_FLAGS = 0;

// Nodes switch timestamps
unsigned long tsNodeSupply = 0;
unsigned long tsNodeHeating = 0;
unsigned long tsNodeFloor = 0;
unsigned long tsNodeHotwater = 0;
unsigned long tsNodeCirculation = 0;
unsigned long tsNodeBoiler = 0;
unsigned long tsNodeSbHeater = 0;

// Nodes forced mode timestamps
unsigned long tsForcedNodeSupply = 0;
unsigned long tsForcedNodeHeating = 0;
unsigned long tsForcedNodeFloor = 0;
unsigned long tsForcedNodeHotwater = 0;
unsigned long tsForcedNodeCirculation = 0;
unsigned long tsForcedNodeBoiler = 0;
unsigned long tsForcedNodeSbHeater = 0;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorsRefresh = 0;
unsigned long tsLastSensorsRead = 0;

unsigned long tsLastSensorTempRoom1 = 0;
unsigned long tsLastSensorHumRoom1 = 0;

unsigned long tsPrev = 0;
unsigned long tsCurr = 0;

uint8_t overflowCount = 0;

// sensor calibration factors
double SENSOR_SUPPLY_FACTOR = 1;
double SENSOR_REVERSE_FACTOR = 1;
double SENSOR_TANK_FACTOR = 1;
double SENSOR_BOILER_FACTOR = 1;
double SENSOR_MIX_FACTOR = 1;
double SENSOR_SB_HEATER_FACTOR = 1;

unsigned long boilerPowersaveCurrentPassivePeriod = 0;

// WiFi
char WIFI_AP[32];
char WIFI_PW[32];
uint8_t SERVER_IP[4] = { 0, 0, 0, 0 };
int SERVER_PORT = 80;
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

// RF24
RF24 radio(RF_CE_PIN, RF_CSN_PIN);

//======== ============ ========//

void setup() {
    // Setup serial ports
    Serial.begin(9600);     // usb
#ifdef __DEBUG__
            Serial.println("STARTING");
#endif
    bt->begin(9600);         // BT
    loadWifiConfig();
    wifiInit();             // WiFi
    wifiConnect();
    wifiStartServer();
#ifdef __DEBUG__
    Serial.print("IP: ");
    char lIP[16];
    sprintf(lIP, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
    Serial.println(lIP);
    Serial.print("free memory: ");
    Serial.println(freeMemory());
#endif
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // Init sensors
    sensors.begin();
    sensors.setResolution(SENSORS_PRECISION);
#ifdef __DEBUG__
    DeviceAddress sensAddr;
    Serial.println("found sensors:");
    oneWire.reset_search();
    while (oneWire.search(sensAddr)) {
        for (uint8_t i = 0; i < 8; i++) {
            // zero pad the address if necessary
            if (sensAddr[i] < 16)
            Serial.print("0");
            Serial.print(sensAddr[i], HEX);
        }
        Serial.println("");
    }
#endif

    // sync clock
    syncClocks();

    // read forced node state flags from EEPROM
    // default node state -- ON
    restoreNodesState();

    // init nodes
    pinMode(NODE_SUPPLY, OUTPUT);
    pinMode(NODE_HEATING, OUTPUT);
    pinMode(NODE_FLOOR, OUTPUT);
    pinMode(NODE_HOTWATER, OUTPUT);
    pinMode(NODE_CIRCULATION, OUTPUT);
    pinMode(NODE_BOILER, OUTPUT);
    pinMode(NODE_SB_HEATER, OUTPUT);

    // restore nodes state
    digitalWrite(NODE_SUPPLY, NODE_STATE_FLAGS & NODE_SUPPLY_BIT);
    digitalWrite(NODE_HEATING, NODE_STATE_FLAGS & NODE_HEATING_BIT);
    digitalWrite(NODE_FLOOR, NODE_STATE_FLAGS & NODE_FLOOR_BIT);
    digitalWrite(NODE_HOTWATER, NODE_STATE_FLAGS & NODE_HOTWATER_BIT);
    digitalWrite(NODE_CIRCULATION, NODE_STATE_FLAGS & NODE_CIRCULATION_BIT);
    digitalWrite(NODE_BOILER, NODE_STATE_FLAGS & NODE_BOILER_BIT);
    digitalWrite(NODE_SB_HEATER, NODE_STATE_FLAGS & NODE_SB_HEATER_BIT);

    // init sensors raw values arrays
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        rawSupplyValues[i] = UNKNOWN_SENSOR_VALUE;
        rawReverseValues[i] = UNKNOWN_SENSOR_VALUE;
        rawTankValues[i] = UNKNOWN_SENSOR_VALUE;
        rawBoilerValues[i] = UNKNOWN_SENSOR_VALUE;
        rawMixValues[i] = UNKNOWN_SENSOR_VALUE;
        rawSbHeaterValues[i] = UNKNOWN_SENSOR_VALUE;
    }

    // read sensors calibration factors from EEPROM
    loadSensorsCalibrationFactors();

    // init RF24 radio
    radio.begin();
    radio.enableDynamicPayloads();
    // listen on 2 channels
    for (int i = 1; i <= 2; i++) {
        radio.openReadingPipe(i, RF_PIPE_BASE + i);
    }
    radio.startListening();
    radio.powerUp();

    // report current state
    if (!wifiGetIP()) {
        wifiConnect();
        wifiStartServer();
        wifiGetIP();
    }
    reportStatus();
}

void loop() {
    tsCurr = getTimestamp();
    if (tsCurr < tsPrev) {
        // sync clocks on overflow
        overflowCount++;
        FORCED_MODE_OVERFLOW_TS_FLAGS = 0;
        syncClocks();
    }
    if (diffTimestamps(tsCurr, tsLastSensorsRead) >= SENSORS_READ_INTERVAL_MSEC) {
        readSensors();
        tsLastSensorsRead = tsCurr;
    }
    if (diffTimestamps(tsCurr, tsLastSensorsRefresh) >= SENSORS_REFRESH_INTERVAL_MSEC) {
#ifdef __DEBUG__
        Serial.print("free memory: ");
        Serial.println(freeMemory());
#endif
        digitalWrite(HEARTBEAT_LED, HIGH);

        // refresh sensors values
        refreshSensorValues();

        // heater <--> tank
        processSupplyCircuit();
        // tank <--> heating system
        processHeatingCircuit();
        // floors
        processFloorCircuit();
        // hot water
        processHotWaterCircuit();
        // circulation
        processCirculationCircuit();
        // boiler heater
        processBoilerHeater();
        // standby heater
        processStandbyHeater();

        if (diffTimestamps(tsCurr, tsLastStatusReport) >= STATUS_REPORTING_PERIOD_MSEC) {
            if (!wifiGetIP()) {
                wifiConnect();
                wifiStartServer();
                wifiGetIP();
            }
            reportStatus();
        }
        delay(100);
        digitalWrite(HEARTBEAT_LED, LOW);
        tsLastSensorsRefresh = tsCurr;
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
    tsPrev = tsCurr;
}

void processSupplyCircuit() {
    uint8_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_SUPPLY_BIT;
    if (isInForcedMode(NODE_SUPPLY_BIT, tsForcedNodeSupply)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_SUPPLY, NODE_SUPPLY_BIT, tsNodeSupply, tsForcedNodeSupply);
    }
    if (diffTimestamps(tsCurr, tsNodeSupply) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_SUPPLY, SENSOR_REVERSE };
        int8_t sensVals[] = { tempSupply, tempReverse };
        if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
            // pump is ON
            if (tempSupply <= tempReverse) {
                // temp is equal
                // turn pump OFF
                switchNodeState(NODE_SUPPLY, sensIds, sensVals, 2);
            } else {
                // delta is big enough
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempSupply >= (tempReverse + PRIMARY_HEATER_SUPPLY_REVERSE_HIST)) {
                // delta is big enough
                // turn pump ON
                switchNodeState(NODE_SUPPLY, sensIds, sensVals, 2);
            } else {
                // temp is equal
                // do nothing
            }
        }
    }
}

void processHeatingCircuit() {
    uint8_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_HEATING_BIT;
    if (isInForcedMode(NODE_HEATING_BIT, tsForcedNodeHeating)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_HEATING, NODE_HEATING_BIT, tsNodeHeating, tsForcedNodeHeating);
    }
    if (diffTimestamps(tsCurr, tsNodeHeating) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_MIX, SENSOR_SB_HEATER };
        int8_t sensVals[] = { tempTank, tempMix, tempSbHeater };
        if (NODE_STATE_FLAGS & NODE_HEATING_BIT) {
            // pump is ON
            if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
                // standby heater is on
                if (tempSbHeater < (HEATING_TEMP_THRESHOLD - HEATING_STANDBY_HIST)) {
                    // temp in standby heater is too low
                    // turn pump OFF
                    switchNodeState(NODE_HEATING, sensIds, sensVals, 3);
                } else {
                    // temp in standby heater is high enough
                    // do nothing
                }
            } else {
                // standby heater is off
                if (tempTank < (HEATING_TEMP_THRESHOLD - HEATING_PRIMARY_HIST)
                        && tempMix < (HEATING_TEMP_THRESHOLD - HEATING_PRIMARY_HIST)
                        && tempSbHeater < (HEATING_TEMP_THRESHOLD - HEATING_PRIMARY_HIST)) {
                    // temp in (tank && mix && sb_heater) is too low
                    // turn pump OFF
                    switchNodeState(NODE_HEATING, sensIds, sensVals, 3);
                } else {
                    // temp is high enough at least in one source
                    if (tsLastSensorTempRoom1 != 0
                            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
                            && tempRoom1 >= HEATING_ROOM_1_THRESHOLD) {
                        // temp in Room1 is high enough
                        // turn pump OFF
                        switchNodeState(NODE_HEATING, sensIds, sensVals, 3);
                    } else {
                        // temp in Room1 is too low
                        // do nothing
                    }
                }
            }
        } else {
            // pump is OFF
            if (tempTank >= HEATING_TEMP_THRESHOLD || tempMix >= HEATING_TEMP_THRESHOLD
                    || tempSbHeater >= HEATING_TEMP_THRESHOLD) {
                // temp in (tank || mix || sb_heater) is high enough
                if (tsLastSensorTempRoom1 != 0
                        && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
                        && tempRoom1 >= HEATING_ROOM_1_THRESHOLD && !(NODE_STATE_FLAGS & NODE_SB_HEATER_BIT)) {
                    // temp in Room1 is high enough and standby heater is off
                    // do nothing
                } else {
                    // temp in Room1 is low or standby heater is on
                    // turn pump ON
                    switchNodeState(NODE_HEATING, sensIds, sensVals, 3);
                }
            } else {
                // temp in (tank && mix && sb_heater) is too low
                // do nothing
            }
        }
    }
}

void processFloorCircuit() { // replicate Heating node
    uint8_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_FLOOR_BIT;
    if (isInForcedMode(NODE_FLOOR_BIT, tsForcedNodeFloor)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
    }
    if (diffTimestamps(tsCurr, tsNodeFloor) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_MIX, SENSOR_SB_HEATER };
        int8_t sensVals[] = { tempTank, tempMix, tempSbHeater };
        if (NODE_STATE_FLAGS & NODE_FLOOR_BIT) {
            // pump is ON
            if (NODE_STATE_FLAGS & NODE_HEATING_BIT) {
                // heating pump is on
                // do nothing
            } else {
                // heating pump is off
                // turn pump OFF
                switchNodeState(NODE_FLOOR, sensIds, sensVals, 3);
            }
        } else {
            // pump is OFF
            if (NODE_STATE_FLAGS & NODE_HEATING_BIT) {
                // heating pump is on
                // turn pump ON
                switchNodeState(NODE_FLOOR, sensIds, sensVals, 3);
            } else {
                // heating pump is off
                // do nothing
            }
        }
    }
}

void processHotWaterCircuit() {
    uint8_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_HOTWATER_BIT;
    if (isInForcedMode(NODE_HOTWATER_BIT, tsForcedNodeHotwater)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_HOTWATER, NODE_HOTWATER_BIT, tsNodeHotwater, tsForcedNodeHotwater);
    }
    if (diffTimestamps(tsCurr, tsNodeHotwater) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_BOILER };
        int8_t sensVals[] = { tempTank, tempBoiler };
        if (NODE_STATE_FLAGS & NODE_HOTWATER_BIT) {
            // pump is ON
            if (tempTank <= tempBoiler) {
                // temp is equal
                // turn pump OFF
                switchNodeState(NODE_HOTWATER, sensIds, sensVals, 2);
            } else {
                // temp in tank is high enough
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempTank > (tempBoiler + TANK_BOILER_HIST)) {
                // temp in tank is high enough
                // turn pump ON
                switchNodeState(NODE_HOTWATER, sensIds, sensVals, 2);
            } else {
                // temp in tank is too low
                // do nothing
            }
        }
    }
}

void processCirculationCircuit() {
    uint8_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_CIRCULATION_BIT;
    if (isInForcedMode(NODE_CIRCULATION_BIT, tsForcedNodeCirculation)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_CIRCULATION, NODE_CIRCULATION_BIT, tsNodeCirculation, tsForcedNodeCirculation);
    }
    if (diffTimestamps(tsCurr, tsNodeCirculation) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_BOILER };
        int8_t sensVals[] = { tempBoiler };
        if (NODE_STATE_FLAGS & NODE_CIRCULATION_BIT) {
            // pump is ON
            if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_ACTIVE_PERIOD_MSEC) {
                // active period is over
                // turn pump OFF
                switchNodeState(NODE_CIRCULATION, sensIds, sensVals, 1);
            } else {
                // active period is going on
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempBoiler >= CIRCULATION_TEMP_THRESHOLD) {
                // temp in boiler is high enough
                if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_PASSIVE_PERIOD_MSEC) {
                    // passive period is over
                    // turn pump ON
                    switchNodeState(NODE_CIRCULATION, sensIds, sensVals, 1);
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

void processBoilerHeater() {
    uint8_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_BOILER_BIT;
    if (isInForcedMode(NODE_BOILER_BIT, tsForcedNodeBoiler)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_BOILER, NODE_BOILER_BIT, tsNodeBoiler, tsForcedNodeBoiler);
    }
    if (diffTimestamps(tsCurr, tsNodeBoiler) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_BOILER };
        int8_t sensVals[] = { tempBoiler };
        if (NODE_STATE_FLAGS & NODE_BOILER_BIT) {
            // boiler is ON
            if (NODE_STATE_FLAGS & NODE_HOTWATER_BIT) {
                // hotwater circuit is ON
                // turn boiler OFF
                switchNodeState(NODE_BOILER, sensIds, sensVals, 1);
            } else {
                // hotwater circuit is OFF
                // do nothing
            }
        } else {
            // boiler is OFF
            if (NODE_STATE_FLAGS & NODE_HOTWATER_BIT) {
                // hotwater circuit is on
                // do nothing
            } else {
                // hotwater circuit is off
                // turn boiler ON
                switchNodeState(NODE_BOILER, sensIds, sensVals, 1);
            }
        }
    }
}

void processStandbyHeater() {
    uint8_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_SB_HEATER_BIT;
    if (isInForcedMode(NODE_SB_HEATER_BIT, tsForcedNodeSbHeater)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);
    }
    if (diffTimestamps(tsCurr, tsNodeSbHeater) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_TEMP_ROOM_1 };
        int8_t sensVals[] = { tempTank, tempRoom1 };
        if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
            // heater is ON
            if (tempTank >= STANDBY_HEATER_WATER_MAX_TEMP_THRESHOLD
                    || (tsLastSensorTempRoom1 != 0
                            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
                            && tempRoom1 >= STANDBY_HEATER_ROOM_MAX_TEMP_THRESHOLD)) {
                // temp in (tank || room1) is high enough
                // turn heater OFF
                switchNodeState(NODE_SB_HEATER, sensIds, sensVals, 2);
            } else {
                // temp in (tank && room1) is too low
                // do nothing
            }
        } else {
            // heater is OFF
            if (tempTank <= STANDBY_HEATER_WATER_MIN_TEMP_THRESHOLD
                    || (tsLastSensorTempRoom1 != 0
                            && diffTimestamps(tsCurr, tsLastSensorTempRoom1) < HEATING_ROOM_1_MAX_VALIDITY_PERIOD
                            && tempRoom1 <= STANDBY_HEATER_ROOM_MIN_TEMP_THRESHOLD)) {
                // temp in (tank || room1) is too low
                // turn heater ON
                switchNodeState(NODE_SB_HEATER, sensIds, sensVals, 2);
            } else {
                // temp in (tank && room1) is high enough
                // do nothing
            }
        }
    }
}

/* ============ Helper methods ============ */

void loadSensorsCalibrationFactors() {
    SENSOR_SUPPLY_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 0);
    SENSOR_REVERSE_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4);
    SENSOR_TANK_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 8);
    SENSOR_BOILER_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 12);
    SENSOR_MIX_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 16);
    SENSOR_SB_HEATER_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 20);
}

double readSensorCalibrationFactor(int offset) {
    double f = EEPROM.readDouble(offset);
    if (f > 0) {
        return f;
    } else {
        return 1;
    }
}

void writeSensorCalibrationFactor(int offset, double value) {
    EEPROM.writeDouble(offset, value);
}

void rfRead(char* msg) {
    uint8_t len = 0;
    bool eoi = false;
    while (!eoi && len < RF_MAX_BODY_SIZE - 1) {
        char packet[RF_PACKET_LENGTH];
        uint8_t l = radio.getDynamicPayloadSize();
        if (l > RF_PACKET_LENGTH) {
            len = 0;
            break;
        }
        radio.read(packet, l);
        for (uint8_t i = 0; i < l && !eoi && len < RF_MAX_BODY_SIZE - 1; i++) {
            msg[len++] = packet[i];
            eoi = (packet[i] == '\n');
        }
    }
    msg[len] = '\0';
}

void readSensors() {
    readSensor(SENSOR_SUPPLY, rawSupplyValues, rawSupplyIdx);
    readSensor(SENSOR_REVERSE, rawReverseValues, rawReverseIdx);
    readSensor(SENSOR_TANK, rawTankValues, rawTankIdx);
    readSensor(SENSOR_BOILER, rawBoilerValues, rawBoilerIdx);
    readSensor(SENSOR_MIX, rawMixValues, rawMixIdx);
    readSensor(SENSOR_SB_HEATER, rawSbHeaterValues, rawSbHeaterIdx);
    // read remote sensors
    if (radio.available()) {
#ifdef __DEBUG__
        Serial.println("got RF message");
#endif
        processRfMsg();
    }
}

void readSensor(const uint8_t id, int8_t* const &values, uint8_t &idx) {
    uint8_t prevIdx;
    uint8_t val;
    if (idx == SENSORS_RAW_VALUES_MAX_COUNT) {
        idx = 0;
    }
    if (idx > 0) {
        prevIdx = idx - 1;
    } else {
        prevIdx = SENSORS_RAW_VALUES_MAX_COUNT;
    }
    val = getSensorValue(id);
    if (values[prevIdx] == UNKNOWN_SENSOR_VALUE || abs(val - values[prevIdx]) < SENSOR_NOISE_THRESHOLD) {
        values[idx] = val;
    }
    idx++;
}

void refreshSensorValues() {
    double temp1 = 0;
    double temp2 = 0;
    double temp3 = 0;
    double temp4 = 0;
    double temp5 = 0;
    double temp6 = 0;
    int j1 = 0;
    int j2 = 0;
    int j3 = 0;
    int j4 = 0;
    int j5 = 0;
    int j6 = 0;
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        if (rawSupplyValues[i] != UNKNOWN_SENSOR_VALUE) {
            temp1 += rawSupplyValues[i];
            j1++;
        }
        if (rawReverseValues[i] != UNKNOWN_SENSOR_VALUE) {
            temp2 += rawReverseValues[i];
            j2++;
        }
        if (rawTankValues[i] != UNKNOWN_SENSOR_VALUE) {
            temp3 += rawTankValues[i];
            j3++;
        }
        if (rawBoilerValues[i] != UNKNOWN_SENSOR_VALUE) {
            temp4 += rawBoilerValues[i];
            j4++;
        }
        if (rawMixValues[i] != UNKNOWN_SENSOR_VALUE) {
            temp5 += rawMixValues[i];
            j5++;
        }
        if (rawSbHeaterValues[i] != UNKNOWN_SENSOR_VALUE) {
            temp6 += rawSbHeaterValues[i];
            j6++;
        }
    }
    if (j1 > 0) {
        tempSupply = int8_t(temp1 / j1 + 0.5);
    }
    if (j2 > 0) {
        tempReverse = int8_t(temp2 / j2 + 0.5);
    }
    if (j3 > 0) {
        tempTank = int8_t(temp3 / j3 + 0.5);
    }
    if (j4 > 0) {
        tempBoiler = int8_t(temp4 / j4 + 0.5);
    }
    if (j5 > 0) {
        tempMix = int8_t(temp5 / j5 + 0.5);
    }
    if (j6 > 0) {
        tempSbHeater = int8_t(temp6 / j6 + 0.5);
    }
}

int8_t getSensorValue(const uint8_t sensor) {
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
    }
    result = (result > 0) ? result + 0.5 : result - 0.5;
    return int8_t(result);
}

unsigned long getTimestamp() {
    // just for testing purpose. normally should be 0
//    unsigned long TIMESHIFT = 4294900000L;
    unsigned long TIMESHIFT = 0L;
    return millis() + TIMESHIFT;
}

unsigned long diffTimestamps(unsigned long hi, unsigned long lo) {
    if (hi < lo) {
        // overflow
        return (MAX_TIMESTAMP - lo) + hi;
    } else {
        return hi - lo;
    }
}

bool isInForcedMode(uint8_t bit, unsigned long ts) {
    if (NODE_FORCED_MODE_FLAGS & bit) { // forced mode
        if (ts == 0) {
            // permanent forced mode
            return true;
        }
        if (FORCED_MODE_OVERFLOW_TS_FLAGS & bit) {
            // before overflow. continue in forced mode
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

void switchNodeState(uint8_t id, uint8_t sensId[], int8_t sensVal[], uint8_t sensCnt) {
    uint8_t bit;
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
    } else if (NODE_BOILER == id) {
        bit = NODE_BOILER_BIT;
        ts = &tsNodeBoiler;
        tsf = &tsForcedNodeBoiler;
    } else if (NODE_SB_HEATER == id) {
        bit = NODE_SB_HEATER_BIT;
        ts = &tsNodeSbHeater;
        tsf = &tsForcedNodeSbHeater;
    }

    if (ts != NULL) {
        if (NODE_STATE_FLAGS & bit) {
            // switch OFF
            digitalWrite(id, LOW);
        } else {
            // switch ON
            digitalWrite(id, HIGH);
        }
        NODE_STATE_FLAGS = NODE_STATE_FLAGS ^ bit;

        // update node states in EEPROM
        EEPROM.write(NODE_STATE_EEPROM_ADDR, NODE_STATE_FLAGS);

        *ts = getTimestamp();
        if (*ts == 0) {
            *ts += 1;
        }
        char tsStr[12];
        char tsfStr[12];
        sprintf(tsStr, "%lu", *ts);
        sprintf(tsfStr, "%lu", *tsf);

        // report state change
        StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root[MSG_TYPE_KEY] = MSG_NODE_STATE_CHANGED;
        root[ID_KEY] = id;
        root[STATE_KEY] = NODE_STATE_FLAGS & bit ? 1 : 0;
        root[TIMESTAMP_KEY] = tsStr;
        root[FORCE_FLAG_KEY] = NODE_FORCED_MODE_FLAGS & bit ? 1 : 0;
        if (NODE_FORCED_MODE_FLAGS & bit) {
            root[FORCE_TIMESTAMP_KEY] = tsfStr;
        }

        JsonArray& sensors = root.createNestedArray(SENSORS_KEY);
        for (unsigned int i = 0; i < sensCnt; i++) {
            JsonObject& sens = jsonBuffer.createObject();
            sens[ID_KEY] = sensId[i];
            sens[VALUE_KEY] = sensVal[i];
            sensors.add(sens);
        }

        char json[JSON_MAX_WRITE_SIZE];
        root.printTo(json, JSON_MAX_WRITE_SIZE);

        broadcastMsg(json);
    }
}

void restoreNodesState() {
    NODE_PERMANENTLY_FORCED_MODE_FLAGS = EEPROM.read(NODE_FORCED_MODE_EEPROM_ADDR);
    NODE_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS;
    NODE_STATE_FLAGS = EEPROM.read(NODE_STATE_EEPROM_ADDR);
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
    } else if (NODE_BOILER == id) {
        forceNodeState(NODE_BOILER, NODE_BOILER_BIT, state, tsForcedNodeBoiler, ts);
        reportNodeStatus(NODE_BOILER, NODE_BOILER_BIT, tsNodeBoiler, tsForcedNodeBoiler);
    } else if (NODE_SB_HEATER == id) {
        forceNodeState(NODE_SB_HEATER, NODE_SB_HEATER_BIT, state, tsForcedNodeSbHeater, ts);
        reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);
    }
    // update node modes in EEPROM
    EEPROM.write(NODE_STATE_EEPROM_ADDR, NODE_STATE_FLAGS);
    EEPROM.write(NODE_FORCED_MODE_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
}

void forceNodeState(uint8_t id, uint8_t bit, uint8_t state, unsigned long &nodeTs, unsigned long ts) {
    NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS | bit;
    if (ts == 0) {
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS | bit;
    } else {
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~bit;
    }
    if (tsCurr > ts) {
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS | bit;
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
    if (NODE_SUPPLY == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SUPPLY_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SUPPLY_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_SUPPLY_BIT;
        reportNodeStatus(NODE_SUPPLY, NODE_SUPPLY_BIT, tsNodeSupply, tsForcedNodeSupply);
    } else if (NODE_HEATING == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_HEATING_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_HEATING_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_HEATING_BIT;
        reportNodeStatus(NODE_HEATING, NODE_HEATING_BIT, tsNodeHeating, tsForcedNodeHeating);
    } else if (NODE_FLOOR == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_FLOOR_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_FLOOR_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_FLOOR_BIT;
        reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
    } else if (NODE_HOTWATER == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_HOTWATER_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_HOTWATER_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_HOTWATER_BIT;
        reportNodeStatus(NODE_HOTWATER, NODE_HOTWATER_BIT, tsNodeHotwater, tsForcedNodeHotwater);
    } else if (NODE_CIRCULATION == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_CIRCULATION_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_CIRCULATION_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_CIRCULATION_BIT;
        reportNodeStatus(NODE_CIRCULATION, NODE_CIRCULATION_BIT, tsNodeCirculation, tsForcedNodeCirculation);
    } else if (NODE_BOILER == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_BOILER_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_BOILER_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_BOILER_BIT;
        reportNodeStatus(NODE_BOILER, NODE_BOILER_BIT, tsNodeBoiler, tsForcedNodeBoiler);
    } else if (NODE_SB_HEATER == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SB_HEATER_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SB_HEATER_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_SB_HEATER_BIT;
        reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);
    }
    // update node modes in EEPROM
    EEPROM.write(NODE_STATE_EEPROM_ADDR, NODE_STATE_FLAGS);
    EEPROM.write(NODE_FORCED_MODE_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
}

void loadWifiConfig() {
    EEPROM.readBlock(WIFI_AP_EEPROM_ADDR, WIFI_AP, sizeof(WIFI_AP));
    EEPROM.readBlock(WIFI_PW_EEPROM_ADDR, WIFI_PW, sizeof(WIFI_PW));
    EEPROM.readBlock(SERVER_IP_EEPROM_ADDR, SERVER_IP, sizeof(SERVER_IP));
    SERVER_PORT = EEPROM.readInt(SERVER_PORT_EEPROM_ADDR);
}

void wifiInit() {
    wifi->begin(115200);
    wifi->println(F("AT+CIOBAUD=9600"));
    wifi->begin(9600);
    wifi->setTimeout(300);
}

void wifiConnect() {
    if (strlen(WIFI_AP) + strlen(WIFI_PW) > 0) {
        char msg[128];
        wifi->println(F("AT+RST"));
        wifi->println(F("AT+CIPMODE=0"));
        wifi->println(F("AT+CWMODE=1"));
        sprintf(msg, "AT+CWJAP=\"%s\",\"%s\"", WIFI_AP, WIFI_PW);
#ifdef __DEBUG__
        Serial.print("wifi connecting: ");
        Serial.println(msg);
#endif
        wifi->println(msg);
        delay(5000);
    }
}

bool wifiGetIP() {
    wifi->println(F("AT+CIFSR"));
    String s = wifi->readStringUntil('\0');
#ifdef __DEBUG__
    Serial.print("wifi: ");
    Serial.println(s);
#endif
    char buff[JSON_MAX_READ_SIZE];
    s.toCharArray(buff, JSON_MAX_READ_SIZE, 0);
    int tmp[4];
    int n = sscanf(strstr(buff, ":STAIP,"), "%*[^,\"],\"%d.%d.%d.%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
    if (n == 4) {
        IP[0] = tmp[0];
        IP[1] = tmp[1];
        IP[2] = tmp[2];
        IP[3] = tmp[3];
        return validIP(IP);
    }
    return false;
}
bool validIP(uint8_t ip[4]) {
    int sum = ip[0] + ip[1] + ip[2] + ip[3];
    return sum > 0 && sum < 255 * 4;
}

bool wifiStartServer() {
    wifi->println(F("AT+CIPMUX=1"));
    if (!wifi->find(OK))
        return false;
    wifi->println(F("AT+CIPSERVER=1,80"));
    return wifi->find(OK);
}

bool wifiRsp(const char* body) {
    char send[32];
    sprintf(send, "AT+CIPSEND=%d,%d", CLIENT_ID, strlen(body));

    char close[16];
    sprintf(close, "AT+CIPCLOSE=%d", CLIENT_ID);

    if (!wifiWrite(send, ">"))
        return false;

    if (!wifiWrite(body, OK, 300, 1))
        return false;

    if (!wifiWrite(close, OK))
        return false;
    return true;
}

bool wifiRsp200() {
    return wifiRsp("HTTP/1.0 200 OK\r\n\r\n");
}

bool wifiRsp404() {
    return wifiRsp("HTTP/1.0 404 Not Found\r\n\r\n");
}

bool wifiRsp400() {
    return wifiRsp("HTTP/1.0 400 Bad Request\r\n\r\n");
}

bool wifiSend(const char* msg) {
    if (validIP(IP) && validIP(SERVER_IP)) {
        char body[HTTP_MAX_BODY_SIZE];
        sprintf(body,
                "POST / HTTP/1.1 \r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s",
                strlen(msg), msg);

        char connect[64];
        sprintf(connect, "AT+CIPSTART=3,\"TCP\",\"%d.%d.%d.%d\",%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2],
                SERVER_IP[3], SERVER_PORT);

        char send[32];
        sprintf(send, "AT+CIPSEND=3,%d", strlen(body));

        char close[16];
        sprintf(close, "AT+CIPCLOSE=3");
        if (!wifiWrite(connect, "CONNECT", 200, 1))
            return false;

        if (!wifiWrite(send, ">"))
            return false;

        if (!wifiWrite(body, OK, 200, 1))
            return false;

        wifiWrite(close, OK);

        return true;
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
    String s = wifi->readStringUntil('\0');
#ifdef __DEBUG__
    Serial.print("wifi req: ");
    Serial.println(s);
#endif
    char buff[JSON_MAX_READ_SIZE];
    s.toCharArray(buff, JSON_MAX_READ_SIZE, 0);
    int clientId;
    int n = sscanf(buff, "%*[^+IPD]+IPD,%d,%*d:GET%s", &clientId, req);
    if (n == 2) {
        CLIENT_ID = clientId;
    } else {
        req[0] = '\0';
    }
}

void reportStatus() {
    reportNodeStatus(NODE_SUPPLY, NODE_SUPPLY_BIT, tsNodeSupply, tsForcedNodeSupply);
    reportNodeStatus(NODE_HEATING, NODE_HEATING_BIT, tsNodeHeating, tsForcedNodeHeating);
    reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
    reportNodeStatus(NODE_HOTWATER, NODE_HOTWATER_BIT, tsNodeHotwater, tsForcedNodeHotwater);
    reportNodeStatus(NODE_CIRCULATION, NODE_CIRCULATION_BIT, tsNodeCirculation, tsForcedNodeCirculation);
    reportNodeStatus(NODE_BOILER, NODE_BOILER_BIT, tsNodeBoiler, tsForcedNodeBoiler);
    reportNodeStatus(NODE_SB_HEATER, NODE_SB_HEATER_BIT, tsNodeSbHeater, tsForcedNodeSbHeater);

    reportSensorStatus(SENSOR_SUPPLY, tempSupply);
    reportSensorStatus(SENSOR_REVERSE, tempReverse);
    reportSensorStatus(SENSOR_TANK, tempTank);
    reportSensorStatus(SENSOR_BOILER, tempBoiler);
    reportSensorStatus(SENSOR_MIX, tempMix);
    reportSensorStatus(SENSOR_SB_HEATER, tempSbHeater);
    reportSensorStatus(SENSOR_TEMP_ROOM_1, tempRoom1, tsLastSensorTempRoom1);
    reportSensorStatus(SENSOR_HUM_ROOM_1, humRoom1, tsLastSensorHumRoom1);

    tsLastStatusReport = tsCurr;
}

void reportNodeStatus(uint8_t id, uint8_t bit, unsigned long ts, unsigned long tsf) {
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

    JsonArray& nodes = root.createNestedArray(NODES_KEY);
    nodes.add(node);

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

#ifdef __DEBUG__
    Serial.print(getTimestamp());
    Serial.print(": ");
    Serial.print("free memory: ");
    Serial.println(freeMemory());
#endif
    broadcastMsg(json);
}

void reportSensorStatus(const uint8_t id, const int8_t value, const unsigned long ts) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CURRENT_STATUS_REPORT;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[VALUE_KEY] = value;
    if (ts != 0) {
        sens[TIMESTAMP_KEY] = ts;
    }

    JsonArray& sensors = root.createNestedArray(SENSORS_KEY);
    sensors.add(sens);

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

#ifdef __DEBUG__
    Serial.print(getTimestamp());
    Serial.print(": ");
    Serial.print("free memory: ");
    Serial.println(freeMemory());
#endif
    broadcastMsg(json);
}

void reportConfiguration() {
    reportSensorConfig(SENSOR_SUPPLY, SENSOR_SUPPLY_FACTOR);
    reportSensorConfig(SENSOR_REVERSE, SENSOR_REVERSE_FACTOR);
    reportSensorConfig(SENSOR_TANK, SENSOR_TANK_FACTOR);
    reportSensorConfig(SENSOR_BOILER, SENSOR_BOILER_FACTOR);
    reportSensorConfig(SENSOR_MIX, SENSOR_MIX_FACTOR);
    reportSensorConfig(SENSOR_SB_HEATER, SENSOR_SB_HEATER_FACTOR);
    reportStringConfig(WIFI_AP_KEY, WIFI_AP);
    reportStringConfig(WIFI_PASSWORD_KEY, WIFI_PW);
    char sIP[16];
    sprintf(sIP, "%d.%d.%d.%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2], SERVER_IP[3]);
    reportStringConfig(SERVER_IP_KEY, sIP);
    reportNumberConfig(SERVER_PORT_KEY, SERVER_PORT);
    char lIP[16];
    sprintf(lIP, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
    reportStringConfig(LOCAL_IP_KEY, lIP);
}

void reportSensorConfig(const uint8_t id, const double value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[CALIBRATION_FACTOR_KEY] = value;

    JsonArray& sensors = root.createNestedArray(SENSORS_KEY);
    sensors.add(sens);

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

    broadcastMsg(json);
}

void reportStringConfig(const char* key, const char* value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

    broadcastMsg(json);
}

void reportNumberConfig(const char* key, const int value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

    broadcastMsg(json);
}

void syncClocks() {
    char tsStr[12];
    sprintf(tsStr, "%lu", getTimestamp());

    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CLOCK_SYNC;
    root[TIMESTAMP_KEY] = tsStr;
    root[OVERFLOW_COUNT_KEY] = overflowCount;

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

    broadcastMsg(json);
}

void broadcastMsg(const char* msg) {
    Serial.println(msg);
    bt->println(msg);
    bool r = wifiSend(msg);
    if (r) {
#ifdef __DEBUG__
        Serial.println("wifi send: OK");
#endif
    } else {
#ifdef __DEBUG__
        Serial.println("wifi send: FAILED");
#endif
    }
}

void processRfMsg() {
    char buff[JSON_MAX_READ_SIZE];
    rfRead(buff);
    parseCommand(buff);
}

void processSerialMsg() {
    char buff[JSON_MAX_READ_SIZE];
    String s = Serial.readStringUntil('\0');
    s.toCharArray(buff, JSON_MAX_READ_SIZE, 0);
    parseCommand(buff);
}

void processBtMsg() {
    char buff[JSON_MAX_READ_SIZE];
    String s = bt->readStringUntil('\0');
    s.toCharArray(buff, JSON_MAX_READ_SIZE, 0);
    parseCommand(buff);
}

void processWifiReq() {
    char req[HTTP_MAX_GET_SIZE];
    wifiRead(req);
    if (strlen(req) > 0) {
        char cmd[4];
        int id, state;
        unsigned long ts;
        int n = sscanf(req, "/%3s/%d/%d/%ld", cmd, &id, &state, &ts);
        if (n > 0) {
            StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();
            if (strcmp(cmd, MSG_CLOCK_SYNC) == 0) {
                if (n == 1) {
                    wifiRsp200();
                    root[MSG_TYPE_KEY] = MSG_CLOCK_SYNC;
                } else {
                    wifiRsp400();
                }
            } else if (strcmp(cmd, MSG_CURRENT_STATUS_REPORT) == 0) {
                if (n == 1) {
                    wifiRsp200();
                    root[MSG_TYPE_KEY] = MSG_CURRENT_STATUS_REPORT;
                } else {
                    wifiRsp400();
                }
            } else if (strcmp(cmd, MSG_NODE_STATE_CHANGED) == 0) {
                if (n > 1) {
                    wifiRsp200();
                    root[MSG_TYPE_KEY] = MSG_NODE_STATE_CHANGED;
                    root[ID_KEY] = id;
                    if (n > 2) {
                        root[STATE_KEY] = state;
                    }
                    if (n > 3) {
                        char tss[12];
                        sprintf(tss, "%lu", ts);
                        root[FORCE_TIMESTAMP_KEY] = tss;
                    }
                } else {
                    wifiRsp400();
                }
            } else {
                wifiRsp404();
            }
            if (root.containsKey(MSG_TYPE_KEY)) {
                char json[JSON_MAX_WRITE_SIZE];
                root.printTo(json, JSON_MAX_WRITE_SIZE);
                parseCommand(json);
            }
        } else {
            wifiRsp404();
        }
    }
}

void parseCommand(char* command) {
#ifdef __DEBUG__
    Serial.print("parsing cmd: ");
    Serial.println(command);
#endif
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(command);
#ifdef __DEBUG__
    Serial.println("parsed");
    Serial.print("free memory: ");
    Serial.println(freeMemory());
#endif
    if (root.success()) {
        const char* msgType = root[MSG_TYPE_KEY];
        if (strcmp(msgType, MSG_CLOCK_SYNC) == 0) {
            // SYNC
            syncClocks();
        } else if (strcmp(msgType, MSG_CURRENT_STATUS_REPORT) == 0) {
            // CSR
            if (root.containsKey(SENSORS_KEY)) {
                JsonArray& sensors = root[SENSORS_KEY];
                for (unsigned char i = 0; i < sensors.size(); i++) {
                    uint8_t id = sensors[i][ID_KEY].as<uint8_t>();
                    uint8_t val = sensors[i][VALUE_KEY].as<uint8_t>();
                    // find sensor
                    if (id == SENSOR_TEMP_ROOM_1) {
                        tempRoom1 = val;
                        tsLastSensorTempRoom1 = tsCurr;
                    } else if (id == SENSOR_HUM_ROOM_1) {
                        humRoom1 = val;
                        tsLastSensorHumRoom1 = tsCurr;
                    } // else if(...)
                }
            } else {
                reportStatus();
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
                            if (ts == 0) {
                                ts++;
                            }
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
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 0, cf);
                        SENSOR_SUPPLY_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 0);
                    } else if (id == SENSOR_REVERSE) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4, cf);
                        SENSOR_REVERSE_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4);
                    } else if (id == SENSOR_TANK) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 8, cf);
                        SENSOR_TANK_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 8);
                    } else if (id == SENSOR_BOILER) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 12, cf);
                        SENSOR_BOILER_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 12);
                    } else if (id == SENSOR_MIX) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 16, cf);
                        SENSOR_MIX_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 16);
                    } else if (id == SENSOR_SB_HEATER) {
                        writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 20, cf);
                        SENSOR_SB_HEATER_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 20);
                    }
                }
            } else if (root.containsKey(WIFI_AP_KEY)) {
                sprintf(WIFI_AP, "%s", root[WIFI_AP_KEY].asString());
                EEPROM.writeBlock(WIFI_AP_EEPROM_ADDR, WIFI_AP, sizeof(WIFI_AP));
            } else if (root.containsKey(WIFI_PASSWORD_KEY)) {
                sprintf(WIFI_PW, "%s", root[WIFI_PASSWORD_KEY].asString());
                EEPROM.writeBlock(WIFI_PW_EEPROM_ADDR, WIFI_PW, sizeof(WIFI_PW));
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
    }
#ifdef __DEBUG__
    Serial.print("free memory: ");
    Serial.println(freeMemory());
#endif
}
