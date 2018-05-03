#include "Thermostat.h"

#include <Arduino.h>
#include <DHT.h>
#include <EEPROMex.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#include <debug.h>
#include <ESP8266.h>

#define __DEBUG__

/*============================= Global configuration ========================*/

// Sensor pin
const uint8_t DHT_PIN = 5;
const uint8_t SENSOR_TEMP = (54 + 16) + (4 * 1) + 0;
const uint8_t SENSOR_HUM = (54 + 16) + (4 * 1) + 1;

const uint8_t SENSORS_ADDR_CFG[] = { SENSOR_TEMP, SENSOR_HUM };

// WiFi pins
const uint8_t WIFI_RST_PIN = 0; // n/a

// Debug serial port
const uint8_t DEBUG_SERIAL_TX_PIN = 2;
const uint8_t DEBUG_SERIAL_RX_PIN = 3;

const uint8_t HEARTBEAT_LED = 13;

// etc
const unsigned long MAX_TIMESTAMP = -1;
const int16_t UNKNOWN_SENSOR_VALUE = -127;

// reporting
const unsigned long STATUS_REPORTING_PERIOD_SEC = 15; // 15s //disabled in LowPower mode
const unsigned long SENSORS_READ_INTERVAL_SEC = 15; // 15s //disabled in LowPower mode
const uint16_t WIFI_FAILURE_GRACE_PERIOD_SEC = 180; // 3 minutes

// JSON
const char MSG_TYPE_KEY[] = "m";
const char ID_KEY[] = "id";
const char SENSORS_KEY[] = "s";
const char VALUE_KEY[] = "v";
const char CALIBRATION_FACTOR_KEY[] = "cf";

const char MSG_CURRENT_STATUS_REPORT[] = "csr";
const char MSG_CONFIGURATION[] = "cfg";
const char WIFI_REMOTE_AP_KEY[] = "rap";
const char WIFI_REMOTE_PASSWORD_KEY[] = "rpw";
const char SERVER_IP_KEY[] = "sip";
const char SERVER_PORT_KEY[] = "sp";
const char LOCAL_IP_KEY[] = "lip";
const char DEBUG_SERIAL_PORT_KEY[] = "dsp";

const uint8_t JSON_MAX_SIZE = 64;
const uint8_t JSON_MAX_BUFFER_SIZE = 128;

/*============================= Global variables ============================*/

//// Sensors
// Values
int8_t tempRoom = 0;
int8_t humRoom = 0;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorRead = 0;

unsigned long tsPrev = 0;
unsigned long tsPrevRaw = 0;
unsigned long tsCurr = 0;

uint8_t overflowCount = 0;

// reporting
uint8_t nextEntryReport = 0;

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
const int SENSORS_FACTORS_EEPROM_ADDR = 0;
const int WIFI_REMOTE_AP_EEPROM_ADDR = SENSORS_FACTORS_EEPROM_ADDR
        + sizeof(double) * sizeof(SENSORS_ADDR_CFG) / sizeof(SENSORS_ADDR_CFG[0]);
const int WIFI_REMOTE_PW_EEPROM_ADDR = WIFI_REMOTE_AP_EEPROM_ADDR + sizeof(WIFI_REMOTE_AP);
const int SERVER_IP_EEPROM_ADDR = WIFI_REMOTE_PW_EEPROM_ADDR + sizeof(WIFI_REMOTE_PW);
const int SERVER_PORT_EEPROM_ADDR = SERVER_IP_EEPROM_ADDR + sizeof(SERVER_IP);

int eepromWriteCount = 0;

/*============================= Connectivity ================================*/

// DHT sensor
DHT dht(DHT_PIN, DHT11);

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

/*============================= Arduino entry point =========================*/

void setup() {
    // Setup serial ports
    serial->begin(115200);
    wifi->begin(115200);

    dbg(debug, F(":STARTING\n"));

    // init heartbeat led
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // init esp8266 hw reset pin. N/A
    //pinMode(WIFI_RST_PIN, OUTPUT);

    // setup WiFi
    loadWifiConfig();
    dbgf(debug, F(":setup wifi:R_AP:%s\n"), &WIFI_REMOTE_AP);
    esp8266.init(wifi, MODE_STA, WIFI_RST_PIN, WIFI_FAILURE_GRACE_PERIOD_SEC);
    esp8266.connect(&WIFI_REMOTE_AP, &WIFI_REMOTE_PW);
    esp8266.getStaIP(WIFI_STA_IP);
    dbgf(debug, F("STA IP: %d.%d.%d.%d"), WIFI_STA_IP[0], WIFI_STA_IP[1], WIFI_STA_IP[2], WIFI_STA_IP[3]);
}

void loop() {
    tsCurr = getTimestamp();
    if (diffTimestamps(tsCurr, tsLastSensorRead) >= SENSORS_READ_INTERVAL_SEC) {
        readSensors();
        tsLastSensorRead = tsCurr;

        digitalWrite(HEARTBEAT_LED, digitalRead(HEARTBEAT_LED) ^ 1);
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
    tempRoom = getSensorValue(SENSOR_TEMP);
    humRoom = getSensorValue(SENSOR_HUM);
}

int8_t getSensorValue(const uint8_t sensor) {
    float result = UNKNOWN_SENSOR_VALUE;
    if (SENSOR_TEMP == sensor) {
        result = dht.readTemperature() * readSensorCF(SENSOR_TEMP);
    } else if (SENSOR_HUM == sensor) {
        result = dht.readHumidity() * readSensorCF(SENSOR_HUM);
    }
    result = (result > 0) ? result + 0.5 : result - 0.5;
    return int8_t(result);
}

/*============================ Reporting ====================================*/

void reportStatus() {
    if (nextEntryReport == 0) {
        // start reporting
        nextEntryReport = SENSOR_TEMP;
    }
    switch (nextEntryReport) {
    case SENSOR_TEMP:
        reportSensorStatus(SENSOR_TEMP, tempRoom);
        nextEntryReport = SENSOR_HUM;
        break;
    case SENSOR_HUM:
        reportSensorStatus(SENSOR_HUM, humRoom);
        nextEntryReport = 0;
        break;
    default:
        break;
    }
}

void reportSensorStatus(const uint8_t id, const uint8_t value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CURRENT_STATUS_REPORT;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[VALUE_KEY] = value;

    root[SENSORS_KEY] = sens;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    broadcastMsg(json);
}

void reportConfiguration() {
    char ip[16];
    reportSensorCfConfig(SENSOR_TEMP);
    reportSensorCfConfig(SENSOR_HUM);
    reportStringConfig(WIFI_REMOTE_AP_KEY, WIFI_REMOTE_AP);
    reportStringConfig(WIFI_REMOTE_PASSWORD_KEY, WIFI_REMOTE_PW);
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
}

void reportNumberConfig(const char* key, const int value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    serial->println(json);
}

void reportStringConfig(const char* key, const char* value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_SIZE];
    root.printTo(json, JSON_MAX_SIZE);

    serial->println(json);
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

    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(command);

    if (root.success()) {
        const char* msgType = root[MSG_TYPE_KEY];
        if (strcmp(msgType, MSG_CURRENT_STATUS_REPORT) == 0) {
            // CSR
            tsLastStatusReport = tsCurr - STATUS_REPORTING_PERIOD_SEC;
        } else if (strcmp(msgType, MSG_CONFIGURATION) == 0) {
            // CFG
            JsonObject& sensor = root[SENSORS_KEY].asObject();
            if (sensor.containsKey(ID_KEY) && sensor.containsKey(CALIBRATION_FACTOR_KEY)) {
                uint8_t id = sensor[ID_KEY].as<uint8_t>();
                double cf = sensor[CALIBRATION_FACTOR_KEY].as<double>();
                saveSensorCF(id, cf);
            } else if (root.containsKey(WIFI_REMOTE_AP_KEY)) {
                sprintf(WIFI_REMOTE_AP, "%s", root[WIFI_REMOTE_AP_KEY].asString());
                eepromWriteCount += EEPROM.updateBlock(WIFI_REMOTE_AP_EEPROM_ADDR, WIFI_REMOTE_AP,
                        sizeof(WIFI_REMOTE_AP));
            } else if (root.containsKey(WIFI_REMOTE_PASSWORD_KEY)) {
                sprintf(WIFI_REMOTE_PW, "%s", root[WIFI_REMOTE_PASSWORD_KEY].asString());
                eepromWriteCount += EEPROM.updateBlock(WIFI_REMOTE_PW_EEPROM_ADDR, WIFI_REMOTE_PW,
                        sizeof(WIFI_REMOTE_PW));
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
