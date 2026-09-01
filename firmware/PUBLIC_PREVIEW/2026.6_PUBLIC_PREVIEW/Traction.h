#pragma once

#define TRACTION_MODULE_VERSION "TRACTION_MODULE_2026.1"

void setupTraction();
void updateTraction(int speed, int steering, int vrbRaw);
void stopTraction();
void setTractionPowerSwitch(bool state); 
void setMotorDiagnostic(char side, int speed);
void disableMotorDiagnostic();
