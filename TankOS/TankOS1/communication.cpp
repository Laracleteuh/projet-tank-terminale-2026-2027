#include "communication.h"
#include "batterie.h"
#include "Config.h"
#include "Traction.h"
#include "ISR.h"
#include "Securite.h"
#include "Tourelle.h"
#include "admin.h" 
#include "systeme.h"

#include <Arduino_FreeRTOS.h>
#include <Adafruit_MCP9808.h>

extern Adafruit_MCP9808 tempsensor;

#define BT_SERIAL Serial1
#define BT_BAUD 9600

static bool setupIsComplete = false;
static TickType_t lastTempSendTick = 0;
static TickType_t lastTempReadTick = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 1000;
unsigned long telemetryIntervalMs = 1000;

bool isTelemetryActive = true;
int manualFanSpeedPercent = 0;
float lastKnownTempC = 25.0;

const char* modeVentilation = "OFF"; 
int vitesseActuellePourcent = 0;
float btSpeedLimit = 1.0;

int dynTempMin = 25;
int dynTempMax = 45;

static char btBuffer[32];
static uint8_t btIndex = 0;
static char serialBuffer[32];
static uint8_t serialIndex = 0;

enum BtBreakState { BT_NORMAL, BT_WAIT_PRINT, BT_BREAK_LOW };
static BtBreakState breakState = BT_NORMAL;
static TickType_t breakTickTimer = 0;

void markSetupComplete() {
  setupIsComplete = true;
}

void triggerBtBreak() {
  if (breakState == BT_NORMAL) {
    breakState = BT_WAIT_PRINT;
    breakTickTimer = xTaskGetTickCount();
  }
}

float getBluetoothSpeedLimit() {
  return btSpeedLimit;
}

static void processCommand(char* buffer, Print& out) {
  if (isAdminCommand(buffer) && !isAdminUnlocked()) {
    out.println(F("E0AUTH - commande reservee a l'admin. Badgez le lecteur NFC pour deverrouiller."));
    return;
  }

  if (strncmp(buffer, "reboot ", 7) == 0) {
    char* argument = buffer + 7;
    if (strcmp(argument, "arduino") == 0 || strcmp(argument, "tank") == 0) {
      out.println(F("rebooting board"));
      rebootTank(); 
    }
    else if (strcmp(argument, "esc") == 0) {
      out.println(F("resetting ESC"));
      setupTourelle();
      out.println(F("done"));
    }
    else if (strcmp(argument, "bt") == 0) {
      out.println(F("rebooting bluetooth module"));
      BT_SERIAL.print(F("AT+RESET")); 
      out.println(F("done"));
    }
    else {
      out.println(F("E0RB - unknown reboot argument"));
    }
  }
  else if (strncmp(buffer, "ver ", 4) == 0) {
    char* argument = buffer + 4;
    if (strcmp(argument, "arduino") == 0) {
      out.println(F("Arduino Mega 2560 - ATMEGA2560"));
    }
    else if (strcmp(argument, "firmware") == 0) {
      out.print(F("firmware version: "));
      out.print(F(FIRMWARE_VERSION));
      out.print(F(" | last build: "));
      out.println(F(LAST_BUILD_DATE));
    }
    else if (strcmp(argument, "drivers") == 0) {
      out.println(F("Drivers versions:"));
      out.print(F(" - Admin handler: ")); out.println(F(AUTH_HANDLER_VERSION));
      out.print(F(" - Battery driver: ")); out.println(F(BATT_DRIVER_VERSION));
      out.print(F(" - Communication driver: ")); out.println(F(COM_DRIVER_VERSION));
      out.print(F(" - ISR handler: ")); out.println(F(ISR_HANDLER_VERSION));
      out.print(F(" - Traction driver: ")); out.println(F(TRACTION_MODULE_VERSION));
      out.print(F(" - Turret driver: ")); out.println(F(TOURELLE_DRIVER_VERSION));
      out.print(F(" - System module: ")); out.println(F(SYSTEM_MODULE_VERSION));
      out.print(F(" - Security module: ")); out.println(F(SECURITY_MODULE_VERSION));
    }
    else {
      out.println(F("E0VR - unknown ver argument"));
    }
  }
  else if (strncmp(buffer, "stats ", 6) == 0) {
    char* arg = buffer + 6;
    if (strncmp(arg, "status ", 7) == 0) {
      char* val = arg + 7;
      if (strcmp(val, "on") == 0) {
        isTelemetryActive = true;
        out.print(F("continuous stats on - "));
        out.print(telemetryIntervalMs);
        out.println(F(" ms interval"));
      }
      else if (strcmp(val, "off") == 0) {
        isTelemetryActive = false;
        out.println(F("continuous stats off"));
      }
    }
    else if (strncmp(arg, "delay ", 6) == 0) {
      int delaySec = atoi(arg + 6);
      if (delaySec > 0) {
        telemetryIntervalMs = delaySec * 1000;
        out.print(F("stats delay set to  "));
        out.print(delaySec);
        out.println(F(" second(s)"));
      } else {
        out.println(F("E0SD - delay must be > 0"));
      }
    }
    else if (strcmp(arg, "battery") == 0) {
      int mainPct = getBatteryPercentage(MAIN_BATTERY_PIN);
      int turretPct = getBatteryPercentage(TURRET_BATTERY_PIN);
      float amps = getCurrentAmps();
      out.print(F("I0ST - battery main: "));
      out.print(mainPct);
      out.print(F("% | turret: "));
      out.print(turretPct);
      out.print(F("% | current: "));
      out.print(amps, 2);
      out.println(F(" A"));
    }
    else if (strcmp(arg, "ram") == 0) {
      out.println(F("--- FreeRTOS RAM Manager (mots/words) ---"));
      out.print(F("Task Safety :   ")); out.println(uxTaskGetStackHighWaterMark(hTaskSafety));
      out.print(F("Task Traction : ")); out.println(uxTaskGetStackHighWaterMark(hTaskTraction));
      out.print(F("Task Tourelle : ")); out.println(uxTaskGetStackHighWaterMark(hTaskTourelle));
      out.print(F("Task Comm :     ")); out.println(uxTaskGetStackHighWaterMark(hTaskCommunication));
      out.print(F("Task Admin :    ")); out.println(uxTaskGetStackHighWaterMark(hTaskAdmin));
      out.println(F("---------------------------------------------------"));
      out.print(F("Available RAM (bytes) : "));
      out.println(uxTaskGetStackHighWaterMark(NULL) * sizeof(configSTACK_DEPTH_TYPE));
    }
    else {
      out.println(F("E0ST - unknown stats argument"));
    }
  }
  else if (strncmp(buffer, "tare ", 5) == 0) {
    char* arg = buffer + 5;
    if (strcmp(arg, "ampSensor") == 0) {
      tareCurrentSensor();
      out.println(F("ampSensor tare complete."));
    } else {
      out.println(F("E0TA - unknown tare argument"));
    }
  }
  else if (strncmp(buffer, "motor ", 6) == 0) {
    char* arg = buffer + 6;
    if (strcmp(arg, "off") == 0) {
      disableMotorDiagnostic();
      out.println(F("I0MD - motor diagnostic disabled"));
    }
    else {
      char side;
      int spd;
      if (sscanf(arg, "%c %d", &side, &spd) == 2) {
        if (side == 'L' || side == 'R') {
          spd = constrain(spd, -100, 100);
          setMotorDiagnostic(side, spd);
          out.print(F("I0MD - motor diagnostic enabled for "));
          out.print(side);
          out.print(F(" a "));
          out.print(spd);
          out.println(F("%"));
        } else {
          out.println(F("E0MD - invalid motor argument"));
        }
      } else {
        out.println(F("E0MD - invalid motor command syntax"));
      }
    }
  }
  else if (strncmp(buffer, "fanrange ", 9) == 0) {
    char* arg = buffer + 9;
    if (strcmp(arg, "season winter") == 0) {
      dynTempMin = 15;
      dynTempMax = 35;
      out.println(F("I0FR - winter season active (15C - 35C)."));
    }
    else if (strcmp(arg, "season summer") == 0) {
      dynTempMin = 30;
      dynTempMax = 55;
      out.println(F("I0FR - summer season active (30C - 55C)."));
    }
    else if (strcmp(arg, "show") == 0) {
      out.print(F("I0FR - current thermal range : "));
      out.print(dynTempMin);
      out.print(F("C -> "));
      out.print(dynTempMax);
      out.println(F("C"));
    }
    else {
      int tmin = 0, tmax = 0;
      if (sscanf(arg, "%d %d", &tmin, &tmax) == 2) {
        if (tmin < tmax) {
          dynTempMin = tmin;
          dynTempMax = tmax;
          out.print(F("I0FR - new thermal range : "));
          out.print(dynTempMin);
          out.print(F("C -> "));
          out.print(dynTempMax);
          out.println(F("C"));
        } else {
          out.println(F("E0FR - T_MIN must be less than T_MAX."));
        }
      } else {
        out.println(F("E0FR - Error: Syntax"));
      }
    }
  }
  else if (strncmp(buffer, "mspd ", 5) == 0) {
    char* arg = buffer + 5;
    if (strcmp(arg, "reset") == 0) {
      btSpeedLimit = 1.0;
      out.println(F("I0MS - motor speed limit reset to 100%"));
    }
    else if (strcmp(arg, "show") == 0) {
      int currentLimitPercent = static_cast<int>(btSpeedLimit * 100);
      out.print(F("I0MS - current motor speed limit: "));
      out.print(currentLimitPercent);
      out.println(F("%"));
    }
    else {
      int val = atoi(arg);
      if (val >= 0 && val <= 100) {
        btSpeedLimit = val / 100.0f;
        out.print(F("I0MS - motor speed limit set to "));
        out.print(val);
        out.println(F("%"));
      } else {
        out.println(F("E0MS - invalid mspd value"));
      }
    }
  }
  else if (strncmp(buffer, "fanspd ", 7) == 0) {
    char* arg = buffer + 7;
    if (strcmp(arg, "show") == 0) {
      out.print(F("I0FS - current fan speed: "));
      out.print(manualFanSpeedPercent);
      out.println(F("%"));
    }
    else if (strcmp(arg, "set ") == 0) {
      int val = atoi(arg);
      if (strcmp(modeVentilation, "AUTO") == 0) {
        out.println(F("E0FS - cannot set fan speed in AUTO mode."));
      }
      else if (strcmp(modeVentilation, "OFF") == 0) {
        out.println(F("E0FS - cannot set fan speed when fans are OFF."));
      }
      else if (strcmp(modeVentilation, "MANUAL") == 0) {
        if (val >= 0 && val <= 100) {
          manualFanSpeedPercent = val;
          out.print(F("I0FS - fan speed set to "));
          out.print(val);
          out.println(F("%"));
        } else {
          out.println(F("E0FS - invalid fanspd value"));
        }
      }
    }
    else {
      out.println(F("E0FS - unknown fanspd argument."));
    }
  }
  else if (strncmp(buffer, "fanctrl ", 8) == 0) {
    char* arg = buffer + 8;
    if (strcmp(arg, "show") == 0) {
      out.print(F("I0FC - current fan control mode: "));
      out.println(modeVentilation);
    }
    else if (strcmp(arg, "auto") == 0) {
      modeVentilation = "AUTO";
      out.println(F("I0FC - fan control mode set to AUTO"));
    }
    else if (strcmp(arg, "manual") == 0) {
      modeVentilation = "MANUAL";
      out.println(F("I0FC - fan control mode set to MANUAL"));
    }
    else if (strcmp(arg, "off") == 0) {
      modeVentilation = "OFF";
      out.println(F("I0FC - fan control mode set to OFF"));
    }
    else {
      out.println(F("E0FC - unknown fanctrl argument."));
    }
  }
  else if (strncmp(buffer, "admin ", 6) == 0) {
    char* arg = buffer + 6;
    if (strcmp(arg, "UID_AUTH show") == 0) {
      printAuthorizedUids(out);
    }
    else if (strcmp(arg, "UID_AUTH add") == 0) {
      out.println(F("E0AUTH - not implemented"));
    }
    else if (strcmp(arg, "UID_AUTH remove") == 0) {
      out.println(F("E0AUTH - not implemented"));
    }
    else {
      out.println(F("E0AUTH - unknown admin argument"));
    }
  }
  else if (strcmp(buffer, "ping") == 0) {
    out.println(F("pong"));
  }
  else if (strcmp(buffer, "whois") == 0) {
    out.println(F(FIRMWARE_AUTHOR));
  }
  else if (strcmp(buffer, "reset") == 0) {
    btSpeedLimit = 1.0;
    manualFanSpeedPercent = 0;
    out.println(F("default values restored"));
  }
  else if (strcmp(buffer, "btbrk") == 0) {
    out.println(F("breaking"));
    out.println(F(" -- CONNECTION LOST --"));
    triggerBtBreak();
  }
  else if (strcmp(buffer, "temp") == 0) {
    out.print(F("current temperature: "));
    out.print(lastKnownTempC);
    out.println(F("C"));
  }
  else {
    out.print(F("E0CMD - unknown command: "));
    out.println(buffer);
  }
}

void setupCommunication() {
  BT_SERIAL.begin(BT_BAUD);
  pinMode(BT_BRK_PIN, OUTPUT);
  digitalWrite(BT_BRK_PIN, HIGH);
  Serial.println(F("Module Communication (BT + Serie) pret. Tapez vos commandes."));
}

void updateCommunication(int swdRaw) {
  TickType_t currentTick = xTaskGetTickCount();

  if (breakState == BT_WAIT_PRINT) {
    if ((currentTick - breakTickTimer) * portTICK_PERIOD_MS >= 50) {
      digitalWrite(BT_BRK_PIN, LOW);
      breakState = BT_BREAK_LOW;
      breakTickTimer = currentTick;
    }
  }
  else if (breakState == BT_BREAK_LOW) {
    if ((currentTick - breakTickTimer) * portTICK_PERIOD_MS >= 200) {
      digitalWrite(BT_BRK_PIN, HIGH);
      breakState = BT_NORMAL;
    }
  }

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

 
  if (isTempSensorPresent() && (currentTick - lastTempReadTick) * portTICK_PERIOD_MS >= TEMP_READ_INTERVAL_MS) {
    lastKnownTempC = tempsensor.readTempC();
    lastTempReadTick = currentTick;
  }


  if (setupIsComplete && isTelemetryActive && ((currentTick - lastTempSendTick) * portTICK_PERIOD_MS >= telemetryIntervalMs)) {
    float amps = getCurrentAmps();
    int mainPct = getBatteryPercentage(MAIN_BATTERY_PIN);
    int turretPct = getBatteryPercentage(TURRET_BATTERY_PIN);

    BT_SERIAL.print(F("T:"));
    BT_SERIAL.print(lastKnownTempC);
    BT_SERIAL.print(F("C | MAIN:"));
    BT_SERIAL.print(mainPct);
    BT_SERIAL.print(F("% | TUR:"));
    BT_SERIAL.print(turretPct);
    BT_SERIAL.print(F("% | "));
    BT_SERIAL.print(amps, 1);
    BT_SERIAL.println(F("A"));
    
    lastTempSendTick = currentTick;
  }
}