#include <Arduino.h>
#include "Context.h"

class MyContext {
public:
    float waterGoalReachedPercent = 0.0f;
    float caloriesGoalReachedPercent = 0.0f;
    float weightGoalReachedPercent = 0.0f;
    float sleepGoalReachedPercent = 0.0f;

    void printStatus() const {
        Serial.print("Water Goal: ");
        Serial.print(waterGoalReachedPercent);
        Serial.print("%, Calories Goal: ");
        Serial.print(caloriesGoalReachedPercent);
        Serial.print("%, Weight Goal: ");
        Serial.print(weightGoalReachedPercent);
        Serial.print("%, Sleep Goal: ");
        Serial.print(sleepGoalReachedPercent);
        Serial.println("%");
    }
};
