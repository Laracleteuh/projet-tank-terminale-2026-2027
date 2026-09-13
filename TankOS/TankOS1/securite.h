#pragma once
#include <Arduino.h>

#define SECURITY_MODULE_VERSION "SECURITY_MODULE_2026.3-RTOS"

void gererAlertesBatterie(int batteryPct, int turretPct ,float currentAmps);
float calculerBridageBatterie(int batteryPct);
float calculerBridageCourant(float currentAmps);