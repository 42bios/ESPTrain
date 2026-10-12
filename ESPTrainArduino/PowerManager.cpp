#include "PowerManager.h"

#include <esp_sleep.h>

void PowerManager::begin() {
  lastActivityMs_ = millis();

  if (Config::kUseStatusLed) {
    pinMode(Config::kStatusLedPin, OUTPUT);
    digitalWrite(Config::kStatusLedPin, Config::kStatusLedActiveHigh ? HIGH : LOW);
  }

  if (Config::kUseBatteryMonitor) {
    analogReadResolution(12);
    batteryVoltage_ = readBatteryVoltage();
    DEBUG_PRINTF("Battery: %.2f V\n", batteryVoltage_);
  }

  DEBUG_PRINTLN("Power: ready");
}

void PowerManager::update(uint32_t nowMs) {
  printBatteryIfDue(nowMs);
}

void PowerManager::markActivity(uint32_t nowMs) {
  lastActivityMs_ = nowMs;
}

bool PowerManager::shouldSleep(uint32_t nowMs) const {
  return nowMs - lastActivityMs_ >= Config::kInactivitySleepMs;
}

void PowerManager::enterDeepSleep() {
  DEBUG_PRINTLN("Power: entering sleep");
  Serial.flush();

  if (Config::kUseStatusLed) {
    digitalWrite(Config::kStatusLedPin, Config::kStatusLedActiveHigh ? LOW : HIGH);
  }

  if (Config::kUseWakeupPin) {
    esp_deep_sleep_enable_gpio_wakeup(1ULL << Config::kWakeupPin,
                                      Config::kWakeupLevel == 0 ? ESP_GPIO_WAKEUP_GPIO_LOW
                                                               : ESP_GPIO_WAKEUP_GPIO_HIGH);
  }

  if (Config::kEnableDeepSleep) {
    esp_deep_sleep_start();
  } else {
    DEBUG_PRINTLN("Power: deep sleep disabled in Config.h");
  }
}

float PowerManager::batteryVoltage() const {
  return batteryVoltage_;
}

bool PowerManager::isBatteryLow() const {
  return Config::kUseBatteryMonitor && batteryVoltage_ > 0.0f &&
         batteryVoltage_ <= Config::kBatteryLowVoltage;
}

bool PowerManager::isBatteryCritical() const {
  return Config::kUseBatteryMonitor && batteryVoltage_ > 0.0f &&
         batteryVoltage_ <= Config::kBatteryCriticalVoltage;
}

float PowerManager::readBatteryVoltage() const {
  uint32_t rawSum = 0;
  for (uint8_t sample = 0; sample < Config::kBatterySampleCount; ++sample) {
    rawSum += analogRead(Config::kBatteryAdcPin);
  }

  const float rawAverage = static_cast<float>(rawSum) / Config::kBatterySampleCount;
  const float measured = (rawAverage / Config::kBatteryAdcMax) *
                         Config::kBatteryAdcReferenceVoltage;
  return measured * Config::kBatteryDividerRatio;
}

void PowerManager::printBatteryIfDue(uint32_t nowMs) {
  if (!Config::kUseBatteryMonitor) {
    return;
  }

  if (nowMs - lastBatteryPrintMs_ < Config::kBatteryPrintIntervalMs) {
    return;
  }

  lastBatteryPrintMs_ = nowMs;
  batteryVoltage_ = readBatteryVoltage();
  DEBUG_PRINTF("Battery: %.2f V", batteryVoltage_);
  if (isBatteryCritical()) {
    DEBUG_PRINT(" critical");
  } else if (isBatteryLow()) {
    DEBUG_PRINT(" low");
  }
  DEBUG_PRINTLN();
}
