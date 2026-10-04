#include <Arduino.h>
#include <TFT_eSPI.h>

#define TFT_BL 27

TFT_eSPI tft = TFT_eSPI();

constexpr int CIRCLE_X = 160;
constexpr int CIRCLE_Y = 240;
constexpr int CIRCLE_RADIUS = 70;

// Calibration values for the resistive touch controller in rotation 0.
uint16_t touchCalibration[5] = {275, 3620, 264, 3532, 1};
bool circleIsRed = false;
uint8_t missedTouchReads = 0;

constexpr uint16_t TOUCH_THRESHOLD = 300;
constexpr uint8_t RELEASE_READS = 8;

void drawCircle(uint16_t color) {
  tft.fillCircle(CIRCLE_X, CIRCLE_Y, CIRCLE_RADIUS, color);
}

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(0);
  tft.setTouch(touchCalibration);
  tft.fillScreen(TFT_BLACK);
  drawCircle(TFT_BLUE);
}

void loop() {
  uint16_t touchX = 0;
  uint16_t touchY = 0;
  bool isTouchingCircle = false;

  if (tft.getTouch(&touchX, &touchY, TOUCH_THRESHOLD)) {
    const int dx = static_cast<int>(touchX) - CIRCLE_X;
    const int dy = static_cast<int>(touchY) - CIRCLE_Y;
    isTouchingCircle = (dx * dx + dy * dy) <=
                       (CIRCLE_RADIUS * CIRCLE_RADIUS);
  }

  if (isTouchingCircle) {
    missedTouchReads = 0;
  } else if (missedTouchReads < RELEASE_READS) {
    missedTouchReads++;
  }

  const bool stableTouch = missedTouchReads < RELEASE_READS;
  if (stableTouch != circleIsRed) {
    circleIsRed = stableTouch;
    drawCircle(circleIsRed ? TFT_RED : TFT_BLUE);
  }

  delay(20);
}
