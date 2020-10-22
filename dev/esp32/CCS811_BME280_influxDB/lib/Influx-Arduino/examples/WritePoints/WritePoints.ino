#include <WiFi.h>
#include "InfluxArduino.hpp"
#include "InfluxCert.hpp"

InfluxArduino influx;
//connection/ database stuff that needs configuring
const char WIFI_NAME[] = "";
const char WIFI_PASS[] = "";
const char INFLUX_DATABASE[] = "";
const char INFLUX_IP[] = "XXX.XXX.XXX.XXX";
const char INFLUX_USER[] = ""; //username if authorization is enabled.
const char INFLUX_PASS[] = ""; //password for if authorization is enabled.
const char INFLUX_MEASUREMENT[] = ""; //measurement name for the database. (in practice, you can use several, this example just uses the one)

unsigned long DELAY_TIME_US = 5 * 1000 * 1000; //how frequently to send data, in microseconds
unsigned long count = 0; //a variable that we gradually increase in the loop

void setup()
{   
    Serial.begin(115200);

    WiFi.begin(WIFI_NAME, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("WiFi connected!");
    influx.configure(INFLUX_DATABASE,INFLUX_IP); //third argument (port number) defaults to 8086
    // influx.authorize(INFLUX_USER,INFLUX_PASS); //if you have set the Influxdb .conf variable auth-enabled to true, uncomment this
    // influx.addCertificate(ROOT_CERT); //uncomment if you have generated a CA cert and copied it into InfluxCert.hpp
    Serial.print("Using HTTPS: ");
    Serial.println(influx.isSecure()); //will be true if you've added the InfluxCert.hpp file.
} 

void loop()
{   
    unsigned long startTime = micros(); //used for timing when to send data next.

    //update our field variables
    float dummy = ((float)random(0, 1000)) / 1000.0;
    count++;

    //write our variables.
    char tags[32];
    char fields[32];

    sprintf(tags,"new_tag=Yes"); //write a tag called new_tag
    sprintf(fields,"count=%d,random_var=%0.3f",count,dummy); //write two fields: count and random_var
    bool writeSuccessful = influx.write(INFLUX_MEASUREMENT,tags,fields);
    if(!writeSuccessful)
    {
        Serial.print("error: ");
        Serial.println(influx.getResponse());
    }

    while ((micros() - startTime) < DELAY_TIME_US)
    {
      //wait until it's time for next reading. Consider using a low power mode if this will be a while.
    }
}
