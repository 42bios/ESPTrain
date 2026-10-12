#include "TrainController.h"
#include "Config.h"

namespace {

constexpr uint8_t kLightColors[] = {
    Config::kLightYellow,
    Config::kLightBlue,
    Config::kLightGreen,
    Config::kLightOrange,
    Config::kLightRed,
    Config::kLightMagenta,
};

constexpr size_t kLightColorCount = sizeof(kLightColors) / sizeof(kLightColors[0]);

bool isTrainAction(SnesButton button) {
  switch (button) {
    case SnesButton::Up:
    case SnesButton::Down:
    case SnesButton::Left:
    case SnesButton::Right:
    case SnesButton::A:
    case SnesButton::B:
    case SnesButton::X:
    case SnesButton::Y:
    case SnesButton::L:
    case SnesButton::R:
      return true;
    case SnesButton::Select:
    case SnesButton::Start:
    case SnesButton::Count:
      return false;
  }
  return false;
}

}  // namespace

void TrainController::begin(LegoDuploTrain& train, Buzzer& buzzer) {
  train_ = &train;
  buzzer_ = &buzzer;
  preferences_.begin("trainctl", false);
  lightColorIndex_ = preferences_.getUChar("lightColor", 0) % kLightColorCount;
  lastConnected_ = train_->isConnected();
}

void TrainController::update(uint32_t nowMs) {
  handleConnectionTone();

  if (directionChangePending_ && static_cast<int32_t>(nowMs - directionChangeAtMs_) >= 0) {
    directionChangePending_ = false;
    applyLevel(direction_ * Config::kTrainDirectionChangeLevel, false);
  }
}

void TrainController::handleButtonEvent(const ButtonEvent& event) {
  const bool shortEvent = event.type == ButtonEventType::ShortPress;
  const bool longEvent = event.type == ButtonEventType::LongPress;

  if (event.type == ButtonEventType::Pressed && buzzer_ != nullptr) {
    buzzer_->click();
  }

  if (!shortEvent && !longEvent) {
    return;
  }

  if (train_ == nullptr || !train_->isConnected()) {
    if (isTrainAction(event.button) && buzzer_ != nullptr) {
      buzzer_->error();
    }
    return;
  }

  switch (event.button) {
    case SnesButton::Up:
      if (shortEvent) {
        direction_ = 1;
        if (level_ < 0) {
          applyLevel(abs(level_));
        } else if (level_ == 0) {
          applyLevel(Config::kTrainStartLevel);
        }
      }
      break;
    case SnesButton::Down:
      if (shortEvent) {
        direction_ = -1;
        if (level_ > 0) {
          applyLevel(-level_);
        } else if (level_ == 0) {
          applyLevel(-Config::kTrainStartLevel);
        }
      }
      break;
    case SnesButton::Right:
      if (shortEvent) {
        applyLevel(level_ + direction_);
      }
      break;
    case SnesButton::Left:
      if (shortEvent) {
        if (level_ > 0) {
          applyLevel(level_ - 1);
        } else if (level_ < 0) {
          applyLevel(level_ + 1);
        }
      }
      break;
    case SnesButton::A:
      if (shortEvent) {
        stopMotion();
      }
      break;
    case SnesButton::B:
      if (shortEvent || longEvent) {
        train_->playHorn();
      }
      break;
    case SnesButton::X:
      if (shortEvent) {
        train_->playWaterSound();
      }
      break;
    case SnesButton::Y:
      if (shortEvent) {
        reverseDirectionSlowly();
      }
      break;
    case SnesButton::L:
      if (shortEvent) {
        toggleLight();
      }
      break;
    case SnesButton::R:
      if (shortEvent) {
        cycleLightColor();
      }
      break;
    default:
      break;
  }
}

void TrainController::stopForSleep() {
  stopMotion();
  if (buzzer_ != nullptr) {
    buzzer_->sleep();
  }
}

int8_t TrainController::clampLevel(int value) const {
  return static_cast<int8_t>(constrain(value, Config::kTrainLevelMin, Config::kTrainLevelMax));
}

void TrainController::applyLevel(int8_t level, bool cancelPendingChange) {
  if (train_ == nullptr) {
    return;
  }

  if (cancelPendingChange) {
    cancelPendingDirectionChange();
  }

  level_ = clampLevel(level);
  if (level_ > 0) {
    direction_ = 1;
  } else if (level_ < 0) {
    direction_ = -1;
  }

  train_->setSpeed(level_ * Config::kTrainPowerPerLevel);
}

void TrainController::stopMotion() {
  if (train_ == nullptr) {
    return;
  }

  cancelPendingDirectionChange();
  train_->stop();
  level_ = 0;
}

void TrainController::toggleLight() {
  lightOn_ = !lightOn_;
  if (lightOn_) {
    train_->setLight(kLightColors[lightColorIndex_]);
  } else {
    train_->turnLightOff();
  }
}

void TrainController::cycleLightColor() {
  lightColorIndex_ = (lightColorIndex_ + 1) % kLightColorCount;
  preferences_.putUChar("lightColor", lightColorIndex_);
  lightOn_ = true;
  train_->setLight(kLightColors[lightColorIndex_]);
}

void TrainController::reverseDirectionSlowly() {
  direction_ = level_ >= 0 ? -1 : 1;

  if (level_ == 0) {
    applyLevel(direction_ * Config::kTrainDirectionChangeLevel);
    return;
  }

  train_->stop();
  level_ = 0;
  directionChangePending_ = true;
  directionChangeAtMs_ = millis() + Config::kTrainDirectionChangePauseMs;
  DEBUG_PRINTF("Train: direction change pending %d\n", direction_);
}

void TrainController::cancelPendingDirectionChange() {
  directionChangePending_ = false;
}

void TrainController::handleConnectionTone() {
  if (train_ == nullptr || buzzer_ == nullptr) {
    return;
  }

  const bool connected = train_->isConnected();
  if (connected == lastConnected_) {
    return;
  }

  lastConnected_ = connected;
  if (connected) {
    level_ = 0;
    buzzer_->connected();
  } else {
    level_ = 0;
    cancelPendingDirectionChange();
    buzzer_->disconnected();
  }
}
