#include <Arduino.h>
#include <SPI.h>
#include "Globals.h"
#include "HAL.h"
#include "MAX31855.h"

// any and all other #include here:

// put function declarations here:
int myFunction(int, int);

void setup() {
  Serial.begin(115200);

  // do SPI initializations before peripherals
  HAL::initCSPins();
  HAL::initHSPI_HAL();
  HAL::initVSPI_HAL();

  // set up peripherals
  MAX31855::setupMAX();

  // initialize all other peripherals here:

}

void loop() {
  // put your main code here, to run repeatedly:

  // read all sensors
  MAX31855::readMAX();

  // get values from all sensors
  float maxF = MAX31855::tempF;

  // print stuff
  Serial.println("MAX31855 reading: " + String(maxF) + " deg Fahrenheit");
  Serial.println("do other stuff now...");

  delay(100);

}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}