#include "Renderer.h"

/**
 * Create a Renderer
 * @param display OLED display
 */
Renderer::Renderer(Adafruit_SSD1306* display) : display(display) {}

/**
 * Sets new cluster units
 * @param newUnits the new unit data (GMLAN_VAL_CLUSTER_UNITS_*)
 */
void Renderer::setUnits(const uint8_t newUnits) {
    this->units = newUnits;

    if (canRender()) {
        needsRender = true;
    }
}

