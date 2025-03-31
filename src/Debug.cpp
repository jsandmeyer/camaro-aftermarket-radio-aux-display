#include <Arduino.h>
#include "Debug.h"
#include "Flash.h"
#include "Watchdog.h"

void Debug::processDebugInput(Renderer** renderers, size_t numRenderers) {
#if DO_DEBUG == 1
    while (Serial && Serial.available()) {
        Serial.print("\n");
        switch (const auto input = Serial.read(); input) {
            case 'r':
                Serial.print(F("Rebooting due to user input.\n"));
                delay(1000);
                digitalWrite(SW_RESET, LOW);
            break;
            case 'm':
            case 'i': {
                const uint8_t units = input == 'm'
                    ? GMLAN_VAL_CLUSTER_UNITS_METRIC
                    : GMLAN_VAL_CLUSTER_UNITS_IMPERIAL;

                Flash::saveUnits(units);

                for (size_t i = 0; i < numRenderers; i++) {
                    renderers[i]->setUnits(units);
                }

                break;
            }
            case 't': {
                uint8_t b[] = { 0, 0x72, 0, 0, 0, 0, 0, 0 };

                for (size_t i = 0; i < numRenderers; i++) {
                    renderers[i]->processMessage(GMLAN_MSG_TEMPERATURE, 2, b);
                }

                break;
            }
            case 'p': {
                uint8_t b[8] = { GMLAN_VAL_PARK_ASSIST_ON, 0x33, 0x22, 0x00, 0, 0, 0, 0 };

                for (size_t i = 0; i < numRenderers; i++) {
                    renderers[i]->processMessage(GMLAN_MSG_PARK_ASSIST, 2, b);
                }

                break;
            }
            default:
                Serial.printf(F("Unrecognized input '%c'\n"), input);
            break;
        }
    }
#endif
}
