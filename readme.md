# Documentación Técnica: Sensor de Calidad de Aire TPM-300A (V2.2)
Esta documentación resume la ingeniería inversa realizada sobre el módulo extraído del monitor de aire XY-W01 (Bereal CDDT02 V2.6).

## ⚠️ AVISO DE SEGURIDAD: Niveles Lógicos y Voltaje (Pin A)
El sensor TPM-300A V2.2 opera con niveles lógicos de 5V, mientras que el ESP32 y la mayoría de los microcontroladores modernos operan a 3.3V.

El peligro del "Pin A" (Alarma/Test)
Durante el funcionamiento normal (aire limpio), el Pin A se mantiene en reposo. Sin embargo, hemos detectado que el sensor tiene un umbral físico interno (aproximadamente a los 1000 ppm del sensor crudo, que equivalen a unos 2500-3000 ppm en la escala filtrada del monitor original).

Comportamiento: Cuando la concentración de gas supera ese umbral, el Pin A se activa y entrega 5V constantes.

Riesgo: Si conectás este pin directamente a un GPIO del ESP32 sin protección, podrías quemar el puerto permanentemente o dañar el microcontrolador.

Recomendaciones de conexión
Uso solo para datos: Si solo te interesa la lectura digital (VOC, eCO2, LPG), NO CONECTES EL PIN A. Con el Pin B (UART) es suficiente para obtener toda la información.

Si necesitás el Pin A: Debes usar un Divisor de Tensión (2 resistencias) o un Level Shifter (conversor de nivel lógico) para bajar esos 5V a un nivel seguro de 3.3V.


## Especificaciones de Hardware

TPM-300A-V2.2

Protocolo de Comunicación: UART (Serial).

Velocidad (Baud Rate): 9600 bps.

Nivel Lógico: 5V (⚠️ Requiere precaución con microcontroladores de 3.3V como ESP32).

## Diagrama de Pines

| Pin | Función | Nota Importante |
| :--- | :--- | :--- |
| **VCC** | Alimentación 5V | Requiere 5V estables. |
| **GND** | Tierra | Común con el microcontrolador. |
| **Pin B (R)** | **Salida de Datos UART** | Transmite la trama de 9 bytes (TX del sensor). Conectar al RX del ESP32. |
| **Pin A (T)** | **Alarma / Test** | ⚠️ **Peligro para 3.3V:** Entrega 5V cuando supera el umbral. Dejar desconectado o usar divisor de tensión. |

---

## Identificación en la Placa Real

Aquí tienes las fotos de ambas caras de la PCB para identificar los puntos de soldadura:

#### Cara Frontal (Sensores y Microcontrolador)
![Cara Frontal TPM-300A V2.2](https://github.com/arquantis/TPM-300A-V2.2/raw/main/TPM-300A-V2.2_001.jpeg)

#### Cara Posterior (Pistas y Conector)
*Esta foto es ideal para seguir las pistas y confirmar las conexiones de los pines.*
![Cara Posterior TPM-300A V2.2](https://github.com/arquantis/TPM-300A-V2.2/raw/main/TPM-300A-V2.2_002.jpeg)

## Estructura de la Trama de Datos (9 Bytes)
El sensor envía una ráfaga de 9 bytes de forma constante.

| Byte | Nombre | Valor | Función |
|:---|:---|:---|:---|
| 0  | Encabezado  | 0x2C | (Identificador de inicio de trama) |
| 1  | Comando     | 0xE4 | (Identifica el tipo de reporte en V2.2) |
| 2  | VOC         | High | Byte alto del Sensor Principal (TVOC) |
| 3  | VOC         | Low  | Byte bajo del Sensor Principal (TVOC) |
| 4  | eCO2        | High | Byte alto del Sensor Aux 1 (Dióxido de Carbono eq) |
| 5  | eCO2        | Low  | Byte bajo del Sensor Aux 1 (Dióxido de Carbono eq) |
| 6  | LPG         | High | Byte alto del Sensor Aux 2 (Gases Combustibles) |
| 7  | LPG         | Low  | Byte bajo del Sensor Aux 2 (Gases Combustibles) |
| 8  | Checksum    | -    | Suma de verificación (Validación de integridad) |

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

Sumás: 0x2C + 0xE4 + VOC_H + VOC_L + eCO2_H + eCO2_L + LPG_H + LPG_L.

Si la suma total da, por ejemplo, 0x03A4, el Checksum será 0xA4.

El código compara 0xA4 con el último byte recibido. Si coinciden, el dato es VÁLIDO.

## Comportamiento Detectado por Pruebas
Prueba de Alcohol: Dispara el Sensor Principal (VOC) al máximo (5000+).

Prueba de Aliento: Afecta principalmente al Aux 1 (eCO2) y levemente al principal por la humedad.

Prueba de Encendedor (Gas): Dispara fuertemente el Aux 2 (LPG) y el Principal.

Pin A (Test): Al ponerlo a GND, el microcontrolador original fuerza el valor a 5000 y activa el buzzer.

## Mantenimiento y Calibración
Burn-in: El sensor requiere estar encendido al menos 24-48 horas después de mucho tiempo sin uso para estabilizar las lecturas.

Línea de Base: Se autocalibra tomando el valor más bajo de las últimas horas como "aire limpio". Se recomienda ventilar el ambiente diariamente.

## Conexión serial
~~~
cat /dev/ttyUSB0 | hexdump -C
~~~

~~~
stty -F /dev/ttyUSB0 9600 raw && stdbuf -o0 xxd -c 9 -g 1 /dev/ttyUSB0 | perl -lane 'if($F[1] eq "2c" && $F[2] eq "e4"){ printf "PPM: %-5d | Aux1: %-5d | Aux2: %-5d\n", hex($F[3])*256+hex($F[4]), hex($F[5])*256+hex($F[6]), hex($F[7])*256+hex($F[8]) }'
~~~

## Script de Diagnóstico "Universal" (Perl)
~~~
stty -F /dev/ttyUSB0 9600 raw && stdbuf -o0 xxd -c 9 -g 1 /dev/ttyUSB0 | perl -lane '
    if($F[1] eq "2c" && $F[2] eq "e4"){ 
        $sum = (hex($F[1]) + hex($F[2]) + hex($F[3]) + hex($F[4]) + hex($F[5]) + hex($F[6]) + hex($F[7]) + hex($F[8])) & 0xFF;
        $check = hex($F[9]);
        if($sum == $check){
            printf "✅ VOC: %-5d | eCO2: %-5d | LPG: %-5d | CS: OK\n", 
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
                    voc  = (data[1] << 8) | data[2]
                    eco2 = (data[3] << 8) | data[4]
                    lpg  = (data[5] << 8) | data[6]
                    
                    print(f"✅ VOC: {voc:4} | eCO2: {eco2:4} | LPG: {lpg:4} | CS: OK")
                else:
                    print(f"❌ Error Checksum: Calc:{hex(checksum_calculado)} Recv:{hex(checksum_recibido)}")

except KeyboardInterrupt:
    print("\nDetenido por el usuario.")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals(): ser.close()
~~~

## Final
"Este proyecto nació de la curiosidad y la necesidad de liberar hardware propietario. El conocimiento no tiene dueño, y este repositorio es un aporte para que estos sensores sigan midiendo aire en lugar de ocupar espacio en un vertedero."

## Licencia
Este proyecto está bajo la Licencia MIT. Consultá el archivo LICENSE para más detalles.
