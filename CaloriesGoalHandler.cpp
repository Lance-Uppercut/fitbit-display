#include <ArduinoJson.h>
#include "Handler.h"      // Include the correct header for Handler class
//#include "Context.h"      // Include the Context header to access the Context class

class CaloriesGoalHandler : public Handler {
public:
  explicit CaloriesGoalHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    if (doc.containsKey("caloriesIncrement")) {
      float increment = doc["caloriesIncrement"];
      context.caloriesGoalReachedPercent += increment;
      if (context.caloriesGoalReachedPercent > 100.0f) {
        context.caloriesGoalReachedPercent = 100.0f;
      }
    }
    Handler::handle(doc);
  }
};
