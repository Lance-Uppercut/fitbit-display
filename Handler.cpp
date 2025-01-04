#include "Handler.h"
#include <ArduinoJson.h>
#include "Context.h"  // Include Context to make sure it's available

Handler::Handler(Context& ctx)
  : context(ctx) {}

Handler::~Handler() {}

void Handler::setNext(Handler* handler) {
  nextHandler = handler;
}

void Handler::handle(DynamicJsonDocument& doc) {
  if (nextHandler) {
    nextHandler->handle(doc);
  }
}
