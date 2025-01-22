#include <ArduinoJson.h>
#include <FastLED.h>
#include <Context.h>

class FitbitGoalsLedHandler {
private:
  static constexpr uint8_t NUM_LEDS = 8;
  static constexpr uint8_t LED_PIN = 26;
  static constexpr uint8_t VOLTS = 5;
  static constexpr uint16_t MAX_MA = 800;

  static constexpr int LEDS_OFF = 0;
  static constexpr int DISCO = 1;
  static constexpr int FITBIT_GOALS = 2;

  CRGBArray<NUM_LEDS> leds;
  CRGB leds[4][60];

  bool isOn = false;
  long red, green, blue, ledIndex;
  String currentModeAsString = String("off");
  int currentModeInt = LEDS_OFF;

  Context& ctx;

public:
  explicit FitbitGoalsLedHandler(Context& context)
    : ctx(context) { // Proper initialization
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  }

  void handle() {
    
    // Implementation for handling additional behavior if needed
  }

  String run() {
    switch (currentModeInt) {
      case FITBIT_GOALS:
        if (isOn) {
          for (int i = 0; i < NUM_LEDS; i++) {
            red = 0;
            green = 128; // Example value for green; adjust as needed
            blue = 0;
            leds[i] = CRGB(red, green, blue);
          }
          FastLED.show();
        }
        isOn = false;
        break;

      case DISCO:
        red = random(256); // Use 256 to include 255 as a possible value
        green = random(256);
        blue = random(256);
        ledIndex = random(NUM_LEDS);
        leds[ledIndex] = CRGB(red, green, blue);
        FastLED.show();
        delay(75);
        break;

      case LEDS_OFF:
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB(0, 0, 0);
        }
        FastLED.show();
        break;

      default:
        // No action needed for invalid mode
        break;
    }
    return currentModeAsString;
  }
};
