/*!
 * @file lcd_ui.cpp
 *
 * UI wrapper for SparkFun 16x2 SerLCD RGB Qwiic display.
 */

#include "lcd_ui.h"
#include "pins.h"

#include <Wire.h>
#include <SerLCD.h>

namespace LCD_UI {

    static SerLCD lcd;
    static bool initialized = false;

    static String fitTo16Chars(const String& text) {
        String out = text;

        if (out.length() > 16) {
            out = out.substring(0, 16);
        }

        while (out.length() < 16) {
            out += " ";
        }

        return out;
    }

    bool init() {
        lcd.begin(Wire, LCD_ADDR);

        delay(100);

        lcd.clear();
        lcd.setBacklight(255, 255, 255);
        lcd.setContrast(5);

        initialized = true;

        writeMessage("TC Thermometer", "LCD ready");
        delay(1000);
        clear();

        return true;
    }

    void clear() {
        if (!initialized) return;
        lcd.clear();
    }

    void writeMessage(const String& line1, const String& line2) {
        if (!initialized) return;

        lcd.setCursor(0, 0);
        lcd.print(fitTo16Chars(line1));

        lcd.setCursor(0, 1);
        lcd.print(fitTo16Chars(line2));
    }

    void displaySetTemp(float tempC) {
        if (!initialized) return;

        String line1 = "Set temp to";
        String line2 = String(tempC, 1) + " C";

        writeMessage(line1, line2);
    }

    void displayReadTemp(float tempC) {
        if (!initialized) return;

        String line1 = "Reading temp:";
        String line2 = String(tempC, 1) + " C";

        writeMessage(line1, line2);
    }

}