// @Andreas_IBZ (Telegram), 19/10/2020
//
// CCS811_BME280_InfluxDB
// Medir eCO2, TVOC, presión, humedad & temperatura ambiental con ESP32, CCS811 y BME280
//
// credits to:
// https://github.com/miguelangelcasanova/codos
// https://github.com/Makespace-Mallorca/codos
// 
// ##### TODO #####

// Falta una rutina para "ponerlo a cero" - en el datasheet se llama grabar una "baseline" al sensor. 
// Esa baseline hay que leer la primera vez después del burn-in a aire fresco y luego repetirlo periódicamente - cuanto más viejo el 
// sensor, más tiempo se puede usar la misma baseline. El mismo sensor no tiene memoria non-volatil para la baseline - hay que grabar 
// la en el EEPROM del ESP y subirla al sensor cada vez que lo reiniciamos. En la librería de Sparkfun hay un ejemplo No. 4  donde pone 
// como se hace eso.
//
// Para ahorrar energía en dispositivos alimentados con batería he introducido un "modem sleep" junto a actuar el nWAKE del CCS811. Al 
// parecer el WiFi.disconnect(true); no funciona igual con cualquier ESP32. Falta por poner un retry etc. si no se puede connectar 
// después del Wake-up. 
//
// ################
//
// Descripción del código
// - Medición del ambiente mediante el BME280
// - Aplicación de la humedad y la temperatura ambiental al algorithmo del CCS811
// - Los valores eCO2, TVOC, Temperatura, Humedad y Presión 
//		 a) salen mediante serial en formato CSV prefijado por el tiempo
// 		 b) se suben al servidor configurado en InfluxDB.h (por defecto mediante HTTPS) a un "measurement" con el nombre lo que poneís en 
//			INFLUX_MEASUREMENT - para facilitar la comparación de los valores propongo usar el Telegram-Nick - ejemplo "@Andreas_IBZ",  
// 			como nombre de la base de datos propongo "CCS811EVAL" 
// - ahorrar energía: modem sleep durante el tiempo cuando no hay que mandar datos (puede que dar problemas con algunos ESP32 - para 
//	 evalorar más en detalle), CCS811 con DriveMode 3 (una medición cada minuto) y actuando el nWAKE para dormir/despertar al interfaz I²C
//
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
//
// Cambios
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
#include "InfluxArduino.hpp" // he modificado la librería para que no hace un Serial.print de la respuesta del servidor InfluxDB - el original es: https://github.com/teebr/Influx-Arduino
#include <SparkFunBME280.h> //Click here to get the library: http://librarymanager/All#SparkFun_BME280
#include <SparkFunCCS811.h> //Click here to get the library: http://librarymanager/All#SparkFun_CCS811
#include "InfluxDB.h" // credenciales y certificado del servidor influx
#include "WiFi_credentials.h" // credenciales del WiFi

// adaptar estos valores
unsigned long PERIOD = 180000; // periodo de captura en millisegundos, 180000 = 180 s = 3 min
String colsep = ";"; // separador columna de datos CSV mediante serial
bool subirAinflux = true; // poner a false si no quereís subir los datos al servidor influx
const char sensortag[] = "CCS811-001"; // cada sensor va a una serie - propongo poner un número único
int DriveMode = 3; // 0 = Idle // 1 = read every 1s // 2 = every 10s // 3 = every 60s // 4 = RAW mode, no algorithms
uint16_t baseline = 0xAFBD; // baseline to set @startup, use getBaseline after warm-up in clean air to get this value - see example No. 4 of SparkFun CCS811 library

// Definiciones CCS811
//#define CCS811_ADDR 0x5B // Default CCS811 I2C Address
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
InfluxArduino influx;

long val_eCO2 = 0; // valor eCO2
long val_TVOC = 0; // valor TVOC
float BMEtempC = 0.0; // valor Temperatura
float BMEhumid = 0.0; // valor Humedad
float BMEpres = 0.0; // valor Presión
uint16_t currBaseline = 0; // current baseline holding variable

void setup()
{   pinMode(PIN_NOT_WAKE, OUTPUT);
    delay(100);
    digitalWrite(PIN_NOT_WAKE, LOW); // despertar CCS811
    delay(100);
    Serial.begin(115200);
    WiFi.begin(WIFI_NAME, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
   }
    Serial.println("WiFi connected!");
    delay(500);
    
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

    // InfluxDB
    if (subirAinflux) {  // only if true - solo si queremos subir datos a influxDB
    influx.configure(INFLUX_DATABASE,INFLUX_IP); // third argument (port number) defaults to 8086
    influx.authorize(INFLUX_USER,INFLUX_PASS); // comment out if you don't have set the Influxdb .conf variable auth-enabled to true
    influx.addCertificate(ROOT_CERT); // comment if you don't have generated a CA cert and copied it into InfluxDB.h
    Serial.print("Using influx with HTTPS: ");
    Serial.println(influx.isSecure()); // will be true if you've added a ROOT_CERT to the InfluxDB.h file.
    delay(1000);
    }

    // Setup servidor de tiempo NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	
	// cabezal del archivo CSV
    Serial.print("Start of measurement: ");
    printLocalTime();
    Serial.println();
    Serial.print("Measurement by: ");
    Serial.println(INFLUX_MEASUREMENT);
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
} 

void loop()
{ digitalWrite(PIN_NOT_WAKE, LOW); // despertar CCS811
  delay(100);  
  WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NAME, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    //    Serial.print(".");
   }
    //Serial.println("WiFi connected!");   
    delay(1000);
    
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

    // si queremos subir datos a influx
    if (subirAinflux) {  
    //write our variables.
    char tags[64];
    char fields[128];
    // concatenar el tag para el sensor
    char tagbuf[32];
    const char sensor[] = "Sensor=";
    strcpy(tagbuf,sensor);
    strcat(tagbuf,sensortag);
    // escribir un tag con una descripción del sensor
    sprintf(tags,tagbuf); 
    // escribir los valores
    sprintf(fields,"eCO2[ppm]=%d,TVOC[ppb]=%d,T[°C]=%0.2f,p[hPa]=%0.2f,rH[o/o]=%0.2f,baseline=%d",val_eCO2,val_TVOC,BMEtempC,BMEpres,BMEhumid,currBaseline); // escribir valores: CO2, TVOC, Temperatura, Presión, Humedad, Baseline
    bool writeSuccessful = influx.write(INFLUX_MEASUREMENT,tags,fields);
    delay(500);
    if(!writeSuccessful)
    {
        Serial.print("error: ");
        Serial.println(influx.getResponse());
    }
    }
    
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
