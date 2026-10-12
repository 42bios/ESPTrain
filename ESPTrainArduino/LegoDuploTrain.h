#pragma once

#include <Arduino.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLERemoteCharacteristic.h>
#include <Preferences.h>

// Minimal LEGO Wireless Protocol client for the DUPLO 10874 train base.
class LegoDuploTrain {
 public:
  void begin();
  void update(uint32_t nowMs);
  bool isConnected() const;
  void disconnect();
  void requestPairing();
  void resetPairing();
  void setSpeed(int8_t speed);
  void stop();
  void emergencyStop();
  void playHorn();
  void playWaterSound();
  void playSound(uint8_t sound);
  void setLight(uint8_t color);
  void turnLightOff();
  void handleNotification(uint8_t* data, size_t length);
  void onDisconnected();

 private:
  bool sendMessage(const uint8_t* data, size_t length, bool enforceRateLimit = true);
  bool sendPortOutputCommand(uint8_t port, uint8_t mode, int8_t value, bool enforceRateLimit = true);
  bool enableSpeaker();
  void releaseClient();
  void releaseClientIfRequested();
  bool canSendNow() const;
  bool scanAndConnect(bool saveAddress);
  bool connectToAdvertisedDevice(BLEAdvertisedDevice& device, bool saveAddress);
  void saveTrainAddress(const String& address);
  String loadTrainAddress();

  Preferences preferences_;
  BLEClient* client_ = nullptr;
  BLERemoteCharacteristic* characteristic_ = nullptr;
  String savedAddress_;
  bool connected_ = false;
  bool pairingRequested_ = false;
  bool bleReady_ = false;
  bool clientReleaseRequested_ = false;
  uint32_t lastReconnectAttemptMs_ = 0;
  uint32_t lastCommandMs_ = 0;
  int8_t lastSpeed_ = 0;
};
