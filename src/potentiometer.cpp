#include "potentiometer.h"
#include "pins.h"

namespace Potentiometer {

    void init() {
        pinMode(PIN_POT_VREF, INPUT);
        analogReadResolution(12);
        analogSetPinAttenuation(PIN_POT_VREF, ADC_11db); // allows close to 3.3 V
    }

    int readRaw() {
        return analogRead(PIN_POT_VREF);
    }

    float readVoltage() {
        int raw = readRaw();
        return (raw / 4095.0f) * 3.3f;
    }

    float voltageToTemperatureC(float voltage) {
        voltage = constrain(voltage, 0.0f, 3.3f);
        return (voltage / 3.3f) * 200.0f;
    }

}