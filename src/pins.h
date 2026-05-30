/*!
 * @file Pins.h
 *
 * Pin assignments
 *
 * Written by Reilly
 *
 */

#ifndef PINS_H
#define PINS_H

// I2C
#define PIN_I2C_SCL 21
#define PIN_I2C_SDA 48
#define INA233_ADDR 0x40
#define MCP4725_ADDR 0x60
#define LCD_ADDR 0x72

// Analog inputs
#define PIN_VMEASURE 9
#define PIN_POT_VREF 5

// Digital outputs / inputs
#define PIN_COMPARATOR_OUT 13
#define PIN_MUX_A0 40
#define PIN_MUX_A1 41
#define PIN_SIGNAL_SEL 42
#define PIN_ADC_RESET 18
#define DATA_NEG 19
#define DATA_POS 20

// Max31855
#define PIN_CS_MAX31855 4
#define PIN_VSCLK 16
#define PIN_VMISO 17

#endif //PINS_H