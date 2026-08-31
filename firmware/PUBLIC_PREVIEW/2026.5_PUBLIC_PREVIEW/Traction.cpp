#include "Traction.h"
#include "Config.h"
#include <SPI.h> 

static float currentSmoothSpeed = 0.0;
static float currentSmoothSteering = 0.0;
static unsigned long lastRampTime = 0;

// FIX : borne max sur dt pour empêcher un "saut" de vitesse instantané.
// Si updateTraction() n'est pas appelée pendant un moment (mode diagnostic
// prolongé, ou tout autre gel temporaire), dt peut devenir très grand au
// prochain appel, ce qui annule totalement l'effet du ramping (maxDelta
// devient énorme et currentSmoothSpeed/Steering sautent directement à la
// cible au lieu d'y monter progressivement). On plafonne donc dt.
static const unsigned long MAX_RAMP_DT_MS = 50;

static bool diagMode = false;
static char diagSide = 'N';
static int diagSpeed = 0;

void setMotorDiagnostic(char side, int speed) {
  diagMode = true;
  diagSide = side;
  diagSpeed = speed;
}

void disableMotorDiagnostic() {
  diagMode = false;
  diagSide = 'N';
  diagSpeed = 0;
}

enum DirState { FWD, BCK, STOP };
enum MotorState { IDLE, SWITCHING, RUNNING };

struct Motor {
  int pinFwd, pinBck, pinCS; 
  DirState currentDir;
  MotorState state;
  int currentPot;
  unsigned long switchStartTime;
};

Motor leftMotor = {leftMotorFwdRelay, leftMotorBckRelay, CS_PIN1, STOP, IDLE, 0, 0};
Motor rightMotor = {rightMotorFwdRelay, rightMotorBckRelay, CS_PIN2, STOP, IDLE, 0, 0};

void setPot(Motor &m, int target) {
  m.currentPot = target; 
  
  // On map la vitesse de [0 à 100] vers les 256 niveaux du MCP [0 à 255]
  int mapValue = map(target, 0, 100, 0, 255);
  mapValue = constrain(mapValue, 0, 255);

  digitalWrite(m.pinCS, LOW);
  SPI.transfer(0x11); 
  SPI.transfer(mapValue); 
  digitalWrite(m.pinCS, HIGH);
}

void updateMotor(Motor &m, int speed) {
  DirState targetDir = (speed > 0) ? FWD : (speed < 0 ? BCK : STOP);
  int targetPot = abs(speed);

  switch (m.state) {
    case IDLE:
      if (speed != 0) {
        m.currentDir = targetDir;
        m.state = RUNNING;
      }
      break;

    case RUNNING:
      if (speed == 0) {
          relayWrite(m.pinFwd, false);
          relayWrite(m.pinBck, false);
          setPot(m, 0); 
          m.currentDir = STOP;
          m.state = IDLE;
      }
      else if (targetDir != m.currentDir && targetDir != STOP && m.currentDir != STOP) {
          setPot(m, 0);
          m.state = SWITCHING;
          m.switchStartTime = millis();
          relayWrite(m.pinFwd, false);
          relayWrite(m.pinBck, false);
      } else {
          relayWrite(m.pinFwd, (targetDir == FWD));
          relayWrite(m.pinBck, (targetDir == BCK));
          setPot(m, targetPot);
      }
      break;

    case SWITCHING:
      if (millis() - m.switchStartTime >= 200) { 
          m.currentDir = targetDir;
          m.state = RUNNING;
      }
      break;
  }
}

void setupTraction() {
  pinMode(leftMotorFwdRelay, OUTPUT);
  pinMode(leftMotorBckRelay, OUTPUT);
  pinMode(rightMotorFwdRelay, OUTPUT);
  pinMode(rightMotorBckRelay, OUTPUT);
  pinMode(leftMotorOnOffSwitch, OUTPUT);
  pinMode(rightMotorOnOffSwitch, OUTPUT);

  pinMode(CS_PIN1, OUTPUT); 
  pinMode(CS_PIN2, OUTPUT);

  digitalWrite(CS_PIN1, HIGH); 
  digitalWrite(CS_PIN2, HIGH);

  SPI.begin(); 

  setPot(leftMotor, 0);
  setPot(rightMotor, 0);
}

void updateTraction(int speed, int steering, int vrbRaw) {
  // 1. GESTION DU MODE DIAGNOSTIC
  if (diagMode) {
    if (diagSide == 'L') {
      updateMotor(leftMotor, diagSpeed);
      updateMotor(rightMotor, 0); // Arrête le droit par sécurité
    } else if (diagSide == 'R') {
      updateMotor(leftMotor, 0);
      updateMotor(rightMotor, diagSpeed); // Arrête le gauche par sécurité
    }
    return; // /!\ TRÈS IMPORTANT : On quitte la fonction ici pour ignorer la radio !
  }
  unsigned long currentTime = millis();
  if (lastRampTime == 0) lastRampTime = currentTime;
  unsigned long dt = currentTime - lastRampTime;
  lastRampTime = currentTime;

  // FIX : voir le commentaire sur MAX_RAMP_DT_MS plus haut.
  if (dt > MAX_RAMP_DT_MS) dt = MAX_RAMP_DT_MS;

  int safeVrb = constrain(vrbRaw, 1000, 2000);
  
  // Si VRB = 1000 -> coef = 0.01 (Très doux / lent)
  // Si VRB = 2000 -> coef = 1.0 (Très nerveux / instantané)
  float rampingFactor = map(safeVrb, 1000, 2000, 1, 100) / 100.0;
  
  float maxDelta = rampingFactor * dt;

  if (speed > currentSmoothSpeed + maxDelta) currentSmoothSpeed += maxDelta;
  else if (speed < currentSmoothSpeed - maxDelta) currentSmoothSpeed -= maxDelta;
  else currentSmoothSpeed = speed;

  if (steering > currentSmoothSteering + maxDelta) currentSmoothSteering += maxDelta;
  else if (steering < currentSmoothSteering - maxDelta) currentSmoothSteering -= maxDelta;
  else currentSmoothSteering = steering;

  int baseSpeed = (int)currentSmoothSpeed;
  int steer = (int)currentSmoothSteering;
  int leftSpeed = baseSpeed;
  int rightSpeed = baseSpeed;

  if (steer > 0) {
    if (baseSpeed >= 0) rightSpeed = baseSpeed - steer;
    else leftSpeed = baseSpeed + steer;
  } 
  else if (steer < 0) {
    if (baseSpeed >= 0) leftSpeed = baseSpeed - abs(steer);
    else rightSpeed = baseSpeed + abs(steer);
  }

  updateMotor(leftMotor, leftSpeed);
  updateMotor(rightMotor, rightSpeed);
}

void stopTraction() {
  relayWrite(leftMotorFwdRelay, false);
  relayWrite(leftMotorBckRelay, false);
  relayWrite(rightMotorFwdRelay, false);
  relayWrite(rightMotorBckRelay, false);
  setPot(leftMotor, 0);
  setPot(rightMotor, 0);
  leftMotor.state = IDLE;
  rightMotor.state = IDLE;
  leftMotor.currentDir = STOP;
  rightMotor.currentDir = STOP;
}

void setTractionPowerSwitch(bool state) {
  relayWrite(leftMotorOnOffSwitch, state);
  relayWrite(rightMotorOnOffSwitch, state);
}
