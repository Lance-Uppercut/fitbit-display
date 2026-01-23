#include <ArduinoJson.h>
#include "Handler.h"  // Include the correct header for Handler class

class StepGoalsHandler : public Handler {
public:
  explicit StepGoalsHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    if (doc.containsKey("steps")) {
      int steps = doc["steps"];
      context.stepsGoalReachedPercent = steps / 8000;
      Serial.print("Steps goal: ");
      Serial.println(context.stepsGoalReachedPercent);
    }
    Handler::handle(doc);
  }
};
