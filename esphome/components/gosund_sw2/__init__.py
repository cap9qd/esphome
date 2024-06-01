import esphome.codegen as cg
from esphome.components import light, output, uart
import esphome.config_validation as cv
from esphome.const import (
    CONF_OUTPUT_ID,
    CONF_GAMMA_CORRECT,
    CONF_DEFAULT_TRANSITION_LENGTH,
    PLATFORM_ESP8266,
    PLATFORM_BK72XX,
)

DEPENDENCIES = ["uart"]

gosund_sw2_ns = cg.esphome_ns.namespace("gosund")
GoSundSw2Component = gosund_sw2_ns.class_(
    "GosundLight", light.LightOutput, cg.Component
)

CONF_MCU_VER = "mcu_ver"
CONF_STATUS_OUTPUT = "status_output"
CONF_DEBUG = "debug"

CONFIG_SCHEMA = cv.All(
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(GoSundSw2Component),
            cv.Optional(CONF_MCU_VER, default=0): cv.int_,
            cv.Required(CONF_STATUS_OUTPUT): cv.use_id(output.BinaryOutput),
            cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
            cv.Optional(
                CONF_DEFAULT_TRANSITION_LENGTH, default="0s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_DEBUG): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA),
    cv.only_on([PLATFORM_ESP8266, PLATFORM_BK72XX]),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_mcu_version(config[CONF_MCU_VER]))
    if CONF_DEBUG in config:
        cg.add(var.set_debug(config[CONF_DEBUG]))

    light_ = await cg.get_variable(config[CONF_STATUS_OUTPUT])
    cg.add(var.set_light(light_))
