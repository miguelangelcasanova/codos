/*****************************************************************************************************************************************
Programa de ejemplo para monitorizar los niveles de Dióxido de Carbono (equivalente) eCO2 y la Tasa de Compuestos Orgánicos Volátiles TVOC
*****************************************************************************************************************************************/

#include "Adafruit_CCS811.h"          // Incluir la librería del sensor
Adafruit_CCS811 ccs;                  // Crear un objeto sensor

void setup() {
  Serial.begin(115200);               // Inicializar el puerto serie
  //  Serial.println("Programa para graficar los datos del sensor CCS811 mediante el Serial Plotter del IDE de Arduino");
  Serial.println("CO2, TVOC");
  if(!ccs.begin()){                   // Intentar inicializar el sensor
    Serial.println("¡No se ha podido inicializar el sensor! Por favor, comprueba el cableado.");
    while(1);
  }
  while(!ccs.available());            // Esperar a que el sensor esté preparado
}

void loop() {
  if(ccs.available()){                // Si el sensor está disponible
    if(!ccs.readData()){              // Cuando haya datos
      Serial.print(ccs.geteCO2());    // Obtener y enviar el nivel del CO2 por el puerto serie
      Serial.print(", ");              // Separar los valores de CO2 y de TVOC 
      Serial.println(ccs.getTVOC());  // Obtener y enviar la tasa de TVOC, enviarla por el puerto serie y acabar la dupla de datos
    }
    else{
      Serial.println("ERROR!");       // Cuando ocurra un error
      while(1);                       // El dispositivo deja de funcionar
    }
  }
  delay(2000);                         // Hacer la medida cada 2 segundos(2000ms)
}
