#include "communication.h"
#include "batterie.h"
#include "Config.h"
#include "ISR.h"
#include "Securite.h"
#include "Systeme.h"
#include "Traction.h"
#include "Tourelle.h"
#include <Adafruit_MCP9808.h>

extern Adafruit_MCP9808 tempsensor;

#define BT_SERIAL Serial1
#define BT_BAUD 9600

// =============================================================================================
// communication.cpp — fusion de bluetooth.cpp + test.cpp
//
// IMPORTANT (syntaxe unifiée) : certaines commandes série USB existantes
// (mspd=50 / fanspd=50) sont remplacées par la syntaxe Bluetooth, plus
// complète (mspd 50 / fanspd 50, avec les sous-commandes show/reset).
// De même, "motor" (série) et "motordiag" (BT) sont unifiés sous "motor".
// "reboot tank" (série) et "reboot arduino" (BT) sont désormais tous les
// deux acceptés, sur les deux interfaces.
// =============================================================================================

// -- État partagé (accessible aux deux interfaces) --
unsigned long lastTempSendTime = 0;
unsigned long lastTempReadTime = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 1000;
unsigned long telemetryIntervalMs = 1000;
bool isTelemetryActive = false;

int manualFanSpeedPercent = 0;
float lastKnownTempC = 25.0;
String modeVentilation = "OFF";
int vitesseActuellePourcent = 0;

float btSpeedLimit = 1.0;

int dynTempMin = 25;
int dynTempMax = 45;

// -- Buffers de réception : un par interface, complètement indépendants --
static char btBuffer[32];
static uint8_t btIndex = 0;

static char serialBuffer[32];
static uint8_t serialIndex = 0;

// -- Machine à états non bloquante pour la coupure BT (btbrk) --
enum BtBreakState { BT_NORMAL, BT_WAIT_PRINT, BT_BREAK_LOW };
static BtBreakState breakState = BT_NORMAL;
static unsigned long breakTimer = 0;

void triggerBtBreak() {
  if (breakState == BT_NORMAL) {
    breakState = BT_WAIT_PRINT;
    breakTimer = millis();
  }
}

float getBluetoothSpeedLimit() {
  return btSpeedLimit;
}

// -- Gate : la télémétrie ne démarre qu'après la fin complète de setup() --
static bool setupComplete = false;

void markSetupComplete() {
  setupComplete = true;
}

// =============================================================================================
// LE PARSEUR UNIQUE : appelé aussi bien pour une ligne reçue par Bluetooth
// que pour une ligne reçue par le moniteur série. "out" est la sortie sur
// laquelle la réponse doit être écrite (BT_SERIAL ou Serial) : Print& marche
// pour les deux car HardwareSerial hérite de Print.
//
// ==> Pour ajouter une commande utilisable à la fois en BT et en série USB,
//     il suffit de l'ajouter UNE SEULE FOIS ici.
// =============================================================================================
static void processCommand(char* buffer, Print& out) {

  // -- reboot ... --
  if (strncmp(buffer, "reboot ", 7) == 0) {
    char* argument = buffer + 7;

    if (strcmp(argument, "arduino") == 0 || strcmp(argument, "tank") == 0) {
      out.println("rebooting board");
      rebootTank(); // Appel de la fonction Watchdog
    }
    else if (strcmp(argument, "esc") == 0) {
      out.println("resetting ESC");
      setupTourelle();
      out.println("done");
    }
    else if (strcmp(argument, "bt") == 0) {
      out.println("rebooting bluetooth module");
      BT_SERIAL.print("AT+RESET"); // Commande propre au module HM-10
      out.println("done");
    }
    else {
      out.println("E0RB - unknown reboot argument");
    }
  }

  // -- ver ... --
  else if (strncmp(buffer, "ver ", 4) == 0) {
    char* argument = buffer + 4;
    if (strcmp(argument, "arduino") == 0) {
      out.println("Arduino Mega 2560 - ATMEGA2560");
    }
    else if (strcmp(argument, "firmware") == 0) {
      out.println("firmware version: " + String(FIRMWARE_VERSION) + " | last build: " + String(LAST_BUILD_DATE));
    }
    else if (strcmp(argument, "bt") == 0) {
      out.println("Bluetooth module HM-10 - driver version: " + String(BT_DRIVER_VERSION));
      out.println("Hardware: " + String(BT_HARDWARE));
    }
    else if (strcmp(argument, "drivers") == 0) {
      out.println("Traction driver: " + String(TRACTION_MODULE_VERSION));
      out.println("Battery driver: " + String(BATT_DRIVER_VERSION));
      out.println("Systeme driver: " + String(SYSTEM_MODULE_VERSION));
      out.println("Tourelle driver: " + String(TOURELLE_DRIVER_VERSION));
      out.println("ISR handler: " + String(ISR_HANDLER_VERSION));
      out.println("Security module: " + String(SECURITY_MODULE_VERSION));
    }
    else {
      out.println("E0VR - unknown ver argument");
    }
  }

  // -- stats ... --
  else if (strncmp(buffer, "stats ", 6) == 0) {
    char* arg = buffer + 6;

    if (strncmp(arg, "status ", 7) == 0) {
      char* val = arg + 7;
      if (strcmp(val, "on") == 0) {
        isTelemetryActive = true;
        out.println("continuous stats on - " + String(telemetryIntervalMs) + " ms interval");
      }
      else if (strcmp(val, "off") == 0) {
        isTelemetryActive = false;
        out.println("continuous stats off");
      }
    }
    else if (strncmp(arg, "delay ", 6) == 0) {
      int delaySec = atoi(arg + 6);
      if (delaySec > 0) {
        telemetryIntervalMs = delaySec * 1000;
        out.print("stats delay set to  ");
        out.print(delaySec);
        out.println(" second(s)");
      } else {
        out.println("E0SD - delay must be > 0");
      }
    }
    else {
      out.println("E0ST - unknown stats argument");
    }
  }

  // -- tare ... --
  else if (strncmp(buffer, "tare ", 5) == 0) {
    char* arg = buffer + 5;
    if (strcmp(arg, "ampSensor") == 0) {
      tareCurrentSensor();
      out.println("ampSensor tare complete.");
    } else {
      out.println("E0TA - unknown tare argument");
    }
  }

  // -- motor ... (unifié : anciennement "motordiag" côté BT, "motor" côté série) --
  else if (strncmp(buffer, "motor ", 6) == 0) {
    char* arg = buffer + 6;

    if (strcmp(arg, "off") == 0) {
      disableMotorDiagnostic();
      out.println("I0MD - motor diagnostic disabled");
    }
    else {
      char side;
      int spd;
      if (sscanf(arg, "%c %d", &side, &spd) == 2) {
        if (side == 'L' || side == 'R') {
          spd = constrain(spd, -100, 100);
          setMotorDiagnostic(side, spd);

          out.print("I0MD - motor diagnostic enabled for ");
          out.print(side);
          out.print(" a ");
          out.print(spd);
          out.println("%");
        } else {
          out.println("E0MD - invalid motor argument (must be L or R)");
        }
      } else {
        out.println("E0MD - invalid motor command syntax (use: motor L 50 or motor off)");
      }
    }
  }

  // -- fanrange ... --
  else if (strncmp(buffer, "fanrange ", 9) == 0) {
    char* arg = buffer + 9;

    if (strcmp(arg, "season winter") == 0) {
      dynTempMin = 15;
      dynTempMax = 35;
      out.println("I0FR - winter season active (15C - 35C).");
    }
    else if (strcmp(arg, "season summer") == 0) {
      dynTempMin = 30;
      dynTempMax = 55;
      out.println("I0FR - summer season active (30C - 55C).");
    }
    else if (strcmp(arg, "show") == 0) {
      out.print("I0FR - current thermal range : ");
      out.print(dynTempMin);
      out.print("C -> ");
      out.print(dynTempMax);
      out.println("C");
    }
    else {
      int tmin = 0, tmax = 0;
      if (sscanf(arg, "%d %d", &tmin, &tmax) == 2) {
        if (tmin < tmax) {
          dynTempMin = tmin;
          dynTempMax = tmax;
          out.print("I0FR - new thermal range : ");
          out.print(dynTempMin);
          out.print("C -> ");
          out.print(dynTempMax);
          out.println("C");
        } else {
          out.println("E0FR - T_MIN must be less than T_MAX.");
        }
      } else {
        out.println("E0FR - Error: Syntax -> fanrange 25 45 or fanrange season winter or fanrange show");
      }
    }
  }

  // -- mspd ... (unifié : "mspd=50" côté série n'existe plus, utiliser "mspd 50") --
  else if (strncmp(buffer, "mspd ", 5) == 0) {
    char* arg = buffer + 5;

    if (strcmp(arg, "reset") == 0) {
      btSpeedLimit = 1.0;
      out.println("I0MS - motor speed limit reset to 100%");
    }
    else if (strcmp(arg, "show") == 0) {
      int currentLimitPercent = static_cast<int>(btSpeedLimit * 100);
      out.print("I0MS - current motor speed limit: ");
      out.print(currentLimitPercent);
      out.println("%");
    }
    else {
      int val = atoi(arg);
      if (val >= 0 && val <= 100) {
        btSpeedLimit = val / 100.0;
        out.print("I0MS - motor speed limit set to ");
        out.print(val);
        out.println("%");
      } else {
        out.println("E0MS - invalid mspd value (must be between 0 and 100)");
      }
    }
  }

  // -- fanspd ... (unifié : "fanspd=50" côté série n'existe plus, utiliser "fanspd 50") --
  else if (strncmp(buffer, "fanspd ", 7) == 0) {
    char* arg = buffer + 7;

    if (strcmp(arg, "show") == 0) {
      out.print("I0FS - current fan speed: ");
      out.print(manualFanSpeedPercent);
      out.println("%");
    }
    else if (strcmp(arg, "set ") == 0) {
      int val = atoi(arg);
      if (modeVentilation == "AUTO") {
        out.println("E0FS - cannot set fan speed in AUTO mode. Switch to MANUAL first.");
      }
      else if (modeVentilation == "OFF") {
        out.println("E0FS - cannot set fan speed when fans are OFF. Switch to MANUAL first.");
      }
      else if (modeVentilation == "MANUAL") {
        if (val >= 0 && val <= 100) {
          manualFanSpeedPercent = val;
          out.print("I0FS - fan speed set to ");
          out.print(val);
          out.println("%");
        } else {
          out.println("E0FS - invalid fanspd value (must be between 0 and 100)");
        }
    }
    else {
      out.println("E0FS - unknown fanspd argument. Use 'fanspd show' or 'fanspd set <value>'");
      }
    }
  }

  // -- fanctrl -- 
  else if (strncmp(buffer, "fanctrl ", 8) == 0) {
    char* arg = buffer + 8;

    if (strcmp(arg, "show") == 0) {
      out.print("I0FC - current fan control mode: ");
      out.println(modeVentilation);
    }
    else if (strcmp(arg, "auto") == 0) {
      modeVentilation = "AUTO";
      out.println("I0FC - fan control mode set to AUTO");
    }
    else if (strcmp(arg, "manual") == 0) {
      modeVentilation = "MANUAL";
      out.println("I0FC - fan control mode set to MANUAL");
    }
    else if (strcmp(arg, "off") == 0) {
      modeVentilation = "OFF";
      out.println("I0FC - fan control mode set to OFF");
    }
    else {
      out.println("E0FC - unknown fanctrl argument. Use 'fanctrl show', 'fanctrl auto', 'fanctrl manual', or 'fanctrl off'");
    }
  }

  // -- commandes simples --
  else if (strcmp(buffer, "ping") == 0) {
    out.println("pong");
  }
  else if (strcmp(buffer, "whois") == 0) {
    out.println(FIRMWARE_AUTHOR);
  }
  else if (strcmp(buffer, "reset") == 0) {
    btSpeedLimit = 1.0;
    manualFanSpeedPercent = 0;
    out.println("default values restored");
  }
  else if (strcmp(buffer, "btbrk") == 0) {
    out.println("breaking");
    out.println(" -- CONNECTION LOST --");
    triggerBtBreak();
  }
  else if (strcmp(buffer, "temp") == 0) {
    out.print("current temperature: ");
    out.print(lastKnownTempC);
    out.println("C");
  }
  else if (strcmp(buffer, "help") == 0) {
    out.println("Available commands:");
    out.println("  reboot <arduino|esc|bt>");
    out.println("  ver <arduino|firmware|bt>");
    out.println("  stats <status on|off> | stats delay <seconds>");
    out.println("  tare ampSensor");
    out.println("  motor <L|R> <speed%> | motor off");
    out.println("  fanrange <T_MIN> <T_MAX> | fanrange season <winter|summer> | fanrange show");
    out.println("  mspd <0-100> | mspd reset | mspd show");
    out.println("  fanspd set <0-100> | fanspd show");
    out.println("  fanctrl <auto|manual|off|show>");
    out.println("  ping");
    out.println("  whois");
    out.println("  reset");
    out.println("  btbrk");
    out.println("  temp");
  }
  else {
    out.print("E0CMD - unknown command: ");
    out.println(buffer);
    out.println("Refer to the documentation for valid commands.");
  }
}

void setupCommunication() {
  BT_SERIAL.begin(BT_BAUD);
  pinMode(BT_BRK_PIN, OUTPUT);
  digitalWrite(BT_BRK_PIN, HIGH);

  Serial.println("Module Communication (BT + Serie) pret. Tapez vos commandes.");
}

void updateCommunication(int swdRaw) {
  unsigned long currentMillis = millis();

  // 1. MACHINE D'ÉTAT DE DÉCONNEXION BLUETOOTH (non bloquante, partagée par
  //    la commande "btbrk" qu'elle arrive du BT ou du port série)
  if (breakState == BT_WAIT_PRINT) {
    if (currentMillis - breakTimer >= 50) {
      digitalWrite(BT_BRK_PIN, LOW);
      breakState = BT_BREAK_LOW;
      breakTimer = currentMillis;
    }
  }
  else if (breakState == BT_BREAK_LOW) {
    if (currentMillis - breakTimer >= 200) {
      digitalWrite(BT_BRK_PIN, HIGH);
      breakState = BT_NORMAL;
    }
  }

  // 2. LECTURE DES COMMANDES REÇUES PAR BLUETOOTH
  while (BT_SERIAL.available() > 0) {
    char c = BT_SERIAL.read();

    if (c == '\n' || c == '\r') {
      if (btIndex > 0) {
        btBuffer[btIndex] = '\0';
        btIndex = 0;
        processCommand(btBuffer, BT_SERIAL);
      }
    }
    else if (btIndex < sizeof(btBuffer) - 1) {
      btBuffer[btIndex++] = c;
    }
  }

  // 3. LECTURE DES COMMANDES REÇUES PAR LE MONITEUR SÉRIE USB
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (serialIndex > 0) {
        serialBuffer[serialIndex] = '\0';
        serialIndex = 0;
        processCommand(serialBuffer, Serial);
      }
    }
    else if (serialIndex < sizeof(serialBuffer) - 1) {
      serialBuffer[serialIndex++] = c;
    }
  }

  // 4. GESTION DES VENTILATEURS
  if (swdRaw < 1250) {
    OCR2B = 0;
    modeVentilation = "OFF";
    vitesseActuellePourcent = 0;
  }
  else if (swdRaw >= 1250 && swdRaw <= 1750) {
    int pwmValue = map(manualFanSpeedPercent, 0, 100, 0, 79);
    OCR2B = constrain(pwmValue, 0, 79);
    modeVentilation = "MANUAL";
    vitesseActuellePourcent = manualFanSpeedPercent;
  }
  else {
    int fanAutoSpeed = map((int)lastKnownTempC, dynTempMin, dynTempMax, 0, 79);
    OCR2B = constrain(fanAutoSpeed, 0, 79);
    modeVentilation = "AUTO";
    vitesseActuellePourcent = map(OCR2B, 0, 79, 0, 100);
  }

  // 5. LECTURE DE LA TEMPÉRATURE (indépendante de l'état de la télémétrie)
  if (currentMillis - lastTempReadTime >= TEMP_READ_INTERVAL_MS) {
    lastKnownTempC = tempsensor.readTempC();
    lastTempReadTime = currentMillis;
  }

  // 6. ENVOI DE LA TÉLÉMÉTRIE (uniquement si activée)
  // FIX : la télémétrie n'était envoyée que sur BT_SERIAL (Serial1), jamais sur
  // Serial (moniteur série USB). Résultat : impossible de voir les stats depuis
  // le moniteur série, même avec "stats status on". On envoie maintenant la
  // même ligne sur les deux interfaces.
  // FEATURE : setupComplete empêche l'envoi des stats tant que setup() n'est
  // pas totalement terminé, pour ne jamais les mélanger avec les messages
  // d'initialisation affichés sur le moniteur série.
  if (setupComplete && isTelemetryActive && (currentMillis - lastTempSendTime >= telemetryIntervalMs)) {
    float amps = getCurrentAmps();
    int mainPct = getBatteryPercentage(MAIN_BATTERY_PIN);
    int turretPct = getBatteryPercentage(TURRET_BATTERY_PIN);

    Print* outputs[2] = { &BT_SERIAL, &Serial };
    for (int i = 0; i < 2; i++) {
      Print& out = *outputs[i];
      out.print("T:");
      out.print(lastKnownTempC);
      out.print("C | MAIN:");
      out.print(mainPct);
      out.print("% | TUR:");
      out.print(turretPct);
      out.print("% | ");
      out.print(amps, 1);
      out.println("A");
    }

    lastTempSendTime = currentMillis;
  }
}