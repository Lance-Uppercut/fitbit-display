#include <ArduinoJson.h>
#include "Handler.h"  // Include the correct header for Handler class

class StepGoalsHandler : public Handler {
public:
  explicit StepGoalsHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    if (doc.containsKey("steps")) {
      int steps = doc["steps"];
      context.stepsGoalReachedPercent = steps / 8000.0f;
      if (context.stepsGoalReachedPercent < 0.0f) {
        context.stepsGoalReachedPercent = 0.0f;
      } else if (context.stepsGoalReachedPercent > 1.0f) {
        context.stepsGoalReachedPercent = 1.0f;
      }
      Serial.print("Steps goal: ");
      Serial.println(context.stepsGoalReachedPercent);
    }
    Handler::handle(doc);
  }
};
