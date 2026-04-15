import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

# Definimos el componente base para que UART funcione
DEPENDENCIES = ['uart']
AUTO_LOAD = ['sensor']

tpm300a_v2_2_ns = cg.esphome_ns.namespace('tpm300a_v2_2')

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(tpm300a_v2_2_ns.class_('TPM300AV12Component', cg.PollingComponent, uart.UARTDevice)),
}).extend(cv.polling_component_schema('1s')).extend(uart.UART_DEVICE_SCHEMA)

def to_code(config):
    # Este archivo puede quedar casi vacío si toda la lógica está en sensor.py
    # Pero ayuda a ESPHome a registrar el namespace correctamente
    pass
