#pragma once

#define SYSTEM_MODULE_VERSION "SYSTEM_MODULE_2026.1"

void setupSysteme();
void updateSwitch(int swcRaw);

void initStatusLED();
void setStatusColor(int r, int g, int b); 
void rebootTank();