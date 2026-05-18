
#include <Arduino.h>
#include <SPI.h>
#include "Globals.h"
#include "HAL.h"
#include "MAX31855.h"

const int VMEASURE_ADCIN = 9; // Define the GPIO pin
const int VIN_SEL = 42;

void setup() {

  Serial.begin(115200);
  Serial.println("before HAL");

  pinMode(VIN_SEL, OUTPUT);
  digitalWrite(VIN_SEL, HIGH);

  // do communication initializations before peripherals
  HAL::init();

  // // set up peripherals
  // MAX31855::setupMAX();
  //MCP4725::init();

  // initialize all other peripherals here:

}

void loop() {
  // put your main code here, to run repeatedly:

  // read all sensors
  // MAX31855::readMAX();

  // get values from all sensors
  // float maxF = MAX31855::tempF;

  int sensorValue = analogRead(VMEASURE_ADCIN); // Read the analog pin
  Serial.println("VMEASURE reading: " + String(sensorValue, 4));
  float voltage = (sensorValue * 3.3) / 4095.0;

  float temp_RAW = voltage * 10;
  Serial.println("VMEASURE reading: " + String(voltage, 4) + "V, TEMP: " + String(temp_RAW) + " deg C"); // Print value to the Serial Monitor

  // print stuff
  // Serial.println("MAX31855 reading: " + String(maxF) + " deg Fahrenheit");
  // Serial.println("do other stuff now...");

  delay(100);

}
