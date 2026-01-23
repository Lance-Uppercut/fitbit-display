#pragma once

#ifndef CONTEXT_H
#define CONTEXT_H
#include <Arduino.h>

class Context {
public:
  float waterGoalReachedPercent = 0.0f;
  float caloriesGoalReachedPercent = 0.0f;
  float sleepGoalReachedPercent = 0.0f;
  float weightGoalReachedPercent = 0.0f;
  float currentWeight = 0.0f;
  float stepsGoalReachedPercent = 0.0f;
  
  void printStatus(Stream& stream) const {
    stream.print("Water Goal: ");
    stream.print(waterGoalReachedPercent);
    stream.print("%, Calories Goal: ");
    stream.print(caloriesGoalReachedPercent);
    stream.print("%, Steps Goal: ");
    stream.print(stepsGoalReachedPercent);
    stream.print("%, Sleep Goal: ");
    stream.print(sleepGoalReachedPercent);
    stream.println("%");
  }
};

#endif
