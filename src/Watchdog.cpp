#include "Watchdog.h"
#include "Debug.h"

/**
 * Create an error handler watchdog
 * @param limit number of errors before a reboot
 */
Watchdog::Watchdog(uint16_t const limit) : limit(limit), errors(0) {
    // setting to INPUT_PULLUP first forces pin HIGH when coming online
    pinMode(SW_RESET, INPUT_PULLUP);
    pinMode(SW_RESET, OUTPUT);
}

/**
 * Reset the error counter
 */
void Watchdog::clearErrors() {
    errors = 0;
}

/**
 * Count an error
 * Reboots if error limit is hit
 */
void Watchdog::countError() {
    errors++;

    if (errors >= limit) {
        resetNow();
    }
}

/**
 * Force an immediate reboot
 */
void Watchdog::resetNow() const {
    DEBUG(Serial.printf(F("Watchdog rebooting after %u failures\n"), errors));
    delay(1000);

    // When this pin is brought LOW it will trigger the reset supervisor IC
    // Not directly writing to RESET pin because it might not be held low long enough
    digitalWrite(SW_RESET, LOW);
}
