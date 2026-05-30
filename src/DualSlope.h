/*!
 * @file DualSlope.h
 *
 * Written for ESP32.
 *
 * Written by 
 *
 */

#ifndef DualSlope_H
#define DualSlope_H

#include <Arduino.h>
#include "Globals.h"

#include <HardwareSerial.h>
#include <SPI.h>
#include <Wire.h>

namespace DualSlope {

    const int S0 = 42;      // SIGNAL SELECT 
    const int S1 = 40;      // MUX_A0
    const int S2 = 41;      // MUX_A1
    const int S3 = 18;      // ADC_RESET

    static constexpr uint32_t S0_MASK = (1UL << (S0 - 32));
    static constexpr uint32_t S1_MASK = (1UL << (S1 - 32));
    static constexpr uint32_t S2_MASK = (1UL << (S2 - 32));
    static constexpr uint32_t S3_MASK = (1UL << S3);

    const int COMPARATOR_OUT = 13;  // NAND_out

    // all in ms
    const float TRESET = 50;
    const float TDELAY = 20;
    const float TREF = 50;
    const float TMEASURE = 0;
    const uint32_t TICK_US = 10;  // 10 µs

    // counted values are uint
    const uint32_t TRESET_COUNTS = TRESET * 1000 / TICK_US;
    const uint32_t TDELAY_COUNTS = TDELAY * 1000 / TICK_US;
    const uint32_t TREF_COUNTS = TREF * 1000 / TICK_US;
    const uint32_t MAX_TIMEOUT_COUNTS = TREF_COUNTS * 2;

    volatile uint32_t tick = 0;
    volatile uint32_t tick_count = 0;
    volatile uint32_t adc_counts = 0;

    // timer
    hw_timer_t *adcTimer = nullptr;
    portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

    enum State {
        RESET,
        WAIT_RESET,
        WAIT_DELAY,
        INTEGRATE,
        DEINTEGRATE,
        COMPUTE,
        DONE,
        HOLD
    };

    // intitialize state
    volatile State state = HOLD;

    // computed values are floats
    volatile float rat = 0;
    volatile float Vin = 0;
    volatile float processtime = 0;

    void IRAM_ATTR onTimer() {
        // Serial.println("SSS - inside the ONTIMER, tick: " + String(tick) + " tick_counts: " + String(tick_count));
        portENTER_CRITICAL_ISR(&timerMux);
        tick++;
        tick_count++;
        portEXIT_CRITICAL_ISR(&timerMux);
    }

    void startTimer() {
        adcTimer = timerBegin(0, 80, true);  
        // 80 MHz / 80 = 1 MHz → 1 tick = 1 µs

        timerAttachInterrupt(adcTimer, &onTimer, true);

        timerAlarmWrite(adcTimer, TICK_US, true);  
        timerAlarmEnable(adcTimer);
    }

    void stopTimer() {
        if (adcTimer) {
            timerAlarmDisable(adcTimer);
        }
    }

    // write LOW to S3 for TRESET
    // wait TDELAY seconds
    // write HIGH to S1, S2, S3, wait for TREF seconds
    // write LOW to S2 after TREF elapses, wait until COMPARATOR_OUT = HIGH (TMEASURE time)
    // write LOW to S1, S3 after TMEASURE elapses
    // write LOW to S3 for TRESET
    void computeADC() {
        switch(state) {
            case RESET:
                // Serial.println("\t1. DUALSLOPE SSS - CASE RESET");
                
                portENTER_CRITICAL(&timerMux);
                tick = 0;
                tick_count = 0;
                portEXIT_CRITICAL(&timerMux);

                // close RESET switch (N/C)
                // digitalWrite(S3, LOW);
                GPIO.out_w1tc = S3_MASK;
                state = WAIT_RESET;
                break;

            case WAIT_RESET:
                // Serial.println("\t2. DUALSLOPE SSS - CASE WAIT RESET");
                // Serial.flush();
                if (tick >= TRESET_COUNTS) {   // wait TRESET seconds
                    // open RESET switch
                    // digitalWrite(S3, HIGH);
                    GPIO.out_w1ts = S3_MASK;

                    portENTER_CRITICAL(&timerMux);
                    tick = 0;
                    portEXIT_CRITICAL(&timerMux);

                    state = WAIT_DELAY;
                }
                break;
            
            case WAIT_DELAY:
                // Serial.println("\t3. DUALSLOPE SSS - CASE WAIT DELAY");
                if (tick >= TDELAY_COUNTS) {   // wait TDELAY seconds
                    // start integration
                    // digitalWrite(S1, HIGH); digitalWrite(S2, HIGH);
                    GPIO.out1_w1ts.val = S1_MASK | S2_MASK;

                    portENTER_CRITICAL(&timerMux);
                    tick = 0;
                    portEXIT_CRITICAL(&timerMux);

                    state = INTEGRATE;
                }
                break;

            case INTEGRATE:
                // Serial.println("\t4. DUALSLOPE SSS - CASE INTEGRATE");
                if (tick >= TREF_COUNTS) {     // integrate for TREF seconds
                    // start deintegration

                    // digitalWrite(S2, LOW);
                    GPIO.out1_w1tc.val = S2_MASK;

                    portENTER_CRITICAL(&timerMux);
                    tick = 0;
                    portEXIT_CRITICAL(&timerMux);

                    state = DEINTEGRATE;
                }
                break;

            case DEINTEGRATE: {
                int comparator_val = GPIO.in & (1 << COMPARATOR_OUT);       // Read comparator
                Serial.println("\t5. DUALSLOPE SSS - comparator value " + String(comparator_val));
                Serial.flush();

                if (comparator_val || tick > MAX_TIMEOUT_COUNTS) {          // stop deintegrating when comparator trips

                    Serial.println("5. DUALSLOPE SSS - comparator value " + String(comparator_val));
                    Serial.println("tick count is: " + String(tick));
                    // get measurement
                    portENTER_CRITICAL(&timerMux);
                    adc_counts = tick;
                    rat = (float) adc_counts / TREF_COUNTS;
                    Vin = 2.048 * rat;
                    processtime = tick_count * TICK_US / 1000;
                    portEXIT_CRITICAL(&timerMux);

                    // reset states of all
                    // digitalWrite(S1, LOW); // digitalWrite(S3, LOW); 
                    GPIO.out1_w1tc.val = S1_MASK;
                    GPIO.out_w1tc = S3_MASK;

                    // print
                    Serial.println("\nreference counts = " + String(TREF_COUNTS) + ", measured counts = " + String(adc_counts));
                    Serial.println("ratio = " + String(rat));
                    Serial.println("Vin = " + String(Vin) + " V, time taken = " + String(processtime) + " ms\n\n");

                    state = DONE;
                }
                break;
            }

            case DONE:
                // Serial.println("\t6. DUALSLOPE SSS - CASE DONE");  
                break;
            case HOLD:
                // Serial.println("\t6. DUALSLOPE SSS - CASE HOLD");  
                break;
        }
    }    

    void setupDualSlope() {
        pinMode(COMPARATOR_OUT, INPUT);

        pinMode(S0, OUTPUT);

        pinMode(S1, OUTPUT);
        pinMode(S2, OUTPUT);
        pinMode(S3, OUTPUT);

        digitalWrite(S0, HIGH);     // TC by default (LOW), selection of LM35 for HIGH
        digitalWrite(S1, LOW);      // set low initially
        digitalWrite(S2, LOW);      // set low initially
        digitalWrite(S3, LOW);      // set low initially –> logic 0 = ADC reset closed

        state = HOLD;   // intially disable

        startTimer();   // start timing only when conversion begins
    }

    int resetDualSlope() {
        state = RESET;
        return 1;
    }

    int stopDualSlope() {
        stopTimer();
        state = HOLD;
        return 1;
    }

}


#endif