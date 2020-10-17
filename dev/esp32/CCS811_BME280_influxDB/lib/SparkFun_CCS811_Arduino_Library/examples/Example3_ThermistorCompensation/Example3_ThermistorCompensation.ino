/******************************************************************************
  Compensating with low-cost NTC thermistor

  Marshall Taylor @ SparkFun Electronics

  April 4, 2017

  https://github.com/sparkfun/CCS811_Air_Quality_Breakout
  https://github.com/sparkfun/SparkFun_CCS811_Arduino_Library

  NOTE: Temperature from an attached NTC Thermistor is no longer supported on the CCS811.
  This example is for reference only and will only work on SparkFun Air Quality Breakouts purchased in 2017.
  For temp/humidity compensation on the CCS811, refer to Example 2 - BME280 Compensation.

  This example uses an NTC thermistor to gather temperature data that is then used
  to compensate the CCS811.  (humidity defaulted at 50%)

  Hardware Connections (Breakoutboard to Arduino):
  3.3V to 3.3V pin
  GND to GND pin
  SDA to A4
  SCL to A5
  SEN-00250 (NTCLE100E3103JB0) between NTC terminals

  Resources:
  Uses Wire.h for i2c operation

  Development environment specifics:
  Arduino IDE 1.8.1

  This code is released under the [MIT License](http://opensource.org/licenses/MIT).

  Please review the LICENSE.md file included with this example. If you have any questions
  or concerns with licensing, please contact techsupport@sparkfun.com.

  Distributed as-is; no warranty is given.
******************************************************************************/
#include <Wire.h>
#include "SparkFunCCS811.h" //Click here to get the library: http://librarymanager/All#SparkFun_CCS811

#define CCS811_ADDR 0x5B //Default I2C Address
//#define CCS811_ADDR 0x5A //Alternate I2C Address

CCS811 myCCS811(CCS811_ADDR);

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Apply NTC data to CCS811 for compensation.");

  Wire.begin();

  //This begins the CCS811 sensor and prints error status of .beginWithStatus()
  CCS811Core::CCS811_Status_e returnCode = myCCS811.beginWithStatus();
  Serial.print("CCS811 begin exited with: ");
  //Pass the error code to a function to print the results
  Serial.print(myCCS811.statusString(returnCode));
  Serial.println();

  myCCS811.setRefResistance(9950);
}

void loop()
{
  if (myCCS811.dataAvailable())
  {
    myCCS811.readAlgorithmResults(); //Calling this function updates the global tVOC and CO2 variables

    Serial.println("CCS811 data:");
    Serial.print(" CO2 concentration : ");
    Serial.print(myCCS811.getCO2());
    Serial.println(" ppm");

    Serial.print(" TVOC concentration : ");
    Serial.print(myCCS811.getTVOC());
    Serial.println(" ppb");

    //.readNTC() causes the CCS811 library to gather ADC data and save value
    myCCS811.readNTC();
    Serial.print(" Measured resistance : ");
    //After .readNTC() is called, .getResistance() can be called to actually
    //get the resistor value.  This is not needed to get the temperature,
    //but can be useful information for debugging.
    //
    //Use the resistance value for custom thermistors, and calculate the
    //temperature yourself.
    Serial.print(myCCS811.getResistance());
    Serial.println(" ohms");

    //After .readNTC() is called, .getTemperature() can be called to get
    //a temperature value providing that part SEN-00250 is used in the
    //NTC terminals. (NTCLE100E3103JB0)
    Serial.print(" Converted temperature : ");
    float readTemperature = myCCS811.getTemperature();
    Serial.print(readTemperature, 2);
    Serial.println(" deg C");

    //Pass the temperature back into the CCS811 to compensate
    myCCS811.setEnvironmentalData(50, readTemperature);

    Serial.println();
  }
  else if (myCCS811.checkForStatusError())
  {
    printSensorError();
  }

  delay(10); //Don't spam the I2C bus
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