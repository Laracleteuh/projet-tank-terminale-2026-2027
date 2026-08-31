#include "Systeme.h"
#include "Config.h"
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <avr/wdt.h>


Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

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

  // -- Change la vitesse d'envoie de 16 MHz à 25 kHz --
  pinMode(fanPWMPin, OUTPUT);
  cli(); 
  TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
  TCCR2B = (1 << WGM22) | (1 << CS21);
  OCR2A = 79;
  OCR2B = 0; 
  sei();
}

void setupSysteme() {
  setupFanPWM();

  if (!tempsensor.begin(0x18)) {
    Serial.println("E0SEN - MCP9808 sensor not found. Check I2 wiring.");
  } else {
    tempsensor.setResolution(0);
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
  delay(100);
  wdt_enable(WDTO_15MS); // Arme le chien de garde à 15 millisecondes
  while(1) {}            // Boucle infinie : l'Arduino plante et le Watchdog force le reset
}

