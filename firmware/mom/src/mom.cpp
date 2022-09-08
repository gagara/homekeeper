#include "mom.h"

#include <Arduino.h>
#include <DHT.h>
#include <EEPROMex.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <Stepper.h>
#include <Servo.h>
#include <EmonLib.h>

#include <debug.h>
#include <ESP8266.h>
#include <jsoner.h>

#define __DEBUG__

/*============================= Global configuration ========================*/

// Sensor pin
const uint8_t DHT_IN_PIN = 6;
const uint8_t DHT_OUT_PIN = 7;
const uint8_t SENSOR_CURRENT_METER_PIN = A0;
const uint8_t SENSOR_VOLTAGE_METER_PIN = A1;
const uint8_t SENSOR_WATER_PUMP_POWER_PIN = A2;

const uint8_t SENSOR_TEMP_IN = (54 + 16) + (4 * 2) + 0;
const uint8_t SENSOR_HUM_IN = (54 + 16) + (4 * 2) + 1;
const uint8_t SENSOR_TEMP_OUT = (54 + 16) + (4 * 2) + 2;
const uint8_t SENSOR_HUM_OUT = (54 + 16) + (4 * 2) + 3;
const uint8_t SENSOR_UAC = (54 + 16) + (4 * 3) + 0;
const uint8_t SENSOR_IAC = (54 + 16) + (4 * 3) + 1;
const uint8_t SENSOR_PAC = (54 + 16) + (4 * 3) + 2;
const uint8_t SENSOR_EAC = (54 + 16) + (4 * 3) + 3;
const uint8_t SENSOR_WATER_PUMP_POWER = (54 + 16) + (4 * 4) + 0;

// Node id
const uint8_t NODE_VENTILATION = 44;
const uint8_t NODE_PV_LOAD_SWITCH = 46; // 0 - Off-grid; 1 - On-grid

// Stepper pins
const uint8_t MOTOR_PIN_1 = 8; // blue
const uint8_t MOTOR_PIN_2 = 9;
const uint8_t MOTOR_PIN_3 = 10;
const uint8_t MOTOR_PIN_4 = 11;
const uint8_t MOTOR_PIN_5 = 12; // red
const uint8_t MOTOR_STEPS = 32;
const uint16_t MOTOR_STEPS_PER_REVOLUTION = 2048;
const uint8_t REVOLUTION_COUNT = 15; // max 15

// Servo pins
const uint8_t PV_LOAD_SWITCH_ON_GRID_PIN = 5;
const uint8_t PV_LOAD_SWITCH_OFF_GRID_PIN = 4;
const uint8_t PV_LOAD_SENSOR_ON_GRID_PIN = 3;
const uint8_t PV_LOAD_SENSOR_OFF_GRID_PIN = 2;

// WiFi pins
const uint8_t WIFI_RST_PIN = 0; // n/a

// Nodes bits
const uint16_t NODE_VENTILATION_BIT     = 0b0000000000000001;
const uint16_t NODE_PV_LOAD_SWITCH_BIT  = 0b0000000000000010;
// last bit reseverd for errors
const uint16_t NODE_ERROR_BIT           = 0b1000000000000000;

const uint8_t HEARTBEAT_LED = 13;

// etc
const unsigned long MAX_TIMESTAMP = -1;
const int16_t UNKNOWN_SENSOR_VALUE = -127;
const unsigned long NODE_SWITCH_SAFE_TIME_SEC = 30;
const uint8_t NODE_ERROR_STATE = 2;

// reporting
const unsigned long STATUS_REPORTING_PERIOD_SEC = 60; // 1 minute
const unsigned long SENSORS_READ_INTERVAL_SEC = 10; // 10 seconds
const uint16_t WIFI_FAILURE_GRACE_PERIOD_SEC = 180; // 3 minutes

// ventilation
const int8_t TEMP_IN_OUT_HIST = 3;

// sensor WaterPumpPower
const int8_t SENSOR_WATER_PUMP_POWER_THERSHOLD = 50;

// min grid voltage
const uint8_t MIN_UAC_VALUE = 160;

// Servo positions
const uint8_t SERVO_POSITION_OFF = 0;
const uint8_t SERVO_POSITION_ON = 90;

const uint8_t JSON_MAX_SIZE = 64;

/*============================= Global variables ============================*/

//// Sensors
// Values
int8_t tempIn = 0;
int8_t humIn = 0;
int8_t tempOut = 0;
int8_t humOut = 0;
double uac = 0; // V
double iac = 0; // A
double pac = 0; // W
double eac = 0; // Wh
int8_t sensorWaterPumpPowerState = 0;

//// Nodes

// Nodes actual state
uint16_t NODE_STATE_FLAGS = 0;
// Nodes errors
uint16_t NODE_ERROR_FLAGS = 0;
// Nodes forced mode
uint16_t NODE_FORCED_MODE_FLAGS = 0;
// will be stored in EEPROM
uint16_t NODE_PERMANENTLY_FORCED_MODE_FLAGS = 0;

// Stepper moves
int motorCurrentMove = 0;
int motorNextMove = 0;

// Servo switch error timestamp
unsigned long tsNodePvLoadSwitchError = 0;

// Nodes switch timestamps
unsigned long tsNodeVentilation = 0;
unsigned long tsNodePvLoadSwitch = 0;

// Nodes forced mode timestamps
unsigned long tsForcedNodeVentilation = 0;
unsigned long tsForcedNodePvLoadSwitch = 0;

// Sensors switch timestamps
unsigned long tsSensorWaterPumpPower = 0;

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
uint16_t TCP_SERVER_PORT = 8084;

// EEPROM addresses
const int NODE_STATE_FLAGS_EEPROM_ADDR = 0;
const int NODE_FORCED_MODE_FLAGS_EEPROM_ADDR = NODE_STATE_FLAGS_EEPROM_ADDR + sizeof(NODE_STATE_FLAGS);
const int WIFI_REMOTE_AP_EEPROM_ADDR = NODE_FORCED_MODE_FLAGS_EEPROM_ADDR + sizeof(NODE_FORCED_MODE_FLAGS);
const int WIFI_REMOTE_PW_EEPROM_ADDR = WIFI_REMOTE_AP_EEPROM_ADDR + sizeof(WIFI_REMOTE_AP);
const int SERVER_IP_EEPROM_ADDR = WIFI_REMOTE_PW_EEPROM_ADDR + sizeof(WIFI_REMOTE_PW);
const int SERVER_PORT_EEPROM_ADDR = SERVER_IP_EEPROM_ADDR + sizeof(SERVER_IP);

int eepromWriteCount = 0;

/*============================= Connectivity ================================*/

// DHT sensors
DHT dhtIn(DHT_IN_PIN, DHT11);
DHT dhtOut(DHT_OUT_PIN, DHT22);

// Energy monitor
EnergyMonitor emon;

// Stepper
Stepper motor(MOTOR_STEPS, MOTOR_PIN_1, MOTOR_PIN_3, MOTOR_PIN_2, MOTOR_PIN_4);

// Servo
Servo servo;

// Serial port
HardwareSerial *serial = &Serial;
#ifdef __DEBUG__
HardwareSerial *debug = serial;
#else
HardwareSerial *debug = NULL;
#endif

//WiFi
HardwareSerial *wifi = &Serial3;
ESP8266 esp8266;

void setup() {
    // Setup serial ports
    serial->begin(57600);
    //wifi->begin(57600); // initialized in ESP8266 lib

    dbg(debug, F(":STARTING\n"));

    // init heartbeat led
    pinMode(HEARTBEAT_LED, OUTPUT);
    digitalWrite(HEARTBEAT_LED, LOW);

    // init water pump power sensor
    pinMode(SENSOR_WATER_PUMP_POWER_PIN, INPUT);

    // init PV load switches
    pinMode(PV_LOAD_SENSOR_ON_GRID_PIN, INPUT);
    pinMode(PV_LOAD_SENSOR_OFF_GRID_PIN, INPUT);

    // init energy meter
    emon.current(SENSOR_CURRENT_METER_PIN, 42.00); // 47 Ohm - 42.55
    emon.voltage(SENSOR_VOLTAGE_METER_PIN, 195, 1.7); // ~ 1024000 / Vcc

    // init motor
    pinMode(MOTOR_PIN_5, OUTPUT);
    digitalWrite(MOTOR_PIN_5, HIGH);
    motor.setSpeed(800);
    // default: closed state
    motorCurrentMove = MOTOR_STEPS_PER_REVOLUTION * REVOLUTION_COUNT;

    // restore forced node state flags from EEPROM
    // default node state -- OFF
    restoreNodesState();

    // open ventilation valve if needed
    if (NODE_STATE_FLAGS & NODE_VENTILATION_BIT) {
        motorNextMove = -MOTOR_STEPS_PER_REVOLUTION * REVOLUTION_COUNT;
    }

    // sync PV switches in according to desired node state
    syncPvLoadSwitches();

    // init esp8266 hw reset pin. N/A
    //pinMode(WIFI_RST_PIN, OUTPUT);

    // setup WiFi
    loadWifiConfig();
    dbgf(debug, F(":setup wifi:R_AP:%s\n"), &WIFI_REMOTE_AP);
    esp8266.init(wifi, MODE_STA, WIFI_RST_PIN, WIFI_FAILURE_GRACE_PERIOD_SEC);
    esp8266.connect(&WIFI_REMOTE_AP, &WIFI_REMOTE_PW);
    esp8266.startTcpServer(TCP_SERVER_PORT);
    esp8266.getStaIP(WIFI_STA_IP);
    dbgf(debug, F("STA IP: %d.%d.%d.%d\n"), WIFI_STA_IP[0], WIFI_STA_IP[1], WIFI_STA_IP[2], WIFI_STA_IP[3]);
}

void loop() {
    tsCurr = getTimestamp();
    if (diffTimestamps(tsCurr, tsLastSensorRead) >= SENSORS_READ_INTERVAL_SEC) {
        readSensors();
        tsLastSensorRead = tsCurr;

        // pv load switch
        processPvLoadSwitch();
        // ventilation valve
        processVentilationValve();
        
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

    stepMotor();

    tsPrev = tsCurr;
}

/*========================= Node processing methods =========================*/

void processPvLoadSwitch() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_PV_LOAD_SWITCH_BIT;
    if (isInForcedMode(NODE_PV_LOAD_SWITCH_BIT, tsForcedNodePvLoadSwitch)) {
        // retry sync in case of error
        if (NODE_ERROR_FLAGS & NODE_PV_LOAD_SWITCH_BIT) {
            if (diffTimestamps(tsCurr, tsNodePvLoadSwitchError) >= NODE_SWITCH_SAFE_TIME_SEC) {
                syncPvLoadSwitches();
            }
        }
        return;
    }
    if (wasForceMode) {
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_PV_LOAD_SWITCH, nodeState(NODE_PV_LOAD_SWITCH_BIT), tsNodePvLoadSwitch,
                          NODE_FORCED_MODE_FLAGS & NODE_PV_LOAD_SWITCH_BIT, tsForcedNodePvLoadSwitch, json,
                          JSON_MAX_SIZE);
        broadcastMsg(json);
    }
    if (diffTimestamps(tsCurr, tsNodePvLoadSwitch) >= NODE_SWITCH_SAFE_TIME_SEC) {
      uint8_t sensIds[] = {};
      int16_t sensVals[] = {};
      uint8_t sensCnt = 0;

      if (NODE_STATE_FLAGS & NODE_PV_LOAD_SWITCH_BIT) {
        // on-grid inverter
        if (uac <= MIN_UAC_VALUE) {
          // grid voltage is too low/no grid available
          // switch to off-grid inverter
          switchNodeState(NODE_PV_LOAD_SWITCH, sensIds, sensVals, sensCnt);
        } else {
          // grid voltage is OK
          // do nothing
        }
      } else {
        // off-grid inverter
        if (uac > MIN_UAC_VALUE) {
          // grid voltage is OK
          // switch to on-grid inverter
          switchNodeState(NODE_PV_LOAD_SWITCH, sensIds, sensVals, sensCnt);
        } else {
          // grid voltage is too low/no grid available
          // do nothing
        }
      }
    }
    // retry sync in case of error
    if (NODE_ERROR_FLAGS & NODE_PV_LOAD_SWITCH_BIT) {
        if (diffTimestamps(tsCurr, tsNodePvLoadSwitchError) >= NODE_SWITCH_SAFE_TIME_SEC) {
            syncPvLoadSwitches();
        }
    }
}

void processVentilationValve() {
    uint16_t wasForceMode = NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT;
    if (isInForcedMode(NODE_VENTILATION_BIT, tsForcedNodeVentilation)) {
        return;
    }
    if (wasForceMode) {
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_VENTILATION, nodeState(NODE_VENTILATION_BIT), tsNodeVentilation,
                          NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT, tsForcedNodeVentilation, json, JSON_MAX_SIZE);
        broadcastMsg(json);
    }
    if (diffTimestamps(tsCurr, tsNodeVentilation) >= NODE_SWITCH_SAFE_TIME_SEC) {
        uint8_t sensIds[] = { SENSOR_TEMP_IN, SENSOR_TEMP_OUT };
        int16_t sensVals[] = { tempIn, tempOut };
        // disabled due to lack of space in JSON buffer
        //uint8_t sensCnt = sizeof(sensIds) / sizeof(sensIds[0]);
        uint8_t sensCnt = 0;
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

        NODE_STATE_FLAGS = NODE_STATE_FLAGS ^ bit;

        if (NODE_STATE_FLAGS & NODE_VENTILATION_BIT) {
            motorNextMove = -MOTOR_STEPS_PER_REVOLUTION * REVOLUTION_COUNT;
        } else {
            motorNextMove = MOTOR_STEPS_PER_REVOLUTION * REVOLUTION_COUNT;
        }

        *ts = getTimestamp();

    } else if (NODE_PV_LOAD_SWITCH == id) {
        bit = NODE_PV_LOAD_SWITCH_BIT;
        ts = &tsNodePvLoadSwitch;
        tsf = &tsForcedNodePvLoadSwitch;

        NODE_STATE_FLAGS = NODE_STATE_FLAGS ^ bit;

        syncPvLoadSwitches();

        *ts = getTimestamp();
    }

    // report state change
    if (ts != NULL) {
        char json[JSON_MAX_SIZE];
        jsonifyNodeStateChange(id, nodeState(bit), *ts, NODE_FORCED_MODE_FLAGS & bit, *tsf, sensId, sensVal, sensCnt,
                               json, JSON_MAX_SIZE);
        broadcastMsg(json);
    }
}

void forceNodeState(uint8_t id, uint8_t state, unsigned long ts) {
    if (NODE_VENTILATION == id) {
        forceNodeState(NODE_VENTILATION, NODE_VENTILATION_BIT, state, tsForcedNodeVentilation, ts);
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_VENTILATION, nodeState(NODE_VENTILATION_BIT), tsNodeVentilation,
                NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT, tsForcedNodeVentilation, json, JSON_MAX_SIZE);
        broadcastMsg(json);
    } else if (NODE_PV_LOAD_SWITCH == id) {
        forceNodeState(NODE_PV_LOAD_SWITCH, NODE_PV_LOAD_SWITCH_BIT, state, tsForcedNodePvLoadSwitch, ts);
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_PV_LOAD_SWITCH, nodeState(NODE_PV_LOAD_SWITCH_BIT), tsNodePvLoadSwitch,
                          NODE_FORCED_MODE_FLAGS & NODE_PV_LOAD_SWITCH_BIT, tsForcedNodePvLoadSwitch, json,
                          JSON_MAX_SIZE);
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
        jsonifyNodeStatus(NODE_VENTILATION, nodeState(NODE_VENTILATION_BIT), tsNodeVentilation,
                          NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT, tsForcedNodeVentilation, json, JSON_MAX_SIZE);
        broadcastMsg(json);
    } else if (NODE_PV_LOAD_SWITCH == id) {
        NODE_FORCED_MODE_FLAGS = NODE_FORCED_MODE_FLAGS & ~NODE_PV_LOAD_SWITCH_BIT;
        NODE_PERMANENTLY_FORCED_MODE_FLAGS = NODE_PERMANENTLY_FORCED_MODE_FLAGS & ~NODE_PV_LOAD_SWITCH_BIT;
        char json[JSON_MAX_SIZE];
        jsonifyNodeStatus(NODE_PV_LOAD_SWITCH, nodeState(NODE_PV_LOAD_SWITCH_BIT), tsNodePvLoadSwitch,
                          NODE_FORCED_MODE_FLAGS & NODE_PV_LOAD_SWITCH_BIT, tsForcedNodePvLoadSwitch, json,
                          JSON_MAX_SIZE);
        broadcastMsg(json);
    }
    // update node modes in EEPROM if unforced from permanent
    if (prevPermanentlyForcedModeFlags != NODE_PERMANENTLY_FORCED_MODE_FLAGS) {
        eepromWriteCount += EEPROM.updateInt(NODE_FORCED_MODE_FLAGS_EEPROM_ADDR, NODE_PERMANENTLY_FORCED_MODE_FLAGS);
    }
}

void stepMotor() {
    if (motorCurrentMove == 0) {
        motorCurrentMove = motorNextMove;
        motorNextMove = 0;
    }
    if (motorCurrentMove > 0) {
        motor.step(1); //close
        motorCurrentMove--;
        if (motorNextMove > 0) {
            // same direction -> cancel
            motorNextMove = 0;
        }
    } else if (motorCurrentMove < 0) {
        motor.step(-1); //open
        motorCurrentMove++;
        if (motorNextMove < 0) {
            // same direction -> cancel
            motorNextMove = 0;
        }
    }
}

void syncPvLoadSwitches() {
    // clear error
    NODE_ERROR_FLAGS = NODE_ERROR_FLAGS & ~NODE_PV_LOAD_SWITCH_BIT;
    tsNodePvLoadSwitchError = 0;

    dbgf(debug, F(":SyncPvSwitch:ong/offg/err/errTs:%d/%d/%d/%d\n"), digitalRead(PV_LOAD_SENSOR_ON_GRID_PIN),
         digitalRead(PV_LOAD_SENSOR_OFF_GRID_PIN), NODE_ERROR_FLAGS & NODE_PV_LOAD_SWITCH_BIT, tsNodePvLoadSwitchError);

    if (NODE_STATE_FLAGS & NODE_PV_LOAD_SWITCH_BIT) {
        // On-grid requested
        if (true/*digitalRead(PV_LOAD_SENSOR_OFF_GRID_PIN) == HIGH*/ && !(NODE_ERROR_FLAGS & NODE_PV_LOAD_SWITCH_BIT)) {
            // Off-grid switch is ON
            // switch it OFF
            moveServo(PV_LOAD_SWITCH_OFF_GRID_PIN, SERVO_POSITION_OFF);
            // check state
            if (false/*digitalRead(PV_LOAD_SENSOR_OFF_GRID_PIN) == HIGH*/) {
                // still ON. Error
                NODE_ERROR_FLAGS = NODE_ERROR_FLAGS | NODE_PV_LOAD_SWITCH_BIT;
                tsNodePvLoadSwitchError = tsCurr;
            } else {
                // wait before switiching next switch
                delay(500);
            }
        } else {
            // Off-grid switch is OFF (or in error state)
            // do nothing
        }
        if (true/*digitalRead(PV_LOAD_SENSOR_ON_GRID_PIN) == LOW*/ && !(NODE_ERROR_FLAGS & NODE_PV_LOAD_SWITCH_BIT)) {
            // On-grid switch is OFF
            // switch it ON
            moveServo(PV_LOAD_SWITCH_ON_GRID_PIN, SERVO_POSITION_ON);
            // check state
            if (false/*digitalRead(PV_LOAD_SENSOR_ON_GRID_PIN) == LOW*/) {
                // still OFF. Error
                NODE_ERROR_FLAGS = NODE_ERROR_FLAGS | NODE_PV_LOAD_SWITCH_BIT;
                tsNodePvLoadSwitchError = tsCurr;
            }
        } else {
            // On-grid switch is ON (or in error state)
            // do nothing
        }
    } else {
        // Off-grid requested
        if (true/*digitalRead(PV_LOAD_SENSOR_ON_GRID_PIN) == HIGH*/ && !(NODE_ERROR_FLAGS & NODE_PV_LOAD_SWITCH_BIT)) {
            // On-grid switch is ON
            // switch it OFF
            moveServo(PV_LOAD_SWITCH_ON_GRID_PIN, SERVO_POSITION_OFF);
            // check state
            if (false/*digitalRead(PV_LOAD_SENSOR_ON_GRID_PIN) == HIGH*/) {
                // still ON. Error
                NODE_ERROR_FLAGS = NODE_ERROR_FLAGS | NODE_PV_LOAD_SWITCH_BIT;
                tsNodePvLoadSwitchError = tsCurr;
            } else {
                delay(500);
            }
        } else {
            // On-grid switch is OFF (or in error state)
            // do nothing
        }
        if (true/*digitalRead(PV_LOAD_SENSOR_OFF_GRID_PIN) == LOW*/ && !(NODE_ERROR_FLAGS & NODE_PV_LOAD_SWITCH_BIT)) {
            // Off-grid switch is OFF
            // switch it ON
            moveServo(PV_LOAD_SWITCH_OFF_GRID_PIN, SERVO_POSITION_ON);
            // check state
            if (false/*digitalRead(PV_LOAD_SENSOR_OFF_GRID_PIN) == LOW*/) {
                // still OFF. Error
                NODE_ERROR_FLAGS = NODE_ERROR_FLAGS | NODE_PV_LOAD_SWITCH_BIT;
                tsNodePvLoadSwitchError = tsCurr;
            }
        } else {
            // Off-grid switch is ON (or in error state)
            //  nothing
        }
    }

    dbgf(debug, F(":SyncPvSwitch:ong/offg/err/errTs:%d/%d/%d/%d\n"), digitalRead(PV_LOAD_SENSOR_ON_GRID_PIN),
         digitalRead(PV_LOAD_SENSOR_OFF_GRID_PIN), NODE_ERROR_FLAGS & NODE_PV_LOAD_SWITCH_BIT, tsNodePvLoadSwitchError);
}

void moveServo(uint8_t servoId, uint8_t pos) {
    servo.attach(servoId, 560, 2500);
    delay(100);
    servo.write(pos);
    delay(100);
    servo.detach();
}

/*====================== Load/Save configuration in EEPROM ==================*/

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
    float v;
    v = dhtIn.readTemperature();
    tempIn = (int8_t) (v > 0) ? v + 0.5 : v - 0.5;
    v = dhtIn.readHumidity();
    humIn = (int8_t) (v > 0) ? v + 0.5 : v - 0.5;
    dbgf(debug, F(":tempIn/humIn:%d/%d\n"), tempIn, humIn);
    v = dhtOut.readTemperature();
    tempOut = (int8_t) (v > 0) ? v + 0.5 : v - 0.5;
    v = dhtOut.readHumidity();
    humOut = (int8_t) (v > 0) ? v + 0.5 : v - 0.5;
    dbgf(debug, F(":tempOut/humOut:%d/%d\n"), tempOut, humOut);

    emon.calcVI(20, 2000);
    uac = emon.Vrms;
    iac = emon.Irms;
    if (emon.realPower < 0) {
        // reverse flow (production)
        iac = iac * (-1);
    }
    pac = uac * iac;
    eac += (pac / (60 * 60)) * (tsCurr - tsLastSensorRead);
    dbgf(debug, F(":uac/iac/pac:%d/%d/%d\n"), (int) round(uac), (int) round(iac), (int) round(pac));

    // read sensorWaterPumpPower value
    int8_t state = getSensorWaterPumpPowerState();
    if (state != sensorWaterPumpPowerState) {
        // state changed
        sensorWaterPumpPowerState = state;
        tsSensorWaterPumpPower = tsCurr;
        char json[JSON_MAX_SIZE];
        jsonifySensorValue(SENSOR_WATER_PUMP_POWER, sensorWaterPumpPowerState, tsSensorWaterPumpPower, json,
                           JSON_MAX_SIZE);
        broadcastMsg(json);
    }
}

int8_t getSensorWaterPumpPowerState() {
    uint16_t max = 0;
    for (uint8_t i = 0; i < 100; i++) {
        uint16_t v = abs(analogRead(SENSOR_WATER_PUMP_POWER_PIN) - 512);
        if (v > max && v < 1000) { // filter out error values. correct value should be <1000
            max = v;
        }
        delay(1);
    }
    uint8_t state = (max > SENSOR_WATER_PUMP_POWER_THERSHOLD) ? 1 : 0;
    dbgf(debug, F(":WaterPumpPower:%d/%d\n"), max, state);
    return state;
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

    jsonifySensorValue(SENSOR_UAC, uac, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifySensorValue(SENSOR_IAC, iac, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifySensorValue(SENSOR_PAC, pac, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifySensorValue(SENSOR_EAC, eac, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    eac = 0; // reset energy counter
    jsonifySensorValue(SENSOR_WATER_PUMP_POWER, sensorWaterPumpPowerState, tsSensorWaterPumpPower, json, JSON_MAX_SIZE);
    broadcastMsg(json);

    jsonifyNodeStatus(NODE_VENTILATION, nodeState(NODE_VENTILATION_BIT), tsNodeVentilation,
                      NODE_FORCED_MODE_FLAGS & NODE_VENTILATION_BIT, tsForcedNodeVentilation, json, JSON_MAX_SIZE);
    broadcastMsg(json);
    jsonifyNodeStatus(NODE_PV_LOAD_SWITCH, nodeState(NODE_PV_LOAD_SWITCH_BIT), tsNodePvLoadSwitch,
                      NODE_FORCED_MODE_FLAGS & NODE_PV_LOAD_SWITCH_BIT, tsForcedNodePvLoadSwitch, json, JSON_MAX_SIZE);
    broadcastMsg(json);
}

void reportConfiguration() {
    char json[JSON_MAX_SIZE];
    char ip[16];

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

uint16_t nodeState(uint16_t bit) {
    uint16_t state = NODE_STATE_FLAGS & bit;
    if (NODE_ERROR_FLAGS & bit) {
        // node in error state
        state = state | NODE_ERROR_BIT;
    }
    return state;
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
            if (root.containsKey(F("rap"))) {
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
