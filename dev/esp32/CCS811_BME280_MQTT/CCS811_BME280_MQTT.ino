// IMPORTANTE: los envios MQTT no funcionan. El problema está relacionado con que en el Loop la wifi se desconecto 

// Descripción del código
// - Medición del ambiente mediante el BME280
// - Aplicación de la humedad y la temperatura ambiental al algorithmo del CCS811
// - Los valores eCO2, TVOC, Temperatura, Humedad y Presión 
//     a) salen mediante serial en formato CSV prefijado por el tiempo
//     b) se envía uno a uno a cada uno de los topics definidos en MQTT.h 
// - ahorrar energía: modem sleep durante el tiempo cuando no hay que mandar datos (puede que dar problemas con algunos ESP32 - para 
//   evalorar más en detalle), CCS811 con DriveMode 3 (una medición cada minuto) y actuando el nWAKE para dormir/despertar al interfaz I²C

// Wiring BME280 - ESP32
// SCL - D22
// SDA - D21
// VCC - 3V3
// GND - GND
//
// Wiring CCS811 - ESP32
// VCC - 3V3
// SCL - D22
// SDA - D21
// GND - GND
// nWAKE - D5

#include <WiFi.h>
#include <PubSubClient.h> //Click here to get the library: http://librarymanager/All#PubSubClient
#include <Wire.h>
#include "time.h"
#include <SparkFunBME280.h> //Click here to get the library: http://librarymanager/All#SparkFun_BME280
#include <SparkFunCCS811.h> //Click here to get the library: http://librarymanager/All#SparkFun_CCS811
#include <Adafruit_NeoPixel.h>
#include "WiFi_credentials.h" // credenciales del WiFi
#include "MQTT.h" // configuración y TOPICs para MQTT
#include "settings.h" // otras configuraciones

#ifdef __AVR__
  #include <avr/power.h>
#endif

// adaptar estos valores
unsigned long PERIOD = 180000; // periodo de captura en millisegundos, 180000 = 180 s = 3 min
String colsep = ";"; // separador columna de datos CSV mediante serial
const char sensortag[] = "CCS811-001"; // cada sensor va a una serie - propongo poner un número único
int DriveMode = 3; // 0 = Idle // 1 = read every 1s // 2 = every 10s // 3 = every 60s // 4 = RAW mode, no algorithms
uint16_t baseline = 0xAFBD; // baseline to set @startup, use getBaseline after warm-up in clean air to get this value - see example No. 4 of SparkFun CCS811 library

// Definiciones CCS811
#define CCS811_ADDR 0x5A // Alternate CCS811 I2C Address
#define PIN_NOT_WAKE 5 // nWAKE Pin en D5

// NTP-Server
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600; // GMT+1 timezone
const int   daylightOffset_sec = 3600; // summer time +1h
char meastime[32];

// Global objects
CCS811 myCCS811(CCS811_ADDR);
BME280 myBME280;

// Varialbles para valores de los sensores
long val_eCO2 = 0; // valor eCO2
long val_TVOC = 0; // valor TVOC
float BMEtempC = 0.0; // valor Temperatura
float BMEhumid = 0.0; // valor Humedad
float BMEpres = 0.0; // valor Presión
uint16_t currBaseline = 0; // current baseline holding variable

/* Configuración MQTT */
WiFiClient espClient;
PubSubClient clientMqtt(espClient);
char msg[50];
String mqttcommand = String(14);

/* Configuración led NEOPIXEL */
Adafruit_NeoPixel pixels_STRIP_1  = Adafruit_NeoPixel(NUMPIXELS_STRIP_1, PIN_STRIP_1, NEO_GRB + NEO_KHZ800);
int color_R;
int color_G;

void setup() {
  pinMode(PIN_NOT_WAKE, OUTPUT);
  delay(100);
  digitalWrite(PIN_NOT_WAKE, LOW); // despertar CCS811
  delay(100);
  Serial.begin(115200);

  // Empezar comunicación I2C
  Wire.begin();
  // This begins the CCS811 sensor and prints error status of .beginWithStatus()
  CCS811Core::CCS811_Status_e returnCode = myCCS811.beginWithStatus();
  Serial.print("CCS811 begin exited with: ");
  Serial.println(myCCS811.statusString(returnCode));
  
  // Change DriveMode
  myCCS811.setDriveMode(DriveMode); 
  
  // Baseline
  //This programs the baseline into the sensor and monitors error states
      returnCode = myCCS811.setBaseline(baseline);
      if (returnCode == CCS811Core::CCS811_Stat_SUCCESS)
      {
        Serial.println("Baseline written to CCS811.");
      }
      else
      {
        Serial.print("Error writing baseline: ");
        Serial.println(myCCS811.statusString(returnCode));
      }
  delay(100);

  setup_wifi();

  // Setup servidor de tiempo NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // cabezal del archivo CSV
  Serial.print("Start of measurement: ");
  printLocalTime();
  Serial.println();
  Serial.print("Sensor: ");
  Serial.println(sensortag);

  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x76;

  //Initialize BME280
  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x76;
  myBME280.settings.runMode = 3; //Normal mode
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;

  //Calling .begin() causes the settings to be loaded
  myBME280.begin();
  delay(100); //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.

  // mandar cabezal del CSV mediante serial
  printInfoSerialHeader();

  delay(100);

  /* Iniciar MQTT */
  clientMqtt.setServer(mqtt_server, mqtt_port);
  clientMqtt.setCallback(callback);

  /* Iniciar NEOPIXEL */
  pixels_STRIP_1.begin(); // This initializes the NeoPixel library. 
  for(int i=0;i<NUMPIXELS_STRIP_1;i++){
     pixels_STRIP_1.setPixelColor(i, pixels_STRIP_1.Color(0,0,0)); // black color.
     pixels_STRIP_1.show(); // This sends the updated pixel color to the hardware.
  }

}

void setup_wifi() {
  delay(10);

  // Comienza el proceso de conexión a la red WiFi
  Serial.println();
  Serial.print("[WIFI]Conectando a ");
  Serial.println(WIFI_NAME);

  // Modo estación
  WiFi.mode(WIFI_STA);
  // Inicio WiFi
  WiFi.begin(WIFI_NAME, WIFI_PASS);
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
  } else {
    CO2_MAX = mqttcommand.toInt();
    Serial.println(CO2_MAX);
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
  digitalWrite(PIN_NOT_WAKE, LOW); // despertar CCS811
  delay(100);  

  unsigned long startTime = millis(); // used for timing when to send data next.

  //Check to see if data is available
  if (myCCS811.dataAvailable())
  {
    //Calling this function updates the global tVOC and eCO2 variables
    myCCS811.readAlgorithmResults();

    BMEtempC = myBME280.readTempC();
    BMEhumid = myBME280.readFloatHumidity();

    //This sends the temperature data to the CCS811
    myCCS811.setEnvironmentalData(BMEhumid, BMEtempC);

    // update other field variables
    val_eCO2 = myCCS811.getCO2();
    val_TVOC = myCCS811.getTVOC();
    BMEpres = (myBME280.readFloatPressure()) / 100; // Conversión a hPa
    currBaseline = myCCS811.getBaseline();
    
    // mandar valores en formato CSV mediante serial 
    printInfoSerial();

    /* Envío de lecturas por MQTT */
    if (!clientMqtt.connected()) {
      reconnect();
    }
    clientMqtt.loop();

    snprintf (msg, 6, "%2.1f", BMEtempC);
    Serial.print("[MQTT]Enviando mensaje de temperatura: ");
    Serial.println(msg);
    clientMqtt.publish(mqtt_pub_topic_temperatura, msg);
    Serial.println(clientMqtt.state());

    char humString[8];
    dtostrf(BMEhumid, 1, 2, humString);
    Serial.print("[MQTT]Enviando mensaje de humedad: ");
    Serial.println(humString);
    clientMqtt.publish(mqtt_pub_topic_humedad, humString);

    snprintf (msg, 6, "%2.1f", val_eCO2);
    Serial.print("[MQTT]Enviando mensaje de CO2: ");
    Serial.println(msg);
    clientMqtt.publish(mqtt_pub_topic_co2, msg);

    /* Asignación de color al NeoPixel */
    color_R = (val_eCO2-CO2_MIN) / ((CO2_MAX-CO2_MIN)/255);
    if(color_R>255){color_R=255;}
    color_G = 256 - color_R;
    if(color_G>255){color_G=255;}
    pixels_STRIP_1.fill(pixels_STRIP_1.Color(color_R, color_G, 0),0, pixels_STRIP_1.numPixels()); 
    pixels_STRIP_1.show();
  }
  else if (myCCS811.checkForStatusError())
  {
    // If the CCS811 found an internal error, print it.
    printSensorError();
  }
  while ((millis() - startTime) < PERIOD)
  {
    // modem sleep until it's time for next reading. Consider using a low power mode if this will be a while.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    digitalWrite(PIN_NOT_WAKE, HIGH); // dormir al CCS811
    delay(1);
  }
  delay(1);
}

void printInfoSerialHeader() // Header for CSV data
  {
    // Serial.println("CCS811 data");
    Serial.println("Time" + colsep + "eCO2 [ppm]" + colsep + "TVOC [ppb]" + colsep + "T [°C]"  + colsep + "p [hPa]"  + colsep + "rH [o/o]" + colsep + "Baseline");
  }
 
void printInfoSerial() // in CSV anotación
{ 
  printLocalTime();
  Serial.print(colsep); 
  //data from CCS811
  Serial.print(val_eCO2);
  Serial.print(colsep); 
  Serial.print(val_TVOC);
  Serial.print(colsep); 
  //data from the BME280
  Serial.print(BMEtempC); 
  Serial.print(colsep); 
  Serial.print(BMEpres); 
  Serial.print(colsep); 
  Serial.print(BMEhumid);
  Serial.print(colsep); 
  Serial.print(currBaseline);  
  Serial.println(); // nueva línea
}

//printSensorError gets, clears, then prints the errors
//saved within the error register.
void printSensorError()
{
  uint8_t error = myCCS811.getErrorRegister();

  if (error == 0xFF) //comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  }
  else
  {
    Serial.print("Error: ");
    if (error & 1 << 5)
      Serial.print("HeaterSupply");
    if (error & 1 << 4)
      Serial.print("HeaterFault");
    if (error & 1 << 3)
      Serial.print("MaxResistance");
    if (error & 1 << 2)
      Serial.print("MeasModeInvalid");
    if (error & 1 << 1)
      Serial.print("ReadRegInvalid");
    if (error & 1 << 0)
      Serial.print("MsgInvalid");
    Serial.println();
  }
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(meastime,20, "%d/%m/%Y %H:%M:%S", &timeinfo);
  Serial.print(meastime);
}
