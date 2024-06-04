#ifdef USE_BK72XX
#include "deep_sleep_component.h"

#include <Esp.h>

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

bool DeepSleepComponent::prepare_to_sleep_() { return true; }

void DeepSleepComponent::deep_sleep_() {
  ESP.deepSleep(*this->sleep_duration_);  // NOLINT(readability-static-accessed-through-instance)
}

void DeepSleepComponent::set_run_duration(WakeupCauseToRunDuration wakeup_cause_to_run_duration) {
  wakeup_cause_to_run_duration_ = wakeup_cause_to_run_duration;
}

}  // namespace deep_sleep
}  // namespace esphome
#endif
