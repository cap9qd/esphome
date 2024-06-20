#include "cht8310.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace cht8310 {

static const char *const TAG = "CHT8310";

static const uint8_t CHT8310_ADDRESS = 0x40;
static const uint8_t CHT8310_REG_TEMPERATURE = 0x00;
static const uint8_t CHT8310_REG_HUMIDITY = 0x01;
static const uint8_t CHT8310_REG_STATUS = 0x02;
static const uint8_t CHT8310_REG_CONFIG = 0x03;
static const uint8_t CHT8310_REG_CONVERT_RATE = 0x04;
static const uint8_t CHT8310_REG_TEMP_HIGH_LIMIT = 0x05;
static const uint8_t CHT8310_REG_TEMP_LOW_LIMIT = 0x06;
static const uint8_t CHT8310_REG_HUM_HIGH_LIMIT = 0x07;
static const uint8_t CHT8310_REG_HUM_LOW_LIMIT = 0x08;
static const uint8_t CHT8310_REG_ONESHOT = 0x0F;
static const uint8_t CHT8310_REG_SWRESET = 0xFC;
static const uint8_t CHT8310_REG_MANUFACTURER = 0xFF;

void CHT8310Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CHT8310...");

  esphome::i2c::ErrorCode stat;
  stat = this->read_register(CHT8310_REG_MANUFACTURER, reinterpret_cast<uint8_t *>(&chip_ver), 2, 1);
  chip_ver = i2c::i2ctohs(chip_ver);
  ESP_LOGV(TAG, "ChipVer read Stat = %d & chip ver=%X", stat, chip_ver);

  if (chip_ver != 0x8215) {
    ESP_LOGD(TAG, "Chip Version '0x%X' not a CHT8210!");
    this->status_set_warning();
    return;
  }

  if(sd_mode_)
  {
    // Enable SD and leave the rest default
    const uint8_t data[2] = {0x48, 0x80};
    if (!this->write_bytes(CHT8310_REG_CONFIG, data, 2)) {
      // as instruction is same as powerup defaults (for now), interpret as warning if this fails
      ESP_LOGW(TAG, "CHT8310 initial config instruction error");
      this->status_set_warning();
      return;
    }
  }
  else
  {
    // Disable SD and leave the rest default
    const uint8_t data[2] = {0x08, 0x80};
    if (!this->write_bytes(CHT8310_REG_CONFIG, data, 2)) {
      // as instruction is same as powerup defaults (for now), interpret as warning if this fails
      ESP_LOGW(TAG, "CHT8310 initial config instruction error");
      this->status_set_warning();
      return;
    }

    uint16_t conv_time = (*conv_t_ << 8) & 0x0700;
    if (!this->write_register(CHT8310_REG_CONVERT_RATE, reinterpret_cast<uint8_t *>(&conv_time), 2, 1) != i2c::ERROR_OK) {
      ESP_LOGW(TAG, "CHT8310 conversion time config instruction error");
      this->status_set_warning();
      return;
    }
  }
}
void CHT8310Component::dump_config() {
  ESP_LOGCONFIG(TAG, "CHT8310:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with CHT8310 failed!");
  }
  LOG_UPDATE_INTERVAL(this);

  LOG_SENSOR("  ", "Temperature", this->temperature_);
  if (min_temperature_.has_value())
    ESP_LOGCONFIG(TAG, "    min: %.1f °C", *min_temperature_);
  if (max_temperature_.has_value())
    ESP_LOGCONFIG(TAG, "    max: %.1f °C", *max_temperature_);

  LOG_SENSOR("  ", "Humidity", this->humidity_);
  if (min_humidity_.has_value())
    ESP_LOGCONFIG(TAG, "    min: %.1f %%", *min_humidity_);
  if (max_humidity_.has_value())
    ESP_LOGCONFIG(TAG, "    max: %.1f %%", *max_humidity_);
  ESP_LOGCONFIG(TAG, "SD:    %d", sd_mode_);
  ESP_LOGCONFIG(TAG, "ConvT: %d", *conv_t_);
}
void CHT8310Component::update() {
  uint16_t raw_temp;

  if (sd_mode_) {
    const uint8_t data[2] = {0x12, 0x34};
    if (this->write_register(CHT8310_REG_ONESHOT, &data[0], 2, 1) != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "Error writing one-shot reg!");
      this->status_set_warning();
      return;
    }
    delay(10);
  }
  else
  {
    if (this->write(&CHT8310_REG_STATUS, 1) != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "Error writing status reg!");
      this->status_set_warning();
      return;
    }
    delay(10);
    if (this->read(reinterpret_cast<uint8_t *>(&raw_temp), 2) != i2c::ERROR_OK) {
      ESP_LOGE(TAG, "Error reading status reg!");
      this->status_set_warning();
      return;
    }
    ESP_LOGD(TAG, "STATUS = %04X", raw_temp);
  }
  
  if (this->write(&CHT8310_REG_TEMPERATURE, 1) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Error writing temperature reg!");
    this->status_set_warning();
    return;
  }
  delay(10);
  if (this->read(reinterpret_cast<uint8_t *>(&raw_temp), 2) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Error reading temperature reg!");
    this->status_set_warning();
    return;
  }

  raw_temp = i2c::i2ctohs(raw_temp);
  float temp = (raw_temp >> 3) * 0.03125f;
  this->temperature_->publish_state(temp);

  uint16_t raw_data;
  if (min_temperature_.has_value() && max_temperature_.has_value()) {
    raw_data = i2c::htoi2cs((uint16_t) ((temp + *min_temperature_) / 0.03125f) << 3);
    if (this->write_register(CHT8310_REG_TEMP_LOW_LIMIT, reinterpret_cast<uint8_t *>(&raw_data), 2, 1) != i2c::ERROR_OK)
      ESP_LOGE(TAG, "Error writing min temperature to CHT3210!");

    raw_data = i2c::htoi2cs((uint16_t) ((temp + *max_temperature_) / 0.03125f) << 3);
    if (this->write_register(CHT8310_REG_TEMP_HIGH_LIMIT, reinterpret_cast<uint8_t *>(&raw_data), 2, 1) != i2c::ERROR_OK)
      ESP_LOGE(TAG, "Error writing max temperature to CHT3210!");
  }

  uint16_t raw_humidity;
  if (this->write(&CHT8310_REG_HUMIDITY, 1) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Error writing humidity reg!");
    this->status_set_warning();
    return;
  }
  delay(10);
  if (this->read(reinterpret_cast<uint8_t *>(&raw_humidity), 2) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Error reading humidity reg!");
    this->status_set_warning();
    return;
  }
  raw_humidity = i2c::i2ctohs(raw_humidity);
  float humidity = (raw_humidity & 0x7FFF) / 327.67f;
  this->humidity_->publish_state(humidity);

  if (min_humidity_.has_value() && max_humidity_.has_value()) {
    raw_data = i2c::htoi2cs((uint16_t) ((humidity + *min_humidity_) * 327.67f) & 0x7F00);
    if (this->write_register(CHT8310_REG_HUM_LOW_LIMIT, reinterpret_cast<uint8_t *>(&raw_data), 2, 1) != i2c::ERROR_OK)
      ESP_LOGE(TAG, "Error writing min humidity to CHT3210!");

    raw_data = i2c::htoi2cs((uint16_t) ((humidity + *max_humidity_) * 327.67f) & 0x7F00);
    if (this->write_register(CHT8310_REG_HUM_HIGH_LIMIT, reinterpret_cast<uint8_t *>(&raw_data), 2, 1) != i2c::ERROR_OK)
      ESP_LOGE(TAG, "Error writing max humidity to CHT3210!");
  }

  ESP_LOGD(TAG, "Got temperature=%.1f°C humidity=%.1f%%", temp, humidity);
  this->status_clear_warning();
}
float CHT8310Component::get_setup_priority() const { return setup_priority::DATA; }

}  // namespace cht8310
}  // namespace esphome
