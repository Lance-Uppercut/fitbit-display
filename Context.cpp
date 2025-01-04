#include <Arduino.h>

class Context {
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
        Serial.print("%, Sleep Goal: ");
        Serial.print(sleepGoalReachedPercent);
        Serial.println("%");
    }
};
