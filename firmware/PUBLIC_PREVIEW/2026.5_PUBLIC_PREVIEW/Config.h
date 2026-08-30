#pragma once
#include <Arduino.h>

// Ce firmware est encore en version preview, et n'a pas encore été testé sur du vrai materiel
// Merci d'en tenir compte si vous l'utilsez.

// -- Firmware information --
#define FIRMWARE_VERSION "2026.5_PUBLIC_PREVIEW"
#define LAST_BUILD_DATE "30.08.2026"
#define FIRMWARE_AUTHOR "Projet Tank - Specialite SI 2026 - 2027"

// -- RELAY STATE MACROS --
#define RELAY_ON LOW
#define RELAY_OFF HIGH

// Fonction utilitaire pour tous les relais
inline void relayWrite(int pin, bool state) {
  digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
}

// -- MOTOR SETUP (Traction) --
#define leftMotorFwdRelay 22
#define leftMotorBckRelay 23
#define rightMotorFwdRelay 24
#define rightMotorBckRelay 25
#define leftMotorOnOffSwitch 26
#define rightMotorOnOffSwitch 27

// -- MCP41100 SETUP (SPI) --
// SCK est matériellement sur la broche 52 de l'Arduino Mega
// SI (MOSI) est matériellement sur la broche 51 de l'Arduino Mega
#define CS_PIN1 42
#define CS_PIN2 43

// -- F-10A SETUP --
#define rightJoystickX A8  
#define rightJoystickY A9  
#define leftJoystickX A10   
#define leftJoystickY A11

#define SWA_PIN 44  
#define SWB_PIN 10 
#define SWC_PIN 11 
#define SWD_PIN 12 
#define VRA_PIN 13 
#define VRB_PIN 45

// -- TOURELLE & ARMEMENT --
#define switchPIN 31
#define servo1Tourelle 2
#define servo2Tourelle 3
#define esc1Tourelle 4
#define esc2Tourelle 5

// -- TOURELLE : L298N (Rotation X) --
#define motorDriverIN1 6
#define motorDriverIN2 7
#define motorDriverENA 8

// -- CAPTEURS ET VENTILATEURS --
#define fanPWMPin 9       

// -- STATUS LEDS --
#define RStatusLED 49
#define GStatusLED 47 
#define BStatusLED 46 
#define TurretControlStatusLED 48 
 
// -- MDRVCT FAILSAFE --
#define MDRVCT_FAILSAFE 32 

// --HM-10 BLUETOOTH MODULE--
#define BT_SERIAL Serial1
#define BT_BRK_PIN 37


// -- BATTERY & CURRENT SENSORS --
#define MAIN_BATTERY_PIN A1    // Batterie Châssis (Traction)
#define TURRET_BATTERY_PIN A2  // Batterie Tourelle (Armement/Servos)
#define CURRENT_SENSOR_PIN A0  // Capteur de courant

// -- RC VARIABLES --
const int rxMin = 980;
const int rxMax = 2000;
const int rxCenter = 1500;
const int deadzone = 50;

// Fonction utilitaire pour la conversion RC
inline int rcToSpeed(int rawValue) {
  if (abs(rawValue - rxCenter) < deadzone){
    return 0;
  }
  int speed = map(rawValue, rxMin, rxMax, -100, 100);
  return constrain(spee
    d, -100, 100);
}
