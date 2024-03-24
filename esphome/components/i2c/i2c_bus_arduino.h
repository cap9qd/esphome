#pragma once

#ifdef USE_ARDUINO

#include "i2c_bus.h"
#include "esphome/core/component.h"

// Moved from i2c_bus_arduino.cpp
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/application.h"
#include <Arduino.h>
#include <cstring>
// end-moved

#ifdef USE_SOFTWIRE

#include "AsyncDelay.h"
#include "SoftWire.h"

#else

#include <Wire.h>

#endif

namespace esphome {
namespace i2c {

enum RecoveryCode {
  RECOVERY_FAILED_SCL_LOW,
  RECOVERY_FAILED_SDA_LOW,
  RECOVERY_COMPLETED,
};

class ArduinoI2CBus : public I2CBus, public Component {
 public:
  void setup() override;
  void dump_config() override;
  ErrorCode readv(uint8_t address, ReadBuffer *buffers, size_t cnt) override;
  ErrorCode writev(uint8_t address, WriteBuffer *buffers, size_t cnt, bool stop) override;
  float get_setup_priority() const override { return setup_priority::BUS; }

  void set_scan(bool scan) { scan_ = scan; }
  void set_sda_pin(uint8_t sda_pin) { sda_pin_ = sda_pin; }
  void set_scl_pin(uint8_t scl_pin) { scl_pin_ = scl_pin; }
  void set_frequency(uint32_t frequency) { frequency_ = frequency; }

 private:
  void recover_();
  RecoveryCode recovery_result_;

#ifdef USE_SOFTWIRE
 protected:
  char swTxBuffer[128];
  char swRxBuffer[128];
  SoftWire *wire_;
#else
 protected:
  TwoWire *wire_;
#endif

  uint8_t sda_pin_;
  uint8_t scl_pin_;
  uint32_t frequency_;
  bool initialized_ = false;
};

}  // namespace i2c
}  // namespace esphome

#endif  // USE_ARDUINO
