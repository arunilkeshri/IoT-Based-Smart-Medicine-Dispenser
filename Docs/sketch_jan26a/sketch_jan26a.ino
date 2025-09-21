#include <Adafruit_NeoPixel.h>

#define LED_PIN    5     // change to your data pin
#define NUM_LEDS   61    // change to your total number of LEDs

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show();      // initialize all pixels off
}

void loop() {
  blinkColor(255, 0,   0);   // RED
  blinkColor(0,   255, 0);   // GREEN
  blinkColor(0,   0,   255); // BLUE
}

void blinkColor(uint8_t r, uint8_t g, uint8_t b) {
  // turn all LEDs to (r,g,b)
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
  delay(500);  // on for 500 ms

  // turn all LEDs off
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
  delay(500);  // off for 500 ms
}
