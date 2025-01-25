#include <ArduinoJson.h>
#include "Handler.h"  // Include the correct header for Handler class

class SleepGoalHandler : public Handler {
private:
  float totalMinutesAsleep = -1.0;
  float minDuration = -1.0;

public:
  explicit SleepGoalHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    if (doc.containsKey("fitbit.get.sleep")) {
      totalMinutesAsleep = doc["fitbit.get.sleep"]["totalMinutesAsleep"];
    } else if (doc.containsKey("fitbit.get.sleep.goal")) {
      minDuration = doc["fitbit.get.sleep.goal"]["goal.minDuration"];
    }

    if (totalMinutesAsleep != -1.0 && minDuration != -1.0) {
      context.sleepGoalReachedPercent = totalMinutesAsleep / minDuration;
    }
    Serial.print("Sleep goal: ");
    Serial.print(" totalMinutesAsleep: ");
    Serial.print(totalMinutesAsleep);
    Serial.print(" minDuration: ");
    Serial.print(minDuration);
    Serial.print("%: ");
    Serial.println(context.sleepGoalReachedPercent);



    Handler::handle(doc);
  }
};
