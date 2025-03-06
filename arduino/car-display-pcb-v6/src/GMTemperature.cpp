#include <Arduino.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans18pt7b.h>

#include "GMTemperature.h"
#include "TextHelper.h"
#include "oled.h"
#include "gmlan.h"

GMTemperature::GMTemperature(Adafruit_SSD1306* display, const bool useImperial): Renderer(display), useImperial(useImperial) {}

/**
 * Processes the exterior temperature sensor data
 * @param arbId not used
 * @param buf is the buffer data from GMLAN
 * @return
 */
void GMTemperature::processMessage(unsigned long const arbId, uint8_t buf[8]) {
    Serial.printf(F("Got temperature: %x\n"), buf[1]);

    /**
     * buf[1] is hex representation of 2 * temperature in C with offset of 40 degrees
     * math is done in rendering function (including Imperial units conversion)
     */

    if (temperature != buf[1]) {
        temperature = buf[1];
        needsRender = true;
    }
}

/**
 * Renders the current Temperature display
 * Should only be called if there is something to render
 * Updates the display
 */
void GMTemperature::render() {
    Serial.println(F("Render Temperature"));
    display->clearDisplay();

    // max text size is realistically 6 - examples "-40  F" or "190  F" or "-40  C" or "88  C"
    char text[10];
    auto convertedTemperature = temperature / 2 - 40;
    const auto unit = useImperial ? 'F' : 'C';

    if (unit == 'F') {
        // F = 1.8*C + 32
        convertedTemperature = static_cast<uint8_t>(lround(1.8 * convertedTemperature)) + 32;
    }

    // extra space is to make room for degree symbol, which isn't available in font
    snprintf(text, 9, "%d  %c", convertedTemperature, unit);

    // temperature text/graphic display
    uint16_t width, height;
    TextHelper::getTextBounds(display, text, &FreeSans18pt7b, &width, &height);
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setFont(&FreeSans18pt7b);

    // x1 is left position of text
    const auto x1 = static_cast<int16_t>((SCREEN_WIDTH - width) / 2);
    // x2 is 25px inside right of text, for degree symbol ('F' and 'C' are similar enough in width)
    const auto x2 = static_cast<int16_t>((SCREEN_WIDTH + width) / 2 - 25);
    // y1 is bottom position of text
    const auto y1 = static_cast<int16_t>((SCREEN_HEIGHT + height) / 2);
    // y2 is used for degree symbol center point
    const auto y2 = static_cast<int16_t>((SCREEN_HEIGHT - height) / 2 + 4);

    Serial.printf(F("Temperature text: \"%s\", x1=%d, x2=%d, y1=%d y2=%d\n"), text, x1, x2, y1, y2);

    // write text
    display->setCursor(x1, y1);
    display->write(text);

    // write degree symbol
    display->drawCircle(x2, y2, 3, SSD1306_WHITE);
    display->drawCircle(x2, y2, 4, SSD1306_WHITE);

    // show display text
    display->display();
    needsRender = false;
}

/**
 * Determines whether there is new data to render
 * Rendering should happen if the temperature changed
 * @return whether rendering should occur
 */
bool GMTemperature::shouldRender() {
    return temperature > 0 && needsRender;
}

/**
 * Determines whether there is data which can be rendered
 * Rendering may happen if there is temperature data
 * @return whether rendering can occur
 */
bool GMTemperature::canRender() {
    return temperature > 0 || needsRender;
}
/**
 * Determines whether this module wants to process this GMLAN message
 * This module only processes Arb ID 0x212
 * @param arbId the arbitration ID to check
 * @return whether this module cares about this arbitration ID
 */
bool GMTemperature::recognizesArbId(unsigned long const arbId) {
    return arbId == GMLAN_MSG_TEMPERATURE;
}

/**
 * Returns the name of this renderer
 * @return the name as a string
 */
const char* GMTemperature::getName() const {
    return "GMTemperature";
}
