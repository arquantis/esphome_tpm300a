import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import CONF_ID, ICON_BLUR, ICON_WATER_PERCENT, ICON_THERMOMETER

# Definimos el namespace y la clase
tmp300a_ns = cg.esphome_ns.namespace('tmp300a')
TMP300AComponent = tmp300a_ns.class_('TMP300AComponent', cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(TMP300AComponent),
    cv.Required('tvoc'): sensor.sensor_schema(icon="mdi:air-filter", accuracy_decimals=0),
    cv.Required('ch2o'): sensor.sensor_schema(icon="mdi:fire", accuracy_decimals=0),
    cv.Required('co2'): sensor.sensor_schema(icon="mdi:molecule-co2", accuracy_decimals=0),
}).extend(cv.polling_component_schema('1s')).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if 'tvoc' in config:
        sens = await sensor.new_sensor(config['tvoc'])
        # Usamos el método setter de C++
        cg.add(var.set_tvoc_sensor(sens))
    if 'ch2o' in config:
        sens = await sensor.new_sensor(config['ch2o'])
        cg.add(var.set_ch2o_sensor(sens))
    if 'co2' in config:
        sens = await sensor.new_sensor(config['co2'])
        cg.add(var.set_co2_sensor(sens))
