#include <bro.h>

#include <Arduino.h>
#include <DHT.h>
#include <EEPROMex.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <ACS712.h>
#include <LiquidCrystal_I2C.h>

#include <debug.h>
#include <jsoner.h>

#define __DEBUG__

/*============================= Global configuration ========================*/

// Sensor pin
const uint8_t DHT_PIN = 5;
const uint8_t SENSOR_HEATER_CURRENT_METER_PIN = A0;

const uint8_t SENSOR_TEMP = (54 + 16) + (4 * 4) + 0;
const uint8_t SENSOR_HUM = (54 + 16) + (4 * 4) + 1;
const uint8_t SENSOR_HEATER_CURRENT = (54 + 16) + (4 * 4) + 2;

// Node id
const uint8_t NODE_HEATER_RESET = 6;

// Nodes State
const uint16_t NODE_HEATER_RESET_BIT = 1;

// Debug serial port
const uint8_t DEBUG_SERIAL_TX_PIN = 2;
const uint8_t DEBUG_SERIAL_RX_PIN = 3;

const uint8_t HEARTBEAT_LED = 13;

// etc
const unsigned long MAX_TIMESTAMP = -1;
const int16_t UNKNOWN_SENSOR_VALUE = -127;
const unsigned long NODE_SWITCH_SAFE_TIME_SEC = 60 * 5;

// reporting
const unsigned long STATUS_REPORTING_PERIOD_SEC = 10; // 10 seconds
const unsigned long SENSORS_READ_INTERVAL_SEC = 10; // 10 seconds

// heater
const int8_t TEMP_HEATER_RESET_THRESHOLD = 18;
const int8_t HEATER_RESET_PERIOD_SEC = 10;
const double HEATER_ACTIVE_MIN_IAC = 0.10;
const unsigned long HEATER_INACTIVE_MIN_PERIOD_SEC = 15 * 60; // 15 minutes

const uint8_t JSON_MAX_SIZE = 64;
const uint8_t LCD_LINE_LENGTH = 40;

/*============================= Global variables ============================*/

//// Sensors
// Values
int8_t roomTemp = 0;
int8_t roomHum = 0;
double heaterIac = 0; // A

//// Nodes

// Nodes actual state
uint16_t NODE_STATE_FLAGS = 0;
// Nodes forced mode
uint16_t NODE_FORCED_MODE_FLAGS = 0;
// will be stored in EEPROM
uint16_t NODE_PERMANENTLY_FORCED_MODE_FLAGS = 0;

// Nodes switch timestamps
unsigned long tsNodeHeaterReset = 0;

// Nodes forced mode timestamps
unsigned long tsForcedNodeHeaterReset = 0;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorRead = 0;
unsigned long tsNodeHeaterActive = 0;

unsigned long tsPrev = 0;
unsigned long tsPrevRaw = 0;
unsigned long tsCurr = 0;

uint8_t overflowCount = 0;

// EEPROM addresses
const int NODE_STATE_FLAGS_EEPROM_ADDR = 0;
const int NODE_FORCED_MODE_FLAGS_EEPROM_ADDR = NODE_STATE_FLAGS_EEPROM_ADDR + sizeof(NODE_STATE_FLAGS);

int eepromWriteCount = 0;

/*============================= Connectivity ================================*/

// DHT sensors
DHT dht(DHT_PIN, DHT11);

// Current sensor
ACS712 acs712(ACS712_30A, SENSOR_HEATER_CURRENT_METER_PIN);

// LCD
LiquidCrystal_I2C lcd(0x27, LCD_LINE_LENGTH, 4);

// Serial port
SoftwareSerial serialPort(DEBUG_SERIAL_RX_PIN, DEBUG_SERIAL_TX_PIN);
HardwareSerial *serial = &Serial;
#ifdef __DEBUG__
SoftwareSerial *debug = &serialPort;
#else
SoftwareSerial *debug = NULL;
#endif

void setup() {
    // Setup serial ports
    serial->begin(57600);
    if (debug) {
        debug->begin(57600);
    }

    dbg(debug, F(":STARTING\n"));

    // init heartbeat led
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // calibrate heater current meter
    acs712.setZeroPoint(511);

    // turn on LCD
    lcd.begin();
    lcd.backlight();

    // restore forced node state flags from EEPROM
    // default node state -- OFF
    restoreNodesState();

    // init nodes pins
    pinMode(NODE_HEATER_RESET, OUTPUT);

    // restore nodes state
    digitalWrite(NODE_HEATER_RESET, (~NODE_STATE_FLAGS & NODE_HEATER_RESET) ? LOW : HIGH);

    tsLastStatusReport -= STATUS_REPORTING_PERIOD_SEC;
}

void loop() {
    tsCurr = getTimestamp();
    if (diffTimestamps(tsCurr, tsLastSensorRead) >= SENSORS_READ_INTERVAL_SEC) {
        readSensors();
        tsLastSensorRead = tsCurr;

        digitalWrite(HEARTBEAT_LED, digitalRead(HEARTBEAT_LED) ^ 1);

        // heater reset node
        processHeaterReset();
    }

    if (serial->available() > 0) {
        processSerialMsg();
    }

    if (diffTimestamps(tsCurr, tsLastStatusReport) >= STATUS_REPORTING_PERIOD_SEC) {
        reportStatus();
        tsLastStatusReport = tsCurr;
    }

    tsPrev = tsCurr;
}

/*========================= Node processing methods =========================*/

void processHeaterReset() {
    // update heater active TS
    if (heaterIac >= HEATER_ACTIVE_MIN_IAC) {
        tsNodeHeaterActive = tsCurr;
    }

    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_HEATER_RESET_BIT;
    if (isInForcedMode(NODE_HEATER_RESET_BIT, tsForcedNodeHeaterReset)) {
        return;
    }
    if (wasForceMode) {
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_HEATER_RESET, NODE_STATE_FLAGS & NODE_HEATER_RESET, tsNodeHeaterReset,
                NODE_FORCED_MODE_FLAGS & NODE_HEATER_RESET_BIT, tsForcedNodeHeaterReset, json, JSON_MAX_SIZE);
        broadcastMsg(json);
    }

    uint8_t sensIds[] = { SENSOR_TEMP };
    int16_t sensVals[] = { roomTemp };
    uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
    if (NODE_STATE_FLAGS & NODE_HEATER_RESET_BIT) {
        // reset mode
        if (diffTimestamps(tsCurr, tsNodeHeaterReset) >= HEATER_RESET_PERIOD_SEC) {
            // RESET period over
            // back to normal mode
            switchNodeState(NODE_HEATER_RESET, sensIds, sensVals, sensCnt);
        }
    } else {
        // normal mode
        if (diffTimestamps(tsCurr, tsNodeHeaterReset) >= NODE_SWITCH_SAFE_TIME_SEC) {
            if (!validSensorValues(sensVals, sensCnt)) {
                return;
            }
            if (roomTemp <= TEMP_HEATER_RESET_THRESHOLD && heaterIac < HEATER_ACTIVE_MIN_IAC
                    && diffTimestamps(tsCurr, tsNodeHeaterActive) >= HEATER_INACTIVE_MIN_PERIOD_SEC) {
                // temp in Room is low && Heater is OFF && OFF period is long enough
                // start RESET
                switchNodeState(NODE_HEATER_RESET, sensIds, sensVals, sensCnt);
            } else {
                // temp in Room is OK || Heater is ON || Heater is OFF for short period
                // do nothing
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
    if (NODE_HEATER_RESET == id) {
        bit = NODE_HEATER_RESET_BIT;
        ts = &tsNodeHeaterReset;
        tsf = &tsForcedNodeHeaterReset;
    }

    if (ts != NULL) {
        digitalWrite(id, digitalRead(id) ^ 1);
        NODE_STATE_FLAGS = NODE_STATE_FLAGS ^ bit;

        *ts = getTimestamp();

        // report state change
        char json[JSON_MAX_SIZE];
        jsonifyNodeStateChange(id, NODE_STATE_FLAGS & bit, *ts, NODE_FORCED_MODE_FLAGS & bit, *tsf, sensId, sensVal,
                sensCnt, json, JSON_MAX_SIZE);
        broadcastMsg(json);
    }
}

void forceNodeState(uint8_t id, uint8_t state, unsigned long ts) {
    if (NODE_HEATER_RESET == id) {
        forceNodeState(NODE_HEATER_RESET, NODE_HEATER_RESET_BIT, state, tsForcedNodeHeaterReset, ts);
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_HEATER_RESET, NODE_STATE_FLAGS & NODE_HEATER_RESET_BIT, tsNodeHeaterReset,
                NODE_FORCED_MODE_FLAGS & NODE_HEATER_RESET_BIT, tsForcedNodeHeaterReset, json, JSON_MAX_SIZE);
        broadcastMsg(json);
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
    if (NODE_HEATER_RESET == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_HEATER_RESET_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_HEATER_RESET_BIT;
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_HEATER_RESET, NODE_STATE_FLAGS & NODE_HEATER_RESET_BIT, tsNodeHeaterReset,
                NODE_FORCED_MODE_FLAGS & NODE_HEATER_RESET_BIT, tsForcedNodeHeaterReset, json, JSON_MAX_SIZE);
        broadcastMsg(json);
    }
    // update node modes in EEPROM if unforced from permanent
    if (prevPermanentlyForcedModeFlags != NODE_PERMANENTLY_FORCED_MODE_FLAGS) {
        eepromWriteCount += EEPROM.updateInt(NODE_FORCED_MODE_FLAGS_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
    }
}

/*====================== Load/Save configuration in EEPROM ==================*/

void restoreNodesState() {
    NODE_STATE_FLAGS = EEPROM.readInt(NODE_STATE_FLAGS_EEPROM_ADDR);
    NODE_FORCED_MODE_FLAGS = EEPROM.readInt(NODE_FORCED_MODE_FLAGS_EEPROM_ADDR);
    NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS;
    NODE_STATE_FLAGS = NODE_STATE_FLAGS & NODE_PERMANENTLY_FORCED_MODE_FLAGS;
}

/*====================== Sensors processing methods =========================*/

void readSensors() {
    float v;
    v = dht.readTemperature();
    roomTemp = (int8_t) (v > 0) ? v + 0.5 : v - 0.5;
    // prevent endless heater reset when sensor disconnected
    roomTemp = roomTemp == 0 ? UNKNOWN_SENSOR_VALUE : roomTemp;
    v = dht.readHumidity();
    roomHum = (int8_t) (v > 0) ? v + 0.5 : v - 0.5;
    // prevent endless heater reset when sensor disconnected
    roomHum = roomHum == 0 ? UNKNOWN_SENSOR_VALUE : roomHum;
    v = acs712.getCurrentAC();
    heaterIac = round(v * 100) / 100;
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
    char json[JSON_MAX_SIZE];

    jsonifySensorValue(SENSOR_TEMP, roomTemp, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifySensorValue(SENSOR_HUM, roomHum, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifySensorValue(SENSOR_HEATER_CURRENT, heaterIac, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifyNodeStatus(NODE_HEATER_RESET, NODE_STATE_FLAGS & NODE_HEATER_RESET_BIT, tsNodeHeaterReset,
            NODE_FORCED_MODE_FLAGS & NODE_HEATER_RESET_BIT, tsForcedNodeHeaterReset, json, JSON_MAX_SIZE);
    broadcastMsg(json);
}

void reportConfiguration() {
    dbgf(debug, F(":EEPROM:written:%d bytes\n"), eepromWriteCount);
}

void reportTimestamp() {
    char json[JSON_MAX_SIZE];
    jsonifyClockSync(getTimestamp(), json, JSON_MAX_SIZE);
    broadcastMsg(json);
}

/*========================= Communication ===================================*/

void processSerialMsg() {
    char buff[JSON_MAX_SIZE + 1];
    uint16_t l = serial->readBytesUntil('\0', buff, JSON_MAX_SIZE);
    buff[l] = '\0';
    parseCommand(buff);
}

void broadcastMsg(const char* msg) {
    serial->println(msg);
    printToDisplay(msg);
}

void printToDisplay(const char* msg) {
    DynamicJsonBuffer jsonBuffer(JSON_MAX_SIZE * 2);
    JsonObject& root = jsonBuffer.parseObject(msg);
    if (root.success()) {
        if (root[F("m")] == F("csr")) {
            if (root.containsKey(F("s"))) {
                JsonObject& data = root[F("s")];
                if (data.containsKey(F("id"))) {
                    uint8_t id = data[F("id")].as<uint8_t>();
                    char str[LCD_LINE_LENGTH + 1];
                    if (id == SENSOR_TEMP && data.containsKey(F("v"))) {
                        snprintf(str, LCD_LINE_LENGTH, "Temp      :%7d C", data[F("v")].as<int8_t>());
                        lcd.setCursor(0, 0);
                    } else if (id == SENSOR_HUM && data.containsKey(F("v"))) {
                        snprintf(str, LCD_LINE_LENGTH, "Hum       :%7d %%", data[F("v")].as<int8_t>());
                        lcd.setCursor(0, 1);
                    } else if (id == SENSOR_HEATER_CURRENT && data.containsKey(F("v"))) {
                        double v = data[F("v")].as<double>();
                        snprintf(str, LCD_LINE_LENGTH, "Power[%3s]:%7d W", v >= HEATER_ACTIVE_MIN_IAC ? "ON" : "OFF",
                                (int16_t) (v * 220));
                        lcd.setCursor(0, 2);
                    }
                    lcd.print(str);
                }
            } else if (root.containsKey(F("n"))) {
                JsonObject& data = root[F("n")];
                if (data.containsKey(F("id"))) {
                    uint8_t id = data[F("id")].as<uint8_t>();
                    char str[LCD_LINE_LENGTH + 1];
                    if (id == NODE_HEATER_RESET && data.containsKey(F("ts"))) {
                        unsigned long ts = data[F("ts")].as<unsigned long>();
                        if (ts > 0) {
                            ts = getTimestamp() - ts;
                        }
                        int d, h, m;
                        d = ts / 60 / 60 / 24;
                        h = (ts / 60 / 60) % 24;
                        m = (ts / 60) % 60;
                        snprintf(str, LCD_LINE_LENGTH, "Last Reset:%03d:%02d:%02d", d, h, m);
                        lcd.setCursor(0, 3);
                    }
                    lcd.print(str);
                }
            }
        }
    }
}

bool parseCommand(char* command) {
    dbgf(debug, F(":parse cmd:%s\n"), command);

    DynamicJsonBuffer jsonBuffer(JSON_MAX_SIZE * 2);
    JsonObject& root = jsonBuffer.parseObject(command);

    if (root.success()) {
        if (root[F("m")] == F("cls")) {
            // SYNC
            reportTimestamp();
        } else if (root[F("m")] == F("csr")) {
            // CSR
            tsLastStatusReport = tsCurr - STATUS_REPORTING_PERIOD_SEC;
        } else if (root[F("m")] == F("nsc")) {
            // NSC
            if (root.containsKey(F("id"))) {
                uint8_t id = root[F("id")].as<uint8_t>();
                if (root.containsKey(F("ns"))) {
                    uint8_t state = root[F("ns")].as<uint8_t>();
                    if (state == 0 || state == 1) {
                        unsigned long ts;
                        if (root.containsKey(F("ft"))) {
                            ts = root[F("ft")].as<unsigned long>();
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
        } else if (root[F("m")] == F("cfg")) {
            // CFG
            if (root.containsKey(F("dsp"))) {
                int sp = root[F("dsp")].as<int>();
                if (sp == -1) {
                    debug = NULL;
                } else if (sp == 1) {
                    debug = &serialPort;
                }
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
