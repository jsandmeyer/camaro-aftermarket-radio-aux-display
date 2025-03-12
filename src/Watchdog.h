#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

#define SW_RESET 17

class Watchdog {
    uint16_t limit;
    uint16_t errors;
public:
    explicit Watchdog(uint16_t limit = 32);
    void clearErrors();
    void countError();
    void resetNow();
};

#endif //WATCHDOG_H
