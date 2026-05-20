/*!
 * @file HAL.h
 *
 * Hardware Abstraction Layer (HAL) to define SPI/I2C pinouts and initialize SPI/I2C bus.
 *
 * Written for ESP32.
 *
 * Written by 
 *
 */

#ifndef HAL_H
#define HAL_H

#include <Arduino.h>
#include "Globals.h"
#include "pins.h"

#include <HardwareSerial.h>
#include <SPI.h>

#include <Wire.h>

namespace HAL { 

    // VSPI
    const uint8_t VSCK_PIN = 16;
    const uint8_t VMISO_PIN = 17;
    const uint8_t VMOSI_PIN = 8;
    extern SPIClass* VSPI_bus;

    // // HSPI : TODO - change for pinout
    // const uint8_t HSCK_PIN = 14;
    // const uint8_t HMISO_PIN = 34;
    // const uint8_t HMOSI_PIN = 13;
    // SPIClass* HSPI_bus;

    // I2C
    const uint8_t INA_ADDR = 0x40;
    const uint8_t SDA_PIN = PIN_I2C_SDA;
    const uint8_t SCL_PIN = PIN_I2C_SCL;
    const uint8_t INA_SDA_PIN = PIN_I2C_SDA;
    const uint8_t INA_SCL_PIN = PIN_I2C_SCL;

    // Peripherals : TODO - add all peripherals here
    const uint8_t MAX31855_CS = PIN_CS_MAX31855;
    // const uint8_t CS2 = 40;

    void init();
    void initCSPins();
    void initVSPI_HAL();
    void initI2C();
}

#endif