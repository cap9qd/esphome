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
      
  protected:
      light::LightState *state_{nullptr};
      output::BinaryOutput *status_led_;
      uint8_t mcuVer = 0;
      bool debugPrint = false;
      
      const char *TAG = "gosund.light.sw2";
      const byte ON_MASK = 0x80;

      const uint8_t MAX_PERCENT = 100;
      const uint8_t MIN_PERCENT = 1;

      float MAX_VALUE = 100.0;

      bool setupError = 0;
};
} // namespace gosund
} // namespace esphome
