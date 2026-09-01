#include "batterie.h"
#include "Config.h"

static float zeroAmpVoltage = 2.5;

void setupBatterie() {
  pinMode(MAIN_BATTERY_PIN, INPUT);
  pinMode(TURRET_BATTERY_PIN, INPUT);
  pinMode(CURRENT_SENSOR_PIN, INPUT);
}

float getBatteryVoltage(uint8_t pin) {
  int valeurADC = analogRead(pin);
  float tensionBroche = valeurADC * (5.0 / 1023.0);
  return tensionBroche * 6.0; 
}

int getBatteryPercentage(uint8_t pin) {
  float tensionBatterie = getBatteryVoltage(pin);
  int tensionEntiere = (int)(tensionBatterie * 10);
  int pourcentage = 0;

  // -- BATTERIE TANK : 6S (2x 3S en série) --
  // Plage : 19.8V (vide) à 25.2V (plein)
  if (pin == MAIN_BATTERY_PIN) {
      pourcentage = map(tensionEntiere, 198, 252, 0, 100);
  } 
  // -- BATTERIE TOURELLE : (Assumons une 3S standard) --
  // Plage : 9.9V (vide) à 12.6V (plein)
  else if (pin == TURRET_BATTERY_PIN) {
      pourcentage = map(tensionEntiere, 99, 126, 0, 100); 
  }

  return constrain(pourcentage, 0, 100);
}

float getCurrentAmps() {
  int valeurADC = analogRead(CURRENT_SENSOR_PIN);
  float tensionBroche = valeurADC * (5.0 / 1023.0);
  float courant = (tensionBroche - zeroAmpVoltage) / 0.040; 
  return courant;
}

void tareCurrentSensor() {
  long sum = 0;
  for(int i = 0; i < 20; i++) {
    sum += analogRead(CURRENT_SENSOR_PIN);
    delay(2);
  }
  float avgADC = sum / 20.0;
  zeroAmpVoltage = avgADC * (5.0 / 1023.0);
}