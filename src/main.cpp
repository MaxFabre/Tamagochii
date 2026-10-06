#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <TJpg_Decoder.h>

#define TFT_BL 27

#define BUZZER 14
#define SD_CS 4
#define BACKGROUND_PATH "/backgrounds/2.jpg"

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
bool leftButtonWasPressed = false;
bool middleButtonWasPressed = false;
bool rightButtonWasPressed = false;
bool touchWasPressed = false;
uint8_t missedTouchReads = RELEASE_READS;
uint32_t lastTouchDiagnostic = 0;

bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) {
    return false;
  }
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

bool loadBackgroundFromSd() {
  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed");
    return false;
  }

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tftOutput);

  const JRESULT decoded = TJpgDec.drawSdJpg(0, 0, BACKGROUND_PATH);
  if (decoded != JDR_OK) {
    Serial.print("JPEG decode failed: ");
    Serial.println(static_cast<int>(decoded));
    return false;
  }

  return true;
}

void playButtonNote(uint16_t frequency, uint32_t duration = 200) {
  tone(BUZZER, frequency, duration);
}

void drawCircle(uint16_t color) {
  tft.fillCircle(CIRCLE_X, CIRCLE_Y, CIRCLE_RADIUS, color);
}

void toggleCircle() {
  circleIsRed = !circleIsRed;
  drawCircle(circleIsRed ? TFT_RED : TFT_BLUE);
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_MIDDLE, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(0);
  tft.setTouch(touchCalibration);

  if (!loadBackgroundFromSd()) {
    tft.fillScreen(TFT_BLACK);
  }

  drawCircle(TFT_BLUE);

  playButtonNote(262, 180); //Do
  playButtonNote(392, 180);  // Sol
}

void loop() {
  uint16_t touchX = 0;
  uint16_t touchY = 0;
  bool isTouchingCircle = false;

  const bool leftButtonPressed = digitalRead(BTN_LEFT) == LOW;
  const bool rightButtonPressed = digitalRead(BTN_RIGHT) == LOW;

  if (leftButtonPressed && !leftButtonWasPressed) {
    Serial.println("Left button pressed");
  }
  if (rightButtonPressed && !rightButtonWasPressed) {
    Serial.println("Right button pressed");
  }
  leftButtonWasPressed = leftButtonPressed;
  rightButtonWasPressed = rightButtonPressed;

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
