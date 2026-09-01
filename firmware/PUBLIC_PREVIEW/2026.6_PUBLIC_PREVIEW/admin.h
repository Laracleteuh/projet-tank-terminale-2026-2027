#pragma once
#include <Arduino.h>

#define AUTH_HANDLER_VERSION "2026.1-RC522"

// =============================================================================================
// admin.h — Verrouillage des commandes de diagnostic via badge RFID (RC522)
//
// Un lecteur RC522 (SPI, partage le bus avec les MCP41010) surveille en
// tache de fond la presence d'un badge autorise. Premier passage ->
// deverrouille les commandes "admin" (diagnostic moteur, reboot, tare,
// reglages thermiques). Second passage du meme badge -> reverrouille.
//
// ATTENTION (a garder en tete) : ceci est un verrou de confort, pas un
// systeme de securite cryptographique. L'UID d'une carte Mifare/RFID se
// lit et se copie facilement. Suffisant pour eviter qu'un utilisateur
// lambda ne tape "reboot arduino" ou "motor L 80" par erreur/curiosite,
// pas pour resister a une personne motivee a le contourner.
// =============================================================================================

// Initialise le lecteur RC522 (bus SPI + broches SS/RST). A appeler dans setup().
void setupAdmin();

// A appeler a CHAQUE tour de boucle (non bloquant, se limite lui-meme en
// frequence de scan via millis() en interne).
void updateAdmin();

// true si le mode admin est actuellement deverrouille.
bool isAdminUnlocked();

// true si la commande (deja isolee, sans le retour a la ligne) fait partie
// des commandes reservees a l'admin.
bool isAdminCommand(const char* buffer);

void printAuthorizedUids(Print& out);