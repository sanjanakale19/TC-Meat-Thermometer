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

#include <HardwareSerial.h>
#include <SPI.h>

#include <Wire.h>

namespace HAL { 

    // VSPI
    const uint8_t VSCK_PIN = 16;
    const uint8_t VMISO_PIN = 17;
    const uint8_t VMOSI_PIN = 8;
    SPIClass* VSPI_bus;

    // // HSPI : TODO - change for pinout
    // const uint8_t HSCK_PIN = 14;
    // const uint8_t HMISO_PIN = 34;
    // const uint8_t HMOSI_PIN = 13;
    // SPIClass* HSPI_bus;

    // I2C : TODO - change for pinout of INA
    // const uint8_t INA_ADDR = 0x40;
    // const uint8_t INA_SDA_PIN = 5;
    // const uint8_t INA_SCL_PIN = 19;

    // Peripherals : TODO - add all peripherals here
    const uint8_t MAX31855_CS = 4;
    // const uint8_t CS2 = 40;

    void initCSPins() {
        Serial.println("before initalizing");
        pinMode(MAX31855_CS, OUTPUT);
        digitalWrite(MAX31855_CS, HIGH);

        Serial.println("initialized CS pins");
    }


    // void initHSPI_HAL() {
    //     HSPI_bus = new SPIClass(HSPI);
    //     HSPI_bus->begin(HSCK_PIN, HMISO_PIN, HMOSI_PIN);
    // }   

    void initVSPI_HAL() {
        VSPI_bus = new SPIClass(HSPI);
        VSPI_bus->begin(VSCK_PIN, VMISO_PIN, VMOSI_PIN);
        Serial.println("initialized VSPI pins");

    } 
}

#endif