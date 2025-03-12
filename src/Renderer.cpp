#include "Renderer.h"

/**
 * Create a Renderer
 * @param display OLED display
 */
Renderer::Renderer(Adafruit_SSD1306* display) : display(display) {}

/**
 * Sets new cluster units
 * @param units the current unit data (GMLAN_VAL_CLUSTER_UNITS_*)
 */
void Renderer::setUnits(const byte units) {
    this->units = units;

    if (canRender()) {
        needsRender = true;
    }
}

