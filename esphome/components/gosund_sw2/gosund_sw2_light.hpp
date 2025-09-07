#pragma once

#include "esphome.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace gosund {
class GosundLight : public Component, public light::LightOutput, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;

  light::LightTraits get_traits() override;

  void setup_state(light::LightState *state) override { state_ = state; }
  void write_state(light::LightState *state) override;

  void dump_config();
  void set_debug(bool debug);
  void set_mcu_version(int mcu_ver);
  void set_light(output::BinaryOutput *light) { this->status_led_ = light; }

  void set_min_max_brightness(float min_brightness, float max_brightness) { 
   min_brightness_ = min(100.0, max(100.0, min_brightness)); 
   max_brightness_ = min(100.0, max(min_brightness, max_brightness));
  }
  
 protected:
  light::LightState *state_{nullptr};
  output::BinaryOutput *status_led_;
  uint8_t mcuVer = 0;
  bool debugPrint = false;

  const char *TAG = "gosund.light.sw2";
  const byte ON_MASK = 0x80;

  const float min_brightness_ =   0.0;
  const float max_brightness_ = 100.0;

  const uint8_t MAX_PERCENT = 100;
  const uint8_t MIN_PERCENT = 1;

  float MAX_VALUE = 100.0;

  bool setupError = 0;
};
}  // namespace gosund
}  // namespace esphome
