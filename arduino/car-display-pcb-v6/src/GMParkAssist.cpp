#include <Arduino.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

#include "debug.h"
#include "GMParkAssist.h"
#include "TextHelper.h"
#include "oled.h"
#include "gmlan.h"

/**
 * Renders the Park Assist rectangle, blanking out the rectangle zone first
 * Rectangle will be rendered visible or invisible based on millis()
 * Does not update display
 */
void GMParkAssist::renderMarkerRectangle() const {
    const auto now = millis();
    display->fillRect(
        0,
        SCREEN_HEIGHT - PA_BAR_H,
        SCREEN_WIDTH,
        PA_BAR_H,
        SSD1306_BLACK
    );

    if (
        parkAssistLevel > 0
        && parkAssistLevel < 5
        && now % parkAssistDisplayMod[parkAssistLevel] < parkAssistDisplayCompare[parkAssistLevel]
    ) {
        display->fillRect(
            PA_BAR_MARGIN + PA_BAR_W * (parkAssistSlot - 1),
            SCREEN_HEIGHT - PA_BAR_H,
            PA_BAR_W + PA_BAR_EXTRA_W,
            PA_BAR_H,
            SSD1306_WHITE
        );
    }
}

/**
 * Renders the Park Assist distance, assumes the display is already blank
 * Does not update display
 */
void GMParkAssist::renderDistance() const {
    // max text size is realistically 9 - examples "255cm" or "12in" or "21ft 3in" or "20ft 10in"
    char text[12];

    switch (units) {
        case GMLAN_VAL_CLUSTER_UNITS_IMPERIAL: {
            // convert cm to inches, then divide out feet
            auto inches = static_cast<uint8_t>(lround(CM_PER_IN * parkAssistDistance));
            const auto feet = inches / 12;
            inches -= feet * 12;

            // only show feet if there is at least 1 foot
            if (feet > 0) {
                snprintf(text, 11, "%dft %din", feet, inches);
            } else {
                snprintf(text, 11, "%din", inches);
            }
        }
        case GMLAN_VAL_CLUSTER_UNITS_METRIC:
        default: {
            snprintf(text, 11, "%dcm", parkAssistDistance);
        }
    }

    // distance text display
    uint16_t width, height;
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setFont(&FreeSans9pt7b);
    TextHelper::getTextBounds(display, text, &FreeSans9pt7b, &width, &height);
    display->setCursor(static_cast<int16_t>(SCREEN_WIDTH - width) / 2, static_cast<int16_t>(height));
    display->write(text);
}

/**
 * Handles the Rear Park Assist "OFF" message
 */
void GMParkAssist::processParkAssistDisableMessage() {
    DEBUG(Serial.println(F("PA OFF")));

    // blanking out all data will prevent future render
    lastTimestamp = 0;
    parkAssistDistance = 0;
    parkAssistLevel = 0;
    parkAssistSlot = 0;
    needsRender = false;
}

/**
 * Handles the Rear Park Assist "ON" message
 * @param buf is the buffer data from GMLAN
 */
void GMParkAssist::processParkAssistInfoMessage(uint8_t buf[8]) {
    /*
     * buf[1] is shortest real distance to nearest object, from 0x00 to 0xFF, in centimeters
     * rendering function will multiply by 0.0328084 for inches if selected
     */

    DEBUG(Serial.printf(F("PA ON, distance: %ucm\n"), buf[1]));

    lastTimestamp = millis() | 1; // never 0 because of bool evaluation elsewhere; value being 1 ms off is OK
    parkAssistDistance = buf[1];

    /*
     * The park assist sensor controller takes 4 sensor streams and pushes them into 3 data streams for lef/mid/right
     * an obstruction can exist in one nibble, or two adjacent nibbles, creating five total combinations.  The goal is
     * to determine the position of the rectangle from five possible positions, and its blink rate.
     * It is OK to assume that in a multi-nibble scenario (like L+M) that the values will match.
     * buf[2] and buf[3] nibbles are [M, R] and [0, L]
     * for each nibble:
     *  0 = nothing seen
     *  1 = stop (red, solid image/beep)
     *  2 = close (red, blinking/beeping fast)
     *  3 = medium (yellow, blinking/beeping medium)
     *  4 = far (yellow, blinking/beeping slow)
     * Example: buf[2], buf[3] == 0b00100010 (0x22), 0b00000000 (0x00) means M+R at level 2 (close)
     */

    const uint8_t slot_m = (buf[2] & 0xF0) >> 4;
    const uint8_t slot_r = buf[2] & 0x0F;
    const uint8_t slot_l = buf[3] & 0x0F;

    if (slot_m) {
        // middle slot active, so obstruction is mid-left, mid, or mid-right
        parkAssistLevel = slot_m;
        if (slot_l) {
            // left slot also active, so obstruction is mid-left
            parkAssistSlot = 1;
        } else if (slot_r) {
            // right slot also active, so obstruction is mid-right
            parkAssistSlot = 3;
        } else {
            // only middle slot, so obstruction is in the middle
            parkAssistSlot = 2;
        }
    } else if (slot_l) {
        // only left slot, so obstruction is only seen by left sensor
        parkAssistLevel = slot_l;
        parkAssistSlot = 0;
    } else if (slot_r) {
        // only right slot, so obstruction is only seen by right sensor
        parkAssistLevel = slot_r;
        parkAssistSlot = 4;
    } else {
        // should not happen, assume middle
        parkAssistLevel = 0;
        parkAssistSlot = 2;
    }

    // will force render of distance text on next call to render()
    needsRender = true;
}

/**
 * Create a GMParkAssist instance
 * @param display the OLED display from SSD1306 library
 */
GMParkAssist::GMParkAssist(Adafruit_SSD1306* display) : Renderer(display) {}

/**
 * Processes the park assist message and sets state
 * @param arbId the arbitration ID GMLAN_MSG_PARK_ASSIST
 * @param len the length of the buffer data
 * @param buf is the buffer data from GMLAN
 */
void GMParkAssist::processMessage(unsigned long const arbId, uint8_t len, uint8_t buf[8]) {
    if (arbId != GMLAN_MSG_PARK_ASSIST) {
        // don't process irrelevant messages
        return;
    }

    /*
     * Right nibble of buf[0] tells whether Rear Park Assist is ON or OFF
     * Left nibble may have unneeded data, so need to mask it out
     */
    const auto state = buf[0] & 0x0F;

    if (state == GMLAN_VAL_PARK_ASSIST_OFF) {
        processParkAssistDisableMessage();
        return;
    }

    if (state == GMLAN_VAL_PARK_ASSIST_ON) {
        processParkAssistInfoMessage(buf);
        return;
    }

    // Don't recognize this message
    DEBUG(Serial.printf(F("PA Unknown value %u\n"), state));
}

/**
 * Renders the current Park Assist display
 * Should only be called if there is something to render
 * Updates the display
 */
void GMParkAssist::render() {
    if (needsRender) {
        display->clearDisplay();
        renderDistance();
        needsRender = false;
    }

    renderMarkerRectangle();
    display->display();
}

/**
 * Determines whether there is new data to render
 * Rendering should happen if PA has not timed out, or if needsRender is true
 * @return whether rendering should occur
 */
bool GMParkAssist::shouldRender() {
    // if lastTimestamp is too long ago, then disable it
    if (lastTimestamp > 0 && millis() > lastTimestamp + PA_TIMEOUT) {
        processParkAssistDisableMessage();
    }

    return needsRender || lastTimestamp > 0;
}

/**
 * Determines whether there is data which can be rendered
 * Logic for this module is same as shouldRender because once OFF message is received,
 * all data is cleared out and there would be nothing to render anyway
 * @return whether rendering can occur
 */
bool GMParkAssist::canRender() {
    return shouldRender();
}

/**
 * Determines whether this module wants to process this GMLAN message
 * This module only processes Arb ID 0x1D4
 * @param arbId the arbitration ID to check
 * @return whether this module cares about this arbitration ID
 */
bool GMParkAssist::recognizesArbId(unsigned long const arbId) {
    return arbId == GMLAN_MSG_PARK_ASSIST || arbId == GMLAN_MSG_CLUSTER_UNITS;
}

/**
 * Returns the name of this renderer
 * @return the name as a string
 */
const char* GMParkAssist::getName() const {
    return "GMParkAssist";
}
