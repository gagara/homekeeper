#include "Thermostat.h"

#include <aJSON.h>
#include <EEPROMex.h>
#include <RF24.h>

/* ========= Configuration ========= */

// Sensor pin
static const uint8_t SENSOR_ROOM = A0 + 16 * 1;

static const uint8_t HEARTBEAT_LED = 13;

static const int RF_PIPE_EEPROM_ADDR = 0;
static const int SENSORS_FACTORS_EEPROM_ADDR = 1;

// sensor calibration factors
double SENSOR_ROOM_FACTOR = 1.0;

// sensors stats
static const uint8_t SENSORS_RAW_VALUES_MAX_COUNT = 20;
static const uint8_t SENSOR_NOISE_THRESHOLD = 20;

// etc
static const unsigned long MAX_TIMESTAMP = -1;
static const uint8_t MAX_SENSOR_VALUE = 255;

// reporting
static const unsigned long STATUS_REPORTING_PERIOD_MSEC = 60000;
static const unsigned long SENSORS_READ_INTERVAL_MSEC = 6000;

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

static const uint8_t JSON_MAX_WRITE_SIZE = 255;
static const uint8_t JSON_MAX_READ_SIZE = 255;

/* ===== End of Configuration ====== */

/* =========== Variables =========== */

//// Sensors
// Values
uint8_t tempRoom = 0;

// stats
uint8_t rawRoomValues[SENSORS_RAW_VALUES_MAX_COUNT];
uint8_t rawRoomIdx = 0;

uint8_t RF_PIPE;

// Timestamps
unsigned long tsLastStatusReport = 0;
unsigned long tsLastSensorRead = 0;
unsigned long tsPrev = 0;
unsigned long tsCurr = 0;

/* ======== End of Variables ======= */

// RF24
RF24 radio(9, 10);

void setup() {
    Serial.begin(9600);

    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    pinMode(SENSOR_ROOM, INPUT);

    // init sensor raw values arrays
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        rawRoomValues[i] = MAX_SENSOR_VALUE;
    }

    // read sensors calibration factors from EEPROM
    loadSensorCalibrationFactor();

    // read rf pipe
    loadRfPipeAddr();

    // init RF24 radio
    radio.begin();
    radio.enableDynamicPayloads();
    radio.openWritingPipe(RF_PIPE_BASE + RF_PIPE);
    radio.powerUp();
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

        delay(100);
        digitalWrite(HEARTBEAT_LED, LOW);
    }
    if (Serial.available() > 0) {
        processSerialMsg();
    }
    tsPrev = tsCurr;
    delay(SENSORS_READ_INTERVAL_MSEC);
}

void loadSensorCalibrationFactor() {
    SENSOR_ROOM_FACTOR = EEPROM.readDouble(SENSORS_FACTORS_EEPROM_ADDR);
    if (isnan(SENSOR_ROOM_FACTOR) || SENSOR_ROOM_FACTOR < 0.01 || SENSOR_ROOM_FACTOR > 3) {
        SENSOR_ROOM_FACTOR = 1;
    }
}

void writeSensorCalibrationFactor(double value) {
    EEPROM.writeDouble(SENSORS_FACTORS_EEPROM_ADDR, value);
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

void readSensors() {
    readSensor(SENSOR_ROOM, rawRoomValues, rawRoomIdx);
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

uint8_t getTemp(uint8_t sensor) {
    int rawVal = analogRead(sensor);
    double voltage = (5000 / 1024) * rawVal;
    double tempC = voltage * 0.1;
    if (SENSOR_ROOM == sensor) {
        tempC = tempC * SENSOR_ROOM_FACTOR;
    }
    return byte(tempC + 0.5);
}

void refreshSensorValues() {
    double temp1 = 0;
    int j1 = 0;
    for (int i = 0; i < SENSORS_RAW_VALUES_MAX_COUNT; i++) {
        if (rawRoomValues[i] != MAX_SENSOR_VALUE) {
            temp1 += rawRoomValues[i];
            j1++;
        }
    }
    if (j1 > 0) {
        tempRoom = byte(temp1 / j1 + 0.5);
    }
}

void rfWrite(char* msg) {
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
        radio.write(packet, l, 0);
    }
    radio.setPayloadSize(1);
    radio.write("\n", 1, 0);
}

void reportStatus() {
    aJsonObject *root, *sensArr, *sens;
    char* outBuf;

    sensArr = aJson.createArray();

    // SENSOR_SUPPLY;
    sens = aJson.createObject();
    aJson.addNumberToObject(sens, ID_KEY, SENSOR_ROOM);
    aJson.addNumberToObject(sens, VALUE_KEY, tempRoom);
    aJson.addItemToArray(sensArr, sens);

    root = aJson.createObject();
    aJson.addStringToObject(root, MSG_TYPE_KEY, MSG_CURRENT_STATUS_REPORT);
    aJson.addItemToObject(root, SENSORS_KEY, sensArr);

    outBuf = (char*) malloc(JSON_MAX_WRITE_SIZE);
    aJsonStringStream stringStream(NULL, outBuf, JSON_MAX_WRITE_SIZE);
    aJson.print(root, &stringStream);

    // report to serial port
    Serial.println(outBuf);

    // report to RF
    rfWrite(outBuf);

    free(outBuf);
    aJson.deleteItem(root);

    tsLastStatusReport = tsCurr;
}

void reportConfiguration() {
    aJsonObject *root, *sensArr, *sens;
    char* outBuf;

    sensArr = aJson.createArray();

    // SENSOR_ROOM;
    sens = aJson.createObject();
    aJson.addNumberToObject(sens, ID_KEY, SENSOR_ROOM);
    aJson.addNumberToObject(sens, CALIBRATION_FACTOR_KEY, SENSOR_ROOM_FACTOR);
    aJson.addItemToArray(sensArr, sens);

    root = aJson.createObject();
    aJson.addStringToObject(root, MSG_TYPE_KEY, MSG_CONFIGURATION);
    aJson.addItemToObject(root, SENSORS_KEY, sensArr);
    aJson.addNumberToObject(root, RF_PIPE_KEY, RF_PIPE);

    outBuf = (char*) malloc(JSON_MAX_WRITE_SIZE);
    aJsonStringStream stringStream(NULL, outBuf, JSON_MAX_WRITE_SIZE);
    aJson.print(root, &stringStream);

    Serial.println(outBuf);

    free(outBuf);
    aJson.deleteItem(root);
}

void processSerialMsg() {
    aJsonObject *root;
    char* buff = (char*) malloc(JSON_MAX_READ_SIZE);
    String s = Serial.readStringUntil('\0');
    s.toCharArray(buff, JSON_MAX_READ_SIZE, 0);
    root = aJson.parse(buff);
    if (root != NULL) {
        aJsonObject *msgType = aJson.getObjectItem(root, MSG_TYPE_KEY);
        if (msgType != NULL) {
            if (strcmp(msgType->valuestring, MSG_CURRENT_STATUS_REPORT) == 0) {
                reportStatus();
            } else if (strcmp(msgType->valuestring, MSG_CONFIGURATION) == 0) {
                aJsonObject *rfPipeObj = aJson.getObjectItem(root, RF_PIPE_KEY);
                aJsonObject *sensorObj = aJson.getObjectItem(root, SENSORS_KEY);
                if (rfPipeObj != NULL) {
                    uint8_t pipe = rfPipeObj->valueint;
                    writeRfPipeAddr(pipe);
                } else if (sensorObj != NULL) {
                    aJsonObject *idObj = aJson.getObjectItem(sensorObj, ID_KEY);
                    if (idObj != NULL) {
                        aJsonObject *cfObj = aJson.getObjectItem(sensorObj, CALIBRATION_FACTOR_KEY);
                        if (cfObj != NULL) {
                            uint8_t id = idObj->valueint;
                            double cf = cfObj->valuefloat;
                            if (id == SENSOR_ROOM) {
                                writeSensorCalibrationFactor(cf);
                                loadSensorCalibrationFactor();
                            }
                        }
                    }
                } else {
                    reportConfiguration();
                }
            }
        }
    }
    free(buff);
    aJson.deleteItem(root);
}
