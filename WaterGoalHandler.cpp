#include <ArduinoJson.h>
#include "Handler.h"
#include <TelnetStream.h>

class WaterGoalHandler : public Handler {
private:
  float water = 0.0f;
  float goal = 0.0f;
public:
  explicit WaterGoalHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    //{"endpointId":"${endpointId}","fitbit.get.water":{"water":0.0}}
    //{"endpointId":"${endpointId}","fitbit.get.water.goal":{"goal":24,"startDate":"2019-03-21"}}

    if (doc.containsKey("fitbit.get.water")) {
      JsonVariant wv = doc["fitbit.get.water"];
      if (wv.is<JsonObject>()) {
        water = wv["water"];
      } else {
        water = wv.as<float>();
      }
    }
    if (doc.containsKey("fitbit.get.water.goal")) {
      JsonVariant gv = doc["fitbit.get.water.goal"];
      if (gv.is<JsonObject>()) {
        goal = gv["goal"];
      } else {
        goal = gv.as<float>();
      }
    }

    if (goal > 0.0f) {
      context.waterGoalReachedPercent = water / goal;
      if (context.waterGoalReachedPercent < 0.0f) {
        context.waterGoalReachedPercent = 0.0f;
      } else if (context.waterGoalReachedPercent > 1.0f) {
        context.waterGoalReachedPercent = 1.0f;
      }
    }

    Serial.print("Water goal: ");
    Serial.print(goal);
    Serial.print(" Water: ");
    Serial.print(water);
    Serial.print(" ratio: ");
    Serial.println(context.waterGoalReachedPercent);
    TelnetStream.print("Water goal: ");
    TelnetStream.print(goal);
    TelnetStream.print(" Water: ");
    TelnetStream.print(water);
    TelnetStream.print(" ratio: ");
    TelnetStream.println(context.waterGoalReachedPercent);

    Handler::handle(doc);
  }
};
