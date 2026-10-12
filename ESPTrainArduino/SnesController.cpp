#include "SnesController.h"

namespace {

constexpr uint8_t kButtonToBit[] = {
    0,   // B
    1,   // Y
    2,   // Select
    3,   // Start
    4,   // Up
    5,   // Down
    6,   // Left
    7,   // Right
    8,   // A
    9,   // X
    10,  // L
    11,  // R
};

static_assert(sizeof(kButtonToBit) == static_cast<uint8_t>(SnesButton::Count),
              "SNES button map must cover every button");

}  // namespace

void SnesController::begin() {
  pinMode(Config::kSnesLatchPin, OUTPUT);
  pinMode(Config::kSnesClockPin, OUTPUT);
  pinMode(Config::kSnesDataPin, INPUT_PULLUP);

  digitalWrite(Config::kSnesLatchPin, LOW);
  digitalWrite(Config::kSnesClockPin, HIGH);
  rawState_ = readRawBits();
}

void SnesController::update(uint32_t nowMs) {
  if (nowMs - lastPollMs_ < Config::kSnesPollIntervalMs) {
    return;
  }
  lastPollMs_ = nowMs;

  rawState_ = readRawBits();
  for (uint8_t index = 0; index < static_cast<uint8_t>(SnesButton::Count); ++index) {
    const auto button = static_cast<SnesButton>(index);
    handleButton(button, rawButtonPressed(rawState_, button), nowMs);
  }
}

void SnesController::setEventCallback(EventCallback callback) {
  callback_ = callback;
}

bool SnesController::isPressed(SnesButton button) const {
  return states_[static_cast<uint8_t>(button)].stablePressed;
}

uint16_t SnesController::rawState() const {
  return rawState_;
}

uint16_t SnesController::readRawBits() const {
  uint16_t raw = 0;

  digitalWrite(Config::kSnesLatchPin, HIGH);
  delayMicroseconds(Config::kSnesLatchPulseUs);
  digitalWrite(Config::kSnesLatchPin, LOW);
  delayMicroseconds(Config::kSnesClockPulseUs);

  for (uint8_t bit = 0; bit < Config::kSnesRawBitCount; ++bit) {
    if (digitalRead(Config::kSnesDataPin) == HIGH) {
      raw |= (1U << bit);
    }

    digitalWrite(Config::kSnesClockPin, LOW);
    delayMicroseconds(Config::kSnesClockPulseUs);
    digitalWrite(Config::kSnesClockPin, HIGH);
    delayMicroseconds(Config::kSnesClockPulseUs);
  }

  return raw;
}

bool SnesController::rawButtonPressed(uint16_t raw, SnesButton button) const {
  const uint8_t index = static_cast<uint8_t>(button);
  const bool bitHigh = (raw & (1U << kButtonToBit[index])) != 0;
  return Config::kSnesActiveLow ? !bitHigh : bitHigh;
}

void SnesController::handleButton(SnesButton button, bool pressed, uint32_t nowMs) {
  ButtonState& state = states_[static_cast<uint8_t>(button)];

  if (pressed != state.candidatePressed) {
    state.candidatePressed = pressed;
    state.candidateSamples = 1;
  } else if (state.candidateSamples < Config::kDebounceSamples) {
    ++state.candidateSamples;
  }

  if (state.candidateSamples < Config::kDebounceSamples) {
    return;
  }

  if (state.stablePressed != state.candidatePressed) {
    state.stablePressed = state.candidatePressed;

    if (state.stablePressed) {
      state.pressedAtMs = nowMs;
      state.lastHoldEventMs = nowMs;
      state.longPressSent = false;
      emit(button, ButtonEventType::Pressed, 0);
    } else {
      const uint32_t heldMs = nowMs - state.pressedAtMs;
      emit(button, ButtonEventType::Released, heldMs);
      if (!state.longPressSent && heldMs < Config::kLongPressMs) {
        emit(button, ButtonEventType::ShortPress, heldMs);
      }
    }

    return;
  }

  if (!state.stablePressed) {
    return;
  }

  const uint32_t heldMs = nowMs - state.pressedAtMs;
  if (!state.longPressSent && heldMs >= Config::kLongPressMs) {
    state.longPressSent = true;
    emit(button, ButtonEventType::LongPress, heldMs);
  }

  if (nowMs - state.lastHoldEventMs >= Config::kHoldRepeatMs) {
    state.lastHoldEventMs = nowMs;
    emit(button, ButtonEventType::Held, heldMs);
  }
}

void SnesController::emit(SnesButton button, ButtonEventType type, uint32_t heldMs) const {
  if (callback_ != nullptr) {
    callback_(ButtonEvent{button, type, heldMs});
  }
}

const char* SnesController::buttonName(SnesButton button) {
  switch (button) {
    case SnesButton::B: return "B";
    case SnesButton::Y: return "Y";
    case SnesButton::Select: return "Select";
    case SnesButton::Start: return "Start";
    case SnesButton::Up: return "Up";
    case SnesButton::Down: return "Down";
    case SnesButton::Left: return "Left";
    case SnesButton::Right: return "Right";
    case SnesButton::A: return "A";
    case SnesButton::X: return "X";
    case SnesButton::L: return "L";
    case SnesButton::R: return "R";
    case SnesButton::Count: break;
  }
  return "?";
}

const char* SnesController::eventName(ButtonEventType type) {
  switch (type) {
    case ButtonEventType::Pressed: return "pressed";
    case ButtonEventType::Released: return "released";
    case ButtonEventType::Held: return "held";
    case ButtonEventType::ShortPress: return "short";
    case ButtonEventType::LongPress: return "long";
  }
  return "?";
}
