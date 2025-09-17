#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>

void processLine(String line);
void handleSerialCommands(String &rxBuf);

#endif // COMMAND_PARSER_H
