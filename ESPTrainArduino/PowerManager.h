#pragma once

#include <Arduino.h>
#include "Config.h"

// Tracks inactivity, optional battery voltage, and the prepared deep-sleep path.
class PowerManager {
 public:
  void begin();
  void update(uint32_t nowMs);
  void markActivity(uint32_t nowMs);
  bool shouldSleep(uint32_t nowMs) const;
  void enterDeepSleep();
  float batteryVoltage() const;
  bool isBatteryLow() const;
  bool isBatteryCritical() const;

 private:
  float readBatteryVoltage() const;
  void printBatteryIfDue(uint32_t nowMs);

  uint32_t lastActivityMs_ = 0;
  uint32_t lastBatteryPrintMs_ = 0;
  float batteryVoltage_ = 0.0f;
};
