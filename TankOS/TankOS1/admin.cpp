#include "admin.h"
#include "Config.h"
#include <SPI.h>
#include <MFRC522.h>
#include <string.h>
#include <Arduino_FreeRTOS.h>

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

static bool adminUnlocked = false;
static TickType_t lastToggleTick = 0;
static TickType_t lastPollTick = 0;
static const unsigned long TOGGLE_COOLDOWN_MS = 2000;
static const unsigned long POLL_INTERVAL_MS = 250;

struct AuthorizedUid {
  uint8_t uid[10];
  uint8_t length;
};

static const AuthorizedUid authorizedUids[] = {
  // { {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 4 },
  { {0x13, 0xD2, 0x0D, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 4 },
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
  SPI.begin();
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  rfid.PCD_Init();
  Serial.println("Module Admin (RFID RC522) pret. Badgez pour deverrouiller le diagnostic.");
}

bool isAdminUnlocked() {
  return adminUnlocked;
}

void updateAdmin() {
  TickType_t currentTick = xTaskGetTickCount();
  if ((currentTick - lastPollTick) * portTICK_PERIOD_MS < POLL_INTERVAL_MS) return;
  lastPollTick = currentTick;

  if (spiMutex != NULL) {
    if (xSemaphoreTake(spiMutex, pdMS_TO_TICKS(5)) != pdTRUE) {
      return;
    }
  }

  bool cardPresent = rfid.PICC_IsNewCardPresent();
  bool cardRead = false;
  
  if (cardPresent) {
    cardRead = rfid.PICC_ReadCardSerial();
  }

  if (cardPresent && cardRead) {
    if ((currentTick - lastToggleTick) * portTICK_PERIOD_MS >= TOGGLE_COOLDOWN_MS) {
      if (isAuthorized(rfid.uid.uidByte, rfid.uid.size)) {
        adminUnlocked = !adminUnlocked;
        lastToggleTick = currentTick;
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
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  if (spiMutex != NULL) {
    xSemaphoreGive(spiMutex);
  }
}

bool isAdminCommand(const char* buffer) {
  if (strncmp(buffer, "motor ", 6) == 0) return true;
  if (strncmp(buffer, "reboot ", 7) == 0) return true;
  if (strncmp(buffer, "tare ", 5) == 0) return true;
  if (strncmp(buffer, "admin ", 6) == 0) return true;
  if (strncmp(buffer, "fanrange ", 9) == 0) {
    if (strcmp(buffer + 9, "show") == 0) return false;
    return true;
  }
  return false;
}