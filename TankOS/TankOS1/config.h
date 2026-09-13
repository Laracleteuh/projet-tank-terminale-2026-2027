#pragma once
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// -- Firmware information --
#define FIRMWARE_VERSION "TankOS 1 (Basé sur FreeRTOS pour microcontrôleur AVR"
#define LAST_BUILD_DATE "13.09.2026"
#define FIRMWARE_AUTHOR "Projet Tank Terminale - Specialite SI 2026 - 2027"

// -- RELAY STATE MACROS --
#define RELAY_ON HIGH
#define RELAY_OFF LOW

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

// -- LECTEUR RFID ADMIN (RC522, SPI, partage le bus SPI0 avec les MCP41010
// de Traction.cpp : SCK=52, MOSI=51, MISO=50, déjà initialisés ailleurs) --
#define RFID_SS_PIN 41
#define RFID_RST_PIN 40

// -- BATTERY & CURRENT SENSORS --
#define MAIN_BATTERY_PIN A1    // Batterie Châssis (Traction)
#define TURRET_BATTERY_PIN A2  // Batterie Tourelle (Armement/Servos)
#define CURRENT_SENSOR_PIN A0  // Capteur de courant

#define LOW_BATTERY_THRESHOLD_PCT 15
#define OVERCURRENT_THRESHOLD_AMPS 20.0f
#define ALERT_REPEAT_INTERVAL_MS 3000

// -- RC VARIABLES --
const int rxMin = 980;
const int rxMax = 2000;
const int rxCenter = 1500;
const int deadzone = 50;

// Fonction utilitaire pour la conversion RC
inline int rcToSpeed(int rawValue) {
  if (abs(rawValue - rxCenter) < deadzone) {
    return 0;
  }
  int speed = map(rawValue, rxMin, rxMax, -100, 100);
  return constrain(speed, -100, 100);
}

// =============================================================================================
// FREERTOS — PRIORITÉS DES TÂCHES
//
// Plus le nombre est élevé, plus la priorité est haute (idle = 0).
// TaskSafety doit toujours pouvoir préempter tout le reste : c'est elle qui
// coupe traction/tourelle en cas de perte de signal radio.
// =============================================================================================
#define PRIO_SAFETY 4   // Failsafe RC + bridage batterie + LED status
#define PRIO_TRACTION 3   // Lecture RC + updateTraction()
#define PRIO_TOURELLE 3   // Lecture RC + contrôle tourelle/ESC
#define PRIO_COMMUNICATION 2   // BT/Série + ventilos + télémétrie
#define PRIO_ADMIN 1   // Scan RFID (basse fréquence, basse priorité)

// -- Tailles de pile --
// IMPORTANT (port AVR) : ces valeurs sont en MOTS (16 bits), pas en octets.
// Une pile de 128 = 256 octets réels. À ajuster avec
// uxTaskGetStackHighWaterMark() une fois le firmware qui tourne.
#define STACK_SAFETY        300
#define STACK_TRACTION      300
#define STACK_TOURELLE      300
#define STACK_COMMUNICATION 400
#define STACK_ADMIN         300

// -- Handles de tâches (définis dans main.cpp) --
extern TaskHandle_t hTaskSafety;
extern TaskHandle_t hTaskTraction;
extern TaskHandle_t hTaskTourelle;
extern TaskHandle_t hTaskCommunication;
extern TaskHandle_t hTaskAdmin;

// -- Mutex partagé pour le bus SPI0 --
// A PRENDRE (xSemaphoreTake) avant tout SPI.transfer() vers le RC522
// (admin.cpp) OU vers les MCP41100 (Traction.cpp), et RELÂCHER juste après.
// Sans ça, TaskTraction et TaskAdmin peuvent se marcher dessus sur le bus
// et corrompre aussi bien les commandes moteur que la lecture des UID.
// (À câbler dans Traction.cpp / admin.cpp à l'étape suivante.)
extern SemaphoreHandle_t spiMutex;

// -- Mutex pour l'état partagé entre TaskSafety et TaskTraction/TaskTourelle
// (failsafe actif, pourcentage batterie, facteur de bridage) --
extern SemaphoreHandle_t stateMutex;

// -- Mutex pour protéger les écritures concurrentes sur Serial et
// BT_SERIAL (Serial1) entre TaskSafety (alertes batterie/courant) et
// TaskCommunication (télémétrie, réponses aux commandes) --
extern SemaphoreHandle_t serialMutex;