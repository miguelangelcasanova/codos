// ESP32_MH-Z19B_ZH07_CCS811_BME280_InfluxDB
// Medir CO2, PM & datos ambientales con ESP32, MH-Z19B, ZH07, CCS811 y BME280
//
// @Andreas_IBZ (Telegram), 03/11/2020
// @Roberbike (Telegram), 24/10/2020
//
// based on / credits to:
// https://github.com/miguelangelcasanova/codos
// https://github.com/Makespace-Mallorca/codos
// https://gitlab.com/brkmk
// https://github.com/AndreasIBZ/ESP32/tree/main/CCS811_BME280_influxDB_Makespace_Mallorca
//
// ##### TODO #####
//
// @Andreas_IBZ
// a) To save energy i introduced a "modem sleep" besides actuating nWAKE of CCS811. It looks like 
// WiFi.disconnect(true); doesn't work the same on every ESP32 out there. There should be some sort of retry etc.
// if it's impossible to connect after un wake-up. 
// b) move code to read ZH07 at the end of this sketch into a library
//
// ################
//
// Descripción del código
// - Medición del nivel CO2 y Temperatura mediante el MH-Z19B
// - Medición del particle matter (PM1.0, PM2.5, PM10) mediante el ZH07 (newest version of ZH03-series)
// - Medición del nivel TVOC y eCO2 mediante el CCS811
// - Medición del ambiente (temperatura, presión, humedad) mediante el BME280
// - Todos los valores
//     a) salen mediante serial en formato CSV prefijado por el tiempo (hay que poner debuginfo = false)
//     b) se suben al servidor configurado en InfluxDB.h (por defecto mediante HTTPS) a un "measurement" con el nombre lo que poneís en 
//      INFLUX_MEASUREMENT - para facilitar la comparación de los valores propongo usar el Telegram-Nick - ejemplo "@Andreas_IBZ",  
//      como nombre de la base de datos propongo "CCS811EVAL" (para apagar esa funcion hay que poner subirAinflux = false)
// - ahorrar energía: modem sleep durante el tiempo cuando no hay que mandar datos
//
// Wiring MH-Z19B -> ESP32
// Vin - 5V (VIN)
// RX - GPIO 32 (Software Serial TX)
// TX - GPIO 33 (Software Serial RX)
// GND - GND
//
// Wiring ZH07 (connector cable) -> ESP32
// Vdd (Pin 1, red)  - 5V (VIN)
// RX (Pin 4, green) - TX2 (GPIO 17)
// TX (Pin 5, yellow) - RX2 (GPIO 16)
// GND (Pin 2, black) - GND
//
// Wiring BME280 / CCS811 -> ESP32
// SCL - D22
// SDA - D21
// VCC - 3V3
// GND - GND
// nWAKE - D5 (only CCS811)
//
// Changelog
// 03/11/2020 - smaller corrections in influx upload string and loop
// 01/11/2020 - added bool switches to deactivate sensors that aren't used
// 31/10/2020 - rework of code to fit in ZH07-sensor without interference on existing sensors, update of influx-connection & serial CSV output
// 25/10/2020 - correciones y cambios, añadido "interruptores" bool para apagar modemsleep y debuginfo
// 24/10/2020 - añadido sensor MH-Z19B
// 19/10/2020 - añadido setBaseline al setup (valor hay que leer antes con ejemplo No. 4 de la librería SparkFun CCS811), getBaseline cada medición
// 17/10/2020 - cambios para pull-request @miguelangelcasanova
// 16/10/2020 - movido credenciales a InfluxDB.h y WiFi_credentials.h, añadido TODO
// 14/10/2020 - añadido setDriveMode(3) - una medición cada minuto para ahorrar energía (-20 mA => 50 mA total) y para ver si afecta a los valores; hay que dedicar tiempo a lo de la BASELINE
// 13/10/2020 - Wake up ESP32 + BME280 + CCS811 cada 3 min, resto modem sleep ( Wifi.mode (WIFI_OFF) ) => 150 mA con WiFi vs. 80 mA en modem sleep; además actuando nWAKE (-10 mA => 70 mA total)
// 12/10/2020 - release inicial
//
// ######################################################################################################################
//
#include <WiFi.h>
#include <Wire.h>
#include "time.h"
#include "WiFi_credentials.h" // credenciales del WiFi
// influx
#include "InfluxArduino.hpp" // he modificado la librería para que no hace un Serial.print de la respuesta del servidor InfluxDB - el original es: https://github.com/teebr/Influx-Arduino
#include "InfluxDB.h" // credenciales y certificado del servidor influx
// BME280
#include <SparkFunBME280.h> // Click here to get the library: http://librarymanager/All#SparkFun_BME280
// MH-Z19B
#include <MHZ19.h> // library for MH-Z19B: https://github.com/strange-v/MHZ19
#include <SoftwareSerial.h> // Click here to get the library: http://librarymanager/All#EspSoftwareSerial
#define TXPin 32 // para MH-Z19B
#define RXPin 33 // para MH-Z19B
// CCS811
#include <SparkFunCCS811.h> // Click here to get the library: http://librarymanager/All#SparkFun_CCS811
//#define CCS811_ADDR 0x5B // Default CCS811 I2C Address
#define CCS811_ADDR 0x5A // Alternate CCS811 I2C Address
#define PIN_NOT_WAKE 5 // nWAKE Pin en D5
// ZH07
// Definitions for the ZH07 Particle-Sensor 
#define LENG 31   //0x42 + 31 bytes equal to 32 bytes
unsigned char buf[LENG];
//
// adapt these values to your needs
unsigned long PERIOD = 60000; // capture period in milliseconds, 60000 = 60 s = 1 min, 180000 = 180 s = 3 min
String colsep = ";"; // column separator in CSV
bool modemsleep = true; // put to false to not use modemsleep feature
bool debuginfo = true; // debuginfo via serial, put to false to write CSV formatted data through serial
// adapt influx
bool subirAinflux = true; // put to false if you don't want to upload data to influx server
const char sensortag[] = "CODOS-001"; // every sensor uploads to a "Serie" which is prefixed with "Sensor" => "Sensor=CODOS_test" - i suggest to use a unique number/name

// adapt used sensors - values of not used sensors will show up with value "0" in influx and serial
// CCS811 - to be able to compensate the CCS811-values there is (at least) a temperature sensor needed; in this sketch we use the BME280 to compensate for temperature AND humidity
bool useCCS811 = true; 
int DriveMode = 3; // CCS811 DriveMode: 0 = Idle // 1 = read every 1s // 2 = every 10s // 3 = every 60s // 4 = RAW mode, no algorithms
uint16_t baseline = 0xAFBD; // CCS811 baseline to set @startup, use getBaseline after warm-up in clean air to get this value - see example No. 4 of SparkFun CCS811 library
// BME280
bool useBME280 = true;
// MH-Z19B
bool useMHZ19B = true; 
// ZH07
bool useZH07 = true; 

// Instantiate the CCS811 TVOC-Sensor with its I²C-Address
CCS811 myCCS811(CCS811_ADDR);

// Instantiate SoftwareSerial
SoftwareSerial ss(RXPin, TXPin);// RX, TX
// Instantiate the MH-Z19B CO2-Sensor with SoftwareSerial
MHZ19 mhz(&ss); 

// Instantiate the BME280 Environment-Sensor with its I²C-Address
BME280 myBME280;

// Instatiate influxDB
InfluxArduino influx; 

// NTP-Server
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600; // GMT+1 timezone
const int   daylightOffset_sec = 3600; // summer time +1h  3600
char meastime[32];

// Measurement variables
long val_CO2 = 0; // valor CO2 [ppm]
float val_tempC = 0.0; // valor Temperatura [° C]
float val_accCO2 = 0.0; // valor accuracy [? % ?]
long val_minCO2 = 0.0; // valor min CO2 [ppm]
long val_PM1_0 = 0; // value PM1.0 [? µg/m³ ?]
long val_PM2_5 = 0; // value PM2.5 [? µg/m³ ?]
long val_PM10;  // value PM10 [? µg/m³ ?]
long val_eCO2 = 0; // valor eCO2
long val_TVOC = 0; // valor TVOC
float BMEtempC = 0.0; // valor Temperatura
float BMEhumid = 0.0; // valor Humedad
float BMEpres = 0.0; // valor Presión
uint16_t currBaseline = 0; // current baseline holding variable

void setup()
{    
  // Start Hardware Serial RX0/TX0 (Arduino Sketch-Upload/Monitor)
  Serial.begin(115200); 

  // WiFi
  Serial.println(F("Starting..."));
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("WiFi connected!");
  delay(500);

  // Start I²C Communication
  Wire.begin();
  
  // wake-up Pin CCS811
  pinMode(PIN_NOT_WAKE, OUTPUT); 
  delay(100);
  digitalWrite(PIN_NOT_WAKE, LOW); // wake-up CCS811 low-active
  delay(100);

if(useZH07){
  // ZH07: Initialize Hardware Serial1 RX2/TX2 with GPIO 16/17
  Serial.println("-- Initializing Serial1 (ZH07) ...");
  Serial1.begin(9600,SERIAL_8N1, 16, 17); 
  Serial1.setTimeout(1500);    //set the Timeout to 1500ms, longer than the data transmission periodic time of the sensor  
}

if (useMHZ19B) {
  // MH-Z19B: Initialize Software Serial
  Serial.println("-- Initializing SoftwareSerial (MH-Z19B)...");
  { ss.begin(9600); 
      delay(100);
      mhz.setAutoCalibration( false );  // default settings - off autocalibration
      Serial.println("-- Reading MH-Z19B --");
      delay(200);
      Serial.print( "Acuracy:" ); Serial.println(mhz.getAccuracy()? "ON" : "OFF" );
      Serial.print( "Detection Range: " ); Serial.println( 5000 );
  }
}

if (useCCS811) {
  // CCS811: Initialize sensor and print error status of .beginWithStatus()
  CCS811Core::CCS811_Status_e returnCode = myCCS811.beginWithStatus();
  Serial.print("CCS811 begin exited with: ");
  Serial.println(myCCS811.statusString(returnCode));
  
  // CCS811: Change DriveMode
  myCCS811.setDriveMode(DriveMode); 
  
  // CCS811: Baseline
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
}

if (useBME280) {  
  // BME280: Set I²C Communication parameters
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
}

  // InfluxDB: Initialize
if (subirAinflux) {  // only if true
  influx.configure(INFLUX_DATABASE,INFLUX_IP); // third argument (port number) defaults to 8086
  influx.authorize(INFLUX_USER,INFLUX_PASS); // comment out if you don't have set the Influxdb .conf variable auth-enabled to true
  influx.addCertificate(ROOT_CERT); // comment if you don't have generated a CA cert and copied it into InfluxDB.h
  Serial.print("Using influx with HTTPS: ");
  Serial.println(influx.isSecure()); // will be true if you've added a ROOT_CERT to the InfluxDB.h file.
  delay(3000);
}

  // Setup servidor de tiempo NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Header of CSV
  Serial.print("Start of measurement: ");
  printLocalTime();
  Serial.println();
  Serial.print("Measurement by: ");
  Serial.println(INFLUX_MEASUREMENT);
  Serial.print("Sensor: ");
  Serial.println(sensortag);

  // print header of CSV through Serial
  printInfoSerialHeader();

  delay(100);
} 

void loop() 
{ 
  if (modemsleep) {
  digitalWrite(PIN_NOT_WAKE, LOW); // wake-up CCS811
  delay(100);  
  WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NAME, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Serial.print(".");
   }
    //Serial.println("WiFi connected!");   
    delay(1000);
   } 
    unsigned long startTime = millis(); // used for timing when to send data next.

if (useMHZ19B) {
  // Check to see if data is available from MH-Z19B
  MHZ19_RESULT response = mhz.retrieveData();
  if (response == MHZ19_RESULT_OK)
  {
    if (debuginfo) {
    Serial.print(F("MH-Z19B CO2: "));
    Serial.print(mhz.getCO2());
    Serial.print(F(", Min CO2: "));
    Serial.print(mhz.getMinCO2());
    Serial.print(F(", Temperature: "));
    Serial.print(mhz.getTemperature());
    Serial.print(F(", Accuracy: "));
    Serial.println(mhz.getAccuracy());
    }
  } else {
    Serial.print( "Error Reading MH-Z19B Module" );
    Serial.print(F("Error, code: "));
    Serial.println(response);
  }
}

  // Get PM data from ZH07 // #################### ToDo: move to library #######################
  if(useZH07 && Serial1.find(0x42)){    //start to read when detect 0x42
    Serial1.readBytes(buf,LENG);
    if(buf[0] == 0x4d){
      if(checkValue(buf,LENG)){
        val_PM1_0 = transmitPM01(buf); //count PM1.0 value of the air detector module
        val_PM2_5 = transmitPM2_5(buf);//count PM2.5 value of the air detector module
        val_PM10 = transmitPM10(buf); //count PM10 value of the air detector module 
          if (debuginfo) {
            Serial.print("ZH07 PM1.0: ");
            Serial.print(val_PM1_0);        
            Serial.print(", PM2.5: ");
            Serial.print(val_PM2_5); 
            Serial.print(", PM10: ");
            Serial.println(val_PM10);
         }
      }           
    } 
  }
  
  // Check to see if CCS811 data is available
  if (useCCS811 && myCCS811.dataAvailable()) {
    // Calling this function updates the global TVOC and eCO2 variables
    myCCS811.readAlgorithmResults();
    // Read main CCS811 values
    delay(100);
    val_eCO2 = myCCS811.getCO2();
    val_TVOC = myCCS811.getTVOC();
    currBaseline = myCCS811.getBaseline();    
      if (debuginfo) {
         Serial.print("CCS811 eCO2: ");
         Serial.print(myCCS811.getCO2());
         Serial.print(", TVOC: ");         
         Serial.println(myCCS811.getTVOC());
      }

    delay(100);
  }

  // Read all the other values
if (useMHZ19B) {
    // Read MH-Z19B data    
    val_tempC = mhz.getTemperature();
    val_CO2 = mhz.getCO2();
    val_accCO2 = mhz.getAccuracy();
    val_minCO2 = mhz.getMinCO2();
}

if (useBME280) {  
    // Read BME280 data
    BMEtempC = myBME280.readTempC();
    BMEhumid = myBME280.readFloatHumidity();
    BMEpres = (myBME280.readFloatPressure()) / 100; // Conversion to hPa
    if (debuginfo) {
            Serial.print("BME280 T: ");
            Serial.print(BMEtempC);        
            Serial.print(", rH: ");
            Serial.print(BMEhumid); 
            Serial.print(", p: ");
            Serial.println(BMEpres);
         }
}

if (useCCS811) {
    // send BME280 values to the CCS811 for compensation
    myCCS811.setEnvironmentalData(BMEhumid, BMEtempC);
}

    // print values in CSV format through Serial
    printInfoSerial();

    // if we want to upload data to
    if (subirAinflux) {  
    // initialize our variables.
    char tags[64];
    char fields[1024];
    // prefix tag of sensor with "Sensor="
    char tagbuf[32];
    const char sensor[] = "Sensor=";
    strcpy(tagbuf,sensor);
    strcat(tagbuf,sensortag);
    // write the tag with a description of the sensor
    sprintf(tags,tagbuf); 
    // write values
    sprintf(fields,"CO2[ppm]=%d,T_MH[°C]=%0.1f,accuracy[o/o]=%0.1f,CO2_min[ppm]=%d,PM1_0[µg/m³]=%d,PM2_5[µg/m³]=%d,PM10[µg/m³]=%d,eCO2[ppm]=%d,TVOC[ppb]=%d,baseline=%d,T_BME[°C]=%0.1f,p[hPa]=%0.1f,rH[o/o]=%0.1f",val_CO2,val_tempC,val_accCO2,val_minCO2,val_PM1_0,val_PM2_5,val_PM10,val_eCO2,val_TVOC,currBaseline,BMEtempC,BMEpres,BMEhumid); // escribir valores: CO2, Temperatura, accuracy y CO2_min
    bool writeSuccessful = influx.write(INFLUX_MEASUREMENT,tags,fields);
    delay(500);
    if(!writeSuccessful)
    {
        Serial.print("error: ");
        Serial.println(influx.getResponse());
    }
    }

    while ((millis() - startTime) < PERIOD)
    {
      // modem sleep until it's time for next reading. Consider using a low power mode if this will be a while.
      if (modemsleep) {  
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        digitalWrite(PIN_NOT_WAKE, HIGH); // sleep CCS811
        delay(100);  
      }
    }
    delay(1);
}

void printInfoSerialHeader() // Header for CSV data
  {
    Serial.println("Time" + colsep + "CO2 [ppm]" + colsep +  "T_MH [°C]" + colsep +  "PM1.0 [µg/m³]" + colsep +  "PM2.5 [µg/m³]" + colsep +  "PM10 [µg/m³]" + colsep + "eCO2 [ppm]" + colsep + "TVOC [ppb]"  + colsep + "Baseline" + colsep + "T_BME [°C]"  + colsep + "p [hPa]"  + colsep + "rH [o/o]");
  }
 
void printInfoSerial() // in CSV anotation
  { 
  printLocalTime();
  //data from MH-Z19B
  Serial.print(colsep); 
  Serial.print(val_CO2);
  Serial.print(colsep); 
  Serial.print(val_tempC); 
  //data from ZH07
  Serial.print(colsep); 
  Serial.print(val_PM1_0);
  Serial.print(colsep);   
  Serial.print(val_PM2_5);
  Serial.print(colsep);  
  Serial.print(val_PM10);
  //data from CCS811
  Serial.print(colsep);  
  Serial.print(val_eCO2);
  Serial.print(colsep); 
  Serial.print(val_TVOC);
  Serial.print(colsep); 
  Serial.print(currBaseline);  
  //data from the BME280
  Serial.print(colsep); 
  Serial.print(BMEtempC); 
  Serial.print(colsep); 
  Serial.print(BMEpres); 
  Serial.print(colsep); 
  Serial.print(BMEhumid);
  Serial.println(); // nueva línea
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

// #################### ToDo ZH07: move to library #######################

char checkValue(unsigned char *thebuf, char leng)
{  
  char receiveflag=0;
  int receiveSum=0;
 
  for(int i=0; i<(leng-2); i++){
  receiveSum=receiveSum+thebuf[i];
  }
  receiveSum=receiveSum + 0x42;
 
  if(receiveSum == ((thebuf[leng-2]<<8)+thebuf[leng-1]))  //check the serial data 
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}
int transmitPM01(unsigned char *thebuf)
{
  int PM01Val;
  PM01Val=((thebuf[3]<<8) + thebuf[4]); //count PM1.0 value of the air detector module
  return PM01Val;
}
//transmit PM Value to PC
int transmitPM2_5(unsigned char *thebuf)
{
  int PM2_5Val;
  PM2_5Val=((thebuf[5]<<8) + thebuf[6]);//count PM2.5 value of the air detector module
  return PM2_5Val;
  }
//transmit PM Value to PC
int transmitPM10(unsigned char *thebuf)
{
  int PM10Val;
  PM10Val=((thebuf[7]<<8) + thebuf[8]); //count PM10 value of the air detector module  
  return PM10Val;
}