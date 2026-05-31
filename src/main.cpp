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

const int VMEASURE_ADCIN = 9; // Define the GPIO pin
const int VIN_SEL = 42;

float adcval = 0;
float conversiontime = 0;
volatile float ambientTempC = 0.0f;

static const int N_AVERAGE = 8;
static uint64_t count_accumulator = 0;
static int sample_count = 0;

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
      } else {
        bool localDone = false;

        taskENTER_CRITICAL(&DualSlope::timerMux);
        localDone = (DualSlope::state == DualSlope::DONE);
        taskEXIT_CRITICAL(&DualSlope::timerMux);

        if (localDone) {
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

  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_DISABLE;
  io_conf.pin_bit_mask = (1ULL << VMEASURE_ADCIN);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

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
  INA::setupINA();

  // LCD_UI::init();
  // LCD_UI::writeMessage("Waiting for", "first read");

  // Serial.println("before xTask");

  xTaskCreatePinnedToCore(
        adcTask,        // funct
        "ADC Task",     // name
        32000,          // stack size
        NULL,           // parameters
      1,              // priority (higher = more important)
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
    uint64_t localAdcCounts = 0;
    bool localWasLM35 = false;

    taskENTER_CRITICAL(&DualSlope::timerMux);
    localVin = adcval;
    localAdcCounts = DualSlope::adc_counts;
    localWasLM35 = DualSlope::lastWasLM35;
    taskEXIT_CRITICAL(&DualSlope::timerMux);

    ready = false;

    if (localWasLM35) {
      const float rawAmbientTempC = DualSlope::PrecisionMath::lm35AdcVoltageToCelsius(localVin);
      ambientTempC = constrain(rawAmbientTempC, 0.0f, 60.0f);
      const float sensedAmbientVoltageV = (localVin - DualSlope::PrecisionMath::LM35_OFFSET_V) / DualSlope::PrecisionMath::LM35_GAIN;

      Serial.println("\n------------------------------------");
      Serial.printf("LM35 frame | Amplified Voltage: %.6f V | LM35 Voltage: %.6f V | Ambient: %.2f C | adc_counts: %llu\n",
                    localVin,
                    sensedAmbientVoltageV,
                    ambientTempC,
                    (unsigned long long)localAdcCounts);
      Serial.println("------------------------------------\n");
    } else {
      count_accumulator += localAdcCounts;
      sample_count++;

      if (sample_count < N_AVERAGE) {
        return;
      }

      const uint64_t averagedCounts = count_accumulator / (uint64_t)N_AVERAGE;
      count_accumulator = 0;
      sample_count = 0;

      float localAmbient = 0.0f;
      taskENTER_CRITICAL(&DualSlope::timerMux);
      localAmbient = ambientTempC;
      taskEXIT_CRITICAL(&DualSlope::timerMux);

      const float cjcMv = DualSlope::PrecisionMath::ambientTempToMillivolts(localAmbient);
      const float averagedVin = 2.048f + (2.048f * ((float)averagedCounts / 50000.0f));
      const float tcMv = DualSlope::PrecisionMath::tcAdcVoltageToMillivolts(averagedVin);
      const float totalMv = tcMv + cjcMv;
      const float compensatedTempC = DualSlope::PrecisionMath::millivoltsToPreciseTemp(totalMv);
      const float compensatedTempF = (compensatedTempC * 9.0f / 5.0f) + 32.0f;
      const float expectedBaselineCounts = ((2.5f - 2.048f) / 2.048f) * 50000.0f;
      const float countsDelta = (float)averagedCounts - expectedBaselineCounts;

      Serial.println("\n------------------------------------");
      Serial.printf("TC average | Averaged counts: %llu | expected baseline: %.0f | delta: %.0f\n",
            (unsigned long long)averagedCounts,
            expectedBaselineCounts,
            countsDelta);
      Serial.printf("TC frame  | Averaged Voltage: %.6f V | Net Seebeck: %.4f mV | CJC: %.4f mV | Final: %.2f C / %.2f F\n",
                    averagedVin,
                    tcMv,
                    cjcMv,
                    compensatedTempC,
                    compensatedTempF);
      MAX31855::readMAX();
      Serial.printf("MAX31855  | TC: %.2f C / %.2f F | Internal: %.2f C\n",
                    MAX31855::tempC,
                    MAX31855::tempF,
                    MAX31855::tempInternal);
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
