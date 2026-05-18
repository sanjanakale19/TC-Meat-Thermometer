/*!
 * @file MCP4725.cpp
 *
 * Driver wrapper for MCP4725 I2C DAC.
 */

#include "MCP4725.h"
#include "pins.h"

#include <Wire.h>
#include <Adafruit_MCP4725.h>

namespace MCP4725 {

    static Adafruit_MCP4725 dac;
    static bool initialized = false;

    bool init() {
        initialized = dac.begin(MCP4725_ADDR);

        if (!initialized) {
            Serial.println("ERROR: MCP4725 not found on I2C bus.");
            return false;
        }

        Serial.println("MCP4725 initialized.");
        return true;
    }

    bool setVoltage(float voltage) {
        if (!initialized) {
            Serial.println("ERROR: MCP4725 not initialized.");
            return false;
        }

        voltage = constrain(voltage, 0.0f, 3.3f);

        uint16_t code = (uint16_t)((voltage / 3.3f) * 4095.0f);

        dac.setVoltage(code, false);

        return true;
    }

    bool setRawCode(uint16_t code) {
        if (!initialized) {
            Serial.println("ERROR: MCP4725 not initialized.");
            return false;
        }

        if (code > 4095) {
            code = 4095;
        }

        dac.setVoltage(code, false);

        return true;
    }

}