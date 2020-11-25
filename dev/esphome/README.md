# Distintos sketches para ESPHome.

### nodemcu_ccs811.yaml

Archivo simple, para nodemcu y un sensor ccs811. 
Es necesario editarlo para configurar las credenciales de WiFi y la dirección del servidor MQTT

### esp32_mhz19_display.yaml

Plantilla para ESP32 con sensor MH-Z19 y display oled. Este archivo depende de otros incluidos en ./co2_common.
Es fácil modificar el tipo de sensor utilizado simplemente comentando/descomentando las líneas correspondientes:

    packages:
      display: !include "co2_common/display.yaml"
      mqtt: !include co2_common/mqtt.yaml 
      mhz19: !include "co2_common/sensor_mhz19.yaml"
      bme280: !include "co2_common/sensor_bme280.yaml"
      #  mq135: !include "co2_common/sensor_mq135.yaml"
      #  ccs811: !include "co2_common/sensor_ccs811.yaml"
 
Es necesario crear un archivo co2_common/secrets.yaml con las credenciales de wifi y otros parámetros. En esa carpeta se encuentra un archivo secrets.yaml.sample de ejemplo.

### wemosD1_ccs811_display.yaml
Mismo sistema de plantilla que el anterior, pero configurado para una placa WemosD1 y sensor CCS811

