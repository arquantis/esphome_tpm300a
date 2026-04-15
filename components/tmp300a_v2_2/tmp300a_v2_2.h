#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace tpm300a_v2_2 {

class TPM300AV22Component : public PollingComponent, public uart::UARTDevice {
 public:
  // Standard setters for the 3 detected gas channels
  void set_tvoc_sensor(sensor::Sensor *s) { tvoc_sensor_ = s; }
  void set_ch2o_sensor(sensor::Sensor *s) { ch2o_sensor_ = s; }
  void set_co2_sensor(sensor::Sensor *s) { co2_sensor_ = s; }

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
              // Main Sensor: TVOC
              float tvoc  = (uint16_t(data[1]) << 8) | data[2];
              // Aux 1: ch2o
              float ch2o = (uint16_t(data[3]) << 8) | data[4];
              // Aux 2: co2
              float co2  = (uint16_t(data[5]) << 8) | data[6];

              if (this->tvoc_sensor_ != nullptr) this->tvoc_sensor_->publish_state(tvoc);
              if (this->ch2o_sensor_ != nullptr) this->ch2o_sensor_->publish_state(ch2o);
              if (this->co2_sensor_ != nullptr) this->co2_sensor_->publish_state(co2);
            }
          } else {
            ESP_LOGW("tpm300a_v2_2", "Checksum mismatch! Data discarded.");
          }
        }
      }
    }
  }

 protected:
  sensor::Sensor *tvoc_sensor_{nullptr};
  sensor::Sensor *ch2o_sensor_{nullptr};
  sensor::Sensor *co2_sensor_{nullptr};
};

}  // namespace tmp300a_v2_2
}  // namespace esphome
