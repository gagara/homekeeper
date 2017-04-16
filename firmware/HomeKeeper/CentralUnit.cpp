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
static const uint8_t SENSOR_SOLAR_PRIMARY = 61;
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
static const DeviceAddress SENSOR_SOLAR_PRIMAY_ADDR = { 0x28, 0xFF, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61 };
static const DeviceAddress SENSOR_SOLAR_SECONDARY_ADDR = { 0x28, 0xFF, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62 };

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
static const uint8_t NODE_SOLAR_PRIMARY = 36;
static const uint8_t NODE_SOLAR_SECONDARY = 38;

// RF pins
static const uint8_t RF_CE_PIN = 9; //5
static const uint8_t RF_CSN_PIN = 10; //6

// WiFi pins
static const uint8_t WIFI_RST_PIN = 12;

static const uint8_t HEARTBEAT_LED = 13;

// Nodes State
static const uint16_t NODE_SUPPLY_BIT = 1;
static const uint16_t NODE_HEATING_BIT = 2;
static const uint16_t NODE_FLOOR_BIT = 4;
static const uint16_t NODE_HOTWATER_BIT = 8;
static const uint16_t NODE_CIRCULATION_BIT = 16;
static const uint16_t NODE_BOILER_BIT = 32;
static const uint16_t NODE_SB_HEATER_BIT = 64;
static const uint16_t NODE_SOLAR_PRIMARY_BIT = 128;
static const uint16_t NODE_SOLAR_SECONDARY_BIT = 256;

// etc
static const unsigned long MAX_TIMESTAMP = -1;
static const int8_t UNKNOWN_SENSOR_VALUE = -127;
static const uint8_t SENSORS_PRECISION = 9;
static const unsigned long NODE_SWITCH_SAFE_TIME_MSEC = 60000;

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
static const unsigned long PRIMARY_HEATER_SHORT_CIRCUIT_PERIOD_MSEC = 1200000; // 20 minutes

// heating
static const uint8_t HEATING_ON_TEMP_THRESHOLD = 40;
static const uint8_t HEATING_OFF_TEMP_THRESHOLD = 36;
static const uint8_t FLOOR_ON_TEMP_THRESHOLD = 35;
static const uint8_t FLOOR_OFF_TEMP_THRESHOLD = 26;
static const uint8_t PRIMARY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD = 20;
static const unsigned long HEATING_ROOM_1_MAX_VALIDITY_PERIOD = 1800000; // 30m

// Boiler heating
static const uint8_t TANK_BOILER_HIST = 3;
static const uint8_t BOILER_MIN_TEMP_THRESHOLD = 40;
static const uint8_t BOILER_MIN_TEMP_HIST = 3;
static const uint8_t BOILER_CRITICAL_MAX_TEMP_THRESHOLD = 80;
static const uint8_t BOILER_CRITICAL_MAX_TEMP_HIST = 5;

// Circulation
static const uint8_t CIRCULATION_TEMP_THRESHOLD = 37;
static const unsigned long CIRCULATION_ACTIVE_PERIOD_MSEC = 180000; // 3m
static const unsigned long CIRCULATION_PASSIVE_PERIOD_MSEC = 3420000; // 57m

// Standby Heater
static const uint8_t STANDBY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD = 10;

// Solar
static const uint8_t SOLAR_PRIMARY_MIN_TEMP_THRESHOLD = 30;
static const uint8_t SOLAR_PRIMARY_MIN_TEMP_HIST = 5;
static const uint8_t SOLAR_PRIMARY_CRITICAL_TEMP_THRESHOLD = 100; // stagnation
static const uint8_t SOLAR_PRIMARY_CRITICAL_TEMP_HIST = 10;
static const uint8_t SOLAR_SECONDARY_BOILER_HIST = 5;

// sensor BoilerPower
static const uint8_t SENSOR_BOILER_POWER_THERSHOLD = 100;

// reporting
static const unsigned long STATUS_REPORTING_PERIOD_MSEC = 5000;
static const unsigned long SENSORS_REFRESH_INTERVAL_MSEC = 5000;
static const unsigned long SENSORS_READ_INTERVAL_MSEC = 5000;

// WiFi
static const unsigned long WIFI_MAX_FAILURE_PERIOD_MSEC = 180000; // 3m

// RF
static const uint64_t RF_PIPE_BASE = 0xE8E8F0F0A2LL;
static const uint8_t RF_PACKET_LENGTH = 32;

// sensors stats
static const uint8_t SENSORS_RAW_VALUES_MAX_COUNT = 1; // disable
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

static const uint8_t RF_MAX_BODY_SIZE = 65;

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
int8_t tempSolarPrimary = UNKNOWN_SENSOR_VALUE;
int8_t tempSolarSecondary = UNKNOWN_SENSOR_VALUE;
int8_t STANDBY_HEATER_ROOM_TEMP_THRESHOLD = STANDBY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD;
int8_t PRIMARY_HEATER_ROOM_TEMP_THRESHOLD = PRIMARY_HEATER_ROOM_TEMP_DEFAULT_THRESHOLD;
int8_t sensorBoilerPowerState = 0;

// stats
int8_t rawSupplyValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawReverseValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawTankValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawBoilerValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawMixValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawSbHeaterValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawSolarPrimaryValues[SENSORS_RAW_VALUES_MAX_COUNT];
int8_t rawSolarSecondaryValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawSupplyIdx = 0;
uint8_t rawReverseIdx = 0;
uint8_t rawTankIdx = 0;
uint8_t rawBoilerIdx = 0;
uint8_t rawMixIdx = 0;
uint8_t rawSbHeaterIdx = 0;
uint8_t rawSolarPrimaryIdx = 0;
uint8_t rawSolarSecondaryIdx = 0;

//// Nodes

// Nodes actual state
uint16_t NODE_STATE_FLAGS = 0;
// Nodes forced mode
uint16_t NODE_FORCED_MODE_FLAGS = 0;
uint16_t FORCED_MODE_OVERFLOW_TS_FLAGS = 0;
// will be stored in EEPROM
uint16_t NODE_PERMANENTLY_FORCED_MODE_FLAGS = 0;

// Nodes switch timestamps
unsigned long tsNodeSupply = 0;
unsigned long tsNodeHeating = 0;
unsigned long tsNodeFloor = 0;
unsigned long tsNodeHotwater = 0;
unsigned long tsNodeCirculation = 0;
unsigned long tsNodeBoiler = 0;
unsigned long tsNodeSbHeater = 0;
unsigned long tsNodeSolarPrimary = 0;
unsigned long tsNodeSolarSecondary = 0;

// Nodes forced mode timestamps
unsigned long tsForcedNodeSupply = 0;
unsigned long tsForcedNodeHeating = 0;
unsigned long tsForcedNodeFloor = 0;
unsigned long tsForcedNodeHotwater = 0;
unsigned long tsForcedNodeCirculation = 0;
unsigned long tsForcedNodeBoiler = 0;
unsigned long tsForcedNodeSbHeater = 0;
unsigned long tsForcedNodeSolarPrimary = 0;
unsigned long tsForcedNodeSolarSecondary = 0;

// Sensors switch timestamps
unsigned long tsSensorBoilerPower = 0;

// reporting
uint8_t nextEntryReport = 0;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorsRefresh = 0;
unsigned long tsLastSensorsRead = 0;

unsigned long tsLastSensorTempRoom1 = 0;
unsigned long tsLastSensorHumRoom1 = 0;

unsigned long tsLastWifiSuccessTransmission = 0;

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

// RF24
RF24 radio(RF_CE_PIN, RF_CSN_PIN);

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
    Serial.print(F("free memory: "));
    Serial.println(freeMemory());
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
    pinMode(NODE_BOILER, OUTPUT);
    pinMode(NODE_SB_HEATER, OUTPUT);
    pinMode(NODE_SOLAR_PRIMARY, OUTPUT);
    pinMode(NODE_SOLAR_SECONDARY, OUTPUT);

    // restore nodes state
    digitalWrite(NODE_SUPPLY, ~NODE_STATE_FLAGS & NODE_SUPPLY_BIT);
    digitalWrite(NODE_HEATING, ~NODE_STATE_FLAGS & NODE_HEATING_BIT);
    digitalWrite(NODE_FLOOR, ~NODE_STATE_FLAGS & NODE_FLOOR_BIT);
    digitalWrite(NODE_HOTWATER, ~NODE_STATE_FLAGS & NODE_HOTWATER_BIT);
    digitalWrite(NODE_CIRCULATION, ~NODE_STATE_FLAGS & NODE_CIRCULATION_BIT);
    digitalWrite(NODE_BOILER, ~NODE_STATE_FLAGS & NODE_BOILER_BIT);
    digitalWrite(NODE_SB_HEATER, ~NODE_STATE_FLAGS & NODE_SB_HEATER_BIT);
    digitalWrite(NODE_SOLAR_PRIMARY, ~NODE_STATE_FLAGS & NODE_SOLAR_PRIMARY_BIT);
    digitalWrite(NODE_SOLAR_SECONDARY, ~NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT);

    // init sensors raw values arrays
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        rawSupplyValues[i] = UNKNOWN_SENSOR_VALUE;
        rawReverseValues[i] = UNKNOWN_SENSOR_VALUE;
        rawTankValues[i] = UNKNOWN_SENSOR_VALUE;
        rawBoilerValues[i] = UNKNOWN_SENSOR_VALUE;
        rawMixValues[i] = UNKNOWN_SENSOR_VALUE;
        rawSbHeaterValues[i] = UNKNOWN_SENSOR_VALUE;
        rawSolarPrimaryValues[i] = UNKNOWN_SENSOR_VALUE;
        rawSolarSecondaryValues[i] = UNKNOWN_SENSOR_VALUE;
    }

    // read sensors calibration factors from EEPROM
    loadSensorsCalibrationFactors();

    // restore sensor thresholds
    STANDBY_HEATER_ROOM_TEMP_THRESHOLD = EEPROM.readByte(STANDBY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR);
    PRIMARY_HEATER_ROOM_TEMP_THRESHOLD = EEPROM.readByte(PRIMARY_HEATER_ROOM_TEMP_THRESHOLD_EEPROM_ADDR);

    // init RF24 radio. not used for now
    radio.begin();
    radio.enableDynamicPayloads();
    // listen on 2 channels
    for (int i = 1; i <= 2; i++) {
        radio.openReadingPipe(i, RF_PIPE_BASE + i);
    }
    radio.startListening();
    radio.powerUp();
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
        Serial.print(F("free memory: "));
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
        // solar primary
        processSolarPrimary();
        // solar secondary
        processSolarSecondary();
        // boiler heater
        processBoilerHeater();
        // standby heater
        processStandbyHeater();

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

    reportStatus();

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
    if (diffTimestamps(tsCurr, tsNodeSupply) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_SUPPLY, SENSOR_REVERSE };
        int8_t sensVals[] = { tempSupply, tempReverse };
        if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
            // pump is ON
            if (tempSupply <= tempReverse) {
                // temp is equal
                if (tempSupply <= PRIMARY_HEATER_SHORT_CIRCUIT_THRESHOLD_TEMP) {
                    // primary heater is on short circuit
                    if (diffTimestamps(tsCurr, tsNodeSupply) < PRIMARY_HEATER_SHORT_CIRCUIT_PERIOD_MSEC) {
                        // pump active period is too short. give it some time
                        // do nothing
                    } else {
                        // pump active period is long enough
                        // turn pump OFF
                        switchNodeState(NODE_SUPPLY, sensIds, sensVals, 2);
                    }
                } else {
                    // primary heater is in production mode
                    // turn pump OFF
                    switchNodeState(NODE_SUPPLY, sensIds, sensVals, 2);
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
                switchNodeState(NODE_SUPPLY, sensIds, sensVals, 2);
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
    if (diffTimestamps(tsCurr, tsNodeHeating) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_MIX, SENSOR_SB_HEATER };
        int8_t sensVals[] = { tempTank, tempMix, tempSbHeater };
        if (NODE_STATE_FLAGS & NODE_HEATING_BIT) {
            // pump is ON
            if (tempTank < HEATING_OFF_TEMP_THRESHOLD && tempMix < HEATING_OFF_TEMP_THRESHOLD
                    && tempSbHeater < HEATING_OFF_TEMP_THRESHOLD) {
                // temp in (tank && mix && sb_heater) is too low
                if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
                    // supply is on
                    // do nothing
                } else {
                    // supply is off
                    // turn pump OFF
                    switchNodeState(NODE_HEATING, sensIds, sensVals, 3);
                }
            } else {
                // temp is high enough at least in one source
                if (room1TempSatisfyMaxThreshold() || (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT)) {
                    // temp in Room1 is high enough OR standby heater is on
                    // turn pump OFF
                    switchNodeState(NODE_HEATING, sensIds, sensVals, 3);
                } else {
                    // temp in Room1 is too low AND standby heater is off
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
                    switchNodeState(NODE_HEATING, sensIds, sensVals, 3);
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
    if (diffTimestamps(tsCurr, tsNodeFloor) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_MIX, SENSOR_SB_HEATER };
        int8_t sensVals[] = { tempTank, tempMix, tempSbHeater };
        if (NODE_STATE_FLAGS & NODE_FLOOR_BIT) {
            // pump is ON
            if (tempTank < FLOOR_OFF_TEMP_THRESHOLD && tempMix < FLOOR_OFF_TEMP_THRESHOLD
                    && tempSbHeater < FLOOR_OFF_TEMP_THRESHOLD) {
                // temp in (tank && mix && sb_heater) is too low
                if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
                    // standby heater is on
                    if (tempSbHeater <= tempMix && tempSbHeater <= tempTank) {
                        // temp in standby heater <= (temp in Mix && Tank)
                        // turn pump OFF
                        switchNodeState(NODE_FLOOR, sensIds, sensVals, 3);
                    } else {
                        // temp in standby heater > (temp in Mix || Tank)
                        // do nothing
                    }
                } else {
                    // standby heater is off
                    // turn pump OFF
                    switchNodeState(NODE_FLOOR, sensIds, sensVals, 3);
                }
            } else {
                // temp is high enough at least in one source
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempTank >= FLOOR_ON_TEMP_THRESHOLD || tempMix >= FLOOR_ON_TEMP_THRESHOLD
                    || tempSbHeater >= FLOOR_ON_TEMP_THRESHOLD) {
                // temp in (tank || mix || sb_heater) is high enough
                // turn pump ON
                switchNodeState(NODE_FLOOR, sensIds, sensVals, 3);
            } else {
                // temp in (tank && mix && sb_heater) is too low
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
    if (diffTimestamps(tsCurr, tsNodeHotwater) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_BOILER };
        int8_t sensVals[] = { tempTank, tempBoiler };
        if (NODE_STATE_FLAGS & NODE_HOTWATER_BIT) {
            // pump is ON
            if (tempBoiler < (BOILER_CRITICAL_MAX_TEMP_THRESHOLD - BOILER_CRITICAL_MAX_TEMP_HIST)) {
                // temp in boiler is normal
                if (tempTank <= tempBoiler) {
                    // temp in tank is low
                    // turn pump OFF
                    switchNodeState(NODE_HOTWATER, sensIds, sensVals, 2);
                } else {
                    // temp in tank is high enough
                    // do nothing
                }
            } else {
                // temp in Boiler critically high
                if (tempTank < tempBoiler) {
                    // tank has capacity
                    // do nothing
                } else {
                    // tank has no capacity
                    // turn pump ON
                    switchNodeState(NODE_HOTWATER, sensIds, sensVals, 2);
                }
            }
        } else {
            // pump is OFF
            if (tempBoiler >= BOILER_CRITICAL_MAX_TEMP_THRESHOLD) {
                // temp in boiler critically high
                if (tempTank < tempBoiler) {
                    // tank has capacity
                    // turn pump ON
                    switchNodeState(NODE_HOTWATER, sensIds, sensVals, 2);
                } else {
                    // tank has no capacity
                    // do nothing
                }
            } else {
                // temp in boiler is normal
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
}

void processCirculationCircuit() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_CIRCULATION_BIT;
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

void processSolarPrimary() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_SOLAR_PRIMARY_BIT;
    if (isInForcedMode(NODE_SOLAR_PRIMARY_BIT, tsForcedNodeSolarPrimary)) {
        return;
    }
    if (wasForceMode) {
        reportNodeStatus(NODE_SOLAR_PRIMARY, NODE_SOLAR_PRIMARY_BIT, tsNodeSolarPrimary, tsForcedNodeSolarPrimary);
    }
    if (diffTimestamps(tsCurr, tsNodeSolarPrimary) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_SOLAR_PRIMARY };
        int8_t sensVals[] = { tempSolarPrimary };
        if (NODE_STATE_FLAGS & NODE_SOLAR_PRIMARY_BIT) {
            // solar primary is ON
            if (tempSolarPrimary >= SOLAR_PRIMARY_CRITICAL_TEMP_THRESHOLD) {
                // temp in solar primary is critically high. stagnation
                // turn solar primary OFF
                switchNodeState(NODE_SOLAR_PRIMARY, sensIds, sensVals, 1);
            } else {
                // temp in solar primary is normal
                if (tempSolarPrimary < (SOLAR_PRIMARY_MIN_TEMP_THRESHOLD - SOLAR_PRIMARY_MIN_TEMP_HIST)) {
                    // temp in solar primary is too low
                    // turn solar primary OFF
                    switchNodeState(NODE_SOLAR_PRIMARY, sensIds, sensVals, 1);
                } else {
                    // temp in solar primary is high enough
                    // do nothing
                }
            }
        } else {
            // solar primary is OFF
            if (tempSolarPrimary < (SOLAR_PRIMARY_CRITICAL_TEMP_THRESHOLD - SOLAR_PRIMARY_CRITICAL_TEMP_HIST)) {
                // temp in solar primary is normal
                if (tempSolarPrimary >= SOLAR_PRIMARY_MIN_TEMP_THRESHOLD) {
                    // temp in solar primary is high enough
                    // turn solar primary ON
                    switchNodeState(NODE_SOLAR_PRIMARY, sensIds, sensVals, 1);
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
    if (diffTimestamps(tsCurr, tsNodeSolarSecondary) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_SOLAR_SECONDARY, SENSOR_BOILER };
        int8_t sensVals[] = { tempSolarSecondary, tempBoiler };
        if (NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) {
            // solar secondary is ON
            if (tempSolarSecondary <= tempBoiler) {
                // temp in solar secondary is too low
                // turn solar secondary OFF
                switchNodeState(NODE_SOLAR_SECONDARY, sensIds, sensVals, 2);
            } else {
                // temp in solar secondary is high enough
                // do nothing
            }
        } else {
            // solar secondary is OFF
            if (tempSolarSecondary > (tempBoiler + SOLAR_SECONDARY_BOILER_HIST)) {
                // temp in solar secondary is high enough
                // turn solar secondary ON
                switchNodeState(NODE_SOLAR_SECONDARY, sensIds, sensVals, 2);
            } else {
                // temp in solar secondary is too low
                // do nothing
            }
        }
    }
}

void processBoilerHeater() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_BOILER_BIT;
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
                if (NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) {
                    // solar circuit ON
                    if (tempBoiler >= (BOILER_MIN_TEMP_THRESHOLD + BOILER_MIN_TEMP_HIST)) {
                        // temp in boiler high enough
                        // turn boiler OFF
                        switchNodeState(NODE_BOILER, sensIds, sensVals, 1);
                    } else {
                        // temp in boiler is too low
                        // do nothing
                    }
                } else {
                    // solar circuit OFF
                    // do nothing
                }
            }
        } else {
            // boiler is OFF
            if (NODE_STATE_FLAGS & NODE_HOTWATER_BIT) {
                // hotwater circuit is ON
                // do nothing
            } else {
                // hotwater circuit is OFF
                if (NODE_STATE_FLAGS & NODE_SOLAR_SECONDARY_BIT) {
                    // solar circuit ON
                    if (tempBoiler < BOILER_MIN_TEMP_THRESHOLD) {
                        // temp in boiler is too low
                        // turn boiler ON
                        switchNodeState(NODE_BOILER, sensIds, sensVals, 1);
                    } else {
                        // temp in boiler is normal
                        // do nothing
                    }
                } else {
                    // solar circuit OFF
                    // turn boiler ON
                    switchNodeState(NODE_BOILER, sensIds, sensVals, 1);
                }
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
    if (diffTimestamps(tsCurr, tsNodeSbHeater) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_TEMP_ROOM_1 };
        int8_t sensVals[] = { tempTank, tempRoom1 };
        if (NODE_STATE_FLAGS & NODE_SB_HEATER_BIT) {
            // heater is ON
            if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
                // primary heater is on
                // turn standby heater OFF
                switchNodeState(NODE_SB_HEATER, sensIds, sensVals, 2);
            } else {
                // primary heater is off
                if (tempTank > STANDBY_HEATER_ROOM_TEMP_THRESHOLD && room1TempReachedMinThreshold()) {
                    // temp in (tank && room1) is high enough
                    // turn heater OFF
                    switchNodeState(NODE_SB_HEATER, sensIds, sensVals, 2);
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
                if (tempTank < STANDBY_HEATER_ROOM_TEMP_THRESHOLD || room1TempFailedMinThreshold()) {
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
}

/* ============ Helper methods ============ */

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
    readSensor(SENSOR_SOLAR_PRIMARY, rawSolarPrimaryValues, rawSolarPrimaryIdx);
    readSensor(SENSOR_SOLAR_SECONDARY, rawSolarSecondaryValues, rawSolarSecondaryIdx);
    // read remote sensors
    if (radio.available()) {
#ifdef __DEBUG__
        Serial.println(F("got RF message"));
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
    double temp7 = 0;
    double temp8 = 0;
    int j1 = 0;
    int j2 = 0;
    int j3 = 0;
    int j4 = 0;
    int j5 = 0;
    int j6 = 0;
    int j7 = 0;
    int j8 = 0;
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
        if (rawSolarPrimaryValues[i] != UNKNOWN_SENSOR_VALUE) {
            temp7 += rawSolarPrimaryValues[i];
            j7++;
        }
        if (rawSolarSecondaryValues[i] != UNKNOWN_SENSOR_VALUE) {
            temp8 += rawSolarSecondaryValues[i];
            j8++;
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
    if (j7 > 0) {
        tempSolarPrimary = int8_t(temp7 / j7 + 0.5);
    }
    if (j8 > 0) {
        tempSolarSecondary = int8_t(temp8 / j8 + 0.5);
    }

    // read sensorBoilerPower value
    int8_t state = getSensorBoilerPowerState();
    if (state != sensorBoilerPowerState) {
        // state changed
        sensorBoilerPowerState = state;
        tsSensorBoilerPower = tsCurr;
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
    } else if (SENSOR_SOLAR_PRIMARY == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_SOLAR_PRIMAY_ADDR)) {
            result = sensors.getTempC(SENSOR_SOLAR_PRIMAY_ADDR) * SENSOR_SOLAR_PRIMARY_FACTOR;
        }
    } else if (SENSOR_SOLAR_SECONDARY == sensor) {
        if (sensors.requestTemperaturesByAddress(SENSOR_SOLAR_SECONDARY_ADDR)) {
            result = sensors.getTempC(SENSOR_SOLAR_SECONDARY_ADDR) * SENSOR_SOLAR_SECONDARY_FACTOR;
        }
    }
    result = (result > 0) ? result + 0.5 : result - 0.5;
    return int8_t(result);
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

bool isInForcedMode(uint16_t bit, unsigned long ts) {
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
            digitalWrite(id, HIGH);
        } else {
            // switch ON
            digitalWrite(id, LOW);
        }
        NODE_STATE_FLAGS = NODE_STATE_FLAGS ^ bit;

        *ts = getTimestamp();
        if (*ts == 0) {
            *ts += 1;
        }

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
    } else if (NODE_BOILER == id) {
        forceNodeState(NODE_BOILER, NODE_BOILER_BIT, state, tsForcedNodeBoiler, ts);
        reportNodeStatus(NODE_BOILER, NODE_BOILER_BIT, tsNodeBoiler, tsForcedNodeBoiler);
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
    }
    // update node modes in EEPROM
    EEPROM.writeInt(NODE_STATE_EEPROM_ADDR, NODE_STATE_FLAGS);
    EEPROM.writeInt(NODE_FORCED_MODE_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
}

void forceNodeState(uint8_t id, uint16_t bit, uint8_t state, unsigned long &nodeTs, unsigned long ts) {
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
    } else if (NODE_SOLAR_PRIMARY == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SOLAR_PRIMARY_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SOLAR_PRIMARY_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_SOLAR_PRIMARY_BIT;
        reportNodeStatus(NODE_SOLAR_PRIMARY, NODE_SOLAR_PRIMARY_BIT, tsNodeSolarPrimary, tsForcedNodeSolarPrimary);
    } else if (NODE_SOLAR_SECONDARY == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SOLAR_SECONDARY_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SOLAR_SECONDARY_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_SOLAR_SECONDARY_BIT;
        reportNodeStatus(NODE_SOLAR_SECONDARY, NODE_SOLAR_SECONDARY_BIT, tsNodeSolarSecondary,
                tsForcedNodeSolarSecondary);
    }
    // update node modes in EEPROM
    EEPROM.writeInt(NODE_FORCED_MODE_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
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
                        && diffTimestamps(tsCurr, tsLastWifiSuccessTransmission) >= WIFI_MAX_FAILURE_PERIOD_MSEC)) {
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
    Serial.print(F("free memory: "));
    Serial.println(freeMemory());
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
    if (diffTimestamps(tsCurr, tsLastStatusReport) >= STATUS_REPORTING_PERIOD_MSEC) {
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
            nextEntryReport = NODE_BOILER;
            break;
        case NODE_BOILER:
            reportNodeStatus(NODE_BOILER, NODE_BOILER_BIT, tsNodeBoiler, tsForcedNodeBoiler);
            nextEntryReport = NODE_SOLAR_PRIMARY;
            break;
        case NODE_SOLAR_PRIMARY:
            reportNodeStatus(NODE_SOLAR_PRIMARY, NODE_SOLAR_PRIMARY_BIT, tsNodeSolarPrimary, tsForcedNodeSolarPrimary);
            nextEntryReport = NODE_SOLAR_SECONDARY;
            break;
        case NODE_SOLAR_SECONDARY:
            reportNodeStatus(NODE_SOLAR_SECONDARY, NODE_SOLAR_SECONDARY_BIT, tsNodeSolarSecondary,
                    tsForcedNodeSolarSecondary);
            nextEntryReport = 0;
            break;
        default:
            break;
        }
        tsLastStatusReport = tsCurr;
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
    Serial.print(getTimestamp());
    Serial.print(F(": "));
    Serial.print(F("free memory: "));
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

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

#ifdef __DEBUG__
    Serial.print(getTimestamp());
    Serial.print(F(": "));
    Serial.print(F("free memory: "));
    Serial.println(freeMemory());
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
    root[OVERFLOW_COUNT_KEY] = overflowCount;

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

void processRfMsg() {
    char buff[JSON_MAX_SIZE + 1];
    rfRead(buff);
    parseCommand(buff);
}

void processSerialMsg() {
    char buff[JSON_MAX_SIZE + 1];
    uint16_t l = Serial.readBytesUntil('\0', buff, JSON_MAX_SIZE);
    buff[l] = '\0';
    parseCommand(buff);
}

void processBtMsg() {
    char buff[JSON_MAX_SIZE + 1];
    uint16_t l = bt->readBytesUntil('\0', buff, JSON_MAX_SIZE);
    buff[l] = '\0';
    parseCommand(buff);
}

void processWifiReq() {
    char buff[JSON_MAX_SIZE + 1];
    wifiRead(buff);
    if (parseCommand(buff)) {
        wifiRsp200();
    } else {
        wifiRsp400();
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
    Serial.print(F("free memory: "));
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
                tsLastStatusReport = tsCurr - STATUS_REPORTING_PERIOD_MSEC;
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
    Serial.print(F("free memory: "));
    Serial.println(freeMemory());
#endif
    return true;
}
