#include "Ventilation.h"

#include <Arduino.h>
#include <DHT.h>
#include <EEPROMex.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <Stepper.h>

#include <debug.h>
#include <ESP8266.h>
#include <jsoner.h>

#define __DEBUG__

/*============================= Global configuration ========================*/

// Sensor pin
const uint8_t DHT_IN_PIN = 5;
const uint8_t DHT_OUT_PIN = 6;
const uint8_t SENSOR_TEMP_IN = (54 + 16) + (4 * 2) + 0;
const uint8_t SENSOR_HUM_IN = (54 + 16) + (4 * 2) + 1;
const uint8_t SENSOR_TEMP_OUT = (54 + 16) + (4 * 2) + 2;
const uint8_t SENSOR_HUM_OUT = (54 + 16) + (4 * 2) + 3;

// Node id
const uint8_t NODE_VENTILATION = 44;

// Stepper pins
const uint8_t MOTOR_PIN_1 = 8;
const uint8_t MOTOR_PIN_2 = 9;
const uint8_t MOTOR_PIN_3 = 10;
const uint8_t MOTOR_PIN_4 = 11;
const uint8_t MOTOR_STEPS = 32;
const uint16_t MOTOR_STEPS_PER_REVOLUTION = 2048;
const uint8_t REVOLUTION_COUNT = 15; // max 15

const uint8_t SENSORS_ADDR_CFG[] = { SENSOR_TEMP_IN, SENSOR_HUM_IN, SENSOR_TEMP_OUT, SENSOR_HUM_OUT };

// WiFi pins
const uint8_t WIFI_RST_PIN = 0; // n/a

// Nodes State
const uint16_t NODE_VENTILATION_BIT = 1;

// Debug serial port
const uint8_t DEBUG_SERIAL_TX_PIN = 2;
const uint8_t DEBUG_SERIAL_RX_PIN = 3;

const uint8_t HEARTBEAT_LED = 13;

// etc
const unsigned long MAX_TIMESTAMP = -1;
const int16_t UNKNOWN_SENSOR_VALUE = -127;
const unsigned long NODE_SWITCH_SAFE_TIME_SEC = 60;

// reporting
const unsigned long STATUS_REPORTING_PERIOD_SEC = 60; // 1 minute
const unsigned long SENSORS_READ_INTERVAL_SEC = 10; // 10 seconds
const uint16_t WIFI_FAILURE_GRACE_PERIOD_SEC = 180; // 3 minutes

// ventilation
const int8_t TEMP_IN_OUT_HIST = 3;

const uint8_t JSON_MAX_SIZE = 64;

/*============================= Global variables ============================*/

//// Sensors
// Values
int8_t tempIn = 0;
int8_t humIn = 0;
int8_t tempOut = 0;
int8_t humOut = 0;

//// Nodes

// Nodes actual state
uint16_t NODE_STATE_FLAGS = 0;
// Nodes forced mode
uint16_t NODE_FORCED_MODE_FLAGS = 0;
// will be stored in EEPROM
uint16_t NODE_PERMANENTLY_FORCED_MODE_FLAGS = 0;

// Nodes switch timestamps
unsigned long tsNodeVentilation = 0;

// Nodes forced mode timestamps
unsigned long tsForcedNodeVentilation = 0;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorRead = 0;

unsigned long tsPrev = 0;
unsigned long tsPrevRaw = 0;
unsigned long tsCurr = 0;

uint8_t overflowCount = 0;

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
const int SENSORS_FACTORS_EEPROM_ADDR = NODE_FORCED_MODE_FLAGS_EEPROM_ADDR + sizeof(NODE_FORCED_MODE_FLAGS);
const int WIFI_REMOTE_AP_EEPROM_ADDR = SENSORS_FACTORS_EEPROM_ADDR
        + sizeof(double) * sizeof(SENSORS_ADDR_CFG) / sizeof(SENSORS_ADDR_CFG[0]);
const int WIFI_REMOTE_PW_EEPROM_ADDR = WIFI_REMOTE_AP_EEPROM_ADDR + sizeof(WIFI_REMOTE_AP);
const int SERVER_IP_EEPROM_ADDR = WIFI_REMOTE_PW_EEPROM_ADDR + sizeof(WIFI_REMOTE_PW);
const int SERVER_PORT_EEPROM_ADDR = SERVER_IP_EEPROM_ADDR + sizeof(SERVER_IP);

int eepromWriteCount = 0;

/*============================= Connectivity ================================*/

// DHT sensors
DHT dhtIn(DHT_IN_PIN, DHT11);
DHT dhtOut(DHT_OUT_PIN, DHT11);

// Stepper
Stepper motor(MOTOR_STEPS, MOTOR_PIN_1, MOTOR_PIN_3, MOTOR_PIN_2, MOTOR_PIN_4);

// Serial port
SoftwareSerial serialPort(DEBUG_SERIAL_RX_PIN, DEBUG_SERIAL_TX_PIN);
SoftwareSerial *serial = &serialPort;
#ifdef __DEBUG__
SoftwareSerial *debug = serial;
#else
SoftwareSerial *debug = NULL;
#endif

//WiFi
HardwareSerial *wifi = &Serial;
ESP8266 esp8266;

void setup() {
    // Setup serial ports
    serial->begin(57600); // s/w serial unreliable on 115200
    wifi->begin(115200);

    dbg(debug, F(":STARTING\n"));

    // init heartbeat led
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // init motor
    motor.setSpeed(800);
    // default: closed state
    motor.step(MOTOR_STEPS_PER_REVOLUTION * REVOLUTION_COUNT); //close

    // restore forced node state flags from EEPROM
    // default node state -- OFF
    restoreNodesState();

    // open ventilation valve if needed
    if (NODE_STATE_FLAGS & NODE_VENTILATION_BIT) {
        motor.step(-MOTOR_STEPS_PER_REVOLUTION * REVOLUTION_COUNT);
    }

    // init esp8266 hw reset pin. N/A
    //pinMode(WIFI_RST_PIN, OUTPUT);

    // setup WiFi
    loadWifiConfig();
    dbgf(debug, F(":setup wifi:R_AP:%s\n"), &WIFI_REMOTE_AP);
    esp8266.init(wifi, MODE_STA, WIFI_RST_PIN, WIFI_FAILURE_GRACE_PERIOD_SEC);
    esp8266.connect(&WIFI_REMOTE_AP, &WIFI_REMOTE_PW);
    esp8266.getStaIP(WIFI_STA_IP);
    dbgf(debug, F("STA IP: %d.%d.%d.%d\n"), WIFI_STA_IP[0], WIFI_STA_IP[1], WIFI_STA_IP[2], WIFI_STA_IP[3]);
}

void loop() {
    tsCurr = getTimestamp();
    if (diffTimestamps(tsCurr, tsLastSensorRead) >= SENSORS_READ_INTERVAL_SEC) {
        readSensors();
        tsLastSensorRead = tsCurr;

        digitalWrite(HEARTBEAT_LED, digitalRead(HEARTBEAT_LED) ^ 1);

        // ventilation valve
        processVentilationValve();
    }

    if (serial->available() > 0) {
        processSerialMsg();
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

void processVentilationValve() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT;
    if (isInForcedMode(NODE_VENTILATION_BIT, tsForcedNodeVentilation)) {
        return;
    }
    if (wasForceMode) {
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_VENTILATION, NODE_STATE_FLAGS & NODE_VENTILATION_BIT, tsNodeVentilation,
                NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT, tsForcedNodeVentilation, json, JSON_MAX_SIZE);
        broadcastMsg(json);
    }
    if (diffTimestamps(tsCurr, tsNodeVentilation) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_TEMP_IN, SENSOR_TEMP_OUT };
        int16_t sensVals[] = { tempIn, tempOut };
        uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        if (!validSensorValues(sensVals, sensCnt)) {
            return;
        }

        if (NODE_STATE_FLAGS & NODE_VENTILATION_BIT) {
            // ventilation is OPEN
            if (tempIn <= tempOut) {
                // temp outside is high
                // CLOSE ventilation
                switchNodeState(NODE_VENTILATION, sensIds, sensVals, sensCnt);
            } else {
                // temp outside is low
                // do nothing
            }
        } else {
            // ventilation is CLOSED
            if (tempIn >= (tempOut + TEMP_IN_OUT_HIST)) {
                // temp outside is low enough
                // OPEN ventilation
                switchNodeState(NODE_VENTILATION, sensIds, sensVals, sensCnt);
            } else {
                // temp outside is high
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
    if (NODE_VENTILATION == id) {
        bit = NODE_VENTILATION_BIT;
        ts = &tsNodeVentilation;
        tsf = &tsForcedNodeVentilation;
        switchVentilationValve();
    }

    if (ts != NULL) {
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
    if (NODE_VENTILATION == id) {
        forceNodeState(NODE_VENTILATION, NODE_VENTILATION_BIT, state, tsForcedNodeVentilation, ts);
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_VENTILATION, NODE_STATE_FLAGS & NODE_VENTILATION_BIT, tsNodeVentilation,
                NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT, tsForcedNodeVentilation, json, JSON_MAX_SIZE);
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
    if (NODE_VENTILATION == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_VENTILATION_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_VENTILATION_BIT;
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_VENTILATION, NODE_STATE_FLAGS & NODE_VENTILATION_BIT, tsNodeVentilation,
                NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT, tsForcedNodeVentilation, json, JSON_MAX_SIZE);
        broadcastMsg(json);
    }
    // update node modes in EEPROM if unforced from permanent
    if (prevPermanentlyForcedModeFlags != NODE_PERMANENTLY_FORCED_MODE_FLAGS) {
        eepromWriteCount += EEPROM.updateInt(NODE_FORCED_MODE_FLAGS_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
    }
}

void switchVentilationValve() {
    if (NODE_STATE_FLAGS & NODE_VENTILATION_BIT) {
        // OPENED
        motor.step(MOTOR_STEPS_PER_REVOLUTION * REVOLUTION_COUNT); //close
    } else {
        // CLOSED
        motor.step(-MOTOR_STEPS_PER_REVOLUTION * REVOLUTION_COUNT); //open
    }
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

/*====================== Sensors processing methods =========================*/

void readSensors() {
    tempIn = getSensorValue(SENSOR_TEMP_IN);
    humIn = getSensorValue(SENSOR_HUM_IN);
    tempOut = getSensorValue(SENSOR_TEMP_OUT);
    humOut = getSensorValue(SENSOR_HUM_OUT);
}

int8_t getSensorValue(const uint8_t sensor) {
    float result = UNKNOWN_SENSOR_VALUE;
    if (SENSOR_TEMP_IN == sensor) {
        result = dhtIn.readTemperature() * readSensorCF(SENSOR_TEMP_IN);
    } else if (SENSOR_HUM_IN == sensor) {
        result = dhtIn.readHumidity() * readSensorCF(SENSOR_HUM_IN);
    } else if (SENSOR_TEMP_OUT == sensor) {
        result = dhtOut.readTemperature() * readSensorCF(SENSOR_TEMP_OUT);
    } else if (SENSOR_HUM_OUT == sensor) {
        result = dhtOut.readHumidity() * readSensorCF(SENSOR_HUM_OUT);
    }

    result = (result > 0) ? result + 0.5 : result - 0.5;
    return int8_t(result);
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

    jsonifySensorValue(SENSOR_TEMP_IN, tempIn, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifySensorValue(SENSOR_HUM_IN, humIn, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifySensorValue(SENSOR_TEMP_OUT, tempOut, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifySensorValue(SENSOR_HUM_OUT, humOut, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifyNodeStatus(NODE_VENTILATION, NODE_STATE_FLAGS & NODE_VENTILATION_BIT, tsNodeVentilation,
            NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT, tsForcedNodeVentilation, json, JSON_MAX_SIZE);
    broadcastMsg(json);
}

void reportConfiguration() {
    char json[JSON_MAX_SIZE];
    char ip[16];

    jsonifySensorConfig(SENSOR_TEMP_IN, F("cf"), readSensorCF(SENSOR_TEMP_IN), json, JSON_MAX_SIZE);
    serial->println(json);
    jsonifySensorConfig(SENSOR_HUM_IN, F("cf"), readSensorCF(SENSOR_HUM_IN), json, JSON_MAX_SIZE);
    serial->println(json);
    jsonifySensorConfig(SENSOR_TEMP_OUT, F("cf"), readSensorCF(SENSOR_TEMP_OUT), json, JSON_MAX_SIZE);
    serial->println(json);
    jsonifySensorConfig(SENSOR_HUM_OUT, F("cf"), readSensorCF(SENSOR_HUM_OUT), json, JSON_MAX_SIZE);
    serial->println(json);
    jsonifyConfig(F("rap"), WIFI_REMOTE_AP, json, JSON_MAX_SIZE);
    serial->println(json);
    jsonifyConfig(F("rpw"), WIFI_REMOTE_PW, json, JSON_MAX_SIZE);
    serial->println(json);
    sprintf(ip, "%d.%d.%d.%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2], SERVER_IP[3]);
    jsonifyConfig(F("sip"), ip, json, JSON_MAX_SIZE);
    serial->println(json);
    jsonifyConfig(F("sp"), SERVER_PORT, json, JSON_MAX_SIZE);
    serial->println(json);
    sprintf(ip, "%d.%d.%d.%d", WIFI_STA_IP[0], WIFI_STA_IP[1], WIFI_STA_IP[2], WIFI_STA_IP[3]);
    jsonifyConfig(F("lip"), ip, json, JSON_MAX_SIZE);
    serial->println(json);
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
    unsigned long start = millis();
    int status = esp8266.send(SERVER_IP, SERVER_PORT, msg);
    dbgf(debug, F(":HTTP:send:%d:[%d msec]\n"), status, millis() - start);
}

bool parseCommand(char* command) {
    dbgf(debug, F(":parse cmd:%s\n"), command);

    if (strstr(command, "AT") == command) {
        // this is AT command for ESP8266
        dbgf(debug, F(":ESP8266:%s\n"), command);
        esp8266.write(command);
        return true;
    }

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
            JsonObject& sensor = root[F("s")].asObject();
            if (sensor.containsKey(F("id")) && sensor.containsKey(F("cf"))) {
                uint8_t id = sensor[F("id")].as<uint8_t>();
                double cf = sensor[F("cf")].as<double>();
                saveSensorCF(id, cf);
            } else if (root.containsKey(F("rap"))) {
                sprintf(WIFI_REMOTE_AP, "%s", root[F("rap")].asString());
                eepromWriteCount += EEPROM.updateBlock(WIFI_REMOTE_AP_EEPROM_ADDR, WIFI_REMOTE_AP,
                        sizeof(WIFI_REMOTE_AP));
            } else if (root.containsKey(F("rpw"))) {
                sprintf(WIFI_REMOTE_PW, "%s", root[F("rpw")].asString());
                eepromWriteCount += EEPROM.updateBlock(WIFI_REMOTE_PW_EEPROM_ADDR, WIFI_REMOTE_PW,
                        sizeof(WIFI_REMOTE_PW));
            } else if (root.containsKey(F("sip"))) {
                sscanf(root[F("sip")], "%d.%d.%d.%d", (int*) &SERVER_IP[0], (int*) &SERVER_IP[1], (int*) &SERVER_IP[2],
                        (int*) &SERVER_IP[3]);
                eepromWriteCount += EEPROM.updateBlock(SERVER_IP_EEPROM_ADDR, SERVER_IP, sizeof(SERVER_IP));
            } else if (root.containsKey(F("sp"))) {
                SERVER_PORT = root[F("sp")].as<int>();
                eepromWriteCount += EEPROM.updateInt(SERVER_PORT_EEPROM_ADDR, SERVER_PORT);
            } else if (root.containsKey(F("dsp"))) {
                int sp = root[F("dsp")].as<int>();
                if (sp == -1) {
                    debug = NULL;
                } else if (sp == 1) {
                    debug = serial;
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
