#include "Buzzer.h"
#include "Config.h"

void Buzzer::begin() {
  if (!Config::kUseBuzzer) {
    return;
  }

  pinMode(Config::kBuzzerPin, OUTPUT);
  digitalWrite(Config::kBuzzerPin, LOW);
}

void Buzzer::update(uint32_t nowMs) {
  if (!Config::kUseBuzzer) {
    return;
  }

  if (playing_ && static_cast<int32_t>(nowMs - stepEndsAtMs_) < 0) {
    return;
  }

  if (playing_) {
    noTone(Config::kBuzzerPin);
    playing_ = false;
  }

  startNext(nowMs);
}

void Buzzer::click() {
  enqueue(Config::kBuzzerClickHz, Config::kBuzzerClickMs);
}

void Buzzer::connected() {
  enqueue(1400, 45);
  enqueue(0, 35);
  enqueue(1800, 55);
}

void Buzzer::disconnected() {
  enqueue(500, 90);
}

void Buzzer::error() {
  enqueue(300, 120);
}

void Buzzer::sleep() {
  enqueue(1200, 60);
  enqueue(0, 35);
  enqueue(800, 80);
  enqueue(0, 35);
  enqueue(500, 120);
}

void Buzzer::enqueue(uint16_t frequencyHz, uint16_t durationMs) {
  if (!Config::kUseBuzzer) {
    return;
  }

  if (count_ >= Config::kBuzzerQueueLength) {
    return;
  }

  queue_[tail_] = ToneStep{frequencyHz, durationMs};
  tail_ = (tail_ + 1) % Config::kBuzzerQueueLength;
  ++count_;
}

void Buzzer::startNext(uint32_t nowMs) {
  if (count_ == 0) {
    return;
  }

  const ToneStep step = queue_[head_];
  head_ = (head_ + 1) % Config::kBuzzerQueueLength;
  --count_;

  if (step.frequencyHz > 0) {
    tone(Config::kBuzzerPin, step.frequencyHz);
  }

  playing_ = true;
  stepEndsAtMs_ = nowMs + step.durationMs;
}
