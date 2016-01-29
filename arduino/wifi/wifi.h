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

bool wifiConnect();
bool isWifiConnected();
bool wifiStartServer();
bool wifiSend(const char* msg, bool eoi=false);
char* wifiRead();

//Do not add code below this line
#endif /* _wifi_H_ */
