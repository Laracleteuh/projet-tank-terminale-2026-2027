#pragma once
#include <Arduino.h>

#define SECURITY_MODULE_VERSION "SECURITY_MODULE_2026.2"

// Gère le clignotement de la LED RGB en fonction du niveau de la batterie
void gererAlertesBatterie(int batteryPct);

// Calcule le coefficient de bridage (Limp Mode) : de 1.0 (pleine puissance) à 0.3 (bridé)
float calculerBridageBatterie(int batteryPct);