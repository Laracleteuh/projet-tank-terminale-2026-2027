#pragma once
#include <Arduino.h>

#define AUTH_HANDLER_VERSION "2026.2-RC522-RTOS"

void setupAdmin();
void updateAdmin();
bool isAdminUnlocked();
bool isAdminCommand(const char* buffer);
void printAuthorizedUids(Print& out);