#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h> */
#include "Adafruit_CCS811.h"
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include "settings.h"

/* Configuración cliente WiFi */
WiFiClient espClient;

/* Configuración MQTT */
PubSubClient clientMqtt(espClient);
char msg[50];
String mqttcommand = String(14);

/* Configuración led NEOPIXEL */
Adafruit_NeoPixel pixels_STRIP_1  = Adafruit_NeoPixel(NUMPIXELS_STRIP_1, PIN_STRIP_1, NEO_GRB + NEO_KHZ800);
int color_R;
int color_G;

/* Configuración sensor CCS811 */
Adafruit_CCS811 ccs;
long lastMsg = 0;


void setup() {
  Serial.begin(9600);

  /* Iniciar sensor CCS811 */
  Serial.println("CCS811 test");
  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
    while(1);
  }
  // Wait for the sensor to be ready
  while(!ccs.available());

  /* Iniciar NEOPIXEL */
  pixels_STRIP_1.begin(); // This initializes the NeoPixel library. 
  for(int i=0;i<NUMPIXELS_STRIP_1;i++){
     pixels_STRIP_1.setPixelColor(i, pixels_STRIP_1.Color(0,0,0)); // black color.
     pixels_STRIP_1.show(); // This sends the updated pixel color to the hardware.
  }

  /* Iniciar wifi */
  setup_wifi();
  clientMqtt.setServer(mqtt_server, mqtt_port);
  clientMqtt.setCallback(callback);
}

void setup_wifi() {
  delay(10);

  // Comienza el proceso de conexión a la red WiFi
  Serial.println();
  Serial.print("[WIFI]Conectando a ");
  Serial.println(ssid);

  // Modo estación
  WiFi.mode(WIFI_STA);
  // Inicio WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("[WIFI]WiFi conectada");
  Serial.print("[WIFI]IP: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT]Mensaje recibido (");
  Serial.print(topic);
  Serial.print(") ");
  mqttcommand = "";
  for (int i = 0; i < length; i++) {
    mqttcommand += (char)payload[i];
  }
  Serial.print(mqttcommand);
  Serial.println();
  // Switch on the LED if an 1 was received as first character
  if (mqttcommand == "LightOn") {
    pixels_STRIP_1.fill(pixels_STRIP_1.Color(255, 255, 255),0, pixels_STRIP_1.numPixels()); // white  
    pixels_STRIP_1.show(); // This sends the updated pixel color to the hardware.
    Serial.println("LightOn");
  } else if (mqttcommand == "LightOff"){
      pixels_STRIP_1.fill(pixels_STRIP_1.Color(0, 0, 0),0, pixels_STRIP_1.numPixels()); // black  
      pixels_STRIP_1.show(); // This sends the updated pixel color to the hardware.
      Serial.println("LightOff");
  }
}

void reconnect() {
  Serial.print("[MQTT]Intentando conectar a servidor MQTT... ");
  // Bucle hasta conseguir conexión
  while (!clientMqtt.connected()) {
    Serial.print(".");
    // Intento de conexión
    if (clientMqtt.connect(mqtt_id)) { // Ojo, para más de un dispositivo cambiar el nombre para evitar conflicto
      Serial.println("");
      Serial.println("[MQTT]Conectado al servidor MQTT");
      // Once connected, publish an announcement...
      clientMqtt.publish(mqtt_sub_topic_healthcheck, "starting");
      // ... and subscribe
      clientMqtt.subscribe(mqtt_sub_topic_operation);
    } else {
      Serial.print("[MQTT]Error, rc=");
      Serial.print(clientMqtt.state());
      Serial.println("[MQTT]Intentando conexión en 5 segundos");

      delay(5000);
    }
  }
}

void loop() {
  if (!clientMqtt.connected()) {
    reconnect();
  }
  clientMqtt.loop();
  
  long now_sensors = millis();
  if (now_sensors - lastMsg > update_time_sensors) {
    lastMsg = now_sensors;

    if(ccs.available()){
      if(!ccs.readData()){
        Serial.println("CO2: ");
        Serial.print(ccs.geteCO2());
        Serial.print("ppm, TVOC: ");
        Serial.println(ccs.getTVOC());
        /* Publicación de co2 y tvoc en cada topic */
        snprintf (msg, sizeof(msg), "%d", ccs.geteCO2());
        clientMqtt.publish(mqtt_pub_topic_co2, msg);
        snprintf (msg, sizeof(msg), "%d", ccs.getTVOC());
        clientMqtt.publish(mqtt_pub_topic_tvoc, msg);

        /* Asignación de color al NeoPixel */
        color_R = (ccs.geteCO2()-CO2_MIN) / ((CO2_MAX-CO2_MIN)/255);
        if(color_R>255){color_R=255;}
        color_G = 255 - ((ccs.geteCO2()-CO2_MIN) / ((CO2_MAX/2-CO2_MIN)/255)) + 1;
        if(color_G>255){color_G=255;}
        pixels_STRIP_1.fill(pixels_STRIP_1.Color(color_R, color_G, 0),0, pixels_STRIP_1.numPixels()); 
        pixels_STRIP_1.show();
      }
      else{
        Serial.println("ERROR!");
        while(1);
      }
    }
  }
}
