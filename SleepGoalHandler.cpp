#include <ArduinoJson.h>
#include "Handler.h"      // Include the correct header for Handler class
//#include "Context.h"      // Include the Context header to access the Context class

class SleepGoalHandler : public Handler {
public:
  explicit SleepGoalHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    if (doc.containsKey("sleepIncrement")) {
      float increment = doc["sleepIncrement"];
      context.sleepGoalReachedPercent += increment;
      if (context.sleepGoalReachedPercent > 100.0f) {
        context.sleepGoalReachedPercent = 100.0f;
      }
    }
    Handler::handle(doc);
  }
};
