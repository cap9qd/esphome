import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

DEPENDENCIES = ["i2c"]

cht8310_ns = cg.esphome_ns.namespace("cht8310")
CHT8310Component = cht8310_ns.class_(
    "CHT8310Component", cg.PollingComponent, i2c.I2CDevice
)

CONF_SD_MODE = "sd_mode"

CONF_MAX_TEMP = "max_temp"
CONF_MIN_TEMP = "min_temp"

CONF_MAX_HUMID = "max_humid"
CONF_MIN_HUMID = "min_humid"

CONF_CONV_T = "conv_time"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CHT8310Component),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                cv.Schema(
                    {
                        cv.Optional(CONF_MAX_TEMP): cv.float_,
                        cv.Optional(CONF_MIN_TEMP): cv.float_,
                    }
                )
            ),
            cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                cv.Schema(
                    {
                        cv.Optional(CONF_MAX_HUMID): cv.float_,
                        cv.Optional(CONF_MIN_HUMID): cv.float_,
                    }
                )
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x40))
    .extend(
        cv.Schema(
            {
                cv.Optional(CONF_SD_MODE, default=True): cv.boolean,
                cv.Optional(CONF_CONV_T, default=4): cv.int_range(0, 0x0F, True, True),
            }
        )
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature(sens))
        # temp_conf = config[CONF_TEMPERATURE]
        # if CONF_MIN_TEMP in temp_conf:
        #    cg.add(var.set_min_temperature(temp_conf[CONF_MIN_TEMP]))
        # if CONF_MAX_TEMP in temp_conf:
        #    cg.add(var.set_max_temperature(temp_conf[CONF_MAX_TEMP]))

    if CONF_HUMIDITY in config:
        sens = await sensor.new_sensor(config[CONF_HUMIDITY])
        cg.add(var.set_humidity(sens))
        # humid_conf = config[CONF_HUMIDITY]
        # if CONF_MIN_HUMID in humid_conf:
        #    cg.add(var.set_min_humidity(humid_conf[CONF_MIN_HUMID]))
        # if CONF_MAX_HUMID in humid_conf:
        #    cg.add(var.set_max_humidity(humid_conf[CONF_MAX_HUMID]))
