#include <ArduinoJson.h>
#include "Handler.h"  // Include the correct header for Handler class

class WeightGoalHandler : public Handler {
private:
  float currentWeight;
public:
  explicit WeightGoalHandler(Context& ctx)
    : Handler(ctx) {}

  void handle(DynamicJsonDocument& doc) override {
    //[WSc] get text: {"endpointId":"CjvJ39w8","weight":100,"weightUnit":"METRIC"}

    if (doc.containsKey("weight")) {
      float weight = doc["weight"];
      currentWeight = weight;
      Serial.printf("Weight: %3.2f\n", weight);
    } else if (doc.containsKey("bmi")) {
      float bmi = doc["bmi"];
      Serial.printf("bmi: %2.2f\n", bmi);
    } else if (doc.containsKey("weightUnit")) {
      String weightUnit = doc["weightUnit"];
      Serial.printf("weightUnit: %s\n", weightUnit);
    } else if (doc.containsKey("weightGoal")) {
      Serial.println("Parsing1");
      Serial.flush();
      //{"endpointId":weight"CjvJ39w8","weightGoal":{"goalType":"LOSE","startDate":"2024-11-26","startWeight":100.0,"weight":98.0,"weightThreshold":0.05}}
      float startWeight = doc["weightGoal"]["startWeight"];
      Serial.printf("startWeight: %3.2f\n", startWeight);
      float targetWeight = doc["weightGoal"]["weight"];
      Serial.printf("targetWeight: %3.2f\n", targetWeight);
      Serial.printf("currentWeight: %3.2f\n", currentWeight);

      float weightLossInPercent = (startWeight - currentWeight) / (currentWeight - targetWeight);
      Serial.printf("weightLossInPercent: %3.2f\n", weightLossInPercent);
      context.weightReachedPercent = weightLossInPercent;
    }
    Handler::handle(doc);
  }
};
