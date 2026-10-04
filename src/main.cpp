#include <Arduino.h>
#include <TFT_eSPI.h>

#define TFT_BL 27

#define BTN_LEFT 16
#define BTN_MIDDLE 17
#define BTN_RIGHT 5

TFT_eSPI tft = TFT_eSPI();

constexpr int CIRCLE_X = 160;
constexpr int CIRCLE_Y = 240;
constexpr int CIRCLE_RADIUS = 70;

// Calibration values for the resistive touch controller in rotation 0.
uint16_t touchCalibration[5] = {275, 3620, 264, 3532, 1};
// Accept the lowest pressure reported by the controller. The library still
// rejects an untouched panel when the measured pressure is zero.
constexpr uint16_t TOUCH_THRESHOLD = 20;
constexpr uint8_t RELEASE_READS = 8;

bool circleIsRed = false;
bool middleButtonWasPressed = false;
bool touchWasPressed = false;
uint8_t missedTouchReads = RELEASE_READS;
uint32_t lastTouchDiagnostic = 0;

void drawCircle(uint16_t color) {
  tft.fillCircle(CIRCLE_X, CIRCLE_Y, CIRCLE_RADIUS, color);
}

void toggleCircle() {
  circleIsRed = !circleIsRed;
  drawCircle(circleIsRed ? TFT_RED : TFT_BLUE);
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_MIDDLE, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
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

  if (digitalRead(BTN_LEFT) == LOW) {
    Serial.println("Left button pressed");
  }
  if (digitalRead(BTN_RIGHT) == LOW) {
    Serial.println("Right button pressed");
  }

  const bool middleButtonPressed = digitalRead(BTN_MIDDLE) == LOW;
  if (middleButtonPressed && !middleButtonWasPressed) {
    Serial.println("Middle button pressed");
    toggleCircle();
  }
  middleButtonWasPressed = middleButtonPressed;

  if (tft.getTouch(&touchX, &touchY, TOUCH_THRESHOLD)) {
    const int dx = static_cast<int>(touchX) - CIRCLE_X;
    const int dy = static_cast<int>(touchY) - CIRCLE_Y;
    isTouchingCircle = (dx * dx + dy * dy) <=
                       (CIRCLE_RADIUS * CIRCLE_RADIUS);

    if (!touchWasPressed) {
      Serial.print("Touch: ");
      Serial.print(touchX);
      Serial.print(", ");
      Serial.print(touchY);
      Serial.print(isTouchingCircle ? " (circle)" : " (outside circle)");
      Serial.println();
    }
  } else if (millis() - lastTouchDiagnostic >= 1000) {
    lastTouchDiagnostic = millis();
    Serial.print("Touch pressure: ");
    Serial.println(tft.getTouchRawZ());
  }

  if (isTouchingCircle) {
    missedTouchReads = 0;
  } else if (missedTouchReads < RELEASE_READS) {
    missedTouchReads++;
  }

  const bool stableTouch = missedTouchReads < RELEASE_READS;
  if (stableTouch && !touchWasPressed) {
    toggleCircle();
  }
  touchWasPressed = stableTouch;

  delay(20);
}
