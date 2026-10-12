#pragma once

#include <Arduino.h>
#include "Config.h"

enum class SnesButton : uint8_t {
  B = 0,
  Y,
  Select,
  Start,
  Up,
  Down,
  Left,
  Right,
  A,
  X,
  L,
  R,
  Count
};

enum class ButtonEventType : uint8_t {
  Pressed,
  Released,
  Held,
  ShortPress,
  LongPress
};

struct ButtonEvent {
  SnesButton button;
  ButtonEventType type;
  uint32_t heldMs;
};

// Reads the original SNES button PCB through its serial latch/clock/data pins.
class SnesController {
 public:
  using EventCallback = void (*)(const ButtonEvent& event);

  void begin();
  void update(uint32_t nowMs);
  void setEventCallback(EventCallback callback);
  bool isPressed(SnesButton button) const;
  uint16_t rawState() const;

  static const char* buttonName(SnesButton button);
  static const char* eventName(ButtonEventType type);

 private:
  struct ButtonState {
    bool stablePressed = false;
    bool candidatePressed = false;
    uint8_t candidateSamples = 0;
    uint32_t pressedAtMs = 0;
    uint32_t lastHoldEventMs = 0;
    bool longPressSent = false;
  };

  uint16_t readRawBits() const;
  bool rawButtonPressed(uint16_t raw, SnesButton button) const;
  void handleButton(SnesButton button, bool pressed, uint32_t nowMs);
  void emit(SnesButton button, ButtonEventType type, uint32_t heldMs) const;

  ButtonState states_[static_cast<uint8_t>(SnesButton::Count)];
  EventCallback callback_ = nullptr;
  uint32_t lastPollMs_ = 0;
  uint16_t rawState_ = 0xFFFF;
};
