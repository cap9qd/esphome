// Forked orginal code from https://github.com/a-lost-shadow/gosund-sw2 for MCU v1
//
// For the BK7231T version (mcu v2?) I used reverse engineering info from:
//    https://www.elektroda.com/rtvforum/topic4014774.html#20825305

#include "gosund_sw2_light.hpp"

namespace esphome {
namespace gosund {
void GosundLight::setup() {}

void GosundLight::dump_config() {
  ESP_LOGCONFIG(TAG, "Setup GoSund Dimmer SW2");
  ESP_LOGCONFIG(TAG, "\tMCU:            v%d", mcuVer);
  ESP_LOGCONFIG(TAG, "\tDebug:          %d", debugPrint);
  ESP_LOGCONFIG(TAG, "\tMin Brightness: %03.1f", min_brightness_);
  ESP_LOGCONFIG(TAG, "\tMin Brightness: %03.1f", max_brightness_);
  ESP_LOGCONFIG(TAG, "\tInit String:    %s", init_string);
  
  // Error message if the MCU version not in [1,2]
  if (setupError)
    ESP_LOGE(TAG, "\tERROR SETTING UP MCU VERSION! Please check config!");
}

void GosundLight::loop() {
  static uint8_t pos = 0;
  static uint8_t mPos = 5;
  static uint8_t tBuffer[5];
  static float dimmerVal = 0.0;
  static bool init = 0;
  
  unsigned int bytes_available = available();
  bool found = false;
  auto call = state_->make_call();
  uint8_t rByte = 0;

  if (bytes_available > 0) {
    if (debugPrint)
      ESP_LOGD(TAG, "UART has %d bytes avaliable.", bytes_available);

    for (int i = 0; i < bytes_available; i++) {
      read_byte(&rByte);
      for (int j = 0; j < 4; j++)
        tBuffer[j] = tBuffer[j + 1];
      tBuffer[4] = rByte;

      if (debugPrint)
        ESP_LOGD(TAG, "Read byte '0x%02X'.", rByte);
      
      if(!init) {
        this->init_string += (char)rByte;
      }

      if ((tBuffer[0] == 0x24) && (0x01 == tBuffer[2]) && (0x23 == tBuffer[4])) {
        if (debugPrint)
          ESP_LOGD(TAG, "Found matching string!");
        init = found = true;
        break;
      }
      if(strcmp((char*)&tBuffer[0], "reset") == 0 ) {
        ESP_LOGW(TAG, "Dimmer MCU Reset detected!");
        call.set_state(false);
        init = found = false;
        break;
      }
      if(!init) {
        this->init_string += (char)rByte;
      }
      if(debugPrint)
        ESP_LOGW(tag, init_string);
    }

    // MCU v1 = 0x24 0xYY 0x01 0x1E 0x23 where 0xYY is the dimmer value
    // MCU v2 = 0x24 0xYY 0x01 0x64 0x23 where 0xYY is the dimmer value
    //  (NOTE: v1 MCU swapped byte 3 around between reboots; idk why...)
    if (found) {
      // Brightness returned is 0x01 - 0x64 for v1
      // Brightness returned is 0x01 - 0x96 for v2
      //dimmerVal = tBuffer[1] / MAX_VALUE;
      dimmerVal = remap<float, uint8_t>(tBuffer[1], MIN_VALUE, MAX_VALUE, 0.0, 1.0);
      
      // Clear buffer so we dont trigger more than 1 time per message.
      memset(&tBuffer[0], 0, 5);

      if (debugPrint) {
        ESP_LOGD(TAG, "Received dimmer value: %01.1f", dimmerVal * 100);
      }

      // Touch sensor only works when turned on
      call.set_state(true);
      call.set_brightness(dimmerVal);
      call.perform();
    }
  }
}

void GosundLight::set_debug(bool debug) { debugPrint = debug; }

void GosundLight::set_mcu_version(int mcu_ver) {
  if (mcu_ver <= 0) {
#if defined(USE_BK72XX)
    mcu_ver = 2;
#endif
#if defined(USE_ESP8266)
    mcu_ver = 1;
#endif
  }
  mcuVer = mcu_ver;
  switch (mcuVer) {
    case 0x01:
      MAX_VALUE = 100.0;
      break;
    case 0x02:
      MAX_VALUE = 150.0;
      break;
    default:
      setupError = true;
      break;
  }
}

light::LightTraits GosundLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  return traits;
}

void GosundLight::write_state(light::LightState *state) {
  auto values = state->current_values;
  
  float brightness = remap<float, float>(values.get_brightness(), 0.0, 1.0, this->min_brightness_, this->max_brightness_);
  uint8_t output = (uint8_t) (brightness);
  uint8_t ledOut = std::max(MIN_PERCENT, (uint8_t) std::ceil(values.get_brightness() * 7.0));

  if (values.get_state() > 0) {
    status_led_->turn_on();

    if (debugPrint) {
      ESP_LOGD(TAG, "Turning on status LED");
    }

    // Last bit indicates ON/OFF state; last bits 0-100% level.
    output |= 0x80;  // Indicates output message and on

    // Only write LED command is we are MCU v2 and not "byTouch" to maintain MQTT version functionality.
    if (mcuVer == 2) {
      ledOut |= 0x40;  // Indicates LED message
      write_byte(ledOut);
      if (debugPrint) {
        ESP_LOGD(TAG, "UART write LED command '0x%02X'", ledOut);
      }
    }
    
    write_byte(output);
    if (debugPrint) {
      ESP_LOGD(TAG, "UART write output command '0x%02X'", output);
    }
  } else {
    status_led_->turn_off();

    if (debugPrint) {
      ESP_LOGD(TAG, "turning off status LED");
    }

    switch (mcuVer) {
      case 1:
        write(output & 0x7F);  // Tell it which level LED to keep on during off
        if (debugPrint) {
          ESP_LOGD(TAG, "UART write output command '0x%02X'", output & 0x7F);
        }
        break;
      case 2:
        write(ledOut & 0x07);  // Tell it which level LED to keep on during off
        if (debugPrint) {
          ESP_LOGD(TAG, "UART write output command '0x%02X'", ledOut & 0x07);
        }
        break;
    }
  }
  if (debugPrint)
    if (mcuVer == 2)
      ESP_LOGD(TAG, "write_state() called with state: %0.1f, brightness: %.02f => output: %02X and LED: %02X",
               values.get_state(), values.get_brightness(), output, ledOut);
    else
      ESP_LOGD(TAG, "write_state() called with state: %0.1f, brightness: %.02f => output: %02X", values.get_state(),
               values.get_brightness(), output);
}
}  // namespace gosund
}  // namespace esphome
