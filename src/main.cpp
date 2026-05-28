#include <Arduino.h>
#include <SPI.h>
#include <math.h>
#include "Globals.h"
#include "HAL.h"
#include "MAX31855.h"
#include "MCP4725.h"
#include "potentiometer.h"
#include "INA.h"
#include "lcd_ui.h"

const int VMEASURE_ADCIN = 9; // Define the GPIO pin
const int VIN_SEL = 42;

const float POT_CHANGE_THRESHOLD_V = 0.05f;
const unsigned long SET_MODE_TIMEOUT_MS = 3000;
const unsigned long DISPLAY_UPDATE_MS = 500; // LCD only updates every 0.5 seconds

const float PLACEHOLDER_READ_TEMP_C = 50.0f;

enum DisplayMode {
  READ_TEMP_MODE,
  SET_TEMP_MODE
};

DisplayMode displayMode = READ_TEMP_MODE;

float lastPotVoltage = 0.0f;
unsigned long lastPotChangeTime = 0;
unsigned long lastDisplayUpdateTime = 0;

float tempCToReferenceVoltage(float tempC) {
  // TODO: replace with real inverse calibration equation.
  // Placeholder: 0 C -> 0.0 V, 200 C -> 3.3 V
  return constrain((tempC / 200.0f) * 3.3f, 0.0f, 3.3f);
}

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println("before HAL");

  pinMode(VIN_SEL, OUTPUT);
  digitalWrite(VIN_SEL, HIGH);

  // do communication initializations before peripherals
  HAL::init();

  // // set up peripherals
  // MAX31855::setupMAX();
  MCP4725::init();
  Potentiometer::init();
  INA::setupINA();

  LCD_UI::init();
  LCD_UI::writeMessage("Waiting for", "first read");

  lastPotVoltage = Potentiometer::readVoltage();
}

void loop() {
  unsigned long now = millis();

  // put your main code here, to run repeatedly:

  // read all sensors
  // MAX31855::readMAX();

  // get values from all sensors
  // float maxF = MAX31855::tempF;

  /*
  int sensorValue = analogRead(VMEASURE_ADCIN); // Read the analog pin
  Serial.println("VMEASURE reading: " + String(sensorValue, 4));
  float voltage = (sensorValue * 3.3) / 4095.0;

  float temp_RAW = voltage * 10;
  Serial.println("VMEASURE reading: " + String(voltage, 4) + "V, TEMP: " + String(temp_RAW) + " deg C"); // Print value to the Serial Monitor
  */

  // print stuff
  // Serial.println("MAX31855 reading: " + String(maxF) + " deg Fahrenheit");
  // Serial.println("do other stuff now...");

  //Testing for INA233 readings
  INA::readINA();

  //State machine for updating set temperature and LCD display
  float potV = Potentiometer::readVoltage();

  if (fabs(potV - lastPotVoltage) >= POT_CHANGE_THRESHOLD_V) {
    displayMode = SET_TEMP_MODE;
    lastPotChangeTime = now;
    lastPotVoltage = potV;
  }
  
  if (displayMode == SET_TEMP_MODE) {
    float setTempC = Potentiometer::voltageToTemperatureC(potV); // calc desired temperature

    float vref = tempCToReferenceVoltage(setTempC); // convert to reference voltage

    MCP4725::setVoltage(vref); // continuous update of reference voltage

    if (now - lastDisplayUpdateTime >= DISPLAY_UPDATE_MS) { 
      LCD_UI::displaySetTemp(setTempC);
      lastDisplayUpdateTime = now;
    }

    if (now - lastPotChangeTime >= SET_MODE_TIMEOUT_MS) { // return to default display after no pot movement for 3 sec
      displayMode = READ_TEMP_MODE;
      LCD_UI::displayReadTemp(PLACEHOLDER_READ_TEMP_C);
    }
  } 
  
  else { // default display mode
    if (now - lastDisplayUpdateTime >= DISPLAY_UPDATE_MS) {
      LCD_UI::displayReadTemp(PLACEHOLDER_READ_TEMP_C);
      lastDisplayUpdateTime = now;
    }
  }
  
  //Test DAC
  //MCP4725::setVoltage(1.65f);

  //Testing for potentiometer readings
  /*float potV = Potentiometer::readVoltage();
  Serial.print("POT_VREF = ");
  Serial.print(potV, 3);
  Serial.println(" V");*/
}
