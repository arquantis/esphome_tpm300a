#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tpm300a_v12 {

class TPM300AV12Component : public PollingComponent, public uart::UARTDevice {
 public:
  // Setter actualizado para tvoc
  void set_tvoc_sensor(sensor::Sensor *s) { tvoc_sensor_ = s; }

  void update() override {
    while (available() >= 5) {
      uint8_t start_byte;
      if (!read_byte(&start_byte)) continue;
      
      if (start_byte == 0x2C) {
        uint8_t data[4];
        if (read_array(data, 4)) {
          // Validación por byte de fin (0xFF)
          if (data[3] == 0xFF) {
            // Reconstrucción de 10 bits: Byte1 es el alto, Byte2 es el bajo
            float tvoc_value = (uint16_t(data[0]) << 8) | data[1];

            if (this->tvoc_sensor_ != nullptr) {
              this->tvoc_sensor_->publish_state(tvoc_value);
            }
          } else {
            ESP_LOGV("tpm300a_v12", "Trama incompleta o desfasada. Saltando...");
          }
        }
      }
    }
  }

 protected:
  sensor::Sensor *tvoc_sensor_{nullptr};
};

}  // namespace tpm300a_v12
}  // namespace esphome
