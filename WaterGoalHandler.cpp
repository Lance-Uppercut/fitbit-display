#include <ArduinoJson.h>
#include "Handler.h"  // Include the correct header for Handler class

class WaterGoalHandler : public Handler {
private:
  float water;
  float goal;
public:
  explicit WaterGoalHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    //{"endpointId":"${endpointId}","fitbit.get.water":{"water":0.0}}
    //{"endpointId":"${endpointId}","fitbit.get.water.goal":{"goal":24,"startDate":"2019-03-21"}}

    if (doc.containsKey("fitbit.get.water")) {
      water = doc["fitbit.get.water"]["water"];
    } else if (doc.containsKey("fitbit.get.water.goal")) {
      goal = doc["fitbit.get.water.goal"]["goal"];
      context.waterGoalReachedPercent = (water / goal);
    }

    Handler::handle(doc);
  }
};
