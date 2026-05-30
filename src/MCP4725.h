/*!
 * @file MCP4725.h
 *
 * Driver wrapper for MCP4725 I2C DAC.
 */

#ifndef MCP4725_H
#define MCP4725_H

#include <Arduino.h>

namespace MCP4725 {
    bool init();
    bool setVoltage(float voltage);
    bool setRawCode(uint16_t code);
}

#endif // MCP4725_H