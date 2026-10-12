#pragma once

#include <Arduino.h>

#define ESPTRAIN_DEBUG_SERIAL 1

namespace Config {

// Serial/debug output
constexpr bool kDebugSerial = ESPTRAIN_DEBUG_SERIAL != 0;
constexpr uint32_t kSerialBaud = 115200;

// SNES controller serial interface
constexpr uint8_t kSnesClockPin = 4;
constexpr uint8_t kSnesLatchPin = 5;
constexpr uint8_t kSnesDataPin = 6;
constexpr uint32_t kSnesPollIntervalMs = 10;
constexpr uint16_t kSnesLatchPulseUs = 12;
constexpr uint16_t kSnesClockPulseUs = 6;
constexpr uint8_t kSnesButtonCount = 12;
constexpr uint8_t kSnesRawBitCount = 16;
constexpr bool kSnesActiveLow = true;
constexpr bool kPrintSnesRawState = false;
constexpr uint32_t kSnesRawPrintIntervalMs = 1000;

constexpr uint8_t kDebounceSamples = 3;
constexpr uint32_t kLongPressMs = 1000;
constexpr uint32_t kHoldRepeatMs = 500;

// Optional status LED
constexpr bool kUseStatusLed = false;
constexpr uint8_t kStatusLedPin = 10;
constexpr bool kStatusLedActiveHigh = true;

// Optional passive buzzer. Use a transistor driver for low-impedance buzzers.
constexpr bool kUseBuzzer = false;
constexpr uint8_t kBuzzerPin = 7;
constexpr uint16_t kBuzzerClickHz = 1800;
constexpr uint16_t kBuzzerClickMs = 12;
constexpr uint8_t kBuzzerQueueLength = 8;

// Optional battery monitor. With a 100k/100k divider, multiply ADC voltage by 2.
constexpr bool kUseBatteryMonitor = false;
constexpr uint8_t kBatteryAdcPin = 0;
constexpr float kBatteryDividerRatio = 2.0f;
constexpr float kBatteryAdcReferenceVoltage = 3.3f;
constexpr uint16_t kBatteryAdcMax = 4095;
constexpr uint8_t kBatterySampleCount = 8;
constexpr uint32_t kBatteryPrintIntervalMs = 30000;
constexpr uint32_t kLowBatteryWarningIntervalMs = 60000;
constexpr float kBatteryLowVoltage = 3.45f;
constexpr float kBatteryCriticalVoltage = 3.25f;

// Power management
constexpr bool kDisableWifi = true;
constexpr bool kLimitCpuFrequency = true;
constexpr uint32_t kCpuFrequencyMhz = 80;
constexpr uint32_t kLoopIdleDelayMs = 1;
constexpr uint32_t kInactivitySleepMs = 5UL * 60UL * 1000UL;
constexpr bool kEnableDeepSleep = false;
constexpr bool kUseWakeupPin = false;
constexpr uint8_t kWakeupPin = 3;
constexpr int kWakeupLevel = 0;

// LEGO Wireless Protocol
constexpr bool kUseBleTrain = true;
constexpr char kBleDeviceName[] = "ESPTrain SNES";
constexpr char kLegoServiceUuid[] = "00001623-1212-efde-1623-785feabcd123";
constexpr char kLegoCharacteristicUuid[] = "00001624-1212-efde-1623-785feabcd123";
constexpr uint32_t kBleScanSeconds = 5;
constexpr uint32_t kBleReconnectIntervalMs = 15000;
constexpr bool kBleActiveScan = true;
constexpr uint16_t kBleScanIntervalMs = 160;
constexpr uint16_t kBleScanWindowMs = 40;
constexpr uint16_t kBleMtu = 97;
constexpr bool kPrintBleNotifications = false;
constexpr uint32_t kBleCommandMinIntervalMs = 120;
constexpr uint8_t kLegoMotorPort = 0x00;
constexpr uint8_t kLegoSoundPort = 0x01;
constexpr uint8_t kLegoLightPort = 0x11;

// Train control uses logical levels so button presses feel predictable.
constexpr int8_t kTrainLevelMin = -10;
constexpr int8_t kTrainLevelMax = 10;
constexpr int8_t kTrainPowerPerLevel = 10;
constexpr int8_t kTrainStartLevel = 3;
constexpr int8_t kTrainDirectionChangeLevel = 2;
constexpr uint32_t kTrainDirectionChangePauseMs = 700;
constexpr int8_t kTrainStopSpeed = 0;
constexpr int8_t kTrainBrakeSpeed = 127;

// Built-in DUPLO train sound and light values.
constexpr uint8_t kSoundWater = 7;
constexpr uint8_t kSoundHorn = 9;
constexpr uint8_t kLightOff = 0;
constexpr uint8_t kLightMagenta = 2;
constexpr uint8_t kLightBlue = 3;
constexpr uint8_t kLightGreen = 6;
constexpr uint8_t kLightYellow = 7;
constexpr uint8_t kLightOrange = 8;
constexpr uint8_t kLightRed = 9;

}  // namespace Config

#if ESPTRAIN_DEBUG_SERIAL
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#define DEBUG_PRINTF(...)
#endif
