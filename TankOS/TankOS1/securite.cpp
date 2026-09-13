#include "Securite.h"
#include "Systeme.h"
#include "Config.h"
#include <Arduino_FreeRTOS.h>
#include <math.h>

// --- Etat interne des alertes (pour ne pas spammer Serial/BT) ---
static bool mainBattAlertActive = false;
static bool turretBattAlertActive = false;
static bool overcurrentAlertActive = false;

static TickType_t lastMainAlertTick = 0;
static TickType_t lastTurretAlertTick = 0;
static TickType_t lastOvercurrentAlertTick = 0;

// -- Verrouille l'accès Serial/BT_SERIAL pour un bloc d'impression atomique --
static bool serialLock() {
  if (serialMutex == NULL) return true;
  return xSemaphoreTake(serialMutex, pdMS_TO_TICKS(20)) == pdTRUE;
}

static void serialUnlock() {
  if (serialMutex != NULL) xSemaphoreGive(serialMutex);
}

void gererAlertesBatterie(int mainPct, int turretPct, float currentAmps) {
  TickType_t now = xTaskGetTickCount();
  static TickType_t lastBlinkTick = 0;
  static bool blinkState = false;

  // -- LED de statut (basée sur la batterie principale) --
  if (mainPct <= LOW_BATTERY_THRESHOLD_PCT) {
    if ((now - lastBlinkTick) * portTICK_PERIOD_MS > 500) {
      blinkState = !blinkState;
      if (blinkState) setStatusColor(255, 0, 0);
      else setStatusColor(0, 0, 0);
      lastBlinkTick = now;
    }
  } else {
    setStatusColor(0, 255, 0);
  }

  // -- Alerte batterie principale faible --
  if (mainPct <= LOW_BATTERY_THRESHOLD_PCT) {
    if (!mainBattAlertActive || (now - lastMainAlertTick) * portTICK_PERIOD_MS >= ALERT_REPEAT_INTERVAL_MS) {
      if (serialLock()) {
        Serial.print(F("W0BATT - batterie principale faible : "));
        Serial.print(mainPct);
        Serial.println(F("%"));
        BT_SERIAL.print(F("W0BATT - batterie principale faible : "));
        BT_SERIAL.print(mainPct);
        BT_SERIAL.println(F("%"));
        serialUnlock();
      }
      lastMainAlertTick = now;
      mainBattAlertActive = true;
    }
  } else if (mainBattAlertActive) {
    if (serialLock()) {
      Serial.println(F("I0BATT - batterie principale revenue a un niveau normal."));
      BT_SERIAL.println(F("I0BATT - batterie principale revenue a un niveau normal."));
      serialUnlock();
    }
    mainBattAlertActive = false;
  }

  // -- Alerte batterie tourelle faible --
  if (turretPct <= LOW_BATTERY_THRESHOLD_PCT) {
    if (!turretBattAlertActive || (now - lastTurretAlertTick) * portTICK_PERIOD_MS >= ALERT_REPEAT_INTERVAL_MS) {
      if (serialLock()) {
        Serial.print(F("W0BATT - batterie tourelle faible : "));
        Serial.print(turretPct);
        Serial.println(F("%"));
        BT_SERIAL.print(F("W0BATT - batterie tourelle faible : "));
        BT_SERIAL.print(turretPct);
        BT_SERIAL.println(F("%"));
        serialUnlock();
      }
      lastTurretAlertTick = now;
      turretBattAlertActive = true;
    }
  } else if (turretBattAlertActive) {
    if (serialLock()) {
      Serial.println(F("I0BATT - batterie tourelle revenue a un niveau normal."));
      BT_SERIAL.println(F("I0BATT - batterie tourelle revenue a un niveau normal."));
      serialUnlock();
    }
    turretBattAlertActive = false;
  }

  // -- Alerte sur-intensite --
  if (fabs(currentAmps) >= OVERCURRENT_THRESHOLD_AMPS) {
    if (!overcurrentAlertActive || (now - lastOvercurrentAlertTick) * portTICK_PERIOD_MS >= ALERT_REPEAT_INTERVAL_MS) {
      if (serialLock()) {
        Serial.print(F("E0CURR - surintensite detectee : "));
        Serial.print(currentAmps, 1);
        Serial.println(F("A"));
        BT_SERIAL.print(F("E0CURR - surintensite detectee : "));
        BT_SERIAL.print(currentAmps, 1);
        BT_SERIAL.println(F("A"));
        serialUnlock();
      }
      lastOvercurrentAlertTick = now;
      overcurrentAlertActive = true;
    }
  } else if (overcurrentAlertActive) {
    if (serialLock()) {
      Serial.println(F("I0CURR - intensite revenue a la normale."));
      BT_SERIAL.println(F("I0CURR - intensite revenue a la normale."));
      serialUnlock();
    }
    overcurrentAlertActive = false;
  }
}

float calculerBridageBatterie(int batteryPct) {
  if (batteryPct <= 20) {
    return map(batteryPct, 0, 20, 30, 100) / 100.0f;
  }
  return 1.0f;
}

float calculerBridageCourant(float currentAmps) {
  if (fabs(currentAmps) >= OVERCURRENT_THRESHOLD_AMPS) {
    return 0.2f;
  }
  return 1.0f;
}