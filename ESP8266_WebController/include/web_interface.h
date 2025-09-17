#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

void setupWebServer();
void handleWebRequests();
void handleRoot();
void handleCommand();
void handleStatus();
void handleNotFound();
String generateControlPage();
String generateJavaScript();
String generateCSS();

#endif // WEB_INTERFACE_H
