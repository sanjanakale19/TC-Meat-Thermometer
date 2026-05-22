/*!
 * @file lcd_ui.h
 *
 * UI for writing messages to lcd
 */

#ifndef LCD_UI_H
#define LCD_UI_H

#include <Arduino.h>

namespace LCD_UI {

    bool init();

    void clear();

    void writeMessage(const String& line1, const String& line2 = ""); // write a two line message

    void displaySetTemp(float tempC);

}

#endif // LCD_UI_H