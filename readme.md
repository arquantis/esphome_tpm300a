# TPM-300A

[English]
This repository provides custom components for the TPM300A air quality sensor family. 
It supports both the 5-byte protocol (v1.2) and the 9-byte protocol (v2.2).

[Español]
Este repositorio contiene componentes personalizados para la familia de sensores de calidad de aire TPM300A. 
Soporta tanto el protocolo de 5 bytes (v1.2) como el de 9 bytes (v2.2).

## Key Features / Características Clave
* **Native ESPHome:** No lambdas required. / Implementación nativa sin necesidad de lambdas.
* **Dual Version Support:** Specific drivers for v1.2 and v2.2. / Drivers específicos para versiones v1.2 y v2.2.
* **TVOC Reporting:** Raw and scaled values. / Reporte de valores TVOC.

## esp-home

v.1.2
~~~
external_components:
  - source: github://arquantis/esphome_tpm300a
    components: [ tpm300a_v1_2 ]

uart:
  id: uart_bus
  baud_rate: 9600
  rx_pin: GPIO33
  
sensor:
  - platform: tpm300a_v1_2
    update_interval: 3s
    voc:
      name: "TVOC"
      icon: "mdi:air-filter"
      unit_of_measurement: "PPM"
      filters:
        - calibrate_linear:
            # Mapeo: (Valor que entra -> Valor que querés ver)
            - 0.0 -> 400.0
            - 1023.0 -> 5000.0
~~~

v2.2
~~~
external_components:
  - source: github://arquantis/esphome_tpm300a
    components: [ tpm300a_v2_2 ]

uart:
  id: uart_bus
  baud_rate: 9600
  rx_pin: GPIO33

sensor:
  - platform: tpm300a_v2_2
    update_interval: 3s
    voc:
      name: "TVOC"
      icon: "mdi:air-filter"
      unit_of_measurement: "PPM"
      filters:
        - calibrate_linear:
            # Mapeo: (Valor que entra -> Valor que querés ver)
            - 0.0 -> 400.0
            - 2000.0 -> 5000.0
    eco2:
      name: "CH2O"
      icon: "mdi:fire"
    co2:
      name: "CO2"
      icon: "mdi:molecule-co2" 
~~~

# v1.2

## Specs / Especificaciones
Protocol / Protocolo: UART

Velocidad / Baud Rate: 9600

Logical Level / Nivel Lógico: 5V

Sensors / Sensores:
tvoc: 0-1023 PPM

## Pinout / Pines

| Pin | Función |
| :--- | :--- |
| **VCC** | 5V |
| **GND** | GND |
| **Pin B** | TX |
| **Pin A** | **N/C** ⚠️ **Desconocido** Se mantiene en 5v. |

## PCB
![TPM-300A V1.2](https://github.com/arquantis/esphome_tpm300a/raw/main/TPM-300A-V1.2_001.jpeg)

![TPM-300A V1.2](https://github.com/arquantis/esphome_tpm300a/raw/main/TPM-300A-V1.2_002.jpeg)

## Estructura de la Trama de Datos v1.2 (5 Bytes)
El sensor envía una ráfaga de 5 bytes de forma constante.

| Byte | Nombre | Valor | Función |
|:---|:---|:---|:---|
| 0 | Encabezado  | 0x2C | Inicio de trama (,) |
| 1 | tvoc High   | 0x00 - 0x03 | Parte alta del sensor (Multiplicador de 256) |
| 2 | tvoc Low    | 0x00 - 0xFF | Parte baja del sensor (Suma simple) |
| 3 | Constante   | 0x03 | Relleno (Fijo) |
| 4 | Sincronismo | 0xFF | Fin de trama |

## Ejemplo de datos del sensor
~~~
stty -F /dev/ttyUSB0 9600 raw
cat /dev/ttyUSB0 | hexdump -C
~~~

~~~
00000000  2c 00 f7 03 ff 25 2c 00  f0 03 ff 1e 2c 00 ea 03  |,....%,.....,...|
00000010  ff 18 2c 00 e7 03 ff 15  2c 00 df 03 ff 0d 2c 00  |..,.....,.....,.|
00000020  dc 03 ff 0a 2c 00 d3 03  ff 01 2c 00 cf 03 ff fd  |....,.....,.....|
~~~

## Conexión serial
~~~
cat /dev/ttyUSB0 | hexdump -C
~~~

~~~
stty -F /dev/ttyUSB0 9600 raw && stdbuf -o0 xxd -c 5 -g 1 /dev/ttyUSB0 | perl -lane 'if($F[1] eq "2c"){ $val = hex($F[2])*256 + hex($F[3]); printf "Calidad Aire (v1.2): %-5d pts | Raw: %s %s\n", $val, $F[2], $F[3] }'
~~~

## Script de Diagnóstico "Universal" (Perl)
~~~
stty -F /dev/ttyUSB0 9600 raw && stdbuf -o0 xxd -c 5 -g 1 /dev/ttyUSB0 | perl -lane '
    if($F[1] eq "2c" && $F[5] eq "ff"){ 
        $val = hex($F[2])*256 + hex($F[3]);
        printf "✅ v1.2 Aire: %-5d pts | Multi: %s | Fino: %s | Sync: OK\n", $val, $F[2], $F[3];
    } elsif($F[1] eq "2c") {
        printf "⚠️ TRAMA INCOMPLETA: %s %s %s %s %s\n", $F[1], $F[2], $F[3], $F[4], $F[5];
    }'
~~~

## Script de Diagnóstico "Universal" (Python)
~~~
import serial

port = "/dev/ttyUSB0"
baud = 9600

try:
    ser = serial.Serial(port, baud, timeout=1)
    print(f"--- Monitoreando TPM-300A v1.2 en {port} ---")
    
    while True:
        # 1. Buscamos el byte de inicio 0x2C (la coma)
        if ser.read() == b'\x2c':
            # 2. Leemos los 4 bytes restantes de la trama v1.2
            data = ser.read(4) 
            if len(data) < 4: continue
            
            # Estructura v1.2: [2c] [multi] [fino] [03] [ff]
            multiplicador = data[0]
            dato_fino     = data[1]
            constante     = data[2] # Debería ser 0x03
            sync_byte     = data[3] # Debería ser 0xff
            
            # 3. Validación de estructura básica
            if sync_byte == 0xff:
                # Calculamos el puntaje unificado (10 bits aprox)
                puntaje = (multiplicador << 8) | dato_fino
                
                print(f"✅ Calidad Aire: {puntaje:4} pts | Multi: {hex(multiplicador)} | Fino: {hex(dato_fino)} | Status: OK")
            else:
                # Si no termina en FF, perdimos el sincronismo
                print(f"⚠️ Trama desfasada: {data.hex()}")

except KeyboardInterrupt:
    print("\nDetenido por el usuario.")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals(): ser.close()
~~~

## V2.2
Esta documentación resume la ingeniería inversa realizada sobre el módulo extraído del monitor de aire XY-T01 (Bereal 210816 CDDT02 V2.6).

## Specs / Especificaciones
Protocol / Protocolo: UART

Velocidad / Baud Rate: 9600

Logical Level / Nivel Lógico: 5V

Sensors / Sensores:
tvoc: 0-2000 PPM
ch2o: ?
co2: ?

## Pinout / Pines
| Pin | Descripción |
| :--- | :--- |
| **VCC** | +5V |
| **GND** | GND |
| **Pin B** | TX  |
| **Pin A** | **Alarm / Test** ⚠️ **Peligro** Entrega 5V cuando supera el umbral. |
---

## PCB

![TPM-300A V2.2](https://github.com/arquantis/esphome_tpm300a/raw/main/TPM-300A-V2.2_001.jpeg)

![TPM-300A V2.2](https://github.com/arquantis/esphome_tpm300a/raw/main/TPM-300A-V2.2_002.jpeg)

## Estructura de la Trama de Datos (9 Bytes)
El sensor envía una ráfaga de 9 bytes de forma constante.

| Byte | Nombre | Valor | Función |
|:---|:---|:---|:---|
| 0  | Encabezado  | 0x2C | (Identificador de inicio de trama) |
| 1  | Comando     | 0xE4 | (Identifica el tipo de reporte en V2.2) |
| 2  | tvoc        | High | Byte alto |
| 3  | tvoc        | Low  | Byte bajo |
| 4  | ch2o        | High | Byte alto |
| 5  | ch2o        | Low  | Byte bajo |
| 6  | co2         | High | Byte alto |
| 7  | co2         | Low  | Byte bajo |
| 8  | Checksum    | -    | Suma de verificación (Validación de integridad) |


## Ejemplo de datos del sensor
~~~
stty -F /dev/ttyUSB0 9600 raw
cat /dev/ttyUSB0 | hexdump -C
~~~

~~~
2C E4 00 5D 00 1B 01 BB 44
~~~

## Reconstrucción de Valores (Matemática de bits)
Para obtener el valor decimal (ej. 1500 ppm) de cada sensor, se unen los dos bytes correspondientes:

~~~
Valor = (ByteHigh x 256) + ByteLow
~~~
En código C++
~~~
(uint16_t(data[High]) << 8) | data[Low]
~~~

## El Checksum (Validación de Integridad)
El Checksum es el mecanismo de seguridad para asegurar que los datos no se corrompieron en el cable.

Regla de cálculo: El último byte de la trama (Byte 8) debe ser igual a la suma de todos los bytes anteriores (0 al 7), quedándose solo con los 8 bits menos significativos.

Ejemplo de validación:

Sumás: 0x2C + 0xE4 + TVOC_H + TVOC_L + CH2O_H + CH2O_L + CO2_H + CO2_L.

Si la suma total da, por ejemplo, 0x03A4, el Checksum será 0xA4.

El código compara 0xA4 con el último byte recibido. Si coinciden, el dato es VÁLIDO.

## Conexión
~~~
stty -F /dev/ttyUSB0 9600 raw
cat /dev/ttyUSB0 | hexdump -C
~~~

~~~
stty -F /dev/ttyUSB0 9600 raw && stdbuf -o0 xxd -c 9 -g 1 /dev/ttyUSB0 | perl -lane 'if($F[1] eq "2c" && $F[2] eq "e4"){ printf "TVOC: %-5d PPM | CH2O: %-5d | CO2: %-5d\n", hex($F[3])*256+hex($F[4]), hex($F[5])*256+hex($F[6]), hex($F[7])*256+hex($F[8]) }'
~~~

## Script de Diagnóstico "Universal" (Perl)
~~~
stty -F /dev/ttyUSB0 9600 raw && stdbuf -o0 xxd -c 9 -g 1 /dev/ttyUSB0 | perl -lane '
    if($F[1] eq "2c" && $F[2] eq "e4"){ 
        $sum = (hex($F[1]) + hex($F[2]) + hex($F[3]) + hex($F[4]) + hex($F[5]) + hex($F[6]) + hex($F[7]) + hex($F[8])) & 0xFF;
        $check = hex($F[9]);
        if($sum == $check){
            printf "✅ TVOC: %-5d | CH2O: %-5d | CO2: %-5d | CS: OK\n", 
            hex($F[3])*256+hex($F[4]), 
            hex($F[5])*256+hex($F[6]), 
            hex($F[7])*256+hex($F[8]);
        } else {
            printf "❌ ERROR CHECKSUM: Calc=%02x Recv=%02x\n", $sum, $check;
        }
    }'
~~~

## Script de Diagnóstico "Universal" (Python)
~~~
import serial

# Configuración del puerto (Ajustá /dev/ttyUSB0 según tu caso)
port = "/dev/ttyUSB0"
baud = 9600

try:
    ser = serial.Serial(port, baud, timeout=1)
    print(f"--- Monitoreando TPM-300A en {port} ---")
    
    while True:
        # Buscamos el byte de inicio 0x2C
        if ser.read() == b'\x2c':
            data = ser.read(8) # Leemos los 8 bytes restantes
            if len(data) < 8: continue
            
            # Verificamos identificador de comando
            if data[0] == 0xe4:
                # 1. Cálculo del Checksum (Suma de los primeros 8 bytes)
                # El 0x2C inicial + los primeros 7 bytes de 'data'
                suma_total = 0x2c + sum(data[:7])
                checksum_calculado = suma_total & 0xFF
                checksum_recibido = data[7]
                
                if checksum_calculado == checksum_recibido:
                    # 2. Reconstrucción de valores
                    tvoc  = (data[1] << 8) | data[2]
                    ch2o = (data[3] << 8) | data[4]
                    co2  = (data[5] << 8) | data[6]
                    
                    print(f"✅ TVOC: {tvoc:4} | CH2O: ch2o:4} | CO2: {co2:4} | CS: OK")
                else:
                    print(f"❌ Error Checksum: Calc:{hex(checksum_calculado)} Recv:{hex(checksum_recibido)}")

except KeyboardInterrupt:
    print("\nDetenido por el usuario.")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals(): ser.close()
~~~

## Licencia
Este proyecto está bajo la Licencia MIT. Consultá el archivo LICENSE para más detalles.
