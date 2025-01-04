#ifndef HANDLER_H
#define HANDLER_H
#include "Context.h"
#include <ArduinoJson.h>  // Include ArduinoJson header to use DynamicJsonDocument

class Context; // Forward declaration of Context

class Handler {
protected:
    Context& context;
    Handler* nextHandler = nullptr;

public:
    explicit Handler(Context& ctx);
    virtual ~Handler();

    void setNext(Handler* handler);
    virtual void handle(DynamicJsonDocument& doc);
};

#endif
