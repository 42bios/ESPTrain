#pragma once

#include <Arduino.h>
#include "Config.h"

// Optional short audio feedback for button, BLE, error, and sleep events.
class Buzzer {
 public:
  void begin();
  void update(uint32_t nowMs);
  void click();
  void connected();
  void disconnected();
  void error();
  void sleep();

 private:
  struct ToneStep {
    uint16_t frequencyHz;
    uint16_t durationMs;
  };

  void enqueue(uint16_t frequencyHz, uint16_t durationMs);
  void startNext(uint32_t nowMs);

  ToneStep queue_[Config::kBuzzerQueueLength];
  uint8_t head_ = 0;
  uint8_t tail_ = 0;
  uint8_t count_ = 0;
  uint32_t stepEndsAtMs_ = 0;
  bool playing_ = false;
};
