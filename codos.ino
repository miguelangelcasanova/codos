/*********
CODOS AKA CO2 
*********/

#include <WiFi.h>                       // Librería Wi-Fi
#include <Wire.h>                   
#include <Adafruit_BME280.h>            // Librería para el sensor BME280
#include <Adafruit_Sensor.h>            // Librería estándar para los sensores de Adafruit
#include "SparkFunCCS811.h"             // Puedes descargar la librería para el sensor de CO2 en: http://librarymanager/All#SparkFun_CCS811

#define SEALEVELPRESSURE_HPA (1013.25)  // Valor de la presión al nivel del mar

#define CCS811_ADDR 0x5B                // Dirección i2c del sensor de CO2
//#define CCS811_ADDR 0x5A              // Dirección i2c alternativa del sensor de CO2

CCS811 CO2_sensor(CCS811_ADDR);   

Adafruit_BME280 BME_sensor(BME280_ADDR);       // Dirección i2c del sensor de humedad, presión y temperatura

// Reemplaza los datos con el identificador de la red WIFI y la contraseña del aula
const char* ssid     = "NOMBRE_DE_TU_RED";
const char* password = "CLAVE_DE_TU_RED";

WiFiServer server(80);                  // Seleccionar el puerto del servidor Web

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  bool status;

  Serial.println("Sensores CCS811 y BME280");

  Wire.begin(); //Inialize I2C Hardware

  if (CO2_sensor.begin() == false)
  {
    Serial.print("Error: el sensor de CO2 CCS811 no se encuentra. Por favor, comprueba el cableado.");
    while (1)
      ;
  }
  delay(10);  
  if (!bme.begin(0x76)) {
    Serial.println("Error: el sensor de temperatura, presión y humedad BME280 no se encuentra. Por favor, comprueba el cableado.");
    while (1);
  }

  // Conectar a la red  Wi-Fi con el SSID y la contraseña selecionadas
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi conectada.");
  Serial.println("Dirección IP del servidor: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the table 
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial;}");
            client.println("table { border-collapse: collapse; width:35%; margin-left:auto; margin-right:auto; }");
            client.println("th { padding: 12px; background-color: #0043af; color: white; }");
            client.println("tr { border: 1px solid #ddd; padding: 12px; }");
            client.println("tr:hover { background-color: #bcbcbc; }");
            client.println("td { border: none; padding: 12px; }");
            client.println(".sensor { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }");
            
            // Web Page Heading
            client.println("</style></head>");
            // Web Page Body
            client.println("<body><h1>ESP32 with BME280</h1>");
            client.println("<table><tr><th>MEASUREMENT</th><th>VALUE</th></tr>");
            client.println("<tr><td>Temp. Celsius</td><td><span class=\"sensor\">");
            client.println(bme.readTemperature());
            client.println(" *C</span></td></tr>");  
            client.println("<tr><td>Pressure</td><td><span class=\"sensor\">");
            client.println(bme.readPressure() / 100.0F);
            client.println(" hPa</span></td></tr>");
            client.println("<tr><td>Approx. Altitude</td><td><span class=\"sensor\">");
            client.println(bme.readAltitude(SEALEVELPRESSURE_HPA));
            client.println(" m</span></td></tr>"); 
            client.println("<tr><td>Humidity</td><td><span class=\"sensor\">");
            client.println(bme.readHumidity());
            client.println(" %</span></td></tr>"); 
            client.println("<tr><td>CO2</td><td><span class=\"sensor\">");
            if (CO2_sensor.dataAvailable())
            {
            client.println(CO2_sensor.getCO2()));
            client.println(" %</span></td></tr>"); 
            client.println("<tr><td>CO2</td><td><span class=\"sensor\">");
            client.println(CO2_sensor.getTVOC()));
            client.println(" %</span></td></tr>"); 
            client.println("</body></html>");
            }
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } 
          else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

