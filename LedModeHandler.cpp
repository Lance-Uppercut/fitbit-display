#include <ArduinoJson.h>
#include "Handler.h"  // Include the correct header for Handler class
#include <FastLED.h>

class LedModeHandler : public Handler {
private:
  static constexpr uint8_t NUM_LEDS = 8;
  static constexpr uint8_t SLEEP_LED_PIN = 19;
  static constexpr uint8_t WATER_LED_PIN = 4;
  static constexpr uint8_t CALS_LED_PIN = 26;
  static constexpr uint8_t STEPS_LED_PIN = 27;

  static constexpr uint8_t VOLTS = 5;
  static constexpr uint16_t MAX_MA = 800;

  static constexpr int LEDS_OFF = 0;
  static constexpr int DISCO = 1;
  static constexpr int FITBIT_GOALS = 2;

  CRGBArray<NUM_LEDS> sleepLeds;
  CRGBArray<NUM_LEDS> waterGoalLeds;
  CRGBArray<NUM_LEDS> calsGoalLeds;
  CRGBArray<NUM_LEDS> stepsGoalLeds;

  bool isOn = false;
  long red, green, blue, ledIndex;
  String currentModeAsString = String("off");
  int currentModeInt = LEDS_OFF;

  void drawCompletion(CRGB leds[], float completionPercent, CRGB color) {

    // Calculate how many LEDs should be green
    //    int numGreenLeds = (completionPercent / 100.0) * NUM_LEDS;
    int numGreenLeds = (completionPercent)*NUM_LEDS;
    //Serial.print("Percent to draw is ");
    //Serial.println(numGreenLeds);
    // Set the appropriate number of LEDs to green
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < numGreenLeds) {
        leds[i] = color;
      } else {
        leds[i] = CRGB::Black;  // Turn off remaining LEDs
      }
    }

    //          for (int i = 0; i < NUM_LEDS; i++) {
    //          red = 0;
    //        green = 155;
    //      blue = 0;
    //    leds[i] = CRGB(red, green, blue);
    //}
    FastLED.show();
  }

  void drawDisco(CRGB leds[]) {
    for (int i = 0; i < NUM_LEDS; i++) {
      red = random(255);
      green = random(255);
      blue = random(255);
      ledIndex = random(NUM_LEDS);
      leds[ledIndex] = CRGB(red, blue, green);
      FastLED.show();
      delay(75);
      //      leds[i] = CRGB(red, green, blue);
    }
    FastLED.show();
  }

public:
  explicit LedModeHandler(Context& ctx)
    : Handler(ctx) {
    FastLED.addLeds<WS2812, SLEEP_LED_PIN, GRB>(sleepLeds, NUM_LEDS);
    FastLED.addLeds<WS2812, WATER_LED_PIN, GRB>(waterGoalLeds, NUM_LEDS);
    FastLED.addLeds<WS2812, CALS_LED_PIN, GRB>(calsGoalLeds, NUM_LEDS);
    FastLED.addLeds<WS2812, STEPS_LED_PIN, GRB>(stepsGoalLeds, NUM_LEDS);
  }

  void handle(DynamicJsonDocument& doc) override {
    if (doc.containsKey("setMode")) {
      String currentMode = doc["setMode"];
      currentModeAsString = currentMode;
      if (currentMode.compareTo("contextDriven") == 0) {
        isOn = true;
        Serial.println("Setting context driven");
        currentModeInt = FITBIT_GOALS;
      } else if (currentMode.compareTo("disco") == 0) {
        Serial.println("Setting disco");
        currentModeInt = DISCO;
      } else if (currentMode.compareTo("off") == 0) {
        currentModeInt = LEDS_OFF;
        Serial.println("Setting off");
      }
    } else if (doc.containsKey("powerstate")) {
      String powerstateCommand = doc["powerstate"];
      if (powerstateCommand.compareTo("TurnOn") == 0) {
        Serial.println("Setting context driven");
        currentModeInt = FITBIT_GOALS;
      } else {
        Serial.println("Setting off");
        currentModeInt = LEDS_OFF;
      }
    } else if (doc.containsKey("adjustBrightnessPercent")) {
      int brightnessPercent = doc["adjustBrightnessPercent"];
      int mappedBrightness = map(brightnessPercent, 0, 100, 0, 255);
      Serial.print("Setting brightness to ");
      Serial.print(mappedBrightness);
      Serial.print(" from ");
      Serial.println(brightnessPercent);
      FastLED.setBrightness(mappedBrightness);
      FastLED.show();
    }
    Handler::handle(doc);
  }

  String run() {

    switch (currentModeInt) {
      case FITBIT_GOALS:
        if (isOn) {

          drawCompletion(waterGoalLeds, context.waterGoalReachedPercent, CRGB::Blue);
          drawCompletion(sleepLeds, context.sleepGoalReachedPercent, CRGB::Green);
          drawCompletion(calsGoalLeds, context.caloriesGoalReachedPercent, CRGB::Red);
          drawCompletion(stepsGoalLeds, context.stepsGoalReachedPercent, CRGB(255, 165, 0));
          FastLED.show();
          delay(50);
        }
        //        Serial.println("Updated for fitbit goals");
        isOn = false;
        //        delay(75);

        // statements
        break;
      case DISCO:
        drawDisco(waterGoalLeds);
        drawDisco(sleepLeds);
        drawDisco(calsGoalLeds);
        drawDisco(stepsGoalLeds);

        break;
      case LEDS_OFF:
        drawCompletion(waterGoalLeds, context.waterGoalReachedPercent, CRGB::Black);
        drawCompletion(sleepLeds, context.sleepGoalReachedPercent, CRGB::Black);
        drawCompletion(calsGoalLeds, context.caloriesGoalReachedPercent, CRGB::Black);
        drawCompletion(stepsGoalLeds, context.weightGoalReachedPercent, CRGB::Black);
        FastLED.show();
        break;
      default:
        // statements
        break;
    }
    return currentModeAsString;
  }
};
