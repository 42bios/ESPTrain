#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "Buzzer.h"
#include "LegoDuploTrain.h"
#include "SnesController.h"

// Maps SNES button events to train actions and keeps local train UI state.
class TrainController {
 public:
  void begin(LegoDuploTrain& train, Buzzer& buzzer);
  void update(uint32_t nowMs);
  void handleButtonEvent(const ButtonEvent& event);
  void stopForSleep();

 private:
  int8_t clampLevel(int value) const;
  void applyLevel(int8_t level, bool cancelPendingChange = true);
  void stopMotion();
  void toggleLight();
  void cycleLightColor();
  void reverseDirectionSlowly();
  void cancelPendingDirectionChange();
  void handleConnectionTone();

  LegoDuploTrain* train_ = nullptr;
  Buzzer* buzzer_ = nullptr;
  int8_t level_ = 0;
  int8_t direction_ = 1;
  bool lightOn_ = false;
  uint8_t lightColorIndex_ = 0;
  bool directionChangePending_ = false;
  uint32_t directionChangeAtMs_ = 0;
  bool lastConnected_ = false;
  Preferences preferences_;
};
