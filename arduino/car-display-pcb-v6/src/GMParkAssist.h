#ifndef GM_PARK_ASSIST_H
#define GM_PARK_ASSIST_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "Renderer.h"

// park assist marker measurements
#define PA_BAR_H 8
#define PA_BAR_MARGIN_TOTAL (SCREEN_WIDTH % 5)
#define PA_BAR_MARGIN (PA_BAR_MARGIN_TOTAL / 2)
#define PA_BAR_W (SCREEN_WIDTH / 5)
#define PA_BAR_EXTRA_W (PA_BAR_MARGIN_TOTAL % 2)

// park assist config
#define PA_TIMEOUT 10000UL // time out park assist mode after 10 seconds
#define CM_PER_IN 0.393701 // for converting park assist distance to Imperial units

class GMParkAssist final : public Renderer {
    /**
     * last timestamp a PA notification was received
     */
    unsigned long lastTimestamp = 0;

    /**
     * rectangle rendering position
     * 0 for off or [1...5] for [left...right]
     */
    uint8_t parkAssistSlot = 0;

    /**
     * Rectangle rendering frequency
     * 0 for off or [1...4] for [close...far] severity/blinking
     * 1 will be rendered solid
     */
    uint8_t parkAssistLevel = 0;

    /**
     * Storage of park assist distance sensor data
     * Value is in approximate centimeters as calculated by rear park assist module
     */
    uint8_t parkAssistDistance = 0;

    /**
     * Used by renderMarkerRectangle to determine blink rate for park assist
     * Note only indexes 1 through 4 are used
     */
    unsigned long parkAssistDisplayMod[5] = {1U, 1U, 300U, 650U, 1000U};

    /**
     * Used by renderMarkerRectangle to determine blink rate for park assist
     * Note only indexes 1 through 4 are used
     */
    unsigned long parkAssistDisplayCompare[5] = {1U, 1U, 150U, 325U, 500U};

    /**
     * Renders the Park Assist rectangle, blanking out the rectangle zone first
     * Rectangle will be rendered visible or invisible based on millis()
     * Does not update display
     */
    void renderMarkerRectangle() const;

    /**
     * Renders the Park Assist distance, assumes the display is already blank
     * Does not update display
     */
    void renderDistance() const;

    /**
     * Handles the Rear Park Assist "OFF" message
     */
    void processParkAssistDisableMessage();

    /**
     * Handles the Rear Park Assist "ON" message
     * @param buf is the buffer data from GMLAN
     */
    void processParkAssistInfoMessage(uint8_t buf[8]);

public:
    /**
     * Create a GMParkAssist instance
     * @param display the OLED display from SSD1306 library
     */
    explicit GMParkAssist(Adafruit_SSD1306 *display);

    /**
     * Process GMLAN message
     * @param arbId the arbitration ID GMLAN_MSG_PARK_ASSIST
     * @param len the length of the buffer data
     * @param buf buffer data from GMLAN
     */
    void processMessage(unsigned long arbId, uint8_t len, uint8_t buf[8]) override;

    /**
     * Renders the current Park Assist display
     * Should only be called if there is something to render
     * Updates the display
     */
    void render() override;

    /**
     * Determines whether there is new data to render
     * Rendering should happen if PA has not timed out, or if needsRender is true
     * @return whether rendering should occur
     */
    bool shouldRender() override;

    /**
     * Determines whether there is data which can be rendered
     * Logic for this module is same as shouldRender because once OFF message is received,
     * all data is cleared out and there would be nothing to render anyway
     * @return whether rendering can occur
     */
    bool canRender() override;

    /**
     * Determines whether this module wants to process this GMLAN message
     * This module only processes Arb ID 0x1D4
     * @param arbId the arbitration ID to check
     * @return whether this module cares about this arbitration ID
     */
    bool recognizesArbId(unsigned long arbId) override;

    /**
     * Returns the name of this renderer
     * @return the name as a string
     */
    [[nodiscard]] const char* getName() const override;
};

#endif //GM_PARK_ASSIST_H
