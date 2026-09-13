#include "Systeme.h"
#include "Config.h"
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <avr/wdt.h>
#include <Arduino_FreeRTOS.h> // Nécessaire pour vTaskDelay et xTaskGetSchedulerState

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
static bool tempSensorOk = false;

void initStatusLED() {
  pinMode(RStatusLED, OUTPUT);
  pinMode(GStatusLED, OUTPUT);
  pinMode(BStatusLED, OUTPUT);
  analogWrite(RStatusLED, 0);
  analogWrite(GStatusLED, 0);
  analogWrite(BStatusLED, 0);
}

void setStatusColor(int r, int g, int b) {
  analogWrite(RStatusLED, r);
  analogWrite(GStatusLED, g);
  analogWrite(BStatusLED, b);
}

void setupFanPWM() {
  // -- Change la vitesse d'envoi de 16 MHz à 25 kHz --
  pinMode(fanPWMPin, OUTPUT);
  cli(); 
  TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
  TCCR2B = (1 << WGM22) | (1 << CS21);
  OCR2A = 79;
  OCR2B = 0; 
  sei();
}

void setupSysteme() {
  // setupFanPWM();
  
  if (!tempsensor.begin(0x18)) {
    Serial.println("E0SEN - MCP9808 sensor not found. Check I2C wiring.");
    tempSensorOk = false;
  } else {
    tempsensor.setResolution(0);
    tempSensorOk = true;
  }
  
  pinMode(TurretControlStatusLED, OUTPUT);
  digitalWrite(TurretControlStatusLED, HIGH);
  
  pinMode(SWC_PIN, INPUT);
  pinMode(SWD_PIN, INPUT);
  pinMode(VRA_PIN, INPUT);
  
  pinMode(switchPIN, OUTPUT);
  digitalWrite(switchPIN, LOW);
}

void updateSwitch(int swcRaw) {
  if (swcRaw > 1750) {
    digitalWrite(switchPIN, HIGH); 
  } else if (swcRaw > 500 && swcRaw < 1250) {
    digitalWrite(switchPIN, LOW);
  }
}

void rebootTank() {
  // Si le scheduler FreeRTOS est démarré, on utilise le délai non bloquant.
  // Sinon (par exemple si on reboot depuis le setup), on utilise delay().
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    vTaskDelay(pdMS_TO_TICKS(100));
  } else {
    delay(100);
  }
  
  wdt_enable(WDTO_15MS); // Arme le chien de garde à 15 millisecondes[cite: 2]
  while(1) {}            // Boucle infinie : l'Arduino plante et le Watchdog force le reset[cite: 2]
}

bool isTempSensorPresent() {
  return tempSensorOk;
}