#pragma once

#define TOURELLE_DRIVER_VERSION "TOURELLE_DRIVER_2026.2-RTOS"
#define TOURELLE_HARDWARE "STD Cannon Turret - POGO Interface"

void setupTourelle();
void updateTourelleElevation(int turretY);
void updateTourelleRotation(int tSpeed);
void updateEscPower(int vraRaw);
void stopTourelle();