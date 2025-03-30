#ifndef FLASH_H
#define FLASH_H

#include <Arduino.h>

class Flash {
    static bool isSetUp();
public:
    static void setDefaults();
    static void saveUnits(uint8_t newUnits);
    [[nodiscard]] static uint8_t getUnits() ;
};

#endif //FLASH_H
