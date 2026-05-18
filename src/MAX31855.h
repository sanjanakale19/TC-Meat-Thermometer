/*!
 * @file MAX31855.h
 *
 */

#ifndef MAX31855_H
#define MAX31855_H

#include <Arduino.h>
#include <Adafruit_MAX31855.h>
#include "HAL.h"

namespace MAX31855 {
    
    Adafruit_MAX31855 max31855_tcdigital(HAL::VSCK_PIN, HAL::MAX31855_CS, HAL::VMISO_PIN);

    float tempC, tempF, tempInternal;
    uint8_t maxerr;


    void setupMAX() {
        for (int i=0; i<300; i++) {
            if(max31855_tcdigital.begin())
                Serial.println("Set up MAX successfully");
                break;    
            DEBUGLN("Failed to set up MAX31855");
            Serial.println("Failed to set up MAX31855");
        }
        DEBUGLN("Timed out when setting up MAX31855");
    }


    
    int readMAX() {
        // update readings
        Serial.println("reading MAX...");
        tempC = max31855_tcdigital.readCelsius();
        tempF = max31855_tcdigital.readFahrenheit();
        tempInternal = max31855_tcdigital.readInternal();
        maxerr = max31855_tcdigital.readError();

        Serial.println("MAX31855 tempC: " + String(tempC));
        Serial.println("MAX31855 tempF: " + String(tempF));
        Serial.println("MAX31855 tempInternal: " + String(tempInternal));
        Serial.println("MAX31855 maxerr: " + String(maxerr));
  
        return 1;
    }

    

}


#endif