#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

#include "Buzzer.h"
#include "Config.h"
#include "LegoDuploTrain.h"
#include "PowerManager.h"
#include "SnesController.h"
#include "TrainController.h"

namespace {

SnesController snes;
PowerManager power;
LegoDuploTrain train;
Buzzer buzzer;
TrainController trainController;

volatile bool sleepRequested = false;
uint32_t lastRawPrintMs = 0;
uint32_t lastLowBatteryWarningMs = 0;

void applySystemPowerSaving() {
  if (Config::kLimitCpuFrequency) {
    setCpuFrequencyMhz(Config::kCpuFrequencyMhz);
  }

  if (Config::kDisableWifi) {
    esp_wifi_stop();
    esp_wifi_deinit();
  }
}

void printWakeupReason() {
  const esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  switch (reason) {
    case ESP_SLEEP_WAKEUP_GPIO:
      DEBUG_PRINTLN("Wakeup: GPIO");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      DEBUG_PRINTLN("Wakeup: timer");
      break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      DEBUG_PRINTLN("Wakeup: power on/reset");
      break;
    default:
      DEBUG_PRINTF("Wakeup: reason %d\n", static_cast<int>(reason));
      break;
  }
}

void onButtonEvent(const ButtonEvent& event) {
  const uint32_t nowMs = millis();
  power.markActivity(nowMs);

  DEBUG_PRINTF("SNES: %-6s %-8s held=%lu ms raw=0x%04X\n",
               SnesController::buttonName(event.button),
               SnesController::eventName(event.type),
               static_cast<unsigned long>(event.heldMs),
               snes.rawState());

  if (event.type == ButtonEventType::LongPress && event.button == SnesButton::Start) {
    sleepRequested = true;
  }

  if (event.type == ButtonEventType::ShortPress && event.button == SnesButton::Start) {
    train.requestPairing();
  }

  if (event.type == ButtonEventType::LongPress && event.button == SnesButton::Select) {
    train.resetPairing();
  }

  trainController.handleButtonEvent(event);
}

}  // namespace

void setup() {
  applySystemPowerSaving();

  if (Config::kDebugSerial) {
    Serial.begin(Config::kSerialBaud);
    delay(500);
  }

  DEBUG_PRINTLN();
  DEBUG_PRINTLN("ESPTrain SNES BLE controller");
  printWakeupReason();

  buzzer.begin();
  power.begin();
  snes.setEventCallback(onButtonEvent);
  snes.begin();
  train.begin();
  trainController.begin(train, buzzer);

  DEBUG_PRINTLN("Setup: complete");
}

void loop() {
  const uint32_t nowMs = millis();

  snes.update(nowMs);
  train.update(nowMs);
  trainController.update(nowMs);
  buzzer.update(nowMs);
  power.update(nowMs);

  if (Config::kPrintSnesRawState && nowMs - lastRawPrintMs >= Config::kSnesRawPrintIntervalMs) {
    lastRawPrintMs = nowMs;
    DEBUG_PRINTF("SNES raw=0x%04X\n", snes.rawState());
  }

  if (power.isBatteryLow() && !power.isBatteryCritical() &&
      nowMs - lastLowBatteryWarningMs >= Config::kLowBatteryWarningIntervalMs) {
    lastLowBatteryWarningMs = nowMs;
    buzzer.error();
  }

  if (sleepRequested || power.shouldSleep(nowMs) || power.isBatteryCritical()) {
    if (power.shouldSleep(nowMs)) {
      DEBUG_PRINTLN("Power: inactivity timeout");
    }
    if (power.isBatteryCritical()) {
      DEBUG_PRINTLN("Power: battery critical");
    }
    trainController.stopForSleep();
    train.disconnect();
    power.enterDeepSleep();
    sleepRequested = false;
    power.markActivity(millis());
  }

  if (Config::kLoopIdleDelayMs > 0) {
    delay(Config::kLoopIdleDelayMs);
  }
}
