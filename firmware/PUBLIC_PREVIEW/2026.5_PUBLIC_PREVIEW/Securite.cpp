#include "Securite.h"
#include "Systeme.h" // Pour avoir accès à setStatusColor()

void gererAlertesBatterie(int batteryPct) {
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = false;

  if (batteryPct <= 15) { 
    // Clignotement rouge si batterie critique (< 15%)
    if (millis() - lastBlinkTime > 500) {
      blinkState = !blinkState;
      if (blinkState) setStatusColor(255, 0, 0); // Rouge
      else setStatusColor(0, 0, 0);              // Éteint
      lastBlinkTime = millis();
    }
  } else {
    // Si tout va bien, LED verte fixe
    setStatusColor(0, 255, 0); 
  }
}

float calculerBridageBatterie(int batteryPct) {
  if (batteryPct <= 20) {
    // Bride progressivement de 100% (à 20% de batterie) jusqu'à 30% (à 0% de batterie)
    return map(batteryPct, 0, 20, 30, 100) / 100.0;
  }
  return 1.0; // Aucune restriction si la batterie est > 20%
}