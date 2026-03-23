import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import CONF_ID, ICON_BLUR, ICON_WATER_PERCENT, ICON_THERMOMETER

# Definimos el namespace y la clase
tmp300a_ns = cg.esphome_ns.namespace('tmp300a')
TMP300AComponent = tmp300a_ns.class_('TMP300AComponent', cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(TMP300AComponent),
    cv.Required('voc'): sensor.sensor_schema(icon=ICON_BLUR, accuracy_decimals=0),
    cv.Required('eco2'): sensor.sensor_schema(icon=ICON_WATER_PERCENT, accuracy_decimals=0),
    cv.Required('lpg'): sensor.sensor_schema(icon=ICON_THERMOMETER, accuracy_decimals=0),
}).extend(cv.polling_component_schema('1s')).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if 'voc' in config:
        sens = await sensor.new_sensor(config['voc'])
        # Usamos el método setter de C++
        cg.add(var.set_voc_sensor(sens))
    if 'eco2' in config:
        sens = await sensor.new_sensor(config['eco2'])
        cg.add(var.set_eco2_sensor(sens))
    if 'lpg' in config:
        sens = await sensor.new_sensor(config['lpg'])
        cg.add(var.set_lpg_sensor(sens))
