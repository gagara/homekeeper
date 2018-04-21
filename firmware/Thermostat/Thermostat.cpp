#include "Thermostat.h"

#include <Arduino.h>
#include <DHT.h>
#include <EEPROMex.h>
#include <MemoryFree.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#include <debug.h>
#include <ESP8266.h>

#define __DEBUG__

/*============================= Global configuration ========================*/

// Sensor pin
static const uint8_t DHT_PIN = 5;
static const uint8_t SENSOR_TEMP = (54 + 16) + (4 * 1) + 0;
static const uint8_t SENSOR_HUM = (54 + 16) + (4 * 1) + 1;

// WiFi pins
static const uint8_t WIFI_RST_PIN = 0; // n/a

// Debug serial port
static const uint8_t DEBUG_SERIAL_TX_PIN = 3;
static const uint8_t DEBUG_SERIAL_RX_PIN = 2;

static const uint8_t HEARTBEAT_LED = 13;

static const int SENSORS_FACTORS_EEPROM_ADDR = 0; // 2 x 4 bytes
static const int WIFI_REMOTE_AP_EEPROM_ADDR = SENSORS_FACTORS_EEPROM_ADDR + (2 * 4); // 32 bytes
static const int WIFI_REMOTE_PW_EEPROM_ADDR = WIFI_REMOTE_AP_EEPROM_ADDR + 32; // 32 bytes
static const int SERVER_IP_EEPROM_ADDR = WIFI_REMOTE_PW_EEPROM_ADDR + 32; // 4 bytes
static const int SERVER_PORT_EEPROM_ADDR = SERVER_IP_EEPROM_ADDR + 4; // 2 bytes

// sensor calibration factors
double SENSOR_TEMP_FACTOR = 1.0;
double SENSOR_HUM_FACTOR = 1.0;

// etc
static const unsigned long MAX_TIMESTAMP = -1;
static const int16_t UNKNOWN_SENSOR_VALUE = -127;

// reporting
static const unsigned long STATUS_REPORTING_PERIOD_SEC = 15; // 15s //disabled in LowPower mode
static const unsigned long SENSORS_READ_INTERVAL_SEC = 15; // 15s //disabled in LowPower mode

// JSON
static const char MSG_TYPE_KEY[] = "m";
static const char ID_KEY[] = "id";
static const char SENSORS_KEY[] = "s";
static const char VALUE_KEY[] = "v";
static const char CALIBRATION_FACTOR_KEY[] = "cf";

static const char MSG_CURRENT_STATUS_REPORT[] = "csr";
static const char MSG_CONFIGURATION[] = "cfg";
static const char WIFI_REMOTE_AP_KEY[] = "rap";
static const char WIFI_REMOTE_PASSWORD_KEY[] = "rpw";
static const char SERVER_IP_KEY[] = "sip";
static const char SERVER_PORT_KEY[] = "sp";
static const char LOCAL_IP_KEY[] = "lip";

static const uint8_t JSON_MAX_SIZE = 64;
static const uint8_t JSON_MAX_BUFFER_SIZE = 128;

/*============================= Global variables ============================*/

//// Sensors
// Values
uint8_t tempRoom = 0;
uint8_t humRoom = 0;

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
    serial->begin(9600);
    wifi->begin(115200);

    dbg(debug, F("STARTING\n"));

    // init heartbeat led
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // init esp8266 hw reset pin. N/A
    //pinMode(WIFI_RST_PIN, OUTPUT);

    // read sensors calibration factors from EEPROM
    loadSensorsCalibrationFactors();

    // setup WiFi
    loadWifiConfig();
    esp8266.init(wifi, MODE_STA_AP, WIFI_RST_PIN);
    esp8266.connect(&WIFI_REMOTE_AP, &WIFI_REMOTE_PW);
    delay(2000);
    esp8266.readStaIp(WIFI_STA_IP);
    dbgf(debug, F("STA IP: %d.%d.%d.%d"), WIFI_STA_IP[0], WIFI_STA_IP[1], WIFI_STA_IP[2], WIFI_STA_IP[3]);

    dbgf(debug, F("%lu: freeMem: %d\n"), getTimestamp(), freeMemory());
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

void loadSensorsCalibrationFactors() {
    SENSOR_TEMP_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 0);
    SENSOR_HUM_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4);
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
        result = dht.readTemperature() * SENSOR_TEMP_FACTOR;
    } else if (SENSOR_HUM == sensor) {
        result = dht.readHumidity() * SENSOR_HUM_FACTOR;
    }
    result = (result > 0) ? result + 0.5 : result - 0.5;
    return int16_t(result);
}

//void wifiCheckConnection() {
//    if (!wifiGetRemoteIP()
//            || (validIP(SERVER_IP)
//                    && diffTimestamps(tsCurr, tsLastWifiSuccessTransmission) >= WIFI_MAX_FAILURE_PERIOD_MSEC)) {
//        wifiInit();
//        wifiSetup();
//        wifiGetRemoteIP();
//        tsLastWifiSuccessTransmission = tsCurr;
//    }
//}
//
//void wifiInit() {
//    // hardware reset
//#ifdef __DEBUG__
//    Serial.println(F("wifi init"));
//#endif
//    digitalWrite(WIFI_RST_PIN, LOW);
//    delay(500);
//    digitalWrite(WIFI_RST_PIN, HIGH);
//    delay(1000);
//
//    wifi.end();
//    wifi.begin(115200);
//    wifi.println(F("AT+CIOBAUD=9600"));
//    wifi.end();
//    wifi.begin(9600);
//    IP[0] = 0;
//    IP[1] = 0;
//    IP[2] = 0;
//    IP[3] = 0;
//}
//
//void wifiSetup() {
//#ifdef __DEBUG__
//    Serial.println(F("wifi setup"));
//#endif
//    char buff[WIFI_MAX_BUFFER_SIZE + 1];
//    wifi.println(F("AT+RST"));
//    delay(500);
//    wifi.println(F("AT+CIPMODE=0"));
//    delay(100);
//    wifi.println(F("AT+CWMODE_CUR=1"));
//#ifdef __DEBUG__
//    Serial.println(F("runing in STATION mode"));
//#endif
//    delay(100);
//#ifdef __DEBUG__
//    Serial.println(F("starting TCP server"));
//#endif
//    wifi.println(F("AT+CIPMUX=1"));
//    delay(100);
//    wifi.println(F("AT+CIPSERVER=1,80"));
//    delay(100);
//    wifi.println(F("AT+CIPSTO=5"));
//    delay(100);
//    if (strlen(WIFI_REMOTE_AP) + strlen(WIFI_REMOTE_PW) > 0) {
//        // join AP
//        sprintf(buff, "AT+CWJAP_CUR=\"%s\",\"%s\"", WIFI_REMOTE_AP, WIFI_REMOTE_PW);
//#ifdef __DEBUG__
//        Serial.print(F("wifi join AP: "));
//        Serial.println(buff);
//#endif
//        wifi.println(buff);
//        delay(10000);
//    } else {
//#ifdef __DEBUG__
//        Serial.println(F("no AP configured"));
//#endif
//    }
//    uint16_t l = wifi.readBytesUntil('\0', buff, WIFI_MAX_BUFFER_SIZE);
//    buff[l] = '\0';
//#ifdef __DEBUG__
//    Serial.print(F("wifi setup rsp: "));
//    Serial.println(buff);
//#endif
//}
//
//bool wifiGetRemoteIP() {
//    wifi.println(F("AT+CIFSR"));
//    char buff[WIFI_MAX_AT_CMD_SIZE + 1];
//    uint16_t l = wifi.readBytesUntil('\0', buff, WIFI_MAX_AT_CMD_SIZE);
//    buff[l] = '\0';
//#ifdef __DEBUG__
//    Serial.print(F("free memory: "));
//    Serial.println(freeMemory());
//    Serial.print(F("wifi rq ip: "));
//    Serial.println(buff);
//#endif
//    int n = 0;
//    int tmp[4];
//    char *substr;
//    substr = strstr(buff, ":STAIP,");
//    if (substr != NULL) {
//        n += sscanf(substr, ":STAIP,\"%d.%d.%d.%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
//    }
//    if (n == 4) {
//        IP[0] = tmp[0];
//        IP[1] = tmp[1];
//        IP[2] = tmp[2];
//        IP[3] = tmp[3];
//#ifdef __DEBUG__
//        char ip[16];
//        sprintf(ip, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
//        Serial.print(F("wifi got ip: "));
//        Serial.println(ip);
//#endif
//        return validIP(IP);
//    }
//    return false;
//}
//
//bool validIP(uint8_t ip[4]) {
//    int sum = ip[0] + ip[1] + ip[2] + ip[3];
//    return sum > 0 && sum < 255 * 4;
//}
//
//bool wifiSend(const char* msg) {
//    if (validIP(IP) && validIP(SERVER_IP)) {
//        char buff[WIFI_MAX_BUFFER_SIZE + 1];
//        char atcmd[WIFI_MAX_AT_CMD_SIZE];
//
//        sprintf(buff,
//                "POST / HTTP/1.1\r\nUser-Agent: ESP8266\r\nAccept: */*\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s\r\n",
//                strlen(msg), msg);
//
//        sprintf(atcmd, "AT+CIPSTART=3,\"TCP\",\"%d.%d.%d.%d\",%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2],
//                SERVER_IP[3], SERVER_PORT);
//        if (!wifiWrite(atcmd, "CONNECT", 300, 3))
//            return false;
//
//        sprintf(atcmd, "AT+CIPSEND=3,%d", strlen(buff));
//        if (!wifiWrite(atcmd, ">", 300, 3))
//            return false;
//
//        if (!wifiWrite(buff, OK, 300, 1))
//            return false;
//
//        delay(2000);
//        bool rsp = false;
//        if (wifi.available()) {
//            uint16_t l = wifi.readBytesUntil('\0', buff, WIFI_MAX_BUFFER_SIZE);
//            buff[l] = '\0';
//#ifdef __DEBUG__
//            Serial.print(F("wifi rsp len: "));
//            Serial.println(l);
//            Serial.print(F("wifi rsp: "));
//            Serial.println(buff);
//#endif
//            if (strstr(buff, "OK") != NULL) {
//                rsp = true;
//            }
//        }
//        sprintf(atcmd, "AT+CIPCLOSE=3");
//        wifiWrite(atcmd, OK, 100, 1);
//        return rsp;
//    } else {
//        return false;
//    }
//}
//
//bool wifiWrite(const char* msg, const char* rsp, const int wait, const uint8_t maxRetry) {
//    char r[strlen(rsp) + 1];
//    strcpy(r, rsp);
//    r[strlen(rsp)] = '\0';
//    byte a = 0;
//    while (a < maxRetry) {
//        wifi.println(msg);
//        delay(wait);
//        if (wifi.find(r)) {
//            break;
//        }
//        a++;
//    }
//    return a < maxRetry;
//}

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
    reportSensorCalibrationFactor(SENSOR_TEMP, SENSOR_TEMP_FACTOR);
    reportSensorCalibrationFactor(SENSOR_HUM, SENSOR_HUM_FACTOR);
    reportStringConfig(WIFI_REMOTE_AP_KEY, WIFI_REMOTE_AP);
    reportStringConfig(WIFI_REMOTE_PASSWORD_KEY, WIFI_REMOTE_PW);
    sprintf(ip, "%d.%d.%d.%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2], SERVER_IP[3]);
    reportStringConfig(SERVER_IP_KEY, ip);
    reportNumberConfig(SERVER_PORT_KEY, SERVER_PORT);
    sprintf(ip, "%d.%d.%d.%d", WIFI_STA_IP[0], WIFI_STA_IP[1], WIFI_STA_IP[2], WIFI_STA_IP[3]);
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
    esp8266.receive(buff, JSON_MAX_SIZE);
    parseCommand(buff);
}

void broadcastMsg(const char* msg) {
    serial->println(msg);
    esp8266.send(SERVER_IP, SERVER_PORT, msg);
}

bool parseCommand(char* command) {
    dbgf(debug, F("parsing cmd: %s\n"), command);

    if (strstr(command, "AT") == command) {
        // this is AT command for ESP8266
        dbgf(debug, F("sending to ESP8266: %s\n"), command);
        esp8266.write(command);
        return true;
    }

    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(command);

    dbg(debug, F("parsed\n"));

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
                if (id == SENSOR_TEMP) {
                    writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 0, cf);
                    SENSOR_TEMP_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 0);
                } else if (id == SENSOR_HUM) {
                    writeSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4, cf);
                    SENSOR_HUM_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4);
                }
            } else if (root.containsKey(WIFI_REMOTE_AP_KEY)) {
                sprintf(WIFI_REMOTE_AP, "%s", root[WIFI_REMOTE_AP_KEY].asString());
                EEPROM.writeBlock(WIFI_REMOTE_AP_EEPROM_ADDR, WIFI_REMOTE_AP, sizeof(WIFI_REMOTE_AP));
            } else if (root.containsKey(WIFI_REMOTE_PASSWORD_KEY)) {
                sprintf(WIFI_REMOTE_PW, "%s", root[WIFI_REMOTE_PASSWORD_KEY].asString());
                EEPROM.writeBlock(WIFI_REMOTE_PW_EEPROM_ADDR, WIFI_REMOTE_PW, sizeof(WIFI_REMOTE_PW));
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
