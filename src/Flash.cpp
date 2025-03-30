#include <EEPROM.h>

#include "Flash.h"
#include "Debug.h"

static constexpr uint8_t header[4] = {0x01, 0xCC, 0x10, 0xF7};

// note index must start at 4 to leave room for header
static constexpr size_t UNITS_INDEX = 4;
static constexpr uint8_t UNITS_DEFAULT = 0;

bool Flash::isSetUp() {
    const auto headerLen = static_cast<size_t>(sizeof(header) / sizeof(header[0]));

    for (size_t i = 0; i < headerLen; i++) {
        if (EEPROM.read(i) != header[i]) {
            return false;
        }
    }

    return true;
}

void Flash::setDefaults() {
    const auto headerLen = static_cast<size_t>(sizeof(header) / sizeof(header[0]));

    if (!isSetUp()) {
        for (size_t i = 0; i < headerLen; i++) {
            EEPROM.write(i, header[i]);
        }

        EEPROM.write(UNITS_INDEX, UNITS_DEFAULT);
    }

    DEBUG(Serial.printf("Flash() units=%x\n", getUnits()));
}

void Flash::saveUnits(const uint8_t newUnits) {
    EEPROM.write(UNITS_INDEX, newUnits);

    DEBUG(Serial.printf("saveUnits() units=%x\n", getUnits()));
}

uint8_t Flash::getUnits() {
    return EEPROM.read(UNITS_INDEX);
}
