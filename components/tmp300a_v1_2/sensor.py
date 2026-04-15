import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import CONF_ID, STATE_CLASS_MEASUREMENT, ICON_AIR_FILTER

# Definimos el namespace y la clase
tpm300a_v12_ns = cg.esphome_ns.namespace('tpm300a_v1_2')
TPM300AV12Component = tpm300a_v12_ns.class_('TPM300AV12Component', cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(TPM300AV12Component),
    cv.Required('tvoc'): sensor.sensor_schema(
        icon=ICON_AIR_FILTER, 
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT
    ),
}).extend(cv.polling_component_schema('1s')).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if 'tvoc' in config:
        sens = await sensor.new_sensor(config['tvoc'])
        # Llamamos al setter actualizado en C++
        cg.add(var.set_tvoc_sensor(sens))
