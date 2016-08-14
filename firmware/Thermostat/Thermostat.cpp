#include "Thermostat.h"

#include <ArduinoJson.h>
#include <DHT.h>
#include <EEPROMex.h>
#include <MemoryFree.h>
#include <RF24.h>
#include <LowPower.h>

/* ========= Configuration ========= */

#undef __DEBUG__

// Sensor pin
static const uint8_t DHT_PIN = 2;
static const uint8_t SENSOR_TEMP = (54 + 16) + (4 * 1) + 0;
static const uint8_t SENSOR_HUM = (54 + 16) + (4 * 1) + 1;

static const uint8_t HEARTBEAT_LED = 13;

static const int RF_PIPE_EEPROM_ADDR = 0;
static const int SENSORS_FACTORS_EEPROM_ADDR = 1;

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
static const unsigned long STATUS_REPORTING_PERIOD_MSEC = 0; // 15 seconds. disabled in LowPower mode
static const unsigned long SENSORS_READ_INTERVAL_MSEC = 0; // disabled in LowPower mode
static const unsigned long RADIO_MAX_INACTIVE_PERION_MSEC = 60000; // 10 minutes

// RF
static const uint64_t RF_PIPE_BASE = 0xE8E8F0F0A2LL;
static const uint8_t RF_PACKET_LENGTH = 32;

// JSON
static const char MSG_TYPE_KEY[] = "m";
static const char ID_KEY[] = "id";
static const char SENSORS_KEY[] = "s";
static const char RF_PIPE_KEY[] = "rfp";
static const char VALUE_KEY[] = "v";
static const char CALIBRATION_FACTOR_KEY[] = "cf";

const char MSG_CURRENT_STATUS_REPORT[] = "csr";
const char MSG_CONFIGURATION[] = "cfg";

static const uint8_t JSON_MAX_BUFFER_SIZE = 255;
static const uint8_t JSON_MAX_WRITE_SIZE = 128;

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

uint8_t RF_PIPE;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorRead = 0;
unsigned long tsLastSuccessTransmit = 0;
unsigned long tsPrev = 0;
unsigned long tsCurr = 0;

/* ======== End of Variables ======= */

// RF24
RF24 radio(9, 10);

// DHT sensor
DHT dht(DHT_PIN, DHT11);

void setup() {
    Serial.begin(9600);
#ifdef __DEBUG__
    Serial.println("STARTING");
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
    loadSensorCalibrationFactor();

    // read rf pipe
    loadRfPipeAddr();

    // init RF24 radio
    initRF24();
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

        if (diffTimestamps(tsCurr, tsLastSuccessTransmit) >= RADIO_MAX_INACTIVE_PERION_MSEC) {
            radio.powerDown();
            initRF24();
        }

        // report status
        reportStatus();

        delay(100);
        digitalWrite(HEARTBEAT_LED, LOW);
#ifdef __DEBUG__
        Serial.print("free memory: ");
        Serial.println(freeMemory());
#endif
    }
    if (Serial.available() > 0) {
        processSerialMsg();
    }
    tsPrev = tsCurr;
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
//    delay(SENSORS_READ_INTERVAL_MSEC);
}

void loadSensorCalibrationFactor() {
    SENSOR_TEMP_FACTOR = EEPROM.readDouble(SENSORS_FACTORS_EEPROM_ADDR + 0);
    SENSOR_HUM_FACTOR = EEPROM.readDouble(SENSORS_FACTORS_EEPROM_ADDR + 4);
    if (isnan(SENSOR_TEMP_FACTOR) || SENSOR_TEMP_FACTOR < 0.01 || SENSOR_TEMP_FACTOR > 3) {
        SENSOR_TEMP_FACTOR = 1;
    }
    if (isnan(SENSOR_HUM_FACTOR) || SENSOR_HUM_FACTOR < 0.01 || SENSOR_HUM_FACTOR > 3) {
        SENSOR_HUM_FACTOR = 1;
    }
}

void writeSensorCalibrationFactor(double value, int addr) {
    EEPROM.writeDouble(addr, value);
}

void loadRfPipeAddr() {
    RF_PIPE = EEPROM.readByte(RF_PIPE_EEPROM_ADDR);
}

void writeRfPipeAddr(uint8_t value) {
    EEPROM.writeByte(RF_PIPE_EEPROM_ADDR, value);
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

void initRF24() {
    radio.begin();
    radio.enableDynamicPayloads();
    radio.openWritingPipe(RF_PIPE_BASE + RF_PIPE);
    radio.powerUp();
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

bool rfWrite(char* msg) {
    uint8_t len = strlen(msg);
    for (uint8_t i = 0; i * (RF_PACKET_LENGTH - 1) < len; i++) {
        char packet[RF_PACKET_LENGTH];
        uint8_t l = 0;
        for (uint8_t j = 0; j < RF_PACKET_LENGTH - 1; j++) {
            if (((i * (RF_PACKET_LENGTH - 1)) + j) < len) {
                packet[l++] = msg[((i * (RF_PACKET_LENGTH - 1)) + j)];
            } else {
                break;
            }
        }
        packet[l] = '\0';
        radio.setPayloadSize(l);
        if (!radio.write(packet, l, 0)) {
            return false;
        }
    }
    radio.setPayloadSize(1);
    return radio.write("\n", 1, 0);
}

void reportStatus() {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root[MSG_TYPE_KEY] = MSG_CURRENT_STATUS_REPORT;

    // Temp
    JsonObject& temp = jsonBuffer.createObject();
    temp[ID_KEY] = SENSOR_TEMP;
    temp[VALUE_KEY] = tempRoom;

    // Temp
    JsonObject& hum = jsonBuffer.createObject();
    hum[ID_KEY] = SENSOR_HUM;
    hum[VALUE_KEY] = humRoom;

    JsonArray& sensors = root.createNestedArray(SENSORS_KEY);
    sensors.add(temp);
    sensors.add(hum);

    char json[JSON_MAX_WRITE_SIZE];
    root.printTo(json, JSON_MAX_WRITE_SIZE);

    // report to serial port
    Serial.println(json);

    // report to RF
    if (rfWrite(json)) {
        tsLastSuccessTransmit = tsCurr;
    }

    tsLastStatusReport = tsCurr;
#ifdef __DEBUG__
    Serial.print("reportStatus: free memory: ");
    Serial.println(freeMemory());
#endif
}

void reportConfiguration() {
    reportSensorConfig(SENSOR_TEMP, SENSOR_TEMP_FACTOR);
    reportSensorConfig(SENSOR_HUM, SENSOR_HUM_FACTOR);
    reportNumberConfig(RF_PIPE_KEY, RF_PIPE);
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

void processSerialMsg() {
    StaticJsonBuffer<JSON_MAX_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(Serial.readStringUntil('\0'));
    if (root.success()) {
        const char* msgType = root[MSG_TYPE_KEY];
        if (strcmp(msgType, MSG_CURRENT_STATUS_REPORT) == 0) {
            // CSR
            reportStatus();
        } else if (strcmp(msgType, MSG_CONFIGURATION) == 0) {
            // CFG
            if (root.containsKey(RF_PIPE_KEY)) {
                writeRfPipeAddr(root[RF_PIPE_KEY].as<uint8_t>());
            } else if (root.containsKey(SENSORS_KEY)) {
                JsonObject& sensor = root[SENSORS_KEY].asObject();
                uint8_t id = sensor[ID_KEY];
                double value = sensor[CALIBRATION_FACTOR_KEY];
                if (id == SENSOR_TEMP) {
                    writeSensorCalibrationFactor(value, SENSORS_FACTORS_EEPROM_ADDR + 0);
                    loadSensorCalibrationFactor();
                } else if (id == SENSOR_HUM) {
                    writeSensorCalibrationFactor(value, SENSORS_FACTORS_EEPROM_ADDR + 4);
                    loadSensorCalibrationFactor();
                }
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
