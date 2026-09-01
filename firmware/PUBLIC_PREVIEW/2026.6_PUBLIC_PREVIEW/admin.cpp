#include "admin.h"
#include "Config.h"
#include <SPI.h>
#include <MFRC522.h>
#include <string.h>

// -- Lecteur RFID RC522, sur le bus SPI0 partagé avec les MCP41010 de
//    Traction.cpp (SCK/MOSI/MISO déjà branchés en dur sur le Mega). Seules
//    deux broches supplémentaires sont nécessaires : SS (chip select) et
//    RST (voir RFID_SS_PIN / RFID_RST_PIN dans Config.h).
//    NOTE MATERIELLE : le RC522 est un composant 3.3V, le Mega pilote son
//    bus SPI en logique 5V. Des résistances série de 330Ω sur SCK et MOSI
//    (entre le Mega et le RC522) sont nécessaires pour une lecture UID
//    fiable — sans elles, la détection (REQA) fonctionne mais la lecture
//    de l'UID (anti-collision) échoue de façon intermittente.
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// -- État interne --
static bool adminUnlocked = false;
static unsigned long lastToggleTime = 0;
static unsigned long lastPollTime = 0;

static const unsigned long TOGGLE_COOLDOWN_MS = 2000;  // anti-rebond (évite de re-toggle
                                                        // tant que le même badge traîne
                                                        // encore dans le champ du lecteur)
static const unsigned long POLL_INTERVAL_MS = 250;      // FIX : limite la fréquence de scan.
                                                        // Sans ça, si une carte reste posée
                                                        // près du lecteur, on la redétecte à
                                                        // CHAQUE tour de loop() (potentiellement
                                                        // des milliers de fois/seconde), ce qui
                                                        // noie le port série sous les messages
                                                        // et empêche les vraies commandes
                                                        // (ping, etc.) d'obtenir une réponse
                                                        // dans un délai raisonnable. 250ms est
                                                        // largement suffisant pour un badge
                                                        // qu'on approche à la main.

// =============================================================================================
// LISTE DES BADGES AUTORISÉS
//
// Comment récupérer l'UID de votre badge :
//   1. Flashez ce code tel quel (liste vide -> aucun badge n'est autorisé).
//   2. Badgez votre carte : l'UID inconnu s'affiche sur le port série USB
//      (moniteur PlatformIO) sous la forme "E0AD - unknown badge UID: ...".
//   3. Copiez ces octets ci-dessous dans authorizedUids[], puis reflashez.
// =============================================================================================
struct AuthorizedUid {
  uint8_t uid[10]; // MFRC522::Uid::uidByte fait jusqu'à 10 octets (UIDs longs, rares)
  uint8_t length;
};

static const AuthorizedUid authorizedUids[] = {
  // Exemple (a remplacer par le vrai UID de votre badge admin) :
  // { {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 4 },
  { { 0x79, 0x43, 0xC1, 0x9F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 4 },
};
static const uint8_t authorizedCount = sizeof(authorizedUids) / sizeof(authorizedUids[0]);

static bool isAuthorized(byte* uid, byte uidLength) {
  for (uint8_t i = 0; i < authorizedCount; i++) {
    if (authorizedUids[i].length != uidLength) continue;
    if (memcmp(authorizedUids[i].uid, uid, uidLength) == 0) return true;
  }
  return false;
}

void printAuthorizedUids(Print& out) {
  out.println("Authorized UID(s):");
  for (uint8_t i = 0; i < authorizedCount; i++) {
    out.print(" - UID ");
    for (uint8_t j = 0; j < authorizedUids[i].length; j++) {
      if (authorizedUids[i].uid[j] < 0x10) out.print("0");
      out.print(authorizedUids[i].uid[j], HEX);
    }
    out.println();
  }
}

void setupAdmin() {
  // FIX : SPI.begin() est sans effet s'il a déjà été appelé (ex: par
  // setupTraction() pour les MCP41010). On l'appelle quand même ici pour
  // que setupAdmin() reste indépendant de l'ordre d'initialisation dans
  // setup().
  SPI.begin();

  // FIX : sur Mega, la broche SS matérielle (53) doit rester en OUTPUT
  // même si on ne l'utilise pas pour le RC522 (on utilise RFID_SS_PIN à la
  // place). Sinon, si elle reste en INPUT et qu'un signal externe la tire
  // à LOW, le contrôleur SPI peut basculer tout seul en mode esclave et
  // casser TOUTES les communications SPI du projet (RC522 ET MCP41010).
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);

  rfid.PCD_Init();

  Serial.println("Module Admin (RFID RC522) pret. Badgez pour deverrouiller le diagnostic.");
}

bool isAdminUnlocked() {
  return adminUnlocked;
}

void updateAdmin() {
  // FIX : limite la fréquence de scan à une tentative toutes les 250ms.
  // Sans ça, une carte laissée près du lecteur serait redétectée à CHAQUE
  // tour de loop() (potentiellement des milliers de fois/seconde), ce qui
  // finit par saturer le port série et retarder le traitement des vraies
  // commandes (ping, etc.). 250ms est largement suffisant pour un badge
  // qu'on approche à la main.
  unsigned long now = millis();
  if (now - lastPollTime < POLL_INTERVAL_MS) return;
  lastPollTime = now;

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  if (now - lastToggleTime >= TOGGLE_COOLDOWN_MS) {
    if (isAuthorized(rfid.uid.uidByte, rfid.uid.size)) {
      adminUnlocked = !adminUnlocked;
      lastToggleTime = now;
      Serial.println(adminUnlocked ? "I0AUTH - admin mode UNLOCKED" : "I0AUTH - admin mode LOCKED");
    } else {
      Serial.print("E0AUTH - unknown badge UID: ");
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
        Serial.print(rfid.uid.uidByte[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }

  // Termine proprement la communication avec la carte (bonne pratique
  // MFRC522, même sans authentification par clé).
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

bool isAdminCommand(const char* buffer) {
  // -- Diagnostic moteur : bypass total de la radio, doit rester admin --
  if (strncmp(buffer, "motor ", 6) == 0) return true;

  // -- Reboot (carte, ESC, module BT) : perturbe tout le systeme --
  if (strncmp(buffer, "reboot ", 7) == 0) return true;

  // -- Tare du capteur de courant : calibration, pas un besoin utilisateur --
  if (strncmp(buffer, "tare ", 5) == 0) return true;

  if (strncmp(buffer, "admin ", 6) == 0) return true;

  // -- Reglages thermiques : "fanrange show" reste public (lecture seule),
  //    modifier les seuils/saisons reste reserve a l'admin --
  if (strncmp(buffer, "fanrange ", 9) == 0) {
    if (strcmp(buffer + 9, "show") == 0) return false;
    return true;
  }

  return false;
}