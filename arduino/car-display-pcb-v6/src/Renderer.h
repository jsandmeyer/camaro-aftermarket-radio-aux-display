#ifndef RENDERER_H
#define RENDERER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

class Renderer {
protected:
    /**
     * Whether this module needs to be rendered.
     */
    bool needsRender = false;

    /**
     * OLED display
     */
    Adafruit_SSD1306 *display;
public:
    virtual ~Renderer() = default;

    /**
     * Create a Renderer
     * @param display OLED display
     */
    explicit Renderer(Adafruit_SSD1306 *display);

    /**
     * Process a GMLAN message
     * @param arbId the Arbitration ID
     * @param buf the buffer data
     */
    virtual void processMessage(unsigned long arbId, uint8_t buf[8]);

    /**
     * Renders data to the display
     */
    virtual void render();

    /**
     * Determine whether there is an update which should be shown on the display now
     * Should return true if there is new data, or if this module needs to make sure its data is shown
     * @return whether the module should render
     */
    virtual bool shouldRender();

    /**
     * Determine whether there is data which could be shown on the display
     * Should return true if there is any low-priority data
     * @return whether the module can render
     */
    virtual bool canRender();

    /**
     * Determine whether this module recognizes the Arbitration ID
     * If true, it is assumed processMessage() will accept this message type
     * @param arbId the Arbitration ID
     * @return whether processMessage() will handle this Arbitration ID
     */
    virtual bool recognizesArbId(unsigned long arbId);

    /**
     * Returns the name of this renderer
     * @return the name as a string
     */
    [[nodiscard]] virtual const char* getName() const;
};

#endif //RENDERER_H
