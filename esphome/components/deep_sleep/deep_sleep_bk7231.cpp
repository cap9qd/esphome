#ifdef USE_BK72XX
#include "deep_sleep_component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace deep_sleep {

static const char *const TAG = "deep_sleep";

optional<uint32_t> DeepSleepComponent::get_run_duration_() const {
  if (this->wakeup_cause_to_run_duration_.has_value()) {
    lt_reboot_reason_t wakeup_cause = lt_get_reboot_reason();
    ESP_LOGD(TAG, "BK71XX wakeup reason '%s'", lt_get_reboot_reason_name(wakeup_cause));
    switch (wakeup_cause) {
      case REBOOT_REASON_SLEEP_GPIO:
        return this->wakeup_cause_to_run_duration_->gpio_cause;
      default:
        return this->wakeup_cause_to_run_duration_->default_cause;
    }
  }
  return this->run_duration_;
}

void DeepSleepComponent::dump_config_platform_() {
  ESP_LOGCONFIG(TAG, "  Default Wakeup Run Duration: %" PRIu32 " ms",
                this->wakeup_cause_to_run_duration_->default_cause);
  ESP_LOGCONFIG(TAG, "  GPIO Wakeup Run Duration: %" PRIu32 " ms", this->wakeup_cause_to_run_duration_->gpio_cause);
}

bool DeepSleepComponent::prepare_to_sleep_() {
  bool clear_to_sleep = true;
  if (lt_gpio_wake_config_.size() > 0) {
    for (auto const &i : lt_gpio_wake_config_) {
      bool digital_val = digitalRead(i.first);
      switch (i.second) {
        case WAKEUP_PIN_MODE_LOW_IGNORE:
        case WAKEUP_PIN_MODE_HIGH_IGNORE:
        case WAKEUP_PIN_MODE_SWAP_LEVEL:
          break;
        case WAKEUP_PIN_MODE_LOW_KEEP_AWAKE:
          if (digital_val == false) {
            if (clear_to_sleep) {
              this->status_set_warning();
              ESP_LOGW(TAG, "Waiting for pin %d to switch state to %d to enter deep sleep...", i.first, !i.second);
            }
            clear_to_sleep = false;
          }
          break;
        case WAKEUP_PIN_MODE_HIGH_KEEP_AWAKE:
          if (digital_val == true) {
            if (clear_to_sleep) {
              this->status_set_warning();
              ESP_LOGW(TAG, "Waiting for pin %d to switch state to %d to enter deep sleep...", i.first, !i.second);
            }
            clear_to_sleep = false;
          }
          break;
      }
    }
  }
  return clear_to_sleep;
}
void DeepSleepComponent::deep_sleep_() {
  if (this->sleep_duration_.has_value())
    lt_deep_sleep_config_timer(*this->sleep_duration_ / 1000);

  if (lt_gpio_wake_config_.size() > 0) {
    // for (i = lt_gpio_wake_config_.begin(); i != lt_gpio_wake_config_.end(); i++) {
    for (auto const &i : lt_gpio_wake_config_) {
      bool digital_val = digitalRead(i.first);

      switch (i.second) {
        case WAKEUP_PIN_MODE_LOW_IGNORE:
          if (digital_val == false) {
            lt_deep_sleep_unset_gpio(1 << i.first);
            ESP_LOGW(TAG, "Pin %d already in desired state LOW; ignoring...", i.first);
            if (!this->sleep_duration_.has_value())
              ESP_LOGW(TAG, "No sleep duration and ignoring GPIO wake. May never wakeup?");
          } else {
            lt_deep_sleep_config_gpio(1 << i.first, false);
          }
          break;
        case WAKEUP_PIN_MODE_HIGH_IGNORE:
          if (digital_val == true) {
            lt_deep_sleep_unset_gpio(1 << i.first);
            ESP_LOGW(TAG, "Pin %d already in desired state HIGH; ignoring...", i.first);
            if (!this->sleep_duration_.has_value())
              ESP_LOGW(TAG, "No sleep duration and ignoring GPIO wake. May never wakeup?");
          } else {
            lt_deep_sleep_config_gpio(1 << i.first, true);
          }
          break;
        case WAKEUP_PIN_MODE_SWAP_LEVEL:
          ESP_LOGW(TAG, "Swapping pin %d wake-up level to %d", i.first, !digital_val);
          lt_deep_sleep_unset_gpio(1 << i.first);
          lt_deep_sleep_config_gpio(1 << i.first, !digital_val);
          break;
        case WAKEUP_PIN_MODE_LOW_KEEP_AWAKE:
          lt_deep_sleep_config_gpio(1 << i.first, false);
          break;
        case WAKEUP_PIN_MODE_HIGH_KEEP_AWAKE:
          lt_deep_sleep_config_gpio(1 << i.first, true);
          break;
      }
    }
  }

  lt_deep_sleep_enter();
}

void DeepSleepComponent::set_run_duration(WakeupCauseToRunDuration wakeup_cause_to_run_duration) {
  wakeup_cause_to_run_duration_ = wakeup_cause_to_run_duration;
}

void DeepSleepComponent::set_lt_gpio_wake(uint8_t pin, LtWakeupPinMode pin_mode) {
  ESP_LOGCONFIG(TAG, "Setting BK GPIO wake on pin %d with mode %d", pin, pin_mode);
  lt_gpio_wake_config_[pin] = pin_mode;
}

void DeepSleepComponent::set_lt_gpio_wake(InternalGPIOPin *pin, LtWakeupPinMode pin_mode) {
  pin->setup();
  ESP_LOGCONFIG(TAG, "Setting BK GPIO wake on pin %d with mode %d", pin, pin_mode);
  lt_gpio_wake_config_[pin->get_pin()] = pin_mode;
}

}  // namespace deep_sleep
}  // namespace esphome
#endif
