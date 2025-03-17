#ifndef TEXT_HELPER_H
#define TEXT_HELPER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class TextHelper {
public:
    /**
     * Calculate boundary of text to display
     * @param display the OLED display which will be used to display the text
     * @param str the text to measure
     * @param font the font to measure with
     * @param width output value for width
     * @param height output value for height
     */
    static void getTextBounds(Adafruit_SSD1306 *display, const char *str, const GFXfont *font, uint16_t *width, uint16_t *height);
};

#endif //TEXT_HELPER_H
