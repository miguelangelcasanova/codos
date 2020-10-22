#include <WiFi.h>
#include "InfluxArduino.hpp"
#include "InfluxCert.hpp"
#include "Adafruit_BME680.h"

#define LED_PIN 13
InfluxArduino influx;
Adafruit_BME680 bme;

//connection/ database stuff that needs configuring
const char WIFI_NAME[] = "";
const char WIFI_PASS[] = "";
const char INFLUX_DATABASE[] = "";
const char INFLUX_IP[] = "";
const char INFLUX_USER[] = "";
const char INFLUX_PASS[] = "";
const char INFLUX_MEASUREMENT[] = "";

unsigned long DELAY_TIME_US = 5 * 1000 * 1000; //how frequently to send data, in microseconds
unsigned long count = 0;                       //a variable that we gradually increase in the loop

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!bme.begin(0x76))
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1)
    {
    }
  }
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  WiFi.begin(WIFI_NAME, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected!");
  influx.configure(INFLUX_DATABASE, INFLUX_IP); //third argument (port number) defaults to 8086
  influx.authorize(INFLUX_USER, INFLUX_PASS);   //if you have set the Influxdb .conf variable auth-enabled to true, uncomment this
  influx.addCertificate(ROOT_CERT);             //uncomment if you have generated a CA cert and copied it into InfluxCert.hpp
  Serial.print("Using HTTPS: ");
  Serial.println(influx.isSecure()); //will be true if you've added the InfluxCert.hpp file.
}

void loop()
{
  unsigned long startTime = micros();

  char tags[16];
  char fields[128];
  char formatString[] = "temperature=%0.3f,pressure=%0.3f,humidity=%0.3f,gas_resistance=%0.3f";

  if (bme.performReading())
  {
    sprintf(tags, "read_ok=true");
    sprintf(fields, formatString, bme.temperature, bme.pressure / 100000.0, bme.humidity, bme.gas_resistance / 1000.0);
  }
  else
  {
    sprintf(tags, "read_ok=false");
    sprintf(fields, formatString, -1.0, -1.0, -1.0, -1.0);
    Serial.println("Failed to perform reading :(");
  }
  bool writeSuccessful = influx.write(INFLUX_MEASUREMENT, tags, fields);
  digitalWrite(LED_PIN, HIGH);
  if (!writeSuccessful)
  {
    Serial.print("error: ");
    Serial.println(influx.getResponse());
  }
  else
  {
    delay(50);
    digitalWrite(LED_PIN, LOW);
  }

  while ((micros() - startTime) < DELAY_TIME_US)
  {
    //kill time until we're ready for the next reading
  }
}
