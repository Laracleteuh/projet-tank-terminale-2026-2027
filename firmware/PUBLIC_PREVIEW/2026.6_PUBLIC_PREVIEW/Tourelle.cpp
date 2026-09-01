#include "Tourelle.h"
#include "Config.h"
#include <Servo.h>

Servo servoElev1;
Servo servoElev2;
Servo esc1;
Servo esc2;

enum TurretDir { T_LEFT, T_RIGHT, T_STOP };
TurretDir currentTurretDir = T_STOP;
unsigned long turretSwitchTime = 0;
bool turretSwitching = false;

void setupTourelle() {
  servoElev1.attach(servo1Tourelle);
  servoElev2.attach(servo2Tourelle);
  esc1.attach(esc1Tourelle);
  esc2.attach(esc2Tourelle);

  // Armement ESC
  esc1.writeMicroseconds(1000);
  esc2.writeMicroseconds(1000); 

  pinMode(motorDriverIN1, OUTPUT);
  pinMode(motorDriverIN2, OUTPUT);
  pinMode(motorDriverENA, OUTPUT);
}

void updateTourelleElevation(int turretY) {
  if (turretY > 900) {
    int anglePivot = map(turretY, rxMin, rxMax, 0, 180);
    anglePivot = constrain(anglePivot, 0, 180);
    servoElev1.write(anglePivot);
    servoElev2.write(anglePivot); 
  }
}

void updateTourelleRotation(int tSpeed) {
  TurretDir targetTDir = T_STOP;

  if (tSpeed > 0) targetTDir = T_RIGHT;
  else if (tSpeed < 0) targetTDir = T_LEFT;

  if (targetTDir != currentTurretDir && targetTDir != T_STOP && currentTurretDir != T_STOP) {
      turretSwitching = true;
      turretSwitchTime = millis();
      analogWrite(motorDriverENA, 0); 
      digitalWrite(motorDriverIN1, LOW);
      digitalWrite(motorDriverIN2, LOW);
      currentTurretDir = T_STOP;
  }

  if (turretSwitching) {
      if (millis() - turretSwitchTime > 200) {
          turretSwitching = false;
          currentTurretDir = targetTDir;
      }
  } else {
      currentTurretDir = targetTDir;
      int pwmOut = map(abs(tSpeed), 0, 100, 0, 255);

      if (currentTurretDir == T_RIGHT) {
          digitalWrite(motorDriverIN1, HIGH);
          digitalWrite(motorDriverIN2, LOW);
          analogWrite(motorDriverENA, pwmOut);
      } else if (currentTurretDir == T_LEFT) {
          digitalWrite(motorDriverIN1, LOW);
          digitalWrite(motorDriverIN2, HIGH);
          analogWrite(motorDriverENA, pwmOut);
      } else {
          digitalWrite(motorDriverIN1, LOW);
          digitalWrite(motorDriverIN2, LOW);
          analogWrite(motorDriverENA, 0);
      }
  }
}

void updateEscPower(int vraRaw) {
  if (vraRaw > 900) {
    // CORRECTION APPLIQUÉE : L'ESC attend un signal entre 1000 et 2000 ms.
    int escPwm = constrain(vraRaw, 1000, 2000);
    esc1.writeMicroseconds(escPwm);
    esc2.writeMicroseconds(escPwm);
  } else {
    // FIX : auparavant, rien ne se passait ici. Si le canal repassait sous le
    // seuil de 900 en fonctionnement normal (hors failsafe), les ESC gardaient
    // la dernière valeur envoyée (potentiellement pleine puissance) au lieu de
    // revenir au ralenti. On force explicitement le retour à l'inactif.
    esc1.writeMicroseconds(1000);
    esc2.writeMicroseconds(1000);
  }
}

void stopTourelle() {
  digitalWrite(motorDriverIN1, LOW);
  digitalWrite(motorDriverIN2, LOW);
  analogWrite(motorDriverENA, 0);
  esc1.writeMicroseconds(1000); 
  esc2.writeMicroseconds(1000);
}
