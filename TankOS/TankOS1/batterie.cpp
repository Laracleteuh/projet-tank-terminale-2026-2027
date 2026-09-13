#include "batterie.h"
#include "Config.h"
#include <Arduino_FreeRTOS.h>

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
  
  if (pin == MAIN_BATTERY_PIN) {
      pourcentage = map(tensionEntiere, 198, 252, 0, 100);
  } else if (pin == TURRET_BATTERY_PIN) {
      pourcentage = map(tensionEntiere, 99, 126, 0, 100);
  } // <-- Accolade ajoutée ici
  
  return constrain(pourcentage, 0, 100);
}

float getCurrentAmps() {
  int valeurADC = analogRead(CURRENT_SENSOR_PIN);
  float tensionBroche = valeurADC * (5.0 / 1023.0);
  // ACS758-100B : sensibilité de 20mV par Ampère (0.020 V/A)
  float courant = (tensionBroche - zeroAmpVoltage) / 0.020; 
  return courant;
}

void tareCurrentSensor() {
  long sum = 0;
  for(int i = 0; i < 20; i++) {
    sum += analogRead(CURRENT_SENSOR_PIN);
    
    // Gère le cas où la commande de tare est lancée à la volée pendant l'exécution
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
      vTaskDelay(pdMS_TO_TICKS(2));
    } else {
      delay(2);
    }
  }
  
  float avgADC = sum / 20.0;
  zeroAmpVoltage = avgADC * (5.0 / 1023.0);
}