#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace tmp300a {

class TMP300AComponent : public PollingComponent, public uart::UARTDevice {
 public:
  // Standard setters for the 3 detected gas channels
  void set_voc_sensor(sensor::Sensor *s) { voc_sensor_ = s; }
  void set_eco2_sensor(sensor::Sensor *s) { eco2_sensor_ = s; }
  void set_lpg_sensor(sensor::Sensor *s) { lpg_sensor_ = s; }

  void update() override {
    // Sensor sends a 9-byte packet constantly
    while (available() >= 9) {
      uint8_t start_byte;
      if (!read_byte(&start_byte)) continue;
      
      // Header check (0x2C)
      if (start_byte == 0x2C) {
        uint8_t data[8];
        if (read_array(data, 8)) {
          
          // --- CHECKSUM VERIFICATION ---
          // Sum of first 8 bytes (Header + Payload)
          uint8_t calculated_checksum = start_byte;
          for (int i = 0; i < 7; i++) {
            calculated_checksum += data[i];
          }

          // Compare with the 9th byte received
          if (data[7] == calculated_checksum) {
            // Identifier for V2.2 firmware (0xE4)
            if (data[0] == 0xE4) {
              // Main Sensor: VOC/Organic Gases
              float voc  = (uint16_t(data[1]) << 8) | data[2];
              // Aux 1: eCO2 (Estimated Carbon Dioxide)
              float eco2 = (uint16_t(data[3]) << 8) | data[4];
              // Aux 2: Combustible Gases (LPG/CH4)
              float lpg  = (uint16_t(data[5]) << 8) | data[6];

              if (this->voc_sensor_ != nullptr) this->voc_sensor_->publish_state(voc);
              if (this->eco2_sensor_ != nullptr) this->eco2_sensor_->publish_state(eco2);
              if (this->lpg_sensor_ != nullptr) this->lpg_sensor_->publish_state(lpg);
            }
          } else {
            ESP_LOGW("tmp300a", "Checksum mismatch! Data discarded.");
          }
        }
      }
    }
  }

 protected:
  sensor::Sensor *voc_sensor_{nullptr};
  sensor::Sensor *eco2_sensor_{nullptr};
  sensor::Sensor *lpg_sensor_{nullptr};
};

}  // namespace tmp300a
}  // namespace esphome
