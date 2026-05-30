
#include <Arduino.h>
#include <SPI.h>
#include "Globals.h"
#include "HAL.h"
#include "MAX31855.h"
#include "DualSlope.h"
#include "test.h"

const int VMEASURE_ADCIN = 9; // Define the GPIO pin
const int VIN_SEL = 42;

float adcval = 0;
float conversiontime = 0;

// having both of these is redundant lowkey
volatile uint32_t ready = false;
volatile bool armed = false;

uint32_t tempSelect = LOW;    // LOW = TC, HIGH = LM35


void adcTask(void *pvParameters) {
    while (true) {
      // Serial.println("SSS - called adcTask function");
      if (!armed) {
        DualSlope::resetDualSlope();
        armed = true;
      } else if (DualSlope::state == DualSlope::DONE) {
        adcval = DualSlope::Vin;
        conversiontime = DualSlope::processtime;
        ready = true;
        armed = false;
      }
      DualSlope::computeADC();

      // delay(100);

      vTaskDelay(1);   // yield
    }
}

void setup() {

  delay(1000);
  Serial.begin(115200);

  pinMode(VIN_SEL, OUTPUT);
  digitalWrite(VIN_SEL, tempSelect);  // LOW for TC, HIGH for LM35

  // do SPI initializations before peripherals
  HAL::initVSPI_HAL();
  HAL::initCSPins();

  // // set up peripherals
  MAX31855::setupMAX();

  DualSlope::setupDualSlope();

  // initialize all other peripherals here:

  Serial.println("before xTask");

  xTaskCreatePinnedToCore(
        adcTask,        // funct
        "ADC Task",     // name
        32000,          // stack size
        NULL,           // parameters
        0,              // priority (higher = more important)
        NULL,           // task handle (optional)
        0               // core 0
    );

  delay(1000);
}
//2.956
//


// core 1 by default
void loop() {
  // put your main code here, to run repeatedly:

  // // read all sensors
  // MAX31855::readMAX();

  // // // change to LM35 eventually for TC
  // float tAmbient = MAX31855::tempInternal;

  // int sensorValue = analogRead(VMEASURE_ADCIN); // Read the analog pin
  // // Serial.println("VMEASURE reading: " + String(sensorValue));
  // float voltage = (sensorValue * 3.3) / 4095.0;


  // if (tempSelect) {     // HIGH = LM35
  //    // for LM35 only
  //   float temp_RAW = (voltage - 2.1656)/3.1826;
  //   temp_RAW = temp_RAW / 0.01;

  //   Serial.println("COUNTS: " + String(sensorValue) +  ", VMEASURE reading: " + String(voltage, 4) + "V, TEMP: " + String(temp_RAW) + " deg C");
  // } else {              // LOW = TC
  //   float voltage_RAW = (voltage - 2.5)/180.0;
  //   voltage_RAW = voltage_RAW * 1000.0;   // in mV

  //   float conv = 41.0 / 1000.0;   // 0.041 mV / degC
  //   float temp_RAW = voltage_RAW / conv;
  //   float tempTC = temp_RAW + tAmbient;
  //   float temp_F = (tempTC * 9.0 / 5.0) + 32;

  //   Serial.println("\nCOUNTS: " + String(sensorValue) +  ", VMEASURE reading: " + String(voltage, 4) + "V, ∆ TC voltage: " + String(voltage_RAW, 4) + " mV");
  //   Serial.println("post conversion: " + String(temp_RAW) +  " deg C, ambient: " + String(tAmbient) + " deg C");
  //   Serial.println("TEMP: " + String(tempTC) +  " deg C, " + String(temp_F) + " deg F\n\n");
  // }

  

  // if (ready) {
  //   ready = false;
  //   Serial.println("\n------------------------------------");
  //   Serial.println("ADC CONVERSION: " + String(adcval));
  //   Serial.println("conversion time: " + String(conversiontime));
  //   Serial.println("------------------------------------\n");
  // }

  // float maxF = MAX31855::tempF;
  // Serial.println("MAX31855 reading: " + String(maxF) + " deg Fahrenheit");
  // Serial.println("do other stuff now...");



  // Test::S2_high();
  // delay(5000);
  // Test::S2_low();
  delay(100);


}
