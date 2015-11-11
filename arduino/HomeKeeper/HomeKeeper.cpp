#include "HomeKeeper.h"

#include <aJSON.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <HardwareSerial.h>
#include <pins_arduino.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WString.h>

/* ========= Configuration ========= */

#undef __DEBUG__

// Sensors pins
static const uint8_t SENSOR_SUPPLY = A0;
static const uint8_t SENSOR_REVERSE = A1;
static const uint8_t SENSOR_TANK = A2;
static const uint8_t SENSOR_BOILER = A3;

// Nodes pins
static const uint8_t NODE_SUPPLY = 6;
static const uint8_t NODE_HEATING = 7;
static const uint8_t NODE_FLOOR = 8;
static const uint8_t NODE_HOTWATER = 9;
static const uint8_t NODE_CIRCULATION = 10;
static const uint8_t NODE_BOILER = 11;

static const uint8_t HEARTBEAT_LED = 13;

// Nodes State
static const uint8_t NODE_SUPPLY_BIT = 1;
static const uint8_t NODE_HEATING_BIT = 2;
static const uint8_t NODE_FLOOR_BIT = 4;
static const uint8_t NODE_HOTWATER_BIT = 8;
static const uint8_t NODE_CIRCULATION_BIT = 16;
static const uint8_t NODE_BOILER_BIT = 32;

// etc
static const unsigned long MAX_TIMESTAMP = -1;
static const uint8_t MAX_SENSOR_VALUE = 255;
static const uint8_t SENSORS_APPROX = 1;
static const unsigned long NODE_SWITCH_SAFE_TIME_MSEC = 60000;
static const int SAVED_STATE_EEPROM_ADDR = 0;

// Heater
static const uint8_t HEATER_VALVE_THRESHOLD = 55;
static const uint8_t HEATER_SUPPLY_REVERSE_THRESHOLD = 3;
static const unsigned long HEATER_SHORT_CIRCUIT_PUMP_PERIOD_SEC = 180;

// heating
static const uint8_t HEATING_TEMP_THRESHOLD = 37;

// Boiler heating
static const uint8_t TANK_BOILER_HIST = 3;

// Circulation
static const uint8_t CIRCULATION_TEMP_THRESHOLD = 40;
static const unsigned long CIRCULATION_ACTIVE_PERIOD_MSEC = 600000;
static const unsigned long CIRCULATION_PASSIVE_PERIOD_MSEC = 1200000;

// reporting
static const unsigned long STATUS_REPORTING_PERIOD_MSEC = 60000;
static const unsigned long SENSORS_REFRESH_INTERVAL_MSEC = 10000;
static const unsigned long SENSORS_READ_INTERVAL_MSEC = 1000;

// sensors stats
static const uint8_t SENSORS_RAW_VALUES_MAX_COUNT = 20;
static const uint8_t SENSOR_NOISE_THRESHOLD = 50;

// sensor correlation numbers
static const double SENSOR_SUPPLY_FACTOR = 1.25;
static const double SENSOR_REVERSE_FACTOR = 1;
static const double SENSOR_TANK_FACTOR = 1;
static const double SENSOR_BOILER_FACTOR = 1;

// JSON
static const char MSG_TYPE_KEY[] = "m";
static const char ID_KEY[] = "id";
static const char STATE_KEY[] = "ns";
static const char FORCE_FLAG_KEY[] = "ff";
static const char TIMESTAMP_KEY[] = "ts";
static const char FORCE_TIMESTAMP_KEY[] = "ft";
static const char OVERFLOW_COUNT_KEY[] = "oc";
static const char NODES_KEY[] = "n";
static const char SENSORS_KEY[] = "s";
static const char VALUE_KEY[] = "v";

const char MSG_CURRENT_STATUS_REPORT[] = "csr";
const char MSG_NODE_STATE_CHANGED[] = "nsc";
const char MSG_CLOCK_SYNC[] = "cls";

static const uint8_t JSON_MAX_WRITE_SIZE = 255;
static const uint8_t JSON_MAX_READ_SIZE = 255;

/* ===== End of Configuration ====== */

/* =========== Variables =========== */

//// Sensors
// Values
uint8_t tempSupply = 0;
uint8_t tempReverse = 0;
uint8_t tempTank = 0;
uint8_t tempBoiler = 0;

// stats
uint8_t rawSupplyValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawReverseValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawTankValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawBoilerValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawSupplyIdx = 0;
uint8_t rawReverseIdx = 0;
uint8_t rawTankIdx = 0;
uint8_t rawBoilerIdx = 0;

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

// Nodes forced mode timestamps
unsigned long tsForcedNodeSupply = 0;
unsigned long tsForcedNodeHeating = 0;
unsigned long tsForcedNodeFloor = 0;
unsigned long tsForcedNodeHotwater = 0;
unsigned long tsForcedNodeCirculation = 0;
unsigned long tsForcedNodeBoiler = 0;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorsRefresh = 0;
unsigned long tsLastSensorsRead = 0;

unsigned long tsPrev = 0;
unsigned long tsCurr = 0;

uint8_t overflowCount = 0;
/* ======== End of Variables ======= */

void setup() {
    // Setup serial port
    Serial.begin(9600);
#ifdef __DEBUG__
    Serial.println("STARTING");
#endif
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // sync clock
    syncClocks();

    // Turn on all pumps and heater (default relay state)
    pinMode(NODE_SUPPLY, OUTPUT);
    digitalWrite(NODE_SUPPLY, HIGH);

    pinMode(NODE_HEATING, OUTPUT);
    digitalWrite(NODE_HEATING, HIGH);

    pinMode(NODE_HOTWATER, OUTPUT);
    digitalWrite(NODE_HOTWATER, HIGH);

    pinMode(NODE_CIRCULATION, OUTPUT);
    digitalWrite(NODE_CIRCULATION, HIGH);

    pinMode(NODE_BOILER, OUTPUT);
    digitalWrite(NODE_BOILER, HIGH);

    // init sensors raw values arrays
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        rawSupplyValues[i] = MAX_SENSOR_VALUE;
        rawReverseValues[i] = MAX_SENSOR_VALUE;
        rawTankValues[i] = MAX_SENSOR_VALUE;
        rawBoilerValues[i] = MAX_SENSOR_VALUE;
    }

    // read forced node state flags from EEPROM
    NODE_PERMANENTLY_FORCED_MODE_FLAGS = EEPROM.read(SAVED_STATE_EEPROM_ADDR);
    NODE_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS;

    // report current state
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

        if (diffTimestamps(tsCurr, tsLastStatusReport) >= STATUS_REPORTING_PERIOD_MSEC) {
            reportStatus();
        }
        delay(100);
        digitalWrite(HEARTBEAT_LED, LOW);
        tsLastSensorsRefresh = tsCurr;
    }
    if (Serial.available() > 0) {
        processIncomingCmd();
    }
    tsPrev = tsCurr;
}

/* ============ Helper methods ============ */
void readSensors() {
    readSensor(SENSOR_SUPPLY, rawSupplyValues, rawSupplyIdx);
    readSensor(SENSOR_REVERSE, rawReverseValues, rawReverseIdx);
    readSensor(SENSOR_TANK, rawTankValues, rawTankIdx);
    readSensor(SENSOR_BOILER, rawBoilerValues, rawBoilerIdx);
}

void readSensor(uint8_t id, uint8_t* const &values, uint8_t &idx) {
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
    val = getTemp(id);
    if (values[prevIdx] == MAX_SENSOR_VALUE || abs(val - values[prevIdx]) < SENSOR_NOISE_THRESHOLD) {
        values[idx] = val;
    }
    idx++;
}

void refreshSensorValues() {
    double temp1 = 0;
    double temp2 = 0;
    double temp3 = 0;
    double temp4 = 0;
    int j1 = 0;
    int j2 = 0;
    int j3 = 0;
    int j4 = 0;
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        if (rawSupplyValues[i] != MAX_SENSOR_VALUE) {
            temp1 += rawSupplyValues[i];
            j1++;
        }
        if (rawReverseValues[i] != MAX_SENSOR_VALUE) {
            temp2 += rawReverseValues[i];
            j2++;
        }
        if (rawTankValues[i] != MAX_SENSOR_VALUE) {
            temp3 += rawTankValues[i];
            j3++;
        }
        if (rawBoilerValues[i] != MAX_SENSOR_VALUE) {
            temp4 += rawBoilerValues[i];
            j4++;
        }
    }
    if (j1 > 0) {
        tempSupply = byte(temp1 / j1 + 0.5);
    }
    if (j2 > 0) {
        tempReverse = byte(temp2 / j2 + 0.5);
    }
    if (j3 > 0) {
        tempTank = byte(temp3 / j3 + 0.5);
    }
    if (j4 > 0) {
        tempBoiler = byte(temp4 / j4 + 0.5);
    }
}

uint8_t getTemp(uint8_t sensor) {
    int rawVal = analogRead(sensor);
    double voltage = (5000 / 1024) * rawVal;
    double tempC = voltage * 0.1;
    if (SENSOR_SUPPLY == sensor) {
        tempC = tempC * SENSOR_SUPPLY_FACTOR;
    } else if (SENSOR_REVERSE == sensor) {
        tempC = tempC * SENSOR_REVERSE_FACTOR;
    } else if (SENSOR_TANK == sensor) {
        tempC = tempC * SENSOR_TANK_FACTOR;
    } else if (SENSOR_BOILER == sensor) {
        tempC = tempC * SENSOR_BOILER_FACTOR;
    }
    return byte(tempC + 0.5);
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

void processSupplyCircuit() {
    if (isInForcedMode(NODE_SUPPLY_BIT, tsForcedNodeSupply)) {
        return;
    }
    if (diffTimestamps(tsCurr, tsNodeSupply) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_SUPPLY, SENSOR_REVERSE };
        uint8_t sensVals[] = { tempSupply, tempReverse };
        if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
            // pump is ON
            if (tempSupply <= (tempReverse + SENSORS_APPROX)) {
                // temp is equal
                if (tempSupply < HEATER_VALVE_THRESHOLD) {
                    // pump is on short circuit
                    if (diffTimestamps(tsCurr, tsNodeSupply) > HEATER_SHORT_CIRCUIT_PUMP_PERIOD_SEC) {
                        // turn pump OFF
                        switchNodeState(NODE_SUPPLY, sensIds, sensVals, 2);
                    } else {
                        // pump is just switched ON on short circuit. give it some time
                        // do nothing
                    }
                } else {
                    // pump is in production
                    // turn pump OFF
                    switchNodeState(NODE_SUPPLY, sensIds, sensVals, 2);
                }
            } else {
                // delta is big enough
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempSupply >= (tempReverse + HEATER_SUPPLY_REVERSE_THRESHOLD)) {
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
    if (isInForcedMode(NODE_HEATING_BIT, tsForcedNodeHeating)) {
        return;
    }
    if (diffTimestamps(tsCurr, tsNodeHeating) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK };
        uint8_t sensVals[] = { tempTank };
        if (NODE_STATE_FLAGS & NODE_HEATING_BIT) {
            // pump is ON
            if (tempTank < HEATING_TEMP_THRESHOLD) {
                // temp in tank is too low
                // turn pump OFF
                switchNodeState(NODE_HEATING, sensIds, sensVals, 1);
            } else {
                // temp in tank is high enough
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempTank >= HEATING_TEMP_THRESHOLD) {
                // temp in tank is high enough
                // turn pump ON
                switchNodeState(NODE_HEATING, sensIds, sensVals, 1);
            } else {
                // temp in tank is too low
                // do nothing
            }
        }
    }
}

void processFloorCircuit() {
    if (isInForcedMode(NODE_FLOOR_BIT, tsForcedNodeFloor)) {
        return;
    }
    if (diffTimestamps(tsCurr, tsNodeFloor) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK };
        uint8_t sensVals[] = { tempTank };
        if (NODE_STATE_FLAGS & NODE_FLOOR_BIT) {
            // pump is ON
            if (tempTank < HEATING_TEMP_THRESHOLD) {
                // temp in tank is too low
                // turn pump OFF
                switchNodeState(NODE_FLOOR, sensIds, sensVals, 1);
            } else {
                // temp in tank is high enough
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempTank >= HEATING_TEMP_THRESHOLD) {
                // temp in tank is high enough
                // turn pump ON
                switchNodeState(NODE_FLOOR, sensIds, sensVals, 1);
            } else {
                // temp in tank is too low
                // do nothing
            }
        }
    }
}

void processHotWaterCircuit() {
    if (isInForcedMode(NODE_HOTWATER_BIT, tsForcedNodeHotwater)) {
        return;
    }
    if (diffTimestamps(tsCurr, tsNodeHotwater) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_TANK, SENSOR_BOILER };
        uint8_t sensVals[] = { tempTank, tempBoiler };
        if (NODE_STATE_FLAGS & NODE_HOTWATER_BIT) {
            // pump is ON
            if (tempTank <= (tempBoiler + SENSORS_APPROX)) {
                // temp is equal
                // turn pump OFF
                switchNodeState(NODE_HOTWATER, sensIds, sensVals, 2);
            } else {
                // temp in tank is high enough
                // do nothing
            }
        } else {
            // pump is OFF
            if (tempTank >= (tempBoiler + TANK_BOILER_HIST)) {
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
    if (isInForcedMode(NODE_CIRCULATION_BIT, tsForcedNodeCirculation)) {
        return;
    }
    if (diffTimestamps(tsCurr, tsNodeCirculation) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_BOILER };
        uint8_t sensVals[] = { tempBoiler };
        if (NODE_STATE_FLAGS & NODE_CIRCULATION_BIT) {
            // pump is ON
            if (tempBoiler < CIRCULATION_TEMP_THRESHOLD) {
                // temp in boiler is too low
                // turn pump OFF
                switchNodeState(NODE_CIRCULATION, sensIds, sensVals, 1);
            } else {
                // temp in boiler is high enough
                if (diffTimestamps(tsCurr, tsNodeCirculation) >= CIRCULATION_ACTIVE_PERIOD_MSEC) {
                    // active period is over
                    // turn pump OFF
                    switchNodeState(NODE_CIRCULATION, sensIds, sensVals, 1);
                } else {
                    // active period going on
                    // do nothing
                }
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
                    // passive period going on
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
    if (isInForcedMode(NODE_BOILER_BIT, tsForcedNodeBoiler)) {
        return;
    }
    if (diffTimestamps(tsCurr, tsNodeBoiler) >= NODE_SWITCH_SAFE_TIME_MSEC) {
        uint8_t sensIds[] = { SENSOR_BOILER };
        uint8_t sensVals[] = { tempBoiler };
        if (NODE_STATE_FLAGS & NODE_BOILER_BIT) {
            // boiler heater is ON
            if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
                // heater is active
                // turn boiler heater OFF
                switchNodeState(NODE_BOILER, sensIds, sensVals, 1);
            } else {
                // heater is passive
                // do nothing
            }
        } else {
            // boiler heater is OFF
            if (NODE_STATE_FLAGS & NODE_SUPPLY_BIT) {
                // heater is active
                // do nothing
            } else {
                // heater is passive
                // turn boiler heater ON
                switchNodeState(NODE_BOILER, sensIds, sensVals, 1);
            }
        }
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

void switchNodeState(uint8_t id, uint8_t sensId[], uint8_t sensVal[], uint8_t sensCnt) {
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

        *ts = getTimestamp();
        if (*ts == 0) {
            *ts += 1;
        }
        char tsStr[12];
        char tsfStr[12];
        sprintf(tsStr, "%lu", *ts);
        sprintf(tsfStr, "%lu", *tsf);

        // report state change
        aJsonObject *root;
        root = aJson.createObject();
        aJson.addStringToObject(root, MSG_TYPE_KEY, MSG_NODE_STATE_CHANGED);
        aJson.addNumberToObject(root, ID_KEY, id);
        aJson.addNumberToObject(root, STATE_KEY, NODE_STATE_FLAGS & bit ? 1 : 0);
        aJson.addStringToObject(root, TIMESTAMP_KEY, tsStr);
        aJson.addNumberToObject(root, FORCE_FLAG_KEY, NODE_FORCED_MODE_FLAGS & bit ? 1 : 0);
        if (NODE_FORCED_MODE_FLAGS & bit) {
            aJson.addStringToObject(root, FORCE_TIMESTAMP_KEY, tsfStr);
        }
        aJsonObject* sensorsArr = aJson.createArray();
        for (unsigned int i = 0; i < sensCnt; i++) {
            aJsonObject *rec = aJson.createObject();
            aJson.addNumberToObject(rec, ID_KEY, sensId[i]);
            aJson.addNumberToObject(rec, VALUE_KEY, sensVal[i]);
            aJson.addItemToArray(sensorsArr, rec);
        }
        aJson.addItemToObject(root, SENSORS_KEY, sensorsArr);
        char* str = aJson.print(root);

        Serial.println(str);

        free(str);
        aJson.deleteItem(root);
    }
}

void forceNodeState(uint8_t id, uint8_t state, unsigned long ts) {
    if (NODE_SUPPLY == id) {
        forceNodeState(id, NODE_SUPPLY_BIT, state, tsForcedNodeSupply, ts);
    } else if (NODE_HEATING == id) {
        forceNodeState(id, NODE_HEATING_BIT, state, tsForcedNodeHeating, ts);
    } else if (NODE_FLOOR == id) {
        forceNodeState(id, NODE_FLOOR_BIT, state, tsForcedNodeFloor, ts);
    } else if (NODE_HOTWATER == id) {
        forceNodeState(id, NODE_HOTWATER_BIT, state, tsForcedNodeHotwater, ts);
    } else if (NODE_CIRCULATION == id) {
        forceNodeState(id, NODE_CIRCULATION_BIT, state, tsForcedNodeCirculation, ts);
    } else if (NODE_BOILER == id) {
        forceNodeState(id, NODE_BOILER_BIT, state, tsForcedNodeBoiler, ts);
    }
    // update node modes in EEPROM
    EEPROM.write(SAVED_STATE_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
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
    if (state != (NODE_STATE_FLAGS & bit)) {
        switchNodeState(id, 0, 0, 0);
    }
}

void unForceNodeState(uint8_t id) {
    if (NODE_SUPPLY == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_SUPPLY_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_SUPPLY_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_SUPPLY_BIT;
    } else if (NODE_HEATING == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_HEATING_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_HEATING_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_HEATING_BIT;
    } else if (NODE_FLOOR == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_FLOOR_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_FLOOR_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_FLOOR_BIT;
    } else if (NODE_HOTWATER == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_HOTWATER_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_HOTWATER_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_HOTWATER_BIT;
    } else if (NODE_CIRCULATION == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_CIRCULATION_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_CIRCULATION_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_CIRCULATION_BIT;
    } else if (NODE_BOILER == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_BOILER_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_BOILER_BIT;
        FORCED_MODE_OVERFLOW_TS_FLAGS = FORCED_MODE_OVERFLOW_TS_FLAGS & ~NODE_BOILER_BIT;
    }
    // update node modes in EEPROM
    EEPROM.write(SAVED_STATE_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
}

void reportStatus() {
    reportNodeStatus(NODE_SUPPLY, NODE_SUPPLY_BIT, tsNodeSupply, tsForcedNodeSupply);
    reportNodeStatus(NODE_HEATING, NODE_HEATING_BIT, tsNodeHeating, tsForcedNodeHeating);
    reportNodeStatus(NODE_FLOOR, NODE_FLOOR_BIT, tsNodeFloor, tsForcedNodeFloor);
    reportNodeStatus(NODE_HOTWATER, NODE_HOTWATER_BIT, tsNodeHotwater, tsForcedNodeHotwater);
    reportNodeStatus(NODE_CIRCULATION, NODE_CIRCULATION_BIT, tsNodeCirculation, tsForcedNodeCirculation);
    reportNodeStatus(NODE_BOILER, NODE_BOILER_BIT, tsNodeBoiler, tsForcedNodeBoiler);
    reportSensorsStatus();

    tsLastStatusReport = tsCurr;
}

void reportNodeStatus(uint8_t id, uint8_t bit, unsigned long ts, unsigned long tsf) {
    aJsonObject *root, *nodesArr, *node;
    char tss[12];
    char tsfs[12];
    char* outBuf;

    sprintf(tss, "%lu", ts);
    sprintf(tsfs, "%lu", tsf);

    node = aJson.createObject();
    aJson.addNumberToObject(node, ID_KEY, id);
    aJson.addNumberToObject(node, STATE_KEY, NODE_STATE_FLAGS & bit ? 1 : 0);
    aJson.addStringToObject(node, TIMESTAMP_KEY, tss);
    aJson.addNumberToObject(node, FORCE_FLAG_KEY, NODE_FORCED_MODE_FLAGS & bit ? 1 : 0);
    if (NODE_FORCED_MODE_FLAGS & bit) {
        aJson.addStringToObject(node, FORCE_TIMESTAMP_KEY, tsfs);
    }

    nodesArr = aJson.createArray();
    aJson.addItemToArray(nodesArr, node);
    root = aJson.createObject();
    aJson.addStringToObject(root, MSG_TYPE_KEY, MSG_CURRENT_STATUS_REPORT);
    aJson.addItemToObject(root, NODES_KEY, nodesArr);

    outBuf = (char*) malloc(JSON_MAX_WRITE_SIZE);
    aJsonStringStream stringStream(NULL, outBuf, JSON_MAX_WRITE_SIZE);
    aJson.print(root, &stringStream);

#ifdef __DEBUG__
    Serial.print(getTimestamp());
    Serial.print(": ");
#endif
    Serial.println(outBuf);

    free(outBuf);
    aJson.deleteItem(root);
}

void reportSensorsStatus() {
    aJsonObject *root, *sensArr, *sens;
    char* outBuf;

    sensArr = aJson.createArray();

    // SENSOR_SUPPLY;
    sens = aJson.createObject();
    aJson.addNumberToObject(sens, ID_KEY, SENSOR_SUPPLY);
    aJson.addNumberToObject(sens, VALUE_KEY, tempSupply);
    aJson.addItemToArray(sensArr, sens);

    // SENSOR_REVERSE;
    sens = aJson.createObject();
    aJson.addNumberToObject(sens, ID_KEY, SENSOR_REVERSE);
    aJson.addNumberToObject(sens, VALUE_KEY, tempReverse);
    aJson.addItemToArray(sensArr, sens);

    // SENSOR_TANK;
    sens = aJson.createObject();
    aJson.addNumberToObject(sens, ID_KEY, SENSOR_TANK);
    aJson.addNumberToObject(sens, VALUE_KEY, tempTank);
    aJson.addItemToArray(sensArr, sens);

    // SENSOR_TANK;
    sens = aJson.createObject();
    aJson.addNumberToObject(sens, ID_KEY, SENSOR_BOILER);
    aJson.addNumberToObject(sens, VALUE_KEY, tempBoiler);
    aJson.addItemToArray(sensArr, sens);

    root = aJson.createObject();
    aJson.addStringToObject(root, MSG_TYPE_KEY, MSG_CURRENT_STATUS_REPORT);
    aJson.addItemToObject(root, SENSORS_KEY, sensArr);

    outBuf = (char*) malloc(JSON_MAX_WRITE_SIZE);
    aJsonStringStream stringStream(NULL, outBuf, JSON_MAX_WRITE_SIZE);
    aJson.print(root, &stringStream);

#ifdef __DEBUG__
    Serial.print(getTimestamp());
    Serial.print(": ");
#endif
    Serial.println(outBuf);

    free(outBuf);
    aJson.deleteItem(root);
}

void syncClocks() {
    char tsStr[12];
    sprintf(tsStr, "%lu", getTimestamp());

    aJsonObject *root;
    root = aJson.createObject();
    aJson.addStringToObject(root, MSG_TYPE_KEY, MSG_CLOCK_SYNC);
    aJson.addStringToObject(root, TIMESTAMP_KEY, tsStr);
    aJson.addNumberToObject(root, OVERFLOW_COUNT_KEY, overflowCount);

    char* str = aJson.print(root);

    Serial.println(str);

    free(str);
    aJson.deleteItem(root);
}

void processIncomingCmd() {
    aJsonObject *root;
    char* buff = (char*) malloc(JSON_MAX_READ_SIZE);
    String s = Serial.readStringUntil('\0');
    s.toCharArray(buff, JSON_MAX_READ_SIZE, 0);
    root = aJson.parse(buff);
    if (root != NULL) {
        aJsonObject *msgType = aJson.getObjectItem(root, MSG_TYPE_KEY);
        if (msgType != NULL) {
            if (strcmp(msgType->valuestring, MSG_CLOCK_SYNC) == 0) {
                syncClocks();
            } else if (strcmp(msgType->valuestring, MSG_CURRENT_STATUS_REPORT) == 0) {
                reportStatus();
            } else if (strcmp(msgType->valuestring, MSG_NODE_STATE_CHANGED) == 0) {
                aJsonObject *nodeIdObj = aJson.getObjectItem(root, ID_KEY);
                if (nodeIdObj != NULL) {
                    aJsonObject *stateObj = aJson.getObjectItem(root, STATE_KEY);
                    if (stateObj != NULL && (stateObj->valueint == 0 || stateObj->valueint == 1)) {
                        unsigned long ts;
                        aJsonObject *tsObj = aJson.getObjectItem(root, FORCE_TIMESTAMP_KEY);
                        if (tsObj != NULL) {
                            ts = atol(tsObj->valuestring);
                            ts += tsCurr;
                            if (ts == 0) {
                                ts++;
                            }
                        } else {
                            ts = 0;
                        }
                        forceNodeState(nodeIdObj->valueint, stateObj->valueint, ts);
                    } else {
                        unForceNodeState(nodeIdObj->valueint);
                    }
                }
            }
        }
    }
    free(buff);
    aJson.deleteItem(root);
}
