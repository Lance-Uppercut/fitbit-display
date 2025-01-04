#include <ArduinoJson.h>
#include "Handler.h"      // Include the correct header for Handler class
//#include "Context.h"      // Include the Context header to access the Context class

class WaterGoalHandler : public Handler {
public:
  explicit WaterGoalHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    if (doc.containsKey("waterIncrement")) {
      float increment = doc["waterIncrement"];
      context.waterGoalReachedPercent += increment;
      if (context.waterGoalReachedPercent > 100.0f) {
        context.waterGoalReachedPercent = 100.0f;
      }
    }
    Handler::handle(doc);
  }
};
