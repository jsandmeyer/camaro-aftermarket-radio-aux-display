#define F_CPU 16000000L
#define MCP_HZ MCP_16MHZ
#define SER_BAUD 115200
#define OLED_SPI_BAUD 1000000UL

#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "oled.h"
#include "gmlan.h"
#include "Renderer.h"
#include "RendererContainer.h"
#include "GMTemperature.h"
#include "GMParkAssist.h"
#include "Watchdog.h"
#include "debug.h"

// pin definitions: CANBUS
#define SPI_CS_PIN_CAN 10
#define CAN0_INT 16

// pin definitions: OLED display
#define SPI_CS_PIN_OLED 2
#define OLED_DC 4
#define OLED_RST 3

// pin definitions: SPI
#define SPI_MOSI 11
#define SPI_MISO 12
#define SPI_SCK 13

// IO control objects
Adafruit_SSD1306 *display;
MCP_CAN *canBus;

// GMLAN parser/renderers
RendererContainer *renderers; // list of renderers, ordered by priority
Renderer *lastRenderer; // last renderer to render, to avoid doubles of same data

void setup() {
    Serial.begin(SER_BAUD);
    Serial.println(F("Booting up"));
    const auto watchdog = new Watchdog();

    delay(10);

    /*
     * CANBUS initialization
     * Starts in listen-only mode, with filters for useful ARB IDs
     * Without the filters, app would have to process many more messages than necessary
     */

    canBus = new MCP_CAN( SPI_CS_PIN_CAN);
    uint8_t canInitResult = CAN_OK;

    Serial.println(F("Initializing MCP2515"));

    do {
        canInitResult = canBus->begin(MCP_STDEXT, CAN_33K3BPS, MCP_HZ);

        if (canInitResult != CAN_OK) {
            Serial.printf(F("MCP2515 initialization failed, error code =%u\n"), canInitResult);
            delay(500);
            watchdog->countError();
        }
    } while (canInitResult != CAN_OK);

    Serial.println(F("Setting MCP2515 masks and filters"));

    do {
        canInitResult = CAN_OK;

        // init mask 0, filters 0-1 which use mask 0, use these to pick out messages
        canInitResult |= canBus->init_Mask(0, 1, 0x00FFE000); // was GMLAN_ARB_MASK
        canInitResult |= canBus->init_Filt(0, 1, GMLAN_R_ARB(GMLAN_MSG_TEMPERATURE)); // becomes 0x00424000
        canInitResult |= canBus->init_Filt(1, 1, GMLAN_R_ARB(GMLAN_MSG_PARK_ASSIST)); // becomes 0x003A8000

        // init mask 1, filters 2-5 which use mask 1, set to ignore everything and no ext
        canInitResult |= canBus->init_Mask(1, 1, 0x00FFE000);
        canInitResult |= canBus->init_Filt(2, 1, GMLAN_R_ARB(GMLAN_MSG_CLUSTER_UNITS));
        canInitResult |= canBus->init_Filt(3, 1, 0x00000000);
        canInitResult |= canBus->init_Filt(4, 1, 0x00000000);
        canInitResult |= canBus->init_Filt(5, 1, 0x00000000);

        if (canInitResult != CAN_OK) {
            Serial.printf(F("MCP2515 masks and filters failed, error code =%u\n"), canInitResult);
            delay(500);
            watchdog->countError();
        }
    } while (canInitResult != CAN_OK);

    do {
        canInitResult = canBus->setMode(MCP_LISTENONLY);

        if (canInitResult != CAN_OK) {
            Serial.printf(F("MCP2515 could not enter listen-only mode, error code =%u\n"), canInitResult);
            delay(500);
            watchdog->countError();
        }
    } while (canInitResult != CAN_OK);

    pinMode(CAN0_INT, INPUT);
    Serial.println(F("MCP2515 initialization complete"));

    /*
     * OLED display setup
     * Will also blank out the display
     */

    Serial.println(F("Initializing SSD1306 OLED"));
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, SPI_CS_PIN_OLED, OLED_SPI_BAUD);

    bool displayInitResult;

    do {
        displayInitResult = display->begin(SSD1306_SWITCHCAPVCC);

        if (!displayInitResult) {
            Serial.println(F("SSD1306 OLED initialization error"));
            delay(500);
            watchdog->countError();
        }
    } while (!displayInitResult);

    display->clearDisplay();
    display->display();
    Serial.println(F("SSD1306 OLED initialization complete"));

    /*
     * Set up Renderer objects
     * These objects both read/process GMLAN data and render to the display when called
     * These should be stored in RendererContainer in order of priority, with most important renderer first
     */

    renderers = new RendererContainer(2);
    renderers->setRenderer(0, new GMParkAssist(display));
    renderers->setRenderer(1, new GMTemperature(display));

    Serial.println(F("Booted up"));
}

void loop() {
    /*
     * If new data is available from the CANBUS, process it
     * CAN0_INT is low if there is a new message in the queue on the CAN controller
     */
    if (!digitalRead(CAN0_INT)) {
        unsigned long canId;
        uint8_t len, buf[8];

        canBus->readMsgBuf(&canId, &len, buf);
        auto const arbId = GMLAN_ARB(canId);
        DEBUG(Serial.printf(F("Checking ARB ID %lx\n"), arbId));

        if (arbId == GMLAN_MSG_CLUSTER_UNITS) {
            const auto units = buf[0] & 0x0F;
            DEBUG(Serial.printf(F("New cluster units: %x\n"), units));

            for (Renderer *renderer : *renderers) {
                renderer->setUnits(units);
            }
        }

        for (Renderer *renderer : *renderers) {
            if (renderer->recognizesArbId(arbId)) {
                DEBUG(Serial.printf(F("Processing via %s ARB ID %lx\n"), renderer->getName(), arbId));
                renderer->processMessage(arbId, len, buf);
            }
        }
    }

    /*
     * Render new data, based on priority, taking the first which "should render"
     * It is always assumed that if a module "should render" that it has new data and must render now
     */
    for (Renderer *renderer : *renderers) {
        if (renderer->shouldRender()) {
            DEBUG(Serial.printf(F("Rendering [1] via %s\n"), renderer->getName()));
            renderer->render();
            lastRenderer = renderer;
            return; // exit loop
        }
    }

    /*
     * Render old data, based on priority, taking the first which "can render"
     * Only the first module which "can render" is considered, this avoids oscillation in display choice
     */
    for (Renderer *renderer : *renderers) {
        if (renderer->canRender()) {
            // if we just rendered, don't waste time re-rendering
            if (lastRenderer == renderer) {
                return;
            }

            DEBUG(Serial.printf(F("Rendering [2] via %s\n"), renderer->getName()));
            renderer->render();
            lastRenderer = renderer;
            return; // exit loop
        }
    }

    /*
     * If there is absolutely nothing that should or can be rendered, clear the display
     */
    display->clearDisplay();
    display->display();
}
