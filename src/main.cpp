#include <Arduino.h>
#include "DataTypes.h" // include before others, especially mcp_can, to normalize data types
#include <SPI.h>
#include <mcp_can.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "OLED.h"
#include "GMLan.h"
#include "Flash.h"
#include "Renderer.h"
#include "GMTemperature.h"
#include "GMParkAssist.h"
#include "Watchdog.h"
#include "Debug.h"

// communications
constexpr auto SER_BAUD = 115200UL;
constexpr auto OLED_SPI_BAUD = 1000000UL;

#if F_CPU == 200000000
constexpr uint8_t MCP_HZ = MCP_20HZ;
#elif F_CPU == 16000000
constexpr uint8_t MCP_HZ = MCP_16MHZ;
#elif F_CPU == 8000000
constexpr uint8_t MCP_HZ = MCP_8MHZ;
#else
constexpr uint8_t MCP_HZ = ERROR_BAD_F_CPU;
#endif

// pin definitions: CANBUS
constexpr uint8_t SPI_CS_PIN_CAN = 7;
constexpr uint8_t CAN_INT = 16;

// pin definitions: OLED display
constexpr uint8_t SPI_CS_PIN_OLED = 2;
constexpr uint8_t OLED_DC = 4;
constexpr uint8_t OLED_RST = 3;

// pin definitions: SPI
constexpr uint8_t SPI_MOSI = 11;
constexpr uint8_t SPI_MISO = 12;
constexpr uint8_t SPI_SCK = 13;

// CANBUS filter data
struct CanMaskFilterData {
    uint8_t ext = 0;
    uint32_t ulData = 0x00000000;
};

constexpr CanMaskFilterData masks[2] = {
    {1, 0x00FFE000},
    {1, 0x00FFE000},
};

constexpr CanMaskFilterData filters[6] = {
    {1, GMLAN_R_ARB(GMLAN_MSG_TEMPERATURE)},
    {1, GMLAN_R_ARB(GMLAN_MSG_PARK_ASSIST)},
    {1, GMLAN_R_ARB(GMLAN_MSG_CLUSTER_UNITS)},
    {1, 0x00000000},
    {1, 0x00000000},
    {1, 0x00000000},
};

/**
 * Run initialization function, and evaluate its result
 * If enough errors happen, the MCUs on the board all get rebooted by the reset supervisor
 * @tparam Delegate type of delegateFn lambda function
 * @tparam Result type of return value of delegateFn lambda function and of expectedValue arg
 * @tparam OnError type of onErrorFn lambda function
 * @param watchdog the watchdog instance
 * @param delegateFn the lambda function to run
 * @param expectedValue the expected (successful) result
 * @param onErrorFn lambda function run on error, usually logging / delay logic
 */
template<typename Delegate, typename Result, typename OnError>
void runWithWatchdog(Watchdog* watchdog, Delegate delegateFn, Result expectedValue, OnError onErrorFn) {
    Result result = expectedValue;

    // loop will eventually exit, once watchdog errors hit limit, it will reboot the device
    do {
        result = delegateFn();

        if (result != expectedValue) {
            onErrorFn(result);
            watchdog->countError();
        }
    } while (result != expectedValue);
}

/**
 * CANBUS initialization
 * Starts in listen-only mode, with filters for useful ARB IDs
 * Without the filters, app would have to process many more messages than necessary
 * @param canBus
 * @param watchdog
 */
void initializeCanBus(MCP_CAN* canBus, Watchdog* watchdog) {
    DEBUG(Serial.println(F("Initializing MCP25625")));

    runWithWatchdog(
        watchdog,
        [canBus] {
            return canBus->begin(MCP_STDEXT, CAN_33K3BPS, MCP_HZ);
        },
        static_cast<uint8_t>(CAN_OK),
        [](const uint8_t errorCode) {
            DEBUG(Serial.printf(F("MCP25625 initialization failed, error code =%u\n"), errorCode));
            delay(500);
        }
    );

    DEBUG(Serial.println(F("Setting MCP25625 masks and filters")));

    for (uint8_t maskId = 0; maskId < 2; maskId++) {
        auto mask = masks[maskId];

        runWithWatchdog(
            watchdog,
            [canBus, maskId, mask] {
                DEBUG(Serial.printf(F("MCP25625 Set mask num=%u ext=%u ulData=0x%08lx\n"), maskId, mask.ext, mask.ulData));
                return canBus->init_Mask(maskId, mask.ext, mask.ulData);
            },
            static_cast<uint8_t>(CAN_OK),
            [maskId](const uint8_t errorCode) {
                DEBUG(Serial.printf(F("MCP25625 setting mask %u failed, error code =%u\n"), maskId, errorCode));
                delay(500);
            }
        );

        const uint8_t filterStart = maskId == 0 ? 0 : 2;
        const uint8_t filterEnd = maskId == 0 ? 2 : 6;

        for (uint8_t filterId = filterStart; filterId < filterEnd; filterId++) {
            auto filter = filters[filterId];

            runWithWatchdog(
                watchdog,
                [canBus, filterId, filter] {
                    DEBUG(Serial.printf(F("MCP25625 Set filter num=%u ext=%u ulData=0x%08lx\n"), filterId, filter.ext, filter.ulData));
                    return canBus->init_Filt(filterId, filter.ext, filter.ulData);
                },
                static_cast<uint8_t>(CAN_OK),
                [filterId](const uint8_t errorCode) {
                    DEBUG(Serial.printf(F("MCP25625 setting filter %u failed, error code =%u\n"), filterId, errorCode));
                    delay(500);
                }
            );
        }
    }

    DEBUG(Serial.println(F("Setting MCP25625 mode to listen")));

    runWithWatchdog(
        watchdog,
        [canBus] {
            return canBus->setMode(MCP_LISTENONLY);
        },
        static_cast<uint8_t>(CAN_OK),
        [](const uint8_t errorCode) {
            DEBUG(Serial.printf(F("MCP25625 could not enter listen-only mode, error code =%u\n"), errorCode));
            delay(500);
        }
    );

    pinMode(CAN_INT, INPUT); // set up interrupt

    DEBUG(Serial.println(F("MCP25625 initialization complete")));
}

/**
 * OLED display setup
 * Will also blank out the display
 * @param display
 * @param watchdog
 */
void initializeOledDisplay(Adafruit_SSD1306* display, Watchdog* watchdog) {
    DEBUG(Serial.println(F("Initializing SSD1306 OLED")));

    runWithWatchdog(
        watchdog,
        [display] {
            return display->begin(SSD1306_SWITCHCAPVCC);
        },
        true,
        [](const bool errorCode) {
            DEBUG(Serial.printf(F("SSD1306 OLED initialization error %u\n"), errorCode));
        }
    );

    display->clearDisplay();
    display->display();
    DEBUG(Serial.println(F("SSD1306 OLED initialization complete")));
}

/**
 * If new data is available from the CANBUS, process it
 * CAN_INT is low if there is a new message in the queue on the CAN controller
 * @param canBus
 * @param renderers
 * @param numRenderers
 */
void readCanBus(MCP_CAN* canBus, Renderer** renderers, const size_t numRenderers) {
    if (!digitalRead(CAN_INT)) {
        uint32_t canId;
        uint8_t len;
        uint8_t buf[8];

        if (canBus->readMsgBuf(&canId, &len, buf) == CAN_NOMSG) {
            return;
        }

        auto const arbId = GMLAN_ARB(canId);
        DEBUG(Serial.printf(F("Checking ARB ID 0x%08lx\n"), arbId));

        if (arbId == GMLAN_MSG_CLUSTER_UNITS) {
            const uint8_t units = buf[0] & 0x0F;
            DEBUG(Serial.printf(F("New cluster units: 0x%02x\n"), units));
            Flash::saveUnits(units);

            for (size_t i = 0; i < numRenderers; i++) {
                renderers[i]->setUnits(units);
            }
        }

        for (size_t i = 0; i < numRenderers; i++) {
            if (renderers[i]->recognizesArbId(arbId)) {
                DEBUG(Serial.printf(F("Processing via %s ARB ID 0x%08lx\n"), renderers[i]->getName(), arbId));
                renderers[i]->processMessage(arbId, len, buf);
            }
        }
    }
}

/**
 * Render data to display
 * @param display
 * @param renderers
 * @param numRenderers
 * @param lastRenderer
 */
void renderDisplay(Adafruit_SSD1306* display, Renderer** renderers, const size_t numRenderers, Renderer*& lastRenderer) {
    /*
     * Render new data, based on priority, taking the first which "should render"
     * It is always assumed that if a module "should render" that it has new data and must render now
     */
    for (size_t i = 0; i < numRenderers; i++) {
        if (renderers[i]->shouldRender()) {
            DEBUG(Serial.printf(F("Rendering [1] via %s\n"), renderers[i]->getName()));
            renderers[i]->render();
            lastRenderer = renderers[i];
            return; // exit loop
        }
    }

    /*
     * Render old data, based on priority, taking the first which "can render"
     * Only the first module which "can render" is considered, this avoids oscillation in display choice
     */
    for (size_t i = 0; i < numRenderers; i++) {
        if (renderers[i]->canRender()) {
            // if we just rendered, don't waste time re-rendering
            if (lastRenderer == renderers[i]) {
                return;
            }

            DEBUG(Serial.printf(F("Rendering [2] via %s\n"), renderers[i]->getName()));
            renderers[i]->render();
            lastRenderer = renderers[i];
            return; // exit loop
        }
    }

    if (lastRenderer != nullptr) {
        /*
         * If there is absolutely nothing that should or can be rendered, clear the display
         */
        display->clearDisplay();
        display->display();
    }
}

/**
 * Main entrypoint
 * Called by int main() by framework
 */
[[noreturn]] void setup() {
    DEBUG(Serial.begin(SER_BAUD));
    DEBUG(Serial.println(F("Booting up")));
    const auto watchdog = new Watchdog();

    delay(10);

    const auto canBus = new MCP_CAN(SPI_CS_PIN_CAN);
    initializeCanBus(canBus, watchdog);

    const auto display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, SPI_CS_PIN_OLED, OLED_SPI_BAUD);
    initializeOledDisplay(display, watchdog);

    /*
     * Set up Renderer objects
     * These objects both read/process GMLAN data and render to the display when called
     * These should be stored in **renderers in order of priority, with most important renderer first
     */

    DEBUG(Serial.println(F("Preparing renderers")));
    Flash::setDefaults();
    const auto units = Flash::getUnits();

    constexpr size_t numRenderers = 2;
    Renderer *lastRenderer = nullptr; // last renderer to render, to avoid doubles of same data
    Renderer* renderers[numRenderers];
    renderers[0] = new GMParkAssist(display, units);
    renderers[1] = new GMTemperature(display, units);

    DEBUG(Serial.println(F("Booted up")));

    // loop in setup to avoid global variables
    while (true) {
        readCanBus(canBus, renderers, numRenderers);
        renderDisplay(display, renderers, numRenderers, lastRenderer);

        Debug::processDebugInput(renderers, numRenderers);
        delay(10);
    }
}

/**
 * Loop entrypoint
 * Called by int main() from framework, but realistically will never run because setup() is [[noreturn]]
 */
void loop() {
    /* do nothing - loop handled inside setup() */
}
