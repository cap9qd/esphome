#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace cht8310 {

class CHT8310Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }
  void set_humidity(sensor::Sensor *humidity) { humidity_ = humidity; }
  
  /*
  void set_max_temperature(float max_temperature) { max_temperature_ = max_temperature; }
  void set_min_temperature(float min_temperature) { min_temperature_ = min_temperature; }
  void set_max_humidity(float max_humidity) { max_humidity_ = max_humidity; }
  void set_min_humidity(float min_humidity) { min_humidity_ = min_humidity; }
  */
  
  /// Setup the sensor and check for connection.
  void setup() override;
  void dump_config() override;
  /// Retrieve the latest sensor values. This operation takes approximately 16ms.
  void update() override;

  float get_setup_priority() const override;

 protected:
  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *humidity_{nullptr};
  /*
  optional<float> min_temperature_;
  optional<float> max_temperature_;
  optional<float> min_humidity_;
  optional<float> max_humidity_;
  */
  uint16_t chip_ver;
};

}  // namespace cht8310
}  // namespace esphome
