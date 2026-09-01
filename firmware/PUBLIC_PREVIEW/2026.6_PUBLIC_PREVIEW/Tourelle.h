#pragma once

#define TOURELLE_DRIVER_VERSION "TOURELLE_DRIVER_2026.1"
#define TOURELLE_HARDWARE "STD Cannon Turret - EH Interface"

void setupTourelle();
void updateTourelleElevation(int turretY);
void updateTourelleRotation(int tSpeed);
void updateEscPower(int vraRaw);
void stopTourelle(); 