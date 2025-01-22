#include <ArduinoJson.h>
#include "Handler.h"  // Include the correct header for Handler class
#include <FastLED.h>

class LedModeHandler : public Handler {
private:
  static constexpr uint8_t NUM_LEDS = 8;
  static constexpr uint8_t LED_PIN = 4;
  static constexpr uint8_t VOLTS = 5;
  static constexpr uint16_t MAX_MA = 800;

  static constexpr int LEDS_OFF = 0;
  static constexpr int DISCO = 1;
  static constexpr int FITBIT_GOALS = 2;
  CRGBArray<NUM_LEDS> leds;
  bool isOn = false;
  long red, green, blue, ledIndex;
  String currentModeAsString = String("off");
  int currentModeInt = LEDS_OFF;
public:
  explicit LedModeHandler(Context& ctx)
    : Handler(ctx) {
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  }

  void handle(DynamicJsonDocument& doc) override {
    if (doc.containsKey("setMode")) {
      String currentMode = doc["setMode"];
      currentModeAsString = currentMode;
      if (currentMode.compareTo("contextDriven") == 0) {
        isOn = true;
        currentModeInt = FITBIT_GOALS;
      } else if (currentMode.compareTo("disco") == 0) {
        currentModeInt = DISCO;
      } else if (currentMode.compareTo("off") == 0) {
        currentModeInt = LEDS_OFF;
      }
    }
    Handler::handle(doc);
  }

  String run() {

    switch (currentModeInt) {
      case FITBIT_GOALS:
        if (isOn) {
          for (int i = 0; i < NUM_LEDS; i++) {
            red = 0;
            green = 155;
            blue = 0;
            leds[i] = CRGB(red, green, blue);
          }
          FastLED.show();
        }
        isOn = false;
        //        delay(75);

        // statements
        break;
      case DISCO:
        red = random(255);
        green = random(255);
        blue = random(255);
        ledIndex = random(NUM_LEDS);
        leds[ledIndex] = CRGB(red, blue, green);
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
        // statements
        break;
    }
    return currentModeAsString;
  }

};
