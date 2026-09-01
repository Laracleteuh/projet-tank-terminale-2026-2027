#pragma once

#define ISR_HANDLER_VERSION "ISR_HANDLER_2026.1"

// Structure contenant toutes les voies de la radio
struct RcData {
  int yRaw;
  int xRaw;
  int swcRaw;
  int swdRaw;
  int vraRaw;
  int vrbRaw;
  int turretY;
  int turretX;
};

// Fonctions accessibles depuis le main.cpp
void setupRecepteur();
bool checkFailsafe();
RcData getRecepteurData(); 