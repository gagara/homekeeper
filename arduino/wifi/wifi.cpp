#include "wifi.h"

static const char WIFI_AP[] = "point39";
static const char WIFI_PW[] = "87654321";
static const char WIFI_SERVER[] = "192.168.0.139";
static char OK[] = "OK";

static const uint8_t JSON_MAX_READ_SIZE = 128;

uint8_t clientId = 0;
uint8_t WIFI_IP[4] = { 0, 0, 0, 0 };

HardwareSerial *wifi = &Serial2;
//SoftwareSerial wifi(2, 3); //RX TX

void setup() {
    Serial.begin(9600);
    wifiInit();
    wifiConnect();
    delay(3000);
    char ip[16];
    if (isWifiConnected(ip)) {
        Serial.print("connected: ");
        Serial.print(WIFI_IP[0]);
        Serial.print(".");
        Serial.print(WIFI_IP[1]);
        Serial.print(".");
        Serial.print(WIFI_IP[2]);
        Serial.print(".");
        Serial.println(WIFI_IP[3]);
    } else {
        Serial.println("not connected");
    }
    if (wifiStartServer()) {
        Serial.println("started");
    }
}

void loop() {
    if (wifi->available() > 0) {
        char* cmd = wifiRead();
        // TODO: parse cmd
        if (strlen(cmd) > 1) {
            wifiRsp200(cmd);
            Serial.print("sending: ");
            Serial.println(cmd);
            if (wifiSend(cmd)) {
                Serial.println("done");
            } else {
                Serial.println("failed");
            }
        }
        free(cmd);
    }
    if (Serial.available() > 0) {
        wifi->println(Serial.readStringUntil('\0'));
    }
    delay(1000);
}

void wifiInit() {
    wifi->begin(9600);
    wifi->begin(115200);
    wifi->println("AT+CIOBAUD=9600");
    wifi->begin(9600);
    wifi->setTimeout(300);
}

void wifiConnect() {
    char s[100];
    wifi->println("AT+RST");
    wifi->println("AT+CIPMODE=0");
    wifi->println("AT+CWMODE=1");
    sprintf(s, "AT+CWJAP=\"%s\",\"%s\"", WIFI_AP, WIFI_PW);
    Serial.println(s);
    wifi->println(s);
    delay(5000);
}

bool isWifiConnected(char* ips) {
    wifi->println("AT+CIFSR");
    String s = wifi->readStringUntil('\0');
    char* buff = (char*) malloc(JSON_MAX_READ_SIZE);
    s.toCharArray(buff, JSON_MAX_READ_SIZE, 0);
    int ip[4];
    int n = sscanf(buff, "%*[^,\"],\"%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
    free(buff);
    if (n == 4) {
        WIFI_IP[0] = ip[0];
        WIFI_IP[1] = ip[1];
        WIFI_IP[2] = ip[2];
        WIFI_IP[3] = ip[3];
        int sum = WIFI_IP[0] + WIFI_IP[1] + WIFI_IP[2] + WIFI_IP[3];
        return sum > 0 && sum < 255 * 4;
    } else {
        return false;
    }
}

bool wifiStartServer() {
    wifi->println("AT+CIPMUX=1");
    if (!wifi->find(OK))
        return false;
    wifi->println("AT+CIPSERVER=1,80");
    return wifi->find(OK);
}

bool wifiRsp200(const char* msg) {
    char body[200];
    sprintf(body, "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", strlen(msg),
            msg);

    char send[32];
    sprintf(send, "AT+CIPSEND=%d,%d", clientId, strlen(body));

    char close[16];
    sprintf(close, "AT+CIPCLOSE=%d", clientId);

    if (!wifiWrite(send, ">"))
        return false;

    if (!wifiWrite(body, OK, 300, 1))
        return false;

    if (!wifiWrite(close, OK))
        return false;
    return true;
}

bool wifiSend(const char* msg) {
    char body[200];
    sprintf(body, "POST / HTTP/1.1 \r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s", strlen(msg),
            msg);

    char connect[64];
    sprintf(connect, "AT+CIPSTART=0,\"TCP\",\"%s\",5000", WIFI_SERVER);

    char send[32];
    sprintf(send, "AT+CIPSEND=0,%d", strlen(body));

    char close[16];
    sprintf(close, "AT+CIPCLOSE=0");

    if (!wifiWrite(connect, "CONNECT", 1000))
        return false;

    if (!wifiWrite(send, ">"))
        return false;

    if (!wifiWrite(body, OK, 300, 1))
        return false;

    wifiWrite(close, OK);

    return true;
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

char* wifiRead() {
    String s = wifi->readStringUntil('\0');
    Serial.println(s);
    uint8_t cidx = s.indexOf("+IPD,");
    if (cidx > 0) {
        clientId = s.substring(cidx + 5, (cidx + 5) + 1).toInt();
        uint8_t rqidx1 = s.indexOf("GET /");
        uint8_t rqidx2 = s.indexOf(" ", rqidx1 + 5);
        if (rqidx1 > 0 && rqidx2 > 0 && (rqidx1 + 5) < rqidx2) {
            String rq = s.substring(rqidx1 + 5, rqidx2);
            char* result = (char*) malloc(JSON_MAX_READ_SIZE);
            rq.toCharArray(result, JSON_MAX_READ_SIZE, 0);
            return result;
        }
    }
    return NULL;
}
