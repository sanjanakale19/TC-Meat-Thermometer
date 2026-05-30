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

    const int S0 = 42;      // SIGNAL SELECT (LOW = TC, HIGH = LM35)
    const int S1 = 40;      // MUX_A0
    const int S2 = 41;      // MUX_A1
    const int S3 = 18;      // ADC_RESET

    static constexpr uint32_t S0_MASK = (1UL << (S0 - 32));
    static constexpr uint32_t S1_MASK = (1UL << (S1 - 32));
    static constexpr uint32_t S2_MASK = (1UL << (S2 - 32));
    static constexpr uint32_t S3_MASK = (1UL << S3);

    const int COMPARATOR_OUT = 13;  // NAND_out

    // timing values in microseconds (us)
    const uint32_t TRESET_US = 50; // 50 µs (increased to allow auto-zero amp to settle)
    const uint32_t TDELAY_US = 10; // 10 µs
    const uint32_t TREF_US = 50UL * 1000UL; // 50 ms -> 50000 µs

    // timeout in microseconds
    const uint32_t MAX_TIMEOUT_US = TREF_US * 2UL;

    // timestamps and counts use 64-bit to avoid wrap during computation
    volatile uint64_t adc_counts = 0;            // measured deintegration duration (us)
    volatile uint64_t comparator_tick = 0;       // timestamp (us) when comparator tripped
    volatile uint64_t deintegrate_start = 0;     // timestamp (us) deintegration started
    volatile uint64_t phase_start = 0;           // generic phase start timestamp
    volatile bool comparator_tripped = false;
    // flag indicating whether the last completed conversion was an ambient (LM35) reading
    volatile bool lastWasLM35 = false;
    // cycle counter: 10 cycles TC, 1 cycle LM35
    volatile uint32_t cycle_count = 0;

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

    void IRAM_ATTR comparatorISR() {
        // Capture the hardware timer value at the comparator edge (timestamp in us).
        if (state != DEINTEGRATE || comparator_tripped) {
            return;
        }
        portENTER_CRITICAL_ISR(&timerMux);
        comparator_tick = timerRead(adcTimer);
        comparator_tripped = true;
        portEXIT_CRITICAL_ISR(&timerMux);
    }

    // TREF alarm ISR: triggered once after TREF_US to switch from INTEGRATE -> DEINTEGRATE
    void IRAM_ATTR trefISR() {
        portENTER_CRITICAL_ISR(&timerMux);
        // stop the alarm (one-shot)
        timerAlarmDisable(adcTimer);

        // switch MUX: S1 HIGH, S2 LOW to start deintegration
        GPIO.out1_w1ts.val = S1_MASK; // set S1 HIGH
        GPIO.out1_w1tc.val = S2_MASK; // set S2 LOW

        // prepare for deintegration
        comparator_tripped = false;
        comparator_tick = 0;
        // record deintegration start timestamp

        // capture exact timer value at the moment we switch to deintegration
        deintegrate_start = timerRead(adcTimer);

        state = DEINTEGRATE;
        portEXIT_CRITICAL_ISR(&timerMux);
    }

    void startTimer() {
        // Initialize a free-running hardware timer at 1 MHz (1 µs resolution).
        adcTimer = timerBegin(0, 80, true);  // 80 MHz / 80 = 1 MHz
        // Do NOT attach a periodic alarm here. We'll use one-shot alarms for TREF only
        // and read the free-running counter with timerRead(adcTimer) on demand.
    }

    void stopTimer() {
        if (adcTimer) {
            // detach any alarm ISR and disable alarm
            timerDetachInterrupt(adcTimer);
            timerAlarmDisable(adcTimer);
            // leave timer running; if desired, timerEnd(adcTimer) could be used
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
            case RESET: {
                // start RESET: short integrator
                // flip S0 at the very beginning of RESET according to cycle pattern
                // 10 cycles TC (S0 LOW), then 1 cycle LM35 (S0 HIGH)
                if ((cycle_count % 11) == 10) {
                    digitalWrite(S0, HIGH); // LM35
                } else {
                    digitalWrite(S0, LOW);  // TC
                }

                portENTER_CRITICAL(&timerMux);
                phase_start = timerRead(adcTimer);
                taskEXIT_CRITICAL(&timerMux);

                GPIO.out_w1tc = S3_MASK; // close RESET (logic 0)
                state = WAIT_RESET;
                break;
            }

            case WAIT_RESET: {
                // wait for TRESET_US to elapse
                uint64_t now = timerRead(adcTimer);
                if ((now - phase_start) >= (uint64_t)TRESET_US) {
                    GPIO.out_w1ts = S3_MASK; // open RESET
                    portENTER_CRITICAL(&timerMux);
                    phase_start = timerRead(adcTimer);
                    taskEXIT_CRITICAL(&timerMux);
                    state = WAIT_DELAY;
                }
                break;
            }

            case WAIT_DELAY: {
                uint64_t now = timerRead(adcTimer);
                if ((now - phase_start) >= (uint64_t)TDELAY_US) {
                    // start integration: S2 HIGH, S1 LOW, ensure S3 HIGH
                    GPIO.out1_w1tc.val = S1_MASK;    // set S1 LOW
                    GPIO.out1_w1ts.val = S2_MASK;    // set S2 HIGH
                    GPIO.out_w1ts = S3_MASK;         // ensure S3 HIGH (open reset)

                    // zero the hardware timer so it counts 0 -> TREF_US during integration
                    timerWrite(adcTimer, 0);

                    // schedule a one-shot alarm to switch to DEINTEGRATE after TREF_US
                    timerDetachInterrupt(adcTimer);
                    timerAttachInterrupt(adcTimer, &trefISR, true);
                    timerAlarmWrite(adcTimer, TREF_US, false); // one-shot at TREF_US (timer was zeroed)
                    timerAlarmEnable(adcTimer);

                    state = INTEGRATE;
                }
                break;
            }

            case INTEGRATE: {
                // integration will end in trefISR which sets state = DEINTEGRATE
                break;
            }

            case DEINTEGRATE: {
                uint64_t now = timerRead(adcTimer);

                // Read ISR-modified variables under critical to avoid races
                taskENTER_CRITICAL(&timerMux);
                uint64_t local_deintegrate_start = deintegrate_start;
                uint64_t local_comparator_tick = comparator_tick;
                bool local_comparator_tripped = comparator_tripped;
                taskEXIT_CRITICAL(&timerMux);

                bool timeout = ((now - local_deintegrate_start) >= (uint64_t)MAX_TIMEOUT_US);

                if (local_comparator_tripped || timeout) {
                    // perform calculations and hardware state changes inside critical section
                        taskENTER_CRITICAL(&timerMux);
                    // compute measured deintegration duration in us
                    if (local_comparator_tripped) {
                        adc_counts = local_comparator_tick - local_deintegrate_start;
                    } else {
                        adc_counts = now - local_deintegrate_start;
                    }

                    rat = (float)adc_counts / (float)TREF_US;
                    const float vref = 2.048f;
                    // Vinput follows the amplifier+offset model; use the ratio form:
                    // Vinput = vref * (adc_counts / TREF) + vref
                    Vin = vref * rat + vref;
                    processtime = (float)adc_counts / 1000.0f; // ms

                    // reset states of all: clear both MUX lines and short integrator
                    GPIO.out1_w1tc.val = S1_MASK | S2_MASK;
                    GPIO.out_w1tc = S3_MASK;

                    // increment cycle counter and set DONE while still in critical section
                    cycle_count++;
                    state = DONE;
                    // set DONE while still in critical section and record whether this conversion used LM35
                    bool thisWasLM35 = ((cycle_count % 11) == 10);
                    lastWasLM35 = thisWasLM35;
                    cycle_count++;
                    state = DONE;
                    taskEXIT_CRITICAL(&timerMux);

                    // NOTE: No Serial printing here; printing must occur outside critical sections
                }
                break;
            }

            case DONE:
                break;
            case HOLD:
                break;
        }
    }

    void setupDualSlope() {
        pinMode(COMPARATOR_OUT, INPUT);
        attachInterrupt(digitalPinToInterrupt(COMPARATOR_OUT), comparatorISR, FALLING);
        // no debug scope pin used

        pinMode(S0, OUTPUT);

        pinMode(S1, OUTPUT);
        pinMode(S2, OUTPUT);
        pinMode(S3, OUTPUT);

        digitalWrite(S0, HIGH);     // LM35 selected (HIGH), TC for LOW
        digitalWrite(S1, LOW);      // set low initially
        digitalWrite(S2, LOW);      // set low initially
        digitalWrite(S3, LOW);      // set low initially –> logic 0 = ADC reset closed

        state = HOLD;   // intially disable

        startTimer();   // start timing only when conversion begins
    }

    int resetDualSlope() {
        comparator_tripped = false;
        comparator_tick = 0;
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