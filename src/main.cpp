#include <Arduino.h>
#include <SPI.h>
#include <math.h>
#include "Globals.h"
#include "HAL.h"
#include "MAX31855.h"
#include "DualSlope.h"
#include "test.h"
#include "MCP4725.h"
#include "potentiometer.h"
#include "INA.h"
#include "lcd_ui.h"

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
        // copy shared variables under the DualSlope timerMux to avoid races
        taskENTER_CRITICAL(&DualSlope::timerMux);
        adcval = DualSlope::Vin;
        conversiontime = DualSlope::processtime;
        taskEXIT_CRITICAL(&DualSlope::timerMux);

        ready = true;

        // pace sampling: wait 500 ms before allowing next conversion
        vTaskDelay(pdMS_TO_TICKS(500));
        armed = false;
      }
      DualSlope::computeADC();

      // delay(100);

      vTaskDelay(0);   // yield without adding 1 ms delay
    }
}
// AUDIO
// const float POT_CHANGE_THRESHOLD_V = 0.05f;
// const unsigned long SET_MODE_TIMEOUT_MS = 3000;
// const unsigned long DISPLAY_UPDATE_MS = 500; // LCD only updates every 0.5 seconds

// const float PLACEHOLDER_READ_TEMP_C = 50.0f;

// enum DisplayMode {
//   READ_TEMP_MODE,
//   SET_TEMP_MODE
// };

// DisplayMode displayMode = READ_TEMP_MODE;

// float lastPotVoltage = 0.0f;
// unsigned long lastPotChangeTime = 0;
// unsigned long lastDisplayUpdateTime = 0;

// float tempCToReferenceVoltage(float tempC) {
//   // TODO: replace with real inverse calibration equation.
//   // Placeholder: 0 C -> 0.0 V, 200 C -> 3.3 V
//   return constrain((tempC / 200.0f) * 3.3f, 0.0f, 3.3f);
// }

void setup() {

  delay(1000);
  Serial.begin(115200);
  delay(1000);

  pinMode(VIN_SEL, OUTPUT);
  digitalWrite(VIN_SEL, tempSelect);  // LOW for TC, HIGH for LM35

  // do SPI initializations before peripherals
  HAL::initVSPI_HAL();
  HAL::initCSPins();

  // // set up peripherals
  MAX31855::setupMAX();

  DualSlope::setupDualSlope();
  // do communication initializations before peripherals
  HAL::init();

  // // set up peripherals
  // MAX31855::setupMAX();
  // MCP4725::init();
  // Potentiometer::init();
  // INA::setupINA();

  // LCD_UI::init();
  // LCD_UI::writeMessage("Waiting for", "first read");

  // Serial.println("before xTask");

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
  // lastPotVoltage = Potentiometer::readVoltage();
}
//2.956
//


// core 1 by default
void loop() {
  unsigned long now = millis();

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

  
  /*
  int sensorValue = analogRead(VMEASURE_ADCIN); // Read the analog pin
  Serial.println("VMEASURE reading: " + String(sensorValue, 4));
  float voltage = (sensorValue * 3.3) / 4095.0;

  float temp_RAW = voltage * 10;
  Serial.println("VMEASURE reading: " + String(voltage, 4) + "V, TEMP: " + String(temp_RAW) + " deg C"); // Print value to the Serial Monitor
  */

  if (ready) {
    // Atomically copy results from ADC task / DualSlope
    float localVin = 0.0f;
    float localProcesstime = 0.0f;
    uint64_t local_adc_counts = 0;

    taskENTER_CRITICAL(&DualSlope::timerMux);
    localVin = adcval;
    localProcesstime = conversiontime;
    local_adc_counts = DualSlope::adc_counts;
    taskEXIT_CRITICAL(&DualSlope::timerMux);

    // Back-calculate Vraw and temperature using calibration constants
    const float vref = 2.048f;
    float vinput = localVin;
    float Vraw = (vinput - (vref * 1.0574f)) / 3.1826f;
    float tempC = Vraw / 0.01f; // 10 mV/degC

    ready = false;
    Serial.println("\n------------------------------------");
    Serial.println("Vinput: " + String(vinput, 6) + " V");
    Serial.println("------------------------------------\n");
  }

  // float maxF = MAX31855::tempF;
  // Serial.println("MAX31855 reading: " + String(maxF) + " deg Fahrenheit");
  // Serial.println("do other stuff now...");



  // Test::S2_high();
  // delay(5000);
  // Test::S2_low();
  delay(100);


  //Testing for INA233 readings
  // INA::readINA();

  // //State machine for updating set temperature and LCD display
  // float potV = Potentiometer::readVoltage();

  // if (fabs(potV - lastPotVoltage) >= POT_CHANGE_THRESHOLD_V) {
  //   displayMode = SET_TEMP_MODE;
  //   lastPotChangeTime = now;
  //   lastPotVoltage = potV;
  // }
  
  // if (displayMode == SET_TEMP_MODE) {
  //   float setTempC = Potentiometer::voltageToTemperatureC(potV); // calc desired temperature

  //   float vref = tempCToReferenceVoltage(setTempC); // convert to reference voltage

  //   MCP4725::setVoltage(vref); // continuous update of reference voltage

  //   if (now - lastDisplayUpdateTime >= DISPLAY_UPDATE_MS) { 
  //     LCD_UI::displaySetTemp(setTempC);
  //     lastDisplayUpdateTime = now;
  //   }

  //   if (now - lastPotChangeTime >= SET_MODE_TIMEOUT_MS) { // return to default display after no pot movement for 3 sec
  //     displayMode = READ_TEMP_MODE;
  //     LCD_UI::displayReadTemp(PLACEHOLDER_READ_TEMP_C);
  //   }
  // } 
  
  // else { // default display mode
  //   if (now - lastDisplayUpdateTime >= DISPLAY_UPDATE_MS) {
  //     LCD_UI::displayReadTemp(PLACEHOLDER_READ_TEMP_C);
  //     lastDisplayUpdateTime = now;
  //   }
  // }
  
  //Test DAC
  //MCP4725::setVoltage(1.65f);

  //Testing for potentiometer readings
  /*float potV = Potentiometer::readVoltage();
  Serial.print("POT_VREF = ");
  Serial.print(potV, 3);
  Serial.println(" V");*/
}
