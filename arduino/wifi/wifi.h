// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _wifi_H_
#define _wifi_H_
#include "Arduino.h"
//add your includes for the project wifi here


//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

//add your function definitions for the project wifi here

void wifiInit();
void wifiConnect();
bool isWifiConnected(char* ip);
bool wifiStartServer();
bool wifiRsp(const char* msg);
bool wifiSend(const char* msg);
bool wifiWrite(const char* msg, const char* rsp, const int wait = 0, const uint8_t maxRetry = 5);
char* wifiRead();

//Do not add code below this line
#endif /* _wifi_H_ */
