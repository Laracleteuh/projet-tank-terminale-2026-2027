#include "ISR.h"
#include "Config.h"
#include <Arduino.h>
#include <EnableInterrupt.h>


static volatile int yRaw_v = 1500, xRaw_v = 1500;
static volatile int swcRaw_v = 1500, swdRaw_v = 1500, vraRaw_v = 1500;
static volatile int turretY_v = 1500, turretX_v = 1500;

static volatile unsigned long timer_y, timer_x, timer_swc, timer_swd, timer_vra, timer_tY, timer_tX;
static volatile unsigned long lastRcSignalTime = 0;

static volatile int vrbRaw_v = 1500;
static volatile unsigned long timer_vrb;

// --- FONCTIONS D'INTERRUPTION ---
void isr_y() {
  if (digitalRead(rightJoystickY) == HIGH) timer_y = micros();
  else {
    yRaw_v = (int)(micros() - timer_y);
    lastRcSignalTime = millis();
  }
} 

void isr_x() {
  if (digitalRead(rightJoystickX) == HIGH) timer_x = micros();
  else xRaw_v = (int)(micros() - timer_x);
}

void isr_swc() {
  if (digitalRead(SWC_PIN) == HIGH) timer_swc = micros();
  else swcRaw_v = (int)(micros() - timer_swc);
}

void isr_swd() {
  if (digitalRead(SWD_PIN) == HIGH) timer_swd = micros();
  else swdRaw_v = (int)(micros() - timer_swd);
}

void isr_vra() {
  if (digitalRead(VRA_PIN) == HIGH) timer_vra = micros();
  else vraRaw_v = (int)(micros() - timer_vra);
}

void isr_vrb() {
  if (digitalRead(VRB_PIN) == HIGH) timer_vrb = micros();
  else vrbRaw_v = (int)(micros() - timer_vrb);
}

void isr_turretY() {
  if (digitalRead(leftJoystickY) == HIGH) timer_tY = micros();
  else turretY_v = (int)(micros() - timer_tY);
}

void isr_turretX() {
  if (digitalRead(leftJoystickX) == HIGH) timer_tX = micros();
  else turretX_v = (int)(micros() - timer_tX);
}

// --- INITIALISATION ---
void setupRecepteur() {
  enableInterrupt(rightJoystickY, isr_y, CHANGE);
  enableInterrupt(rightJoystickX, isr_x, CHANGE);
  enableInterrupt(SWC_PIN, isr_swc, CHANGE);
  enableInterrupt(SWD_PIN, isr_swd, CHANGE);
  enableInterrupt(VRA_PIN, isr_vra, CHANGE);
  enableInterrupt(VRB_PIN, isr_vrb, CHANGE);
  enableInterrupt(leftJoystickY, isr_turretY, CHANGE);
  enableInterrupt(leftJoystickX, isr_turretX, CHANGE);
}

// -- Failsafe -- 
bool checkFailsafe() {
  return (millis() - lastRcSignalTime > 200);
}

// -- RC Data --
RcData getRecepteurData() {
  RcData data;
  
  noInterrupts();
  data.yRaw = yRaw_v;
  data.xRaw = xRaw_v;
  data.swcRaw = swcRaw_v;
  data.swdRaw = swdRaw_v;
  data.vraRaw = vraRaw_v;
  data.vrbRaw = vrbRaw_v;
  data.turretY = turretY_v;
  data.turretX = turretX_v;
  interrupts();
  
  return data;
}