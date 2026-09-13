#pragma once
#include <Arduino.h>

#define SYSTEM_MODULE_VERSION "SYSTEM_MODULE_2026.2-RTOS"

void setupSysteme();
void updateSwitch(int swcRaw);
void initStatusLED();
void setStatusColor(int r, int g, int b);
void rebootTank();
bool isTempSensorPresent();