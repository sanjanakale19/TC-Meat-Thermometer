#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <Arduino.h>

namespace Potentiometer {
    void init();
    int readRaw();
    float readVoltage();
    float voltageToTemperatureC(float voltage);
}

#endif