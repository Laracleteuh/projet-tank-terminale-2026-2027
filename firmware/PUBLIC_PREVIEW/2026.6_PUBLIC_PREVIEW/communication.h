#pragma once
#include <Arduino.h>

// =============================================================================================
// communication.h — fusion de bluetooth.h + test.h
//
// Un seul point d'entrée pour toutes les commandes texte du tank, qu'elles
// arrivent par le module Bluetooth HM-10 (Serial1) ou par le moniteur série
// USB (Serial). Toute la logique de commande vit dans communication.cpp,
// dans une seule fonction partagée : plus besoin de dupliquer un ajout de
// commande dans deux fichiers différents.
// =============================================================================================

#define BT_DRIVER_VERSION "BT_DRIVER_2026.1"
#define BT_HARDWARE "HM-10 Arduino Module, please refer to the module's documentation for more information."

#define COM_DRIVER_VERSION "COM_DRIVER_2026.3 using BT_DRIVER_2026.1-HM10"

// Initialise le port Bluetooth (Serial1), le pin BRK, et affiche le message
// de bienvenue sur le moniteur série USB.
void setupCommunication();

// À appeler dans loop(). Gère :
//  - la lecture/interprétation des commandes reçues par Bluetooth
//  - la lecture/interprétation des commandes reçues par le moniteur série USB
//  - la machine à états non bloquante de coupure BT (btbrk)
//  - la lecture température + gestion ventilateurs (dépend de swdRaw)
//  - l'envoi de la télémétrie périodique par Bluetooth
void updateCommunication(int swdRaw);

// Facteur de bride vitesse (0.0 - 1.0) réglable via la commande "mspd"
float getBluetoothSpeedLimit();

// Arme la séquence non bloquante de coupure de connexion BT (utilisée par
// la commande "btbrk", accessible depuis le BT ou le port série)
void triggerBtBreak();

// À appeler une seule fois, en toute dernière ligne de setup() dans main.cpp.
// Tant que cette fonction n'a pas été appelée, la télémétrie périodique
// (bloc "T:...") ne sera pas envoyée, même si isTelemetryActive vaut true.
// Objectif : ne jamais mélanger les stats avec les messages d'initialisation
// affichés sur le moniteur série pendant setup().
void markSetupComplete();