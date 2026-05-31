#include <Arduino.h>
#include <SPI.h>
#include <math.h>
#include "driver/gpio.h"
#include "soc/rtc_io_reg.h"
#include "Globals.h"
#include "HAL.h"
#include "MAX31855.h"
#include "DualSlope.h"
#include "test.h"
#include "MCP4725.h"
#include "potentiometer.h"
#include "INA.h"
#include "lcd_ui.h"

volatile float adcval = 0;
volatile float conversiontime = 0;
volatile float ambientTempC = 0.0f;

static const int N_AVERAGE = 8;
static uint64_t count_accumulator = 0;
static int sample_count = 0;

// having both of these is redundant lowkey
volatile uint32_t ready = false;
volatile bool armed = false;

void adcTask(void *pvParameters) {
    while (true) {
      taskENTER_CRITICAL(&DualSlope::timerMux);
      bool localArmed = armed;
      bool dataPending = ready;
      taskEXIT_CRITICAL(&DualSlope::timerMux);

      // NEW: If we are not armed AND data is still pending, Core 1 is behind.
      // Do NOT spin at Priority 23. Yield the core for 10ms to let the RTOS breathe.
      if (!localArmed && dataPending) {
          vTaskDelay(pdMS_TO_TICKS(10));
          continue; 
      }

      if (!localArmed && !dataPending) {
        taskENTER_CRITICAL(&DualSlope::timerMux);
        DualSlope::resetDualSlope();
        armed = true;
        taskEXIT_CRITICAL(&DualSlope::timerMux);
      } else {
        bool localDone = false;

        taskENTER_CRITICAL(&DualSlope::timerMux);
        localDone = (DualSlope::state == DualSlope::DONE);
        taskEXIT_CRITICAL(&DualSlope::timerMux);

        if (localDone) {
          taskENTER_CRITICAL(&DualSlope::timerMux);
          adcval = DualSlope::Vin;
          conversiontime = DualSlope::processtime;
          ready = true;
          armed = false; 
          taskEXIT_CRITICAL(&DualSlope::timerMux);

          vTaskDelay(pdMS_TO_TICKS(500));
        }
      }

      taskENTER_CRITICAL(&DualSlope::timerMux);
      bool shouldCompute = armed;
      taskEXIT_CRITICAL(&DualSlope::timerMux);

      if (shouldCompute) {
        DualSlope::computeADC();
      }

      vTaskDelay(0);   
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
  INA::setupINA();

  // LCD_UI::init();
  // LCD_UI::writeMessage("Waiting for", "first read");

  // Serial.println("before xTask");

  xTaskCreatePinnedToCore(
        adcTask,        // funct
        "ADC Task",     // name
        32000,          // stack size
        NULL,           // parameters
      configMAX_PRIORITIES - 1, // highest application priority for tight polling
        NULL,           // task handle (optional)
        0               // core 0
    );
  // lastPotVoltage = Potentiometer::readVoltage();
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

  
  /*
  int sensorValue = analogRead(VMEASURE_ADCIN); // Read the analog pin
  Serial.println("VMEASURE reading: " + String(sensorValue, 4));
  float voltage = (sensorValue * 3.3) / 4095.0;

  float temp_RAW = voltage * 10;
  Serial.println("VMEASURE reading: " + String(voltage, 4) + "V, TEMP: " + String(temp_RAW) + " deg C"); // Print value to the Serial Monitor
  */
if (ready) {
    float localVin = 0.0f;
    uint64_t localAdcCounts = 0;
    bool localWasLM35 = false;
    float localAmbient = 0.0f;

    // Capture all volatile parameters under lock immediately
    taskENTER_CRITICAL(&DualSlope::timerMux);
    localVin = adcval;
    localAdcCounts = DualSlope::adc_counts;
    localWasLM35 = DualSlope::lastWasLM35;
    localAmbient = ambientTempC;
    ready = false; // Clear flag to unlock Core 0
    taskEXIT_CRITICAL(&DualSlope::timerMux);

    if (localWasLM35) {
      const float rawAmbientTempC = DualSlope::PrecisionMath::lm35AdcVoltageToCelsius(localVin);
      ambientTempC = constrain(rawAmbientTempC, 0.0f, 60.0f);
      const float sensedAmbientVoltageV = (localVin - DualSlope::PrecisionMath::LM35_OFFSET_V) / DualSlope::PrecisionMath::LM35_GAIN;

      Serial.println("\n------------------------------------");
      Serial.printf("DS-ADC     | Amplified Voltage: %.6f V | LM35 Voltage: %.6f V | Ambient: %.2f C | adc_counts: %llu\n",
                    localVin,
                    sensedAmbientVoltageV,
                    ambientTempC,
                    (unsigned long long)localAdcCounts);
      Serial.println("------------------------------------\n");
    } else {
      const uint64_t counts = localAdcCounts;
      const float cjcMv = DualSlope::PrecisionMath::ambientTempToMillivolts(localAmbient);
      
      // Calculate vin directly using the static captured count value
      const float vin = 2.048f + (2.048f * ((float)counts / 50000.0f));
      const float tcMv = DualSlope::PrecisionMath::tcAdcVoltageToMillivolts(vin);
      const float totalMv = tcMv + cjcMv;
      const float compensatedTempC = DualSlope::PrecisionMath::millivoltsToPreciseTemp(totalMv);
      const float compensatedTempF = (compensatedTempC * 9.0f / 5.0f) + 32.0f;
      const float expectedBaselineCounts = ((2.5f - 2.048f) / 2.048f) * 50000.0f;
      const float countsDelta = (float)counts - expectedBaselineCounts;

      Serial.println("\n------------------------------------");
      Serial.printf("DS-ADC     | Counts: %llu | expected baseline: %.0f | delta: %.0f\n",
                    (unsigned long long)counts,
                    expectedBaselineCounts,
                    countsDelta);
      Serial.printf("DS-ADC     | Voltage: %.6f V | Net Seebeck: %.4f mV | CJC: %.4f mV | Final: %.2f C / %.2f F\n",
                    vin,
                    tcMv,
                    cjcMv,
                    compensatedTempC,
                    compensatedTempF);
      Serial.println("------------------------------------\n");
    }
  }

  // float maxF = MAX31855::tempF;
  // Serial.println("MAX31855 reading: " + String(maxF) + " deg Fahrenheit");
  // Serial.println("do other stuff now...");



  // Test::S2_high();
  // delay(5000);
  // Test::S2_low();
  delay(100);


  //Testing for INA233 readings
  INA::readINA();

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
