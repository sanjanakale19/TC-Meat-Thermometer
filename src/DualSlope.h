/*!
 * @file DualSlope.h
 *
 * Written for ESP32.
 */

#ifndef DualSlope_H
#define DualSlope_H

#include <Arduino.h>
#include "Globals.h"
#include <HardwareSerial.h>
#include <SPI.h>
#include <Wire.h>

namespace DualSlope {

    namespace PrecisionMath {
        static constexpr float AMBIENT_MIN_C = 0.0f;
        static constexpr float AMBIENT_MAX_C = 60.0f;
        static constexpr float TC_GAIN = 140.86f;
        static constexpr float TC_OFFSET_V = 2.5f;
        static constexpr float LM35_GAIN = 3.1826f;
        static constexpr float LM35_OFFSET_V = 2.048f * 1.0574f;

        static constexpr float NIST_K_POSITIVE_INV_COEFFS[9] = {
            0.000000e+00f,
            2.508355e+01f,
            7.860106e-02f,
           -2.503131e-01f,
            8.315270e-02f,
           -1.228034e-02f,
            9.804036e-04f,
           -4.413030e-05f,
            1.057734e-06f
        };

        inline float ambientTempToMillivolts(float T_ambient) {
            const float boundedTemp = constrain(T_ambient, AMBIENT_MIN_C, AMBIENT_MAX_C);
            return boundedTemp * (0.039474f + (0.000035f * boundedTemp));
        }

        inline float tcAdcVoltageToMillivolts(float V_adc) {
            return ((V_adc - TC_OFFSET_V) / TC_GAIN) * 1000.0f;
        }

        inline float lm35AdcVoltageToCelsius(float V_adc) {
            const float rawVoltage = (V_adc - LM35_OFFSET_V) / LM35_GAIN;
            return rawVoltage / 0.010f;
        }

        inline float millivoltsToPreciseTemp(float V_total_mv) {
            float temperatureC = NIST_K_POSITIVE_INV_COEFFS[8];
            for (int i = 7; i >= 0; --i) {
                temperatureC = (temperatureC * V_total_mv) + NIST_K_POSITIVE_INV_COEFFS[i];
            }
            
            return temperatureC;
        }

        inline float preciseTempToVoltageDivided(float targetTempC) {
            const float TEMP_LOW_C = 0.0f;
            const float TEMP_HIGH_C = 200.0f;

            const float RAW_MV_LOW = 0.0f;
            const float RAW_MV_HIGH = 8.138f; // Type K approx at 200 C

            const float V_LOW = 2.157f;   // post filter + gain voltage at TEMP_LOW_C
            const float V_HIGH = 3.646f;  // post filter + gain voltage at TEMP_HIGH_C

            const float R1 = 6200.0f;
            const float R2 = 10000.0f;

            // Clamp temp
            if (targetTempC < TEMP_LOW_C) targetTempC = TEMP_LOW_C;
            if (targetTempC > TEMP_HIGH_C) targetTempC = TEMP_HIGH_C;

            // Binary search RAW thermocouple mV, not post-gain volts
            float low_mV = RAW_MV_LOW;
            float high_mV = RAW_MV_HIGH;

            for (int i = 0; i < 30; ++i) {
                float mid_mV = 0.5f * (low_mV + high_mV);

                float midTempC = millivoltsToPreciseTemp(mid_mV);

                if (midTempC < targetTempC) {
                    low_mV = mid_mV;
                } else {
                    high_mV = mid_mV;
                }
            }

            float target_mV = 0.5f * (low_mV + high_mV);

            // Convert thermocouple mV position to post-gain circuit voltage
            float ratio = (target_mV - RAW_MV_LOW) / (RAW_MV_HIGH - RAW_MV_LOW);
            float postGainVoltage = V_LOW + ratio * (V_HIGH - V_LOW);

            // Apply voltage divider
            float dividedVoltage = postGainVoltage * (R2 / (R1 + R2));

            return dividedVoltage;
        }
    }

    const int S0 = 42;      // SIGNAL SELECT (LOW = TC, HIGH = LM35)
    const int S1 = 40;      // MUX_A0
    const int S2 = 41;      // MUX_A1
    const int S3 = 18;      // ADC_RESET

    static constexpr uint32_t S0_MASK = (1UL << (S0 - 32));
    static constexpr uint32_t S1_MASK = (1UL << (S1 - 32));
    static constexpr uint32_t S2_MASK = (1UL << (S2 - 32));
    static constexpr uint32_t S3_MASK = (1UL << S3);

    const int COMPARATOR_OUT = 13;  // NAND_out

    // Timing constants in microseconds (us)
    const uint32_t TRESET_US = 50; 
    const uint32_t TDELAY_US = 5; // Blanking window constant
    const uint32_t TREF_US = 50UL * 1000UL; // 50 ms -> 50000 µs
    const uint32_t MAX_TIMEOUT_US = TREF_US * 2UL;

    volatile uint64_t adc_counts = 0;            
    volatile uint64_t comparator_tick = 0;       
    volatile uint64_t deintegrate_start = 0;     
    volatile uint64_t phase_start = 0;           
    volatile bool comparator_tripped = false;
    volatile bool lastWasLM35 = false;
    volatile uint32_t cycle_count = 0;

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

    volatile State state = HOLD;

    volatile float rat = 0;
    volatile float Vin = 0;
    volatile float processtime = 0;

    void IRAM_ATTR comparatorISR() {
        if (state != DEINTEGRATE || comparator_tripped) {
            return;
        }
        uint64_t now = timerRead(adcTimer);
        
        // FIXED: Replaced the hardcoded 10000UL with your true TDELAY_US blanking limit
        if ((now - deintegrate_start) < (uint64_t)TDELAY_US) {
            return;
        }
        portENTER_CRITICAL_ISR(&timerMux);
        comparator_tick = now;
        comparator_tripped = true;
        portEXIT_CRITICAL_ISR(&timerMux);
    }

    void IRAM_ATTR trefISR() {
        portENTER_CRITICAL_ISR(&timerMux);
        timerAlarmDisable(adcTimer);

        GPIO.out1_w1ts.val = S1_MASK; // S1 HIGH
        GPIO.out1_w1tc.val = S2_MASK; // S2 LOW

        comparator_tripped = false;
        comparator_tick = 0;
        deintegrate_start = timerRead(adcTimer);

        state = DEINTEGRATE;
        portEXIT_CRITICAL_ISR(&timerMux);
    }

    void startTimer() {
        adcTimer = timerBegin(0, 80, true);  
    }

    void stopTimer() {
        if (adcTimer) {
            timerDetachInterrupt(adcTimer);
            timerAlarmDisable(adcTimer);
        }
    }

    void computeADC() {
        switch(state) {
            case RESET: {
                lastWasLM35 = ((cycle_count % 11) == 10);
                if (lastWasLM35) {
                    digitalWrite(S0, HIGH);
                } else {
                    digitalWrite(S0, LOW);
                }

                taskENTER_CRITICAL(&timerMux);
                phase_start = timerRead(adcTimer);
                taskEXIT_CRITICAL(&timerMux);

                GPIO.out_w1tc = S3_MASK; 
                state = WAIT_RESET;
                break;
            }

            case WAIT_RESET: {
                uint64_t now = timerRead(adcTimer);
                if ((now - phase_start) >= (uint64_t)TRESET_US) {
                    GPIO.out_w1ts = S3_MASK; 
                    taskENTER_CRITICAL(&timerMux);
                    phase_start = timerRead(adcTimer);
                    taskEXIT_CRITICAL(&timerMux);
                    state = WAIT_DELAY;
                }
                break;
            }

            case WAIT_DELAY: {
                uint64_t now = timerRead(adcTimer);
                if ((now - phase_start) >= (uint64_t)TDELAY_US) {
                    GPIO.out1_w1tc.val = S1_MASK;    
                    GPIO.out1_w1ts.val = S2_MASK;    
                    GPIO.out_w1ts = S3_MASK;         

                    timerWrite(adcTimer, 0);

                    timerDetachInterrupt(adcTimer);
                    timerAttachInterrupt(adcTimer, &trefISR, true);
                    timerAlarmWrite(adcTimer, TREF_US, false); 
                    timerAlarmEnable(adcTimer);

                    state = INTEGRATE;
                }
                break;
            }

            case INTEGRATE: {
                break;
            }

            case DEINTEGRATE: {
                uint64_t now = timerRead(adcTimer);

                taskENTER_CRITICAL(&timerMux);
                uint64_t local_deintegrate_start = deintegrate_start;
                uint64_t local_comparator_tick = comparator_tick;
                bool local_comparator_tripped = comparator_tripped;
                taskEXIT_CRITICAL(&timerMux);

                bool timeout = ((now - local_deintegrate_start) >= (uint64_t)MAX_TIMEOUT_US);

                if (local_comparator_tripped || timeout) {
                    uint64_t local_adc_counts = 0;

                    taskENTER_CRITICAL(&timerMux);
                    if (local_comparator_tripped && (local_comparator_tick >= local_deintegrate_start)) {
                        local_adc_counts = local_comparator_tick - local_deintegrate_start;
                    } else if (local_comparator_tripped) {
                        local_adc_counts = 1; 
                    } else {
                        local_adc_counts = now - local_deintegrate_start;
                    }

                    GPIO.out1_w1tc.val = S1_MASK | S2_MASK;
                    GPIO.out_w1tc = S3_MASK;

                    cycle_count++;
                    state = DONE;
                    taskEXIT_CRITICAL(&timerMux);

                    const float local_rat = (float)local_adc_counts / (float)TREF_US;
                    const float vref = 2.048f;
                    const float local_vin = vref * local_rat + vref;
                    const float local_processtime = (float)local_adc_counts / 1000.0f; 

                    taskENTER_CRITICAL(&timerMux);
                    adc_counts = local_adc_counts;
                    rat = local_rat;
                    Vin = local_vin;
                    processtime = local_processtime;
                    taskEXIT_CRITICAL(&timerMux);
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
        pinMode(S0, OUTPUT);
        pinMode(S1, OUTPUT);
        pinMode(S2, OUTPUT);
        pinMode(S3, OUTPUT);

        digitalWrite(S0, HIGH);     
        digitalWrite(S1, LOW);      
        digitalWrite(S2, LOW);      
        digitalWrite(S3, LOW);      
        state = HOLD;   
        startTimer();   
    }

    int resetDualSlope() {
        comparator_tripped = false;
        comparator_tick = 0;
        if (cycle_count == 0) {
            cycle_count = 10;
        }
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