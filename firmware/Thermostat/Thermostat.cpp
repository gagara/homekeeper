#include "Thermostat.h"

#include <Arduino.h>
#include <DHT.h>
#include <EEPROMex.h>
#include <MemoryFree.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

/* ========= Configuration ========= */

#undef __DEBUG__

// Sensor pin
static const uint8_t DHT_PIN = 5;
static const uint8_t SENSOR_TEMP = (54 + 16) + (4 * 1) + 0;
static const uint8_t SENSOR_HUM = (54 + 16) + (4 * 1) + 1;

// WiFi pins
static const uint8_t WIFI_RST_PIN = 4;
static const uint8_t WIFI_TX_PIN = 3;
static const uint8_t WIFI_RX_PIN = 2;

static const uint8_t HEARTBEAT_LED = 13;

static const int SENSORS_FACTORS_EEPROM_ADDR = 0; // 2 x 4 bytes
static const int WIFI_REMOTE_AP_EEPROM_ADDR = SENSORS_FACTORS_EEPROM_ADDR + (2 * 4); // 32 bytes
static const int WIFI_REMOTE_PW_EEPROM_ADDR = WIFI_REMOTE_AP_EEPROM_ADDR + 32; // 32 bytes
static const int SERVER_IP_EEPROM_ADDR = WIFI_REMOTE_PW_EEPROM_ADDR + 32; // 4 bytes
static const int SERVER_PORT_EEPROM_ADDR = SERVER_IP_EEPROM_ADDR + 4; // 2 bytes

// sensor calibration factors
double SENSOR_TEMP_FACTOR = 1.0;
double SENSOR_HUM_FACTOR = 1.0;

// sensors stats
static const uint8_t SENSORS_RAW_VALUES_MAX_COUNT = 1;
static const uint8_t SENSOR_NOISE_THRESHOLD = 255; // disabled

// etc
static const unsigned long MAX_TIMESTAMP = -1;
static const uint8_t MAX_SENSOR_VALUE = 255;

// reporting
static const unsigned long STATUS_REPORTING_PERIOD_MSEC = 17000; // 17s //disabled in LowPower mode
static const unsigned long SENSORS_READ_INTERVAL_MSEC = 17000; // 17s //disabled in LowPower mode

// WiFi
static const unsigned long WIFI_MAX_FAILURE_PERIOD_MSEC = 300000; // 5m

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

static const uint8_t JSON_MAX_WRITE_SIZE = 64;
static const uint8_t JSON_MAX_READ_SIZE = 64;
static const uint8_t JSON_MAX_BUFFER_SIZE = 128;

static const uint8_t WIFI_MAX_READ_SIZE = 64;
static const uint16_t WIFI_MAX_WRITE_SIZE = 255;

/* ===== End of Configuration ====== */

/* =========== Variables =========== */

//// Sensors
// Values
uint8_t tempRoom = 0;
uint8_t humRoom = 0;

// stats
uint8_t rawTempValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawHumValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawTempIdx = 0;
uint8_t rawHumIdx = 0;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorRead = 0;
unsigned long tsLastSuccessTransmit = 0;
unsigned long tsLastWifiSuccessTransmission = 0;
unsigned long tsPrev = 0;
unsigned long tsCurr = 0;

// WiFi
char WIFI_REMOTE_AP[32];
char WIFI_REMOTE_PW[32];
uint8_t SERVER_IP[4] = { 0, 0, 0, 0 };
uint16_t SERVER_PORT = 80;
uint8_t IP[4] = { 0, 0, 0, 0 };
char OK[] = "OK";

// do reporting in round robin fashion because central unit
// sometimes unable to accept all report at a time
uint8_t rrFlag = 1;

/* ======== End of Variables ======= */

//WiFi
SoftwareSerial wifi(WIFI_RX_PIN, WIFI_TX_PIN);

// DHT sensor
DHT dht(DHT_PIN, DHT11);

void setup() {
    Serial.begin(9600);
#ifdef __DEBUG__
    Serial.println("STARTING");
#endif
    loadWifiConfig();
    pinMode(WIFI_RST_PIN, OUTPUT);
    wifiInit();             // WiFi
    wifiSetup();
    wifiGetRemoteIP();
#ifdef __DEBUG__
    Serial.print("Remote IP: ");
    char ipStr[16];
    sprintf(ipStr, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
    Serial.println(ipStr);
    Serial.print("free memory: ");
    Serial.println(freeMemory());
#endif
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // init sensor raw values arrays
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        rawTempValues[i] = MAX_SENSOR_VALUE;
        rawHumValues[i] = MAX_SENSOR_VALUE;
    }

    // read sensors calibration factors from EEPROM
    loadSensorsCalibrationFactors();
}

void loop() {
    tsCurr = getTimestamp();
    if (diffTimestamps(tsCurr, tsLastSensorRead) >= SENSORS_READ_INTERVAL_MSEC) {
        readSensors();
        tsLastSensorRead = tsCurr;
    }
    if (diffTimestamps(tsCurr, tsLastStatusReport) >= STATUS_REPORTING_PERIOD_MSEC) {
        digitalWrite(HEARTBEAT_LED, HIGH);

        // refresh sensors values
        refreshSensorValues();

        // report status
        reportStatus();

        digitalWrite(HEARTBEAT_LED, LOW);
#ifdef __DEBUG__
        Serial.print("free memory: ");
        Serial.println(freeMemory());
#endif
    }
    if (Serial.available() > 0) {
        processSerialMsg();
    }
#ifdef __DEBUG__
    if (wifi.available() > 0) {
        Serial.print("wifi msg: ");
        Serial.println(wifi.readStringUntil('\0'));
    }
#endif
    tsPrev = tsCurr;
    // LowPower disabled for now
    //LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    //delay(SENSORS_READ_INTERVAL_MSEC);
}

void loadSensorsCalibrationFactors() {
    SENSOR_TEMP_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 0);
    SENSOR_HUM_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4);
}

double readSensorCalibrationFactor(int offset) {
    double f = EEPROM.readDouble(offset);
    if (isnan(f) || f < 0.01 || f > 3) {
        f = 1;
    }
    return f;
}

void writeSensorCalibrationFactor(double value, int addr) {
    EEPROM.writeDouble(addr, value);
}

unsigned long getTimestamp() {
    return millis();
}

unsigned long diffTimestamps(unsigned long hi, unsigned long lo) {
    if (hi < lo) {
        // overflow
        return (MAX_TIMESTAMP - lo) + hi;
    } else {
        return hi - lo;
    }
}

void readSensors() {
    readSensor(SENSOR_TEMP, rawTempValues, rawTempIdx);
    readSensor(SENSOR_HUM, rawHumValues, rawHumIdx);
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
    val = getSensorValue(id);
    if (values[prevIdx] == MAX_SENSOR_VALUE || abs(val - values[prevIdx]) < SENSOR_NOISE_THRESHOLD) {
        values[idx] = val;
    }
    idx++;
}

uint8_t getSensorValue(uint8_t sensor) {
    float value = 0;
    if (SENSOR_TEMP == sensor) {
        value = dht.readTemperature();
        value = value * SENSOR_TEMP_FACTOR;
    } else if (SENSOR_HUM == sensor) {
        value = dht.readHumidity();
        value = value * SENSOR_HUM_FACTOR;
    }
    return byte(value + 0.5);
}

uint8_t getAnalogSensorValue(uint8_t sensor) {
    int rawVal = analogRead(sensor);
    double voltage = (5000 / 1024) * rawVal;
    double tempC = voltage * 0.1;
    if (SENSOR_TEMP == sensor) {
        tempC = tempC * SENSOR_TEMP_FACTOR;
    } else if (SENSOR_HUM == sensor) {
        tempC = tempC * SENSOR_HUM_FACTOR;
    }
    return byte(tempC + 0.5);
}

void refreshSensorValues() {
    double val1 = 0;
    double val2 = 0;
    int j1 = 0;
    int j2 = 0;
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        if (rawTempValues[i] != MAX_SENSOR_VALUE) {
            val1 += rawTempValues[i];
            j1++;
        }
        if (rawHumValues[i] != MAX_SENSOR_VALUE) {
            val2 += rawHumValues[i];
            j2++;
        }
    }
    if (j1 > 0) {
        tempRoom = byte(val1 / j1 + 0.5);
    }
    if (j2 > 0) {
        humRoom = byte(val2 / j2 + 0.5);
    }
}

void loadWifiConfig() {
    EEPROM.readBlock(WIFI_REMOTE_AP_EEPROM_ADDR, WIFI_REMOTE_AP, sizeof(WIFI_REMOTE_AP));
    EEPROM.readBlock(WIFI_REMOTE_PW_EEPROM_ADDR, WIFI_REMOTE_PW, sizeof(WIFI_REMOTE_PW));
    EEPROM.readBlock(SERVER_IP_EEPROM_ADDR, SERVER_IP, sizeof(SERVER_IP));
    SERVER_PORT = EEPROM.readInt(SERVER_PORT_EEPROM_ADDR);
}

void wifiInit() {
    // hardware reset
#ifdef __DEBUG__
    Serial.println("reboot wifi");
#endif
    digitalWrite(WIFI_RST_PIN, LOW);
    delay(500);
    digitalWrite(WIFI_RST_PIN, HIGH);
    delay(1000);

    wifi.begin(115200);
    wifi.println(F("AT+CIOBAUD=9600"));
    wifi.begin(9600);
    IP[0] = 0;
    IP[1] = 0;
    IP[2] = 0;
    IP[3] = 0;
}

void wifiSetup() {
    char msg[128];
    wifi.println(F("AT+RST"));
    delay(500);
    wifi.println(F("AT+CIPMODE=0"));
    delay(100);
    wifi.println(F("AT+CWMODE_CUR=1"));
#ifdef __DEBUG__
    Serial.println("runing in STATION mode");
#endif
    delay(100);
#ifdef __DEBUG__
    Serial.println("starting TCP server");
#endif
    wifi.println(F("AT+CIPMUX=1"));
    delay(100);
    wifi.println(F("AT+CIPSERVER=1,80"));
    delay(100);
    // join AP
    sprintf(msg, "AT+CWJAP_CUR=\"%s\",\"%s\"", WIFI_REMOTE_AP, WIFI_REMOTE_PW);
#ifdef __DEBUG__
    Serial.print("wifi join AP: ");
    Serial.println(msg);
#endif
    wifi.println(msg);
    delay(100);
    delay(10000);
}

void wifiCheckConnection() {
    if (!wifiGetRemoteIP()
            || (validIP(SERVER_IP)
                    && diffTimestamps(tsCurr, tsLastWifiSuccessTransmission) >= WIFI_MAX_FAILURE_PERIOD_MSEC)) {
        wifiInit();
        wifiSetup();
        wifiGetRemoteIP();
    }
}

bool wifiGetRemoteIP() {
    wifi.println(F("AT+CIFSR"));
    char s[WIFI_MAX_READ_SIZE + 1];
    uint16_t l = wifi.readBytesUntil('\0', s, WIFI_MAX_READ_SIZE);
    s[l] = '\0';
#ifdef __DEBUG__
    Serial.print("wifi: ");
    Serial.println(s);
#endif
    int n = 0;
    int tmp[4];
    char *substr;
    substr = strstr(s, ":STAIP,");
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
        Serial.print("wifi got ip: ");
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

bool wifiSend(const char* msg) {
    if (validIP(IP) && validIP(SERVER_IP)) {
        char body[WIFI_MAX_WRITE_SIZE];
        sprintf(body,
                "POST / HTTP/1.1\r\nUser-Agent: ESP8266\r\nAccept: */*\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s\r\n",
                strlen(msg), msg);

        char connect[64];
        sprintf(connect, "AT+CIPSTART=3,\"TCP\",\"%d.%d.%d.%d\",%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2],
                SERVER_IP[3], SERVER_PORT);

        char send[32];
        sprintf(send, "AT+CIPSEND=3,%d", strlen(body));
        char close[16];
        sprintf(close, "AT+CIPCLOSE=3");
        if (!wifiWrite(connect, "CONNECT", 200, 2))
            return false;

        if (!wifiWrite(send, ">"))
            return false;

        if (!wifiWrite(body, OK, 300, 1))
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
        wifi.println(msg);
        delay(wait);
        if (wifi.find(r)) {
            break;
        }
        a++;
    }
    return a < maxRetry;
}

void reportStatus() {
    // check WiFi connectivity
    wifiCheckConnection();

    if (rrFlag) {
        reportSensorStatus(SENSOR_TEMP, tempRoom);
    } else {
        reportSensorStatus(SENSOR_HUM, humRoom);
    }
    rrFlag = rrFlag ^ 1;

    tsLastStatusReport = tsCurr;
}

void reportSensorStatus(const uint8_t id, const uint8_t value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CURRENT_STATUS_REPORT;

    JsonObject& sens = jsonBuffer.createObject();
    sens[ID_KEY] = id;
    sens[VALUE_KEY] = value;

    root[SENSORS_KEY] = sens;

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
    reportSensorConfig(SENSOR_TEMP, SENSOR_TEMP_FACTOR);
    reportSensorConfig(SENSOR_HUM, SENSOR_HUM_FACTOR);
    reportStringConfig(WIFI_REMOTE_AP_KEY, WIFI_REMOTE_AP);
    reportStringConfig(WIFI_REMOTE_PASSWORD_KEY, WIFI_REMOTE_PW);
    char sIP[16];
    sprintf(sIP, "%d.%d.%d.%d", SERVER_IP[0], SERVER_IP[1], SERVER_IP[2], SERVER_IP[3]);
    reportStringConfig(SERVER_IP_KEY, sIP);
    reportNumberConfig(SERVER_PORT_KEY, SERVER_PORT);
    char lIP[16];
    sprintf(lIP, "%d.%d.%d.%d", IP[0], IP[1], IP[2], IP[3]);
    reportStringConfig(LOCAL_IP_KEY, lIP);
#ifdef __DEBUG__
    Serial.print("reportConfiguration: free memory: ");
    Serial.println(freeMemory());
#endif
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

    Serial.println(json);
}

void reportNumberConfig(const char* key, const int value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

    Serial.println(json);
}

void reportStringConfig(const char* key, const char* value) {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CONFIGURATION;
    root[key] = value;

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

    Serial.println(json);
}

void broadcastMsg(const char* msg) {
    Serial.println(msg);
    bool r = wifiSend(msg);
    if (r) {
        tsLastWifiSuccessTransmission = tsCurr;
#ifdef __DEBUG__
        Serial.println("wifi send: OK");
#endif
    } else {
#ifdef __DEBUG__
        Serial.println("wifi send: FAILED");
#endif
    }
}

void processSerialMsg() {
    char buff[JSON_MAX_READ_SIZE + 1];
    uint16_t l = Serial.readBytesUntil('\0', buff, JSON_MAX_READ_SIZE);
    buff[l] = '\0';
    parseCommand(buff);
}

void parseCommand(char* command) {
#ifdef __DEBUG__
    Serial.print("parsing cmd: ");
    Serial.println(command);
    if (strstr(command, "AT") == command) {
        // this is AT command for ESP8266
        Serial.print("sending to wifi: ");
        Serial.println(command);
        wifi.println(command);
        return;
    }
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
        if (strcmp(msgType, MSG_CURRENT_STATUS_REPORT) == 0) {
            // CSR
            // workaround to prevent OOM
            //reportStatus();
            tsLastStatusReport = tsCurr - STATUS_REPORTING_PERIOD_MSEC;
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
                    SENSOR_TEMP_FACTOR = readSensorCalibrationFactor(SENSORS_FACTORS_EEPROM_ADDR + 4);
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
    }
#ifdef __DEBUG__
    Serial.print("processSerialMsg: free memory: ");
    Serial.println(freeMemory());
#endif
}
