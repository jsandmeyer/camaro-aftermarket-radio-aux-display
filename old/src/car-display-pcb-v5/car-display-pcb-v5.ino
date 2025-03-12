#define F_CPU 16000000L
#define MCPHZ MCP_16MHZ
#define SER_BAUD 115200
#define OLED_SPI_BAUD 1000000UL

#include <math.h>
#include <SPI.h>
#include <mcp_can.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#define GMLAN_PRI_MASK 0x1C000000
#define GMLAN_PRI_SHIFT 0x1A
#define GMLAN_ARB_MASK 0x03FFE000
#define GMLAN_ARB_SHIFT 0x0D
#define GMLAN_SENDER_MASK 0x00001FFF
#define GMLAN_SENDER_SHIFT 0x00

#define GMLAN_MS(v, m, s) ((v & m) >> s)
#define GMLAN_PRI(v) GMLAN_MS(v, GMLAN_PRI_MASK, GMLAN_PRI_SHIFT)
#define GMLAN_ARB(v) GMLAN_MS(v, GMLAN_ARB_MASK, GMLAN_ARB_SHIFT)
#define GMLAN_SENDER(v) GMLAN_MS(v, GMLAN_SENDER_MASK, GMLAN_SENDER_SHIFT)

#define CAN_INT 7
#define OLED_DC 5
#define OLED_RST 9
#define UNIT_SW 15
#define SPI_CS_PIN_CAN 10
#define SPI_CS_PIN_OLED 6

#define SPI_MOSI 11
#define SPI_MISO 12
#define SPI_SCK 13

#define PGOOD_3V3 16

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define PA_BAR_H 8
#define PA_BAR_MARGIN_TOTAL (SCREEN_WIDTH % 5)
#define PA_BAR_MARGIN (PA_BAR_MARGIN_TOTAL / 2)
#define PA_BAR_W (SCREEN_WIDTH / 5)
#define PA_BAR_EXTRA_W (PA_BAR_MARGIN_TOTAL % 2)
#define PA_TIMEOUT 10000UL

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, SPI_MOSI, SPI_SCK, OLED_DC, OLED_RST, SPI_CS_PIN_OLED);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, SPI_CS_PIN_OLED, OLED_SPI_BAUD);

MCP_CAN CAN0(SPI_CS_PIN_CAN);

unsigned char last_rendered_id = 255;
bool use_imperial = false;
unsigned char slot_map[8] = {0, 1, 3, 2, 5, 3, 4, 3};

struct {
  unsigned char id; // increments each time this is changed
  unsigned char temperature; // 0 for off, or 1+
  unsigned long last_pa; // time of last park assist notification
  unsigned char park_assist_slot; // 0 through 5
  unsigned char park_assist_level; // 0 or 1-4
  unsigned char park_assist_distance; // in cm
} data_state;

void getTextBounds(const char *str, GFXfont *font, int *width, int *height) {
  uint16_t x, y, x1, y1;
  display.setTextSize(1);
  display.setFont(font);
  display.getTextBounds(str, &x, &y, &x1, &y1, width, height);
}

void calculateTemperature(int* result) {
  *result = ((int)data_state.temperature / 2) - 40;

  if (use_imperial) {
    *result = round((1.8 * (float)*result) + 32.0);
  }
}

void calculateImperialDistance(unsigned char *feet, unsigned char *inches) {
  float inches_f = 0.393701 * data_state.park_assist_distance;
  *feet = inches_f / 12;
  *inches = inches_f - (*feet * 12);
}

void renderTemperature() {
  display.clearDisplay();

  char text[10];
  char unit = use_imperial ? 'F' : 'C';
  int temperature, width, height;

  calculateTemperature(&temperature);
  sprintf(text, "%d  %c", temperature, unit);

  getTextBounds(text, &FreeSans18pt7b, &width, &height);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setFont(&FreeSans18pt7b);

  display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT + height) / 2);
  display.write(text);
  Serial.println(text);
  Serial.println((SCREEN_WIDTH - width) / 2);
  Serial.println((SCREEN_HEIGHT + height) / 2);

  int degOffsetX = ((SCREEN_WIDTH + width) / 2) - 25;
  display.drawCircle(degOffsetX, 8, 3, SSD1306_WHITE);
  display.drawCircle(degOffsetX, 8, 4, SSD1306_WHITE);

  display.display();
}

void calculateParkAssistLevelAndSlot(unsigned char buf[8], unsigned char *slot, unsigned char *level) {
  unsigned char tmpLevel = 255;
  unsigned char tmpSlot = 0;

  // MR, 0L
  for (size_t index = 0; index < 2; index++) {
    unsigned char a = (buf[3 - index] & 0x0F) - 1; //L,R
    unsigned char b = ((buf[3 - index] & 0xF0) >> 4) - 1; //0,M

    if (a < 255) {
      tmpSlot |= (1 << (index * 2));
    }

    if (b < 255) {
      tmpSlot |= (1 << ((index * 2) + 1));
    }

    if (a <= b && tmpLevel > a) {
      tmpLevel = a;
    } else if (b <= a && tmpLevel > b) {
      tmpLevel = b;
    }
  }

  *slot = tmpSlot > 7 ? slot_map[7] : slot_map[tmpSlot];
  *level = tmpLevel + 1;
}

bool shouldShowRect() {
  unsigned long now = millis();
  // use level - 4 = slow, 1 = fast
  switch (data_state.park_assist_level) {
    case 4:
      return now % 1000 < 500;
    case 3:
      return now % 650 < 325;
    case 2:
      return now % 300 < 150;
    case 1:
      return true;
    default:
      return false;
  }
}

void renderParkAssistRect() {
  display.fillRect(0, SCREEN_HEIGHT - PA_BAR_H, SCREEN_WIDTH, PA_BAR_H, SSD1306_BLACK);

  if (shouldShowRect()) {
    display.fillRect(PA_BAR_MARGIN + ((data_state.park_assist_slot - 1) * PA_BAR_W), SCREEN_HEIGHT - PA_BAR_H, PA_BAR_W + PA_BAR_EXTRA_W, PA_BAR_H, SSD1306_WHITE);
  }
}

void renderParkAssist() {
  display.clearDisplay();

  char text[12];
  unsigned char feet, inches;
  int width, height;
  calculateImperialDistance(&feet, &inches);

  if (use_imperial) {
    calculateImperialDistance(&feet, &inches);

    if (feet > 0) {
      sprintf(text, "%dft %din", feet, inches);
    } else {
      sprintf(text, "%din", inches);
    }
  } else {
    sprintf(text, "%dcm", data_state.park_assist_distance);
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setFont(&FreeSans9pt7b);

  // distance measurement
  getTextBounds(text, &FreeSans9pt7b, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, height);
  display.write(text);

  renderParkAssistRect();
  display.display();
}

void setup() {
  Serial.begin(SER_BAUD);
  Serial.println("Hello!");

  pinMode(UNIT_SW, INPUT);
  pinMode(PGOOD_3V3, INPUT);

  use_imperial = !digitalRead(UNIT_SW);

  if (use_imperial) {
    Serial.println("Imperial units selected");
  } else {
    Serial.println("Metric units selected");
  }

  while (CAN_OK != CAN0.begin(MCP_STDEXT, CAN_33K3BPS, MCPHZ)) {
    Serial.println("Waiting for CANBUS bootup");
    delay(500);
  }

  Serial.println("Setting CANBUS filters");

  CAN0.init_Mask(0, 1, 0x00FFE000);
  CAN0.init_Filt(0, 1, 0x00424000);
  CAN0.init_Filt(1, 1, 0x003A8000);
  CAN0.init_Mask(1, 0, 0x00FF0000);
  CAN0.init_Filt(2, 0, 0x00000000);
  CAN0.init_Filt(3, 0, 0x00000000);
  CAN0.init_Filt(4, 0, 0x00000000);
  CAN0.init_Filt(5, 0, 0x00000000);
  // delay(100);
  CAN0.setMode(MCP_LISTENONLY);
  pinMode(CAN_INT, INPUT);

  Serial.println("CANBUS bootup complete");

  // wait until PGOOD_3V3 is OK, then wait additional 250ms before trying OLED
  while (!digitalRead(PGOOD_3V3)) {
    Serial.println("Waiting for 3.3V PGOOD signal");
    delay(250);
  }

  delay(250);
  Serial.println("Confirmed 3.3V PGOOD signal");
  
  while (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("Waiting for OLED bootup");
    delay(500);
  }

  display.clearDisplay();
  display.display();
  Serial.println("OLED bootup complete");

  Serial.println("Go!");
}

void loop() {
  unsigned long now = millis();

  unsigned long canId, arbId;
  unsigned char len;
  unsigned char buf[8];

  if (!digitalRead(CAN_INT)) {
    CAN0.readMsgBuf(&canId, &len, buf);
    arbId = GMLAN_ARB(canId);

    if (arbId == 0x1D4) { // 0x103A80BB or 0x??3A8???
      // rear park assist sonar
      unsigned char PA_STATUS = (buf[0] & 0x0F);
      if (PA_STATUS == 0x00) {
        Serial.println("PA ON");
        Serial.println(buf[1]);
        // PA ON
        // buf[1] is shortest real distance to closest object, from 0x00 to 0xFF, in centimeters (multiply by 0.0328084 for feet!)
        // buf[2~3] nibbles are [M, R, 0, L]
        // expect car to display single value in L, L+M, M, M+R, R - but take lowest nonzero
        // intent is to show distance in one of five positions on screen
        // 0 = nothing, 1 = stop (red/solid), 2 = close (red/blinking fast), 3 = medium (yellow/blinking medium), 4 = far (yellow/blinking slow)

        data_state.last_pa = now | 1; // never 0, buf 1 millis off is OK
        data_state.park_assist_distance = buf[1];
        calculateParkAssistLevelAndSlot(buf, &data_state.park_assist_slot, &data_state.park_assist_level);
        data_state.id++; // always increment because last_pa is different
      } else if (PA_STATUS == 0x0F) {
        Serial.println("PA OFF");
        // PA OFF
        data_state.last_pa = 0;
        data_state.park_assist_distance = 0;
        data_state.park_assist_level = 0;
        data_state.park_assist_slot = 0;
        data_state.id++;
      } else {
        Serial.println("PA UNK");
        Serial.println(PA_STATUS);
      }
    } else if (arbId == 0x212) { // 0x10424060 or 0x??424???
      Serial.println("TEMP");
      Serial.println(buf[1]);
      if (data_state.temperature != buf[1]) {
        data_state.temperature = buf[1];
        data_state.id++;
      }
    }
  }

  if (data_state.last_pa && data_state.last_pa + PA_TIMEOUT < now) {
    Serial.println("Last PA too old");
    data_state.last_pa = 0;
    data_state.park_assist_distance = 0;
    data_state.park_assist_level = 0;
    data_state.park_assist_slot = 0;
    data_state.id++;
  }

  if (last_rendered_id != data_state.id) {
    last_rendered_id = data_state.id;
    if (data_state.last_pa) {
      Serial.println("Render PA");
      renderParkAssist();
    } else if (data_state.temperature) {
      Serial.println("Render TEMP");
      renderTemperature();
    } else {
      Serial.println("Clear");
      display.clearDisplay();
      display.display();
    }
  } else if (data_state.last_pa) { // if active PA
    Serial.println("Render PA Rect");
    renderParkAssistRect();
    display.display();
  }
}
