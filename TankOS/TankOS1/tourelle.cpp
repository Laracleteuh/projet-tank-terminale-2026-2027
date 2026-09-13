#include "Tourelle.h"
#include "Config.h"
#include <Servo.h>
#include <Arduino_FreeRTOS.h>

Servo servoElev1;
Servo servoElev2;
Servo esc1;
Servo esc2;

enum TurretDir { T_LEFT, T_RIGHT, T_STOP };
TurretDir currentTurretDir = T_STOP;
TickType_t turretSwitchTick = 0;
bool turretSwitching = false;

void setupTourelle() {
  servoElev1.attach(servo1Tourelle);
  servoElev2.attach(servo2Tourelle);
  esc1.attach(esc1Tourelle);
  esc2.attach(esc2Tourelle);

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
      turretSwitchTick = xTaskGetTickCount();
      analogWrite(motorDriverENA, 0); 
      digitalWrite(motorDriverIN1, LOW);
      digitalWrite(motorDriverIN2, LOW);
      currentTurretDir = T_STOP;
  }

  if (turretSwitching) {
      if ((xTaskGetTickCount() - turretSwitchTick) * portTICK_PERIOD_MS > 200) {
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
    int escPwm = constrain(vraRaw, 1000, 2000);
    esc1.writeMicroseconds(escPwm);
    esc2.writeMicroseconds(escPwm);
  } else {
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