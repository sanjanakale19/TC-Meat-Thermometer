#include "HAL.h"

namespace HAL {

    SPIClass* VSPI_bus = nullptr;

    void init() {
        initCSPins();
        initI2C();
        initVSPI_HAL();

        Serial.println("HAL initializations done");
    }

    void initCSPins() {
        DEBUGLN("before initalizing");
        pinMode(MAX31855_CS, OUTPUT);
        digitalWrite(MAX31855_CS, HIGH);

        DEBUGLN("initialized CS pins");
    }

    void initVSPI_HAL() {
        VSPI_bus = new SPIClass(HSPI);
        VSPI_bus->begin(VSCK_PIN, VMISO_PIN, VMOSI_PIN);
        DEBUGLN("initialized VSPI pins");

    } 

    void initI2C() {
        Wire.begin(SDA_PIN, SCL_PIN);
        Wire.setClock(100000);
        DEBUGLN("initialized I2C pins");
    }

}