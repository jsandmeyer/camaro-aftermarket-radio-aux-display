#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

#define SW_RESET 17

/**
 * Error handler watchdog
 * Reboots after its error recorder is invoked too many times
 */
class Watchdog {
    /**
     * Maximum number of errors before a reboot
     */
    uint16_t limit;

    /**
     * Number of errors recorded
     */
    uint16_t errors;
public:
    /**
     * Create an error handler watchdog
     * @param limit number of errors before a reboot
     */
    explicit Watchdog(uint16_t limit = 32);

    /**
     * Reset the error counter
     */
    void clearErrors();

    /**
     * Count an error
     * Reboots if error limit is hit
     */
    void countError();

    /**
     * Force an immediate reboot
     */
    void resetNow() const;
};

#endif //WATCHDOG_H
