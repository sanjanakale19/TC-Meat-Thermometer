#include <Arduino.h>


const int S0 = 42;      // SIGNAL SELECT 
const int S1 = 40;      // MUX_A0
const int S2 = 41;      // MUX_A1
const int S3 = 18;      // ADC_RESET

static constexpr uint32_t S0_MASK = (1UL << (S0 - 32));
static constexpr uint32_t S1_MASK = (1UL << (S1 - 32));
static constexpr uint32_t S2_MASK = (1UL << (S2 - 32));
static constexpr uint32_t S3_MASK = (1UL << S3);


namespace Test {

    void testsetup() {
        pinMode(S0, OUTPUT);
        pinMode(S1, OUTPUT);
        pinMode(S2, OUTPUT);
        pinMode(S3, OUTPUT);

        digitalWrite(S0, HIGH);
        digitalWrite(S1, LOW);      // set low initially
        digitalWrite(S2, LOW);      // set low initially
        digitalWrite(S3, LOW);      // set low initially –> logic 0 = ADC reset closed
    }

    void S2_low() {
        // digitalWrite(S2, LOW);
        GPIO.out1_w1tc.val = S0_MASK;
    }

    void S2_high() {
        // digitalWrite(S2, HIGH);
        GPIO.out1_w1ts.val = S0_MASK;
    }

    void S3_low() {
        GPIO.out_w1tc = S3_MASK;
    }

    void S3_high() {
        GPIO.out_w1ts = S3_MASK;
    }

}