#include "TextHelper.h"

/**
 * Calculate boundary of text to display
 * @param display the OLED display which will be used to display the text
 * @param str the text to measure
 * @param font the font to measure with
 * @param width output value for width
 * @param height output value for height
 */
void TextHelper::getTextBounds(Adafruit_SSD1306 *display, const char *str, const GFXfont *font, uint16_t *width, uint16_t *height) {
    int16_t x1 = 0;
    int16_t y1 = 0;
    display->setTextSize(1);
    display->setFont(font);
    display->getTextBounds(str, 0, 0, &x1, &y1, width, height);
}
