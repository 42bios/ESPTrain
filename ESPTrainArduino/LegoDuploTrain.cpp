#include "LegoDuploTrain.h"
#include "Config.h"

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLERemoteService.h>

namespace {

BLEUUID legoServiceUuid(Config::kLegoServiceUuid);
BLEUUID legoCharacteristicUuid(Config::kLegoCharacteristicUuid);
LegoDuploTrain* activeTrain = nullptr;

void notifyCallback(BLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
  (void)characteristic;
  (void)isNotify;
  if (activeTrain != nullptr) {
    activeTrain->handleNotification(data, length);
  }
}

class TrainClientCallbacks : public BLEClientCallbacks {
 public:
  void onConnect(BLEClient* client) override {
    (void)client;
    DEBUG_PRINTLN("BLE: connected");
  }

  void onDisconnect(BLEClient* client) override {
    (void)client;
    if (activeTrain != nullptr) {
      activeTrain->onDisconnected();
    }
  }
};

}  // namespace

void LegoDuploTrain::begin() {
  if (!Config::kUseBleTrain) {
    DEBUG_PRINTLN("BLE: disabled");
    return;
  }

  activeTrain = this;
  BLEDevice::init(Config::kBleDeviceName);
  preferences_.begin("esptrain", false);
  savedAddress_ = loadTrainAddress();
  bleReady_ = true;

  if (savedAddress_.length() > 0) {
    DEBUG_PRINTF("BLE: saved train %s\n", savedAddress_.c_str());
    DEBUG_PRINTLN("BLE: will reconnect automatically");
  } else {
    pairingRequested_ = true;
    DEBUG_PRINTLN("BLE: no train saved, auto scan/pair will start");
  }
}

void LegoDuploTrain::update(uint32_t nowMs) {
  if (!bleReady_) {
    return;
  }

  releaseClientIfRequested();

  if (pairingRequested_) {
    pairingRequested_ = false;
    lastReconnectAttemptMs_ = nowMs;
    scanAndConnect(true);
    return;
  }

  if (!connected_ && savedAddress_.length() == 0 &&
      nowMs - lastReconnectAttemptMs_ >= Config::kBleReconnectIntervalMs) {
    lastReconnectAttemptMs_ = nowMs;
    DEBUG_PRINTLN("BLE: pairing retry scan");
    scanAndConnect(true);
    return;
  }

  if (!connected_ && savedAddress_.length() > 0 &&
      nowMs - lastReconnectAttemptMs_ >= Config::kBleReconnectIntervalMs) {
    lastReconnectAttemptMs_ = nowMs;
    DEBUG_PRINTLN("BLE: reconnect scan");
    scanAndConnect(false);
  }
}

bool LegoDuploTrain::isConnected() const {
  return connected_;
}

void LegoDuploTrain::disconnect() {
  if (client_ != nullptr && client_->isConnected()) {
    client_->disconnect();
  }
  connected_ = false;
  characteristic_ = nullptr;
  releaseClient();
  DEBUG_PRINTLN("BLE: disconnected");
}

void LegoDuploTrain::requestPairing() {
  if (!bleReady_) {
    DEBUG_PRINTLN("BLE: not ready");
    return;
  }
  if (connected_) {
    DEBUG_PRINTLN("BLE: already connected, pairing ignored");
    return;
  }
  DEBUG_PRINTLN("BLE: pairing requested");
  pairingRequested_ = true;
}

void LegoDuploTrain::resetPairing() {
  stop();
  disconnect();
  savedAddress_ = "";
  preferences_.remove("addr");
  DEBUG_PRINTLN("BLE: saved train cleared");
}

void LegoDuploTrain::setSpeed(int8_t speed) {
  speed = constrain(speed, -100, 100);
  if (sendPortOutputCommand(Config::kLegoMotorPort, 0x00, speed)) {
    lastSpeed_ = speed;
    DEBUG_PRINTF("Train: speed %d\n", speed);
  }
}

void LegoDuploTrain::stop() {
  if (sendPortOutputCommand(Config::kLegoMotorPort, 0x00, Config::kTrainStopSpeed, false)) {
    lastSpeed_ = 0;
    DEBUG_PRINTLN("Train: stop");
  }
}

void LegoDuploTrain::emergencyStop() {
  if (sendPortOutputCommand(Config::kLegoMotorPort, 0x00, Config::kTrainBrakeSpeed, false)) {
    lastSpeed_ = 0;
    DEBUG_PRINTLN("Train: emergency brake");
  }
}

void LegoDuploTrain::playHorn() {
  playSound(Config::kSoundHorn);
}

void LegoDuploTrain::playWaterSound() {
  playSound(Config::kSoundWater);
}

void LegoDuploTrain::playSound(uint8_t sound) {
  if (sendPortOutputCommand(Config::kLegoSoundPort, 0x01, static_cast<int8_t>(sound))) {
    DEBUG_PRINTF("Train: sound %u\n", sound);
  }
}

void LegoDuploTrain::setLight(uint8_t color) {
  if (sendPortOutputCommand(Config::kLegoLightPort, 0x00, static_cast<int8_t>(color))) {
    DEBUG_PRINTF("Train: light %u\n", color);
  }
}

void LegoDuploTrain::turnLightOff() {
  setLight(Config::kLightOff);
}

bool LegoDuploTrain::sendMessage(const uint8_t* data, size_t length, bool enforceRateLimit) {
  if (!connected_ || characteristic_ == nullptr) {
    DEBUG_PRINTLN("BLE: cannot send, train not connected");
    return false;
  }

  if (enforceRateLimit && !canSendNow()) {
    DEBUG_PRINTLN("BLE: command skipped by rate limit");
    return false;
  }

  if (!characteristic_->writeValue(const_cast<uint8_t*>(data), length, false)) {
    DEBUG_PRINTLN("BLE: command failed");
    return false;
  }

  lastCommandMs_ = millis();
  return true;
}

bool LegoDuploTrain::sendPortOutputCommand(uint8_t port, uint8_t mode, int8_t value,
                                           bool enforceRateLimit) {
  uint8_t message[] = {
      0x08,  // length
      0x00,  // hub id
      0x81,  // port output command
      port,
      0x11,  // execute immediately, request feedback
      0x51,  // WriteDirectModeData
      mode,
      static_cast<uint8_t>(value),
  };

  return sendMessage(message, sizeof(message), enforceRateLimit);
}

bool LegoDuploTrain::enableSpeaker() {
  const uint8_t message[] = {0x0A, 0x00, 0x41, Config::kLegoSoundPort, 0x01,
                             0x01, 0x00, 0x00, 0x00, 0x01};
  return sendMessage(message, sizeof(message));
}

void LegoDuploTrain::releaseClient() {
  if (client_ == nullptr) {
    clientReleaseRequested_ = false;
    return;
  }

  delete client_;
  client_ = nullptr;
  clientReleaseRequested_ = false;
}

void LegoDuploTrain::releaseClientIfRequested() {
  if (!clientReleaseRequested_ || connected_) {
    return;
  }

  releaseClient();
}

bool LegoDuploTrain::canSendNow() const {
  return millis() - lastCommandMs_ >= Config::kBleCommandMinIntervalMs;
}

bool LegoDuploTrain::scanAndConnect(bool saveAddress) {
  DEBUG_PRINTF("BLE: scanning for LEGO train for %lu s\n",
               static_cast<unsigned long>(Config::kBleScanSeconds));

  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(Config::kBleActiveScan);
  scan->setInterval(Config::kBleScanIntervalMs);
  scan->setWindow(Config::kBleScanWindowMs);

  BLEScanResults* results = scan->start(Config::kBleScanSeconds, false);
  if (results == nullptr) {
    DEBUG_PRINTLN("BLE: scan failed");
    return false;
  }

  const int count = results->getCount();
  DEBUG_PRINTF("BLE: scan complete, devices=%d\n", count);

  for (int i = 0; i < count; ++i) {
    BLEAdvertisedDevice device = results->getDevice(i);
    const String address = device.getAddress().toString().c_str();
    const bool addressMatches = savedAddress_.length() > 0 && address.equalsIgnoreCase(savedAddress_);
    const bool serviceMatches = device.haveServiceUUID() && device.isAdvertisingService(legoServiceUuid);

    if (serviceMatches || addressMatches) {
      DEBUG_PRINTF("BLE: candidate %s %s\n", address.c_str(), device.getName().c_str());
      const bool ok = connectToAdvertisedDevice(device, saveAddress || serviceMatches);
      scan->clearResults();
      return ok;
    }
  }

  scan->clearResults();
  DEBUG_PRINTLN("BLE: no LEGO train found");
  return false;
}

bool LegoDuploTrain::connectToAdvertisedDevice(BLEAdvertisedDevice& device, bool saveAddress) {
  disconnect();

  client_ = BLEDevice::createClient();
  client_->setClientCallbacks(new TrainClientCallbacks());

  DEBUG_PRINTF("BLE: connecting to %s\n", device.getAddress().toString().c_str());
  if (!client_->connect(&device)) {
    DEBUG_PRINTLN("BLE: connect failed");
    releaseClient();
    return false;
  }

  client_->setMTU(Config::kBleMtu);

  BLERemoteService* service = client_->getService(legoServiceUuid);
  if (service == nullptr) {
    DEBUG_PRINTLN("BLE: LEGO service not found after connect");
    client_->disconnect();
    characteristic_ = nullptr;
    releaseClient();
    return false;
  }

  characteristic_ = service->getCharacteristic(legoCharacteristicUuid);
  if (characteristic_ == nullptr) {
    DEBUG_PRINTLN("BLE: LEGO characteristic not found");
    client_->disconnect();
    characteristic_ = nullptr;
    releaseClient();
    return false;
  }

  if (characteristic_->canNotify()) {
    characteristic_->registerForNotify(notifyCallback);
  }

  connected_ = true;
  lastCommandMs_ = millis() - Config::kBleCommandMinIntervalMs;
  enableSpeaker();
  if (saveAddress) {
    saveTrainAddress(device.getAddress().toString().c_str());
  }

  DEBUG_PRINTLN("BLE: LEGO train paired/connected");
  return true;
}

void LegoDuploTrain::saveTrainAddress(const String& address) {
  savedAddress_ = address;
  preferences_.putString("addr", savedAddress_);
  DEBUG_PRINTF("BLE: saved train address %s\n", savedAddress_.c_str());
}

String LegoDuploTrain::loadTrainAddress() {
  return preferences_.getString("addr", "");
}

void LegoDuploTrain::handleNotification(uint8_t* data, size_t length) {
  if (!Config::kPrintBleNotifications) {
    return;
  }

  DEBUG_PRINT("BLE notify:");
  for (size_t i = 0; i < length; ++i) {
    DEBUG_PRINTF(" %02X", data[i]);
  }
  DEBUG_PRINTLN();
}

void LegoDuploTrain::onDisconnected() {
  connected_ = false;
  characteristic_ = nullptr;
  clientReleaseRequested_ = true;
  DEBUG_PRINTLN("BLE: disconnected by train");
}
