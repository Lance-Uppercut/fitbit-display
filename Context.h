#pragma once

#ifndef CONTEXT_H
#define CONTEXT_H

class Context {
public:
    float waterGoalReachedPercent = 0.0f;
    float caloriesGoalReachedPercent = 0.0f;
    float sleepGoalReachedPercent = 0.0f;
    float weightReachedPercent = 0.0f;
    float currentWeight = 0.0f;
    void printStatus() const;
};

#endif
