// =============================================================================================
// Programme Projet Tank Terminale - Spé SI
// Version : 2026.5_PUBLIC_RELEASE
// Carte : Arduino Mega 2560
// Baudrate : 115200
// Last Build : 30.08.2026 => SUCCESS
// Last Board upload : 30.08.2026 => SUCCESS
//
// ATTENTION : Ce programme à été compilé avec l'outil PlatformIO et
//             contient plusieurs fichiers. Assurez vous d'utiliser
//             PlatformIO ou tout autre logiciel simmilaire pour
//             build le code. 
//
// Pour build le code, configurez PlatformIO, puis dans le sous
// dossier /src/, coller l'ensemble des fichiers : 
//
// - Config.h
// - ISR.cpp
// - ISR.h
// - main.cpp
// - Systeme.cpp
// - Systeme.h
// - Tourelle.cpp
// - Tourelle.h
// - Traction.cpp
// - Traction.h
// - communication.cpp   (fusion de bluetooth.cpp + test.cpp)
// - communication.h     (fusion de bluetooth.h + test.h)
//
// Puis entrez le raccourci "Ctrl + Alt + B" pour build le projet.
// Si tout ce passe bien, une mention [SUCCESS] devrait apparaître.
// =============================================================================================


#include <Arduino.h>  // Librairie PlatformIO

#include "Config.h"   
#include "Systeme.h"  
#include "Traction.h"
#include "Tourelle.h"
#include "ISR.h"
#include "communication.h" // FIX : remplace bluetooth.h + test.h
#include "batterie.h"
#include "Securite.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Startup du firmware Projet Tank Terminale - Version " + String(FIRMWARE_VERSION));
  Serial.println("Last build : " + String(LAST_BUILD_DATE));
  Serial.println("Author : " + String(FIRMWARE_AUTHOR));
  initStatusLED();
  setStatusColor(255, 0, 0);
  delay(500);
  Serial.println("Initialisation des modules...");
  
  setupCommunication(); // FIX : remplace setupBluetooth() + setupTestModule()
  setupBatterie();

  setStatusColor(0, 0, 255);

  setupSysteme();
  setupTraction();

  pinMode(MDRVCT_FAILSAFE, OUTPUT); // FIX : pinMode manquant avant l'écriture ci-dessous
  digitalWrite(MDRVCT_FAILSAFE, LOW);

  setupTourelle();
  setupRecepteur();

  delay(500);
  setStatusColor(0, 255, 0);
  Serial.println("Initialisation terminee. Firmware pret à l'emploi.");

  // FEATURE : à appeler en tout dernier, une fois que tous les messages de
  // setup() ont été affichés sur le moniteur série. Débloque l'envoi de la
  // télémétrie périodique (stats), qui reste bloquée jusqu'ici.
  markSetupComplete();
}

void loop() {
  int mainBatteryPct = getBatteryPercentage(MAIN_BATTERY_PIN);
  RcData rc = getRecepteurData();
  bool failsafeActive = checkFailsafe();

  int speed = 0;
  int steering = 0;

  if (failsafeActive) {
    stopTraction();
    stopTourelle();
    setTractionPowerSwitch(false);
    setStatusColor(255, 255, 0); // Jaune - failsafe actif
  } else {
    setTractionPowerSwitch(true);
    gererAlertesBatterie(mainBatteryPct);

    // -- Application des Limites (Limp Mode batterie x Bride Bluetooth) --
    float safetyFactor = calculerBridageBatterie(mainBatteryPct);
    float userMaxSpeed = getBluetoothSpeedLimit();
    float totalFactor = safetyFactor * userMaxSpeed;

    speed = rcToSpeed(rc.yRaw) * totalFactor;
    steering = rcToSpeed(rc.xRaw) * totalFactor;

    // -- Switchs --
    updateSwitch(rc.swcRaw);

    // -- Contrôle tourelle --
    updateEscPower(rc.vraRaw);
    updateTourelleElevation(rc.turretY);

    int tSpeed = 0;
    if (rc.turretX > 900) {
      tSpeed = rcToSpeed(rc.turretX);
    }
    updateTourelleRotation(tSpeed);
  }

  // updateTraction() gère elle-même le mode diagnostic moteur (bypass volontaire
  // de la radio pour les tests au banc). On l'appelle donc toujours : hors mode
  // diagnostic, speed/steering valent 0 pendant le failsafe donc rien ne bouge ;
  // en mode diagnostic, le test au banc reste utilisable même sans signal radio.
  updateTraction(speed, steering, rc.vrbRaw);

  // Continue de tourner même en failsafe : ventilation/température, télémétrie,
  // et écoute des commandes reçues par Bluetooth ET par le port série USB
  // (utile pour diagnostiquer ou désactiver le mode diagnostic à distance).
  updateCommunication(rc.swdRaw); // FIX : remplace updateBluetoothAndFans() + updateTestModule()
}
