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
    
    Adafruit_MAX31855 max31855_tcdigital(HAL::MAX31855_CS);

    float tempC, tempF, tempInternal, mV_deltaTC;
    uint8_t maxerr;

    void setupMAX() {
        for (int i=0; i<300; i++) {
            if (max31855_tcdigital.begin()) {
                DEBUGLN("Set up MAX successfully");
                break;
            }
            DEBUGLN("Failed to set up MAX31855");
        }
        DEBUGLN("Timed out when setting up MAX31855");
    }


    
    int readMAX() {
        // update readings
        tempC = max31855_tcdigital.readCelsius();
        tempF = max31855_tcdigital.readFahrenheit();
        tempInternal = max31855_tcdigital.readInternal();
        maxerr = max31855_tcdigital.readError();

        mV_deltaTC = (tempC - tempInternal) / 41.0;
  
        return 1;
    }

    

}


#endif