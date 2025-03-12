#include "Watchdog.h"
#include "debug.h"

Watchdog::Watchdog(uint16_t limit) : limit(limit), errors(0) {
    // setting to INPUT_PULLUP first forces pin HIGH when coming online
    pinMode(SW_RESET, INPUT_PULLUP);
    pinMode(SW_RESET, OUTPUT);
}

void Watchdog::clearErrors() {
    errors = 0;
}

void Watchdog::countError() {
    errors++;

    if (errors >= limit) {
        resetNow();
    }
}

void Watchdog::resetNow() {
    DEBUG(Serial.print(F("Watchdog rebooting %lx\n")));
    delay(1000);
    digitalWrite(SW_RESET, LOW);
}
