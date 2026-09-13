// =============================================================================================
// Programme Projet Tank Terminale - Spé SI
// Version : TankOS 1 (Basé sur FreeRTOS pour microcontrôleur AVR)
// Révision : 13 Septembre 2026
// Carte : Arduino Mega 2560
// Baudrate : 115200
// =============================================================================================

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <avr/wdt.h>

// ==============================================================================
// S'exécute matériellement AVANT le démarrage du framework Arduino.
// Il garantit que le Watchdog est neutralisé dès la mise sous tension.
// ==============================================================================
#ifdef __cplusplus
extern "C" {
#endif

void wdt_init(void) __attribute__((naked, used, section(".init3")));

void wdt_init(void) {
    MCUSR = 0;       // Etape 1 OBLIGATOIRE : Effacer le drapeau de reset
    wdt_disable();   // Etape 2 : Désactiver le chien de garde
} // <--- PREMIÈRE ACCOLADE ICI : Elle ferme la fonction wdt_init

#ifdef __cplusplus
} // <--- SECONDE ACCOLADE ICI : Elle ferme le bloc "extern C"
#endif 

#include "Config.h"
#include "Systeme.h"
#include "Traction.h"
#include "Tourelle.h"
#include "ISR.h"
#include "communication.h"
#include "batterie.h"
#include "Securite.h"
#include "admin.h"

// -- Définition des handles/mutex déclarés extern dans Config.h --
TaskHandle_t hTaskSafety = NULL;
TaskHandle_t hTaskTraction = NULL;
TaskHandle_t hTaskTourelle = NULL;
TaskHandle_t hTaskCommunication = NULL;
TaskHandle_t hTaskAdmin = NULL;

SemaphoreHandle_t spiMutex = NULL;
SemaphoreHandle_t stateMutex = NULL;
SemaphoreHandle_t serialMutex = NULL;

// -- État partagé, calculé par TaskSafety et consommé par TaskTraction/TaskTourelle --
static volatile bool  g_failsafeActive = true;
static volatile float g_safetyFactor   = 0.0f;

void TaskSafety(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    bool failsafe = checkFailsafe();
    int mainPct = getBatteryPercentage(MAIN_BATTERY_PIN);
    float amps = getCurrentAmps();

    float battSafety = calculerBridageBatterie(mainPct);
    float currentSafety = calculerBridageCourant(amps);
    float safety = min(battSafety, currentSafety);

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      g_failsafeActive = failsafe;
      g_safetyFactor = safety;
      xSemaphoreGive(stateMutex);
    }

    if (failsafe) {
      stopTraction();
      stopTourelle();
      setTractionPowerSwitch(false);
      setStatusColor(255, 255, 0);
    } else {
      setTractionPowerSwitch(true);
      int turretPct = getBatteryPercentage(TURRET_BATTERY_PIN);
      gererAlertesBatterie(mainPct, turretPct, amps);
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void TaskTraction(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    RcData rc = getRecepteurData();

    bool failsafe = true;
    float safetyFactor = 0.0f;
    
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      failsafe = g_failsafeActive;
      safetyFactor = g_safetyFactor;
      xSemaphoreGive(stateMutex);
    }

    int speed = 0;
    int steering = 0;

    if (!failsafe) {
      float userMaxSpeed = getBluetoothSpeedLimit();
      float totalFactor = safetyFactor * userMaxSpeed;
      speed = rcToSpeed(rc.yRaw) * totalFactor;
      steering = rcToSpeed(rc.xRaw) * totalFactor;
    }

    updateTraction(speed, steering, rc.vrbRaw);

    vTaskDelay(pdMS_TO_TICKS(34));
  }
}

void TaskTourelle(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    RcData rc = getRecepteurData();

    bool failsafe = true;
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      failsafe = g_failsafeActive;
      xSemaphoreGive(stateMutex);
    }

    if (!failsafe) {
      updateSwitch(rc.swcRaw);
      updateEscPower(rc.vraRaw);
      updateTourelleElevation(rc.turretY);

      int tSpeed = 0;
      if (rc.turretX > 900) {
        tSpeed = rcToSpeed(rc.turretX);
      }
      updateTourelleRotation(tSpeed);
    }

    vTaskDelay(pdMS_TO_TICKS(34));
  }
}

void TaskCommunication(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    RcData rc = getRecepteurData();
    updateCommunication(rc.swdRaw);

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void TaskAdmin(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    updateAdmin();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  Serial.print(F("E0STACK - overflow tache: "));
  Serial.println(pcTaskName);
  Serial.flush();
  while (1) {}
}

void setup() {

  _delay_ms(1500);
  Serial.begin(115200);
  
  // Utilisation de F() pour ne pas surcharger la RAM
  Serial.print(F("Demarrage de "));
  Serial.println(F(FIRMWARE_VERSION));
  Serial.print(F("Last build : "));
  Serial.println(F(LAST_BUILD_DATE));
  Serial.print(F("Auteur : "));
  Serial.println(F(FIRMWARE_AUTHOR));
  
  initStatusLED();
  setStatusColor(255, 0, 0);
  _delay_ms(500);
  
  Serial.println(F("Initialisation des modules..."));

  setupCommunication();
  setupBatterie();
  setupAdmin();

  setStatusColor(0, 0, 255);

  setupSysteme();
  setupTraction();

  pinMode(MDRVCT_FAILSAFE, OUTPUT);
  digitalWrite(MDRVCT_FAILSAFE, LOW);

  setupTourelle();
  setupRecepteur();

  spiMutex = xSemaphoreCreateMutex();
  stateMutex = xSemaphoreCreateMutex();
  serialMutex = xSemaphoreCreateMutex();

  if (spiMutex == NULL || stateMutex == NULL || serialMutex == NULL) {
    Serial.println(F("E0RTOS - echec creation des mutex. Arret."));
    while (1) {}
  }

  BaseType_t okCommunication = xTaskCreate(TaskCommunication, "Communication", STACK_COMMUNICATION, NULL, PRIO_COMMUNICATION, &hTaskCommunication);
  BaseType_t okSafety = xTaskCreate(TaskSafety, "Safety", STACK_SAFETY, NULL, PRIO_SAFETY, &hTaskSafety);
  BaseType_t okTraction = xTaskCreate(TaskTraction, "Traction", STACK_TRACTION, NULL, PRIO_TRACTION, &hTaskTraction);
  BaseType_t okTourelle = xTaskCreate(TaskTourelle, "Tourelle", STACK_TOURELLE, NULL, PRIO_TOURELLE, &hTaskTourelle);
  BaseType_t okAdmin = xTaskCreate(TaskAdmin, "Admin", STACK_ADMIN, NULL, PRIO_ADMIN, &hTaskAdmin);

  if (okCommunication != pdPASS || okSafety != pdPASS || okTraction != pdPASS ||
      okTourelle != pdPASS || okAdmin != pdPASS) {
    Serial.print(F("E0RTOS - echec creation tache(s) : "));
    if (okCommunication != pdPASS) Serial.print(F("Communication "));
    if (okSafety != pdPASS) Serial.print(F("Safety "));
    if (okTraction != pdPASS) Serial.print(F("Traction "));
    if (okTourelle != pdPASS) Serial.print(F("Tourelle "));
    if (okAdmin != pdPASS) Serial.print(F("Admin "));
    Serial.println();
    while (1) {}
  }
  
  _delay_ms(500);

  tareCurrentSensor();
  
  setStatusColor(0, 255, 0);
  Serial.println(F("Initialisation terminee. Demarrage du scheduler FreeRTOS"));

  markSetupComplete();

  vTaskStartScheduler();

  Serial.println(F("E0RTOS - echec demarrage du scheduler."));
  while (1) {}
}

void loop() {
  // Vide
}