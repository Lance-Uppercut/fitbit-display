#pragma once

#include <ArduinoJson.h>
#include "Handler.h"  // Include the correct header for Handler class

class CaloriesGoalHandler : public Handler {
private:
  int caloriesOut;
public:
  explicit CaloriesGoalHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    //[WSc] get text: {"endpointId":"CjvJ39w8","caloriesOut":2284,"activityCalories":564,"steps":1371,"veryActiveMinutes":26,"caloriesBMR":1777,"sedentaryMinutes":1245}
    //{"endpointId":"${endpointId}","caloriesOut":2628,"activityCalories":525,"steps":1698,"veryActiveMinutes":0,"caloriesBMR":1973,"sedentaryMinutes":802}

    if (doc.containsKey("caloriesOut")) {
      caloriesOut = doc["caloriesOut"];
      context.caloriesGoalReachedPercent = caloriesOut / 2500.0;
    }


    Handler::handle(doc);
  }
};
