//DOIT ESP32 DEVKIT V1 worked
#define ESP32 123

#include "OTA.h"
#include <credentials.h>
#include <WebSocketsClient.h>
#include <SimpleTimer.h>
#include <ArduinoJson.h>
#include <cbor.h>
#include "WaterGoalHandler.cpp"
#include "SleepGoalHandler.cpp"
#include "CaloriesGoalHandler.cpp"
#include "WeightGoalHandler.cpp"
#include "LedModeHandler.cpp"
#include "StepGoalsHandler.cpp"
#include "Context.h"
//#include <ESP8266httpUpdate.h>
//#include <ESP8266HTTPClient.h>
#include "buildTime.h"
#include "version.h"
#include <DHT.h>


//#define DHTPIN 32      //(rød gpio0, pin D3) (3,3 volt)
#define DHTPIN 25      //(rød gpio0, pin D3) (3,3 volt)
#define DHTTYPE DHT11  // DHT 11
//#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321
float temperature = 0;
float humidity = 0;
DHT dht(DHTPIN, DHTTYPE);


WiFiClient http;

const int ledPin = LED_BUILTIN;  // the number of the LED pin
int ledState = LOW;              // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;  // will store last time LED was updated

WebSocketsClient webSocketClient;
//for prod
//char host[] = "www.offbeat-iot.com";
char host[] = "offbeat-iot.com";

char user[] = "k3rbg";
char socketPassword[] = "d09VJd47";
char path[] = "/ws?device=CjvJ39w8";
char deviceId[] = "CjvJ39w8";
const char websocketHeaders[] = "Accept: application/cbor\r\nAuthorization: Basic azNyYmc6ZDA5VkpkNDc=";

#define USE_SERIAL Serial

// Create the context
Context context;

// Create handlers
WaterGoalHandler waterHandler(context);
CaloriesGoalHandler caloriesHandler(context);
SleepGoalHandler sleepHandler(context);
WeightGoalHandler weightGoalHandler(context);
LedModeHandler ledModeHandler(context);
StepGoalsHandler stepsGoalHandler(context);
SimpleTimer timer;


String currentModeAsString = String("off");
constexpr size_t OUTBOUND_CBOR_BUFFER_SIZE = 384;
constexpr size_t INBOUND_DOC_OVERHEAD = 512;
struct CborCursor;
constexpr unsigned long WIFI_RECONNECT_INTERVAL_MS = 15000;
unsigned long lastWifiReconnectAttemptAt = 0;
bool webSocketStarted = false;

void ensureWifiConnected();
void startWebSocketIfNeeded();
void logDnsResolution(const char* targetHost);
#if defined(ESP32)
void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
#endif

void turnOnLed() {
  digitalWrite(LED_BUILTIN, HIGH);
}
void turnOffLed() {
  digitalWrite(LED_BUILTIN, LOW);
}


bool sendStatusAsCbor(const char* state, const char* newState) {
  if (state == nullptr) {
    return false;
  }
  if (newState == nullptr) {
    newState = "";
  }

  uint8_t payload[OUTBOUND_CBOR_BUFFER_SIZE];
  CborEncoder encoder;
  CborEncoder rootMap;
  cbor_encoder_init(&encoder, payload, sizeof(payload), 0);

  CborError error = cbor_encoder_create_map(&encoder, &rootMap, 2);
  if (error == CborNoError) {
    error = cbor_encode_text_stringz(&rootMap, "deviceId");
  }
  if (error == CborNoError) {
    error = cbor_encode_text_stringz(&rootMap, deviceId);
  }
  if (error == CborNoError) {
    error = cbor_encode_text_stringz(&rootMap, state);
  }
  if (error == CborNoError) {
    error = cbor_encode_text_stringz(&rootMap, newState);
  }
  if (error == CborNoError) {
    error = cbor_encoder_close_container(&encoder, &rootMap);
  }
  if (error != CborNoError) {
    return false;
  }

  size_t payloadLength = cbor_encoder_get_buffer_size(&encoder, payload);
  webSocketClient.sendBIN(payload, payloadLength);
  return true;
}

void updateStatus(const char* currentDeviceId, const char* state, const char* newState) {
  if (currentDeviceId == nullptr || !webSocketClient.isConnected()) {
    Serial.println("Not connected when updating status");
    return;
  }

  bool sent = sendStatusAsCbor(state, newState);
  if (sent) {
    TelnetStream.printf("Sent CBOR command: %s=%s\r\n", state, newState == nullptr ? "" : newState);
  } else {
    Serial.println("Failed to encode CBOR status message");
  }
}

void reportVersion() {
  char theVersion[256];
  sprintf(theVersion, "%04d%02d%02d-%02d_%02d_%02d", BUILD_YEAR, BUILD_MONTH, BUILD_DAY, BUILD_HOUR, BUILD_MIN, BUILD_SEC);
  Serial.print(F("version: "));
  Serial.println(theVersion);
  updateStatus(deviceId, "calculatedVersion", theVersion);
}

char ipaddressString[256];
void reportIPAddress() {
  // send message to server when Connected
  IPAddress ipAddress = WiFi.localIP();
  sprintf(ipaddressString, "%d.%d.%d.%d", ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3]);
  Serial.println(ipaddressString);

  updateStatus(deviceId, "ipaddress", ipaddressString);
}

void updateBrightness() {
  updateStatus(deviceId, "sync", "adjustBrightnessPercent");
}
void updateMode() {
  updateStatus(deviceId, "sync", "setMode");
  timer.setTimeout(500, updateBrightness);
}
void getWater() {
  updateStatus(deviceId, "fitbit.get.water", "");
}
void getSleep() {
  updateStatus(deviceId, "fitbit.get.sleep", "");
}

void getSleepGoal() {
  updateStatus(deviceId, "fitbit.get.sleep.goal", "");
}

void getWaterGoal() {
  //{"endpointId":"${endpointId}","fitbit.get.water.goal":{"goal":24,"startDate":"2019-03-21"}}
  updateStatus(deviceId, "fitbit.get.water.goal", "");
}

void getFitbitWeight() {
  Serial.println("Getting fitbit weight");
  updateStatus(deviceId, "fitbit.get.weight", "");
}

void getFitbitWeightGoal() {
  //taking the easy road of just sending strings
  Serial.println("Getting fitbit weightgoal");
  updateStatus(deviceId, "fitbit.get.weight.goal", "");
  //  webSocketClient.sendTXT("{\"fitbit.get.weight.goal\":\"\"}");
}
void getFitbitDailyActivities() {
  //taking the easy road of just sending strings
  Serial.println("Getting fitbit activities");
  updateStatus(deviceId, "fitbit.activities", "");
  //  webSocketClient.sendTXT("{\"fitbit.get.weight.goal\":\"\"}");
}

void getCurrentWeight() {
  updateStatus(deviceId, "fitbit.get.current.weight", "");
}

void sendNextFitbitRequest() {
  static int step = 0;
  switch (step) {
    case 0: getFitbitWeight(); break;
    case 1: getFitbitWeightGoal(); break;
    case 2: getFitbitDailyActivities(); break;
    case 3: getCurrentWeight(); break;
    case 4: getWater(); break;
    case 5: getWaterGoal(); break;
    case 6: getSleep(); break;
    case 7: getSleepGoal(); break;
    default: step = 0; return;
  }
  step++;
  if (step < 8) {
    timer.setTimeout(1000, sendNextFitbitRequest);
  } else {
    step = 0;
  }
}

void updateFitbitValues() {
  sendNextFitbitRequest();
}


boolean shouldReport = false;
boolean pumpOn = false;
void turnOnPump() {
  turnOnLed();
  pumpOn = true;
  updateStatus(deviceId, "powerstate", "On");
}

void turnOffPump() {
  turnOffLed();
  pumpOn = false;
  updateStatus(deviceId, "powerstate", "Off");
}

void reportMode() {
  updateStatus(deviceId, "ledControl", (char*)currentModeAsString.c_str());
}
// a function to be executed periodically

void repeatMe() {
  Serial.print("Uptime (s): ");
  Serial.println(millis() / 1000);
}

boolean gotPing = false;
void reconnectIfNoPing() {

  if (!gotPing) {
    TelnetStream.println(F("No ping received, reconnecting"));
    USE_SERIAL.println(F("No ping received, reconnecting"));
    //webSocketClient.disconnect();
    //ESP.restart();
  } else {
    gotPing = false;
  }
}

void logDnsResolution(const char* targetHost) {
  if (targetHost == nullptr || WiFi.status() != WL_CONNECTED) {
    return;
  }

  IPAddress resolved;
  if (WiFi.hostByName(targetHost, resolved)) {
    USE_SERIAL.printf("DNS %s -> %s\r\n", targetHost, resolved.toString().c_str());
  } else {
    USE_SERIAL.printf("DNS lookup failed for %s (status=%d)\r\n", targetHost, static_cast<int>(WiFi.status()));
  }
}

#if defined(ESP32)
void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      USE_SERIAL.printf("[WiFi] disconnected (reason=%d)\r\n", static_cast<int>(info.wifi_sta_disconnected.reason));
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      USE_SERIAL.printf("[WiFi] connected with IP %s\r\n", WiFi.localIP().toString().c_str());
      logDnsResolution(host);
      break;
    default:
      break;
  }
}
#endif

void ensureWifiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long now = millis();
  if ((now - lastWifiReconnectAttemptAt) < WIFI_RECONNECT_INTERVAL_MS) {
    return;
  }

  lastWifiReconnectAttemptAt = now;
  USE_SERIAL.printf("[WiFi] offline (status=%d), retrying connect\r\n", static_cast<int>(WiFi.status()));
  WiFi.disconnect(false, false);
  WiFi.begin(mySSID, myPASSWORD);
}

void startWebSocketIfNeeded() {
  if (webSocketStarted || WiFi.status() != WL_CONNECTED) {
    return;
  }

  Serial.println("Connecting to websocket");
  logDnsResolution(host);
  webSocketClient.beginSSL(host, 443, path, "", "cbor");
  webSocketStarted = true;
}


void calculateTempHum() {

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    USE_SERIAL.println("Failed to read from DHT sensor!");
    //turnOffDht();
    //timer.setTimeout(2500, turnOnDht);
    return;
  }

  temperature = t;
  humidity = h;

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
  TelnetStream.printf("Humidity: %.2f.%% Temperature: %0.f *C, Heat index: %.2f *C. %.2f *F\r\n", h, t, hic, hif);
  USE_SERIAL.print("Humidity: ");
  USE_SERIAL.print(h);
  USE_SERIAL.print(" %\t");
  USE_SERIAL.print("Temperature: ");
  USE_SERIAL.print(t);
  USE_SERIAL.print(" *C ");
  USE_SERIAL.print(f);
  USE_SERIAL.print(" *F\t");
  USE_SERIAL.print("Heat index: ");
  USE_SERIAL.print(hic);
  USE_SERIAL.print(" *C ");
  USE_SERIAL.print(hif);
  USE_SERIAL.println(" *F");
}


bool readCborTextString(CborValue* value, String& out) {
  size_t textLength = 0;
  CborError error = cbor_value_calculate_string_length(value, &textLength);
  if (error != CborNoError) {
    return false;
  }

  char* buffer = static_cast<char*>(malloc(textLength + 1));
  if (buffer == nullptr) {
    return false;
  }

  error = cbor_value_copy_text_string(value, buffer, &textLength, value);
  if (error != CborNoError) {
    free(buffer);
    return false;
  }
  buffer[textLength] = '\0';
  out = String(buffer);
  free(buffer);
  return true;
}

bool readCborByteStringAsHex(CborValue* value, String& out) {
  size_t length = 0;
  CborError error = cbor_value_calculate_string_length(value, &length);
  if (error != CborNoError) {
    return false;
  }

  uint8_t* buffer = static_cast<uint8_t*>(malloc(length));
  if (buffer == nullptr) {
    return false;
  }

  error = cbor_value_copy_byte_string(value, buffer, &length, value);
  if (error != CborNoError) {
    free(buffer);
    return false;
  }

  out = "";
  out.reserve(length * 2);
  for (size_t i = 0; i < length; i++) {
    char hex[3];
    sprintf(hex, "%02X", buffer[i]);
    out += hex;
  }
  free(buffer);
  return true;
}

String bytesToHex(const uint8_t* payload, size_t length) {
  String out = "";
  out.reserve(length * 2);
  for (size_t i = 0; i < length; i++) {
    char hex[3];
    sprintf(hex, "%02X", payload[i]);
    out += hex;
  }
  return out;
}

bool readCborFloatingPointValue(CborValue* value, double& out) {
  CborType type = cbor_value_get_type(value);

  if (type == CborFloatType) {
    uint32_t raw = (value->flags & CborIteratorFlag_IntegerValueTooLarge)
      ? static_cast<uint32_t>(_cbor_value_decode_int64_internal(value))
      : static_cast<uint32_t>(value->extra);
    float floatValue = 0.0f;
    memcpy(&floatValue, &raw, sizeof(floatValue));
    out = static_cast<double>(floatValue);
    return true;
  }

  if (type == CborDoubleType) {
    uint64_t raw = (value->flags & CborIteratorFlag_IntegerValueTooLarge)
      ? _cbor_value_decode_int64_internal(value)
      : static_cast<uint64_t>(value->extra);
    double doubleValue = 0.0;
    memcpy(&doubleValue, &raw, sizeof(doubleValue));
    out = doubleValue;
    return true;
  }

  return false;
}

struct CborCursor {
  const uint8_t* data;
  size_t length;
  size_t position;
};

bool cborReadByte(CborCursor& cursor, uint8_t& out) {
  if (cursor.position >= cursor.length) {
    return false;
  }
  out = cursor.data[cursor.position++];
  return true;
}

bool cborPeekByte(const CborCursor& cursor, uint8_t& out) {
  if (cursor.position >= cursor.length) {
    return false;
  }
  out = cursor.data[cursor.position];
  return true;
}

bool cborReadUint(CborCursor& cursor, uint8_t additionalInfo, uint64_t& out) {
  if (additionalInfo < 24) {
    out = additionalInfo;
    return true;
  }

  if (additionalInfo == 24) {
    uint8_t value8 = 0;
    if (!cborReadByte(cursor, value8)) {
      return false;
    }
    out = value8;
    return true;
  }

  if (additionalInfo == 25) {
    uint8_t b0 = 0;
    uint8_t b1 = 0;
    if (!cborReadByte(cursor, b0) || !cborReadByte(cursor, b1)) {
      return false;
    }
    out = (static_cast<uint64_t>(b0) << 8) | static_cast<uint64_t>(b1);
    return true;
  }

  if (additionalInfo == 26) {
    uint8_t b0 = 0;
    uint8_t b1 = 0;
    uint8_t b2 = 0;
    uint8_t b3 = 0;
    if (!cborReadByte(cursor, b0) || !cborReadByte(cursor, b1) || !cborReadByte(cursor, b2) || !cborReadByte(cursor, b3)) {
      return false;
    }
    out = (static_cast<uint64_t>(b0) << 24) |
      (static_cast<uint64_t>(b1) << 16) |
      (static_cast<uint64_t>(b2) << 8) |
      static_cast<uint64_t>(b3);
    return true;
  }

  if (additionalInfo == 27) {
    uint8_t bytes[8];
    for (size_t i = 0; i < 8; i++) {
      if (!cborReadByte(cursor, bytes[i])) {
        return false;
      }
    }
    out = (static_cast<uint64_t>(bytes[0]) << 56) |
      (static_cast<uint64_t>(bytes[1]) << 48) |
      (static_cast<uint64_t>(bytes[2]) << 40) |
      (static_cast<uint64_t>(bytes[3]) << 32) |
      (static_cast<uint64_t>(bytes[4]) << 24) |
      (static_cast<uint64_t>(bytes[5]) << 16) |
      (static_cast<uint64_t>(bytes[6]) << 8) |
      static_cast<uint64_t>(bytes[7]);
    return true;
  }

  return false;
}

bool decodeCborItem(CborCursor& cursor, JsonVariant target);

bool decodeCborTextString(CborCursor& cursor, String& out) {
  uint8_t initialByte = 0;
  if (!cborReadByte(cursor, initialByte)) {
    return false;
  }

  uint8_t majorType = initialByte >> 5;
  uint8_t additionalInfo = initialByte & 0x1f;

  if (majorType == 6) {
    uint64_t ignoredTag = 0;
    if (!cborReadUint(cursor, additionalInfo, ignoredTag)) {
      return false;
    }
    return decodeCborTextString(cursor, out);
  }

  if (majorType != 3) {
    return false;
  }

  if (additionalInfo == 31) {
    out = "";
    while (true) {
      uint8_t nextByte = 0;
      if (!cborPeekByte(cursor, nextByte)) {
        return false;
      }
      if (nextByte == 0xff) {
        cursor.position++;
        return true;
      }

      String chunk;
      if (!decodeCborTextString(cursor, chunk)) {
        return false;
      }
      out += chunk;
    }
  }

  uint64_t length = 0;
  if (!cborReadUint(cursor, additionalInfo, length)) {
    return false;
  }

  if (length > (cursor.length - cursor.position)) {
    return false;
  }

  out.reserve(static_cast<size_t>(length));
  out = "";
  for (uint64_t i = 0; i < length; i++) {
    out += static_cast<char>(cursor.data[cursor.position++]);
  }
  return true;
}

bool decodeCborMap(CborCursor& cursor, uint8_t additionalInfo, JsonVariant target) {
  JsonObject object = target.to<JsonObject>();
  USE_SERIAL.printf("[CBOR] map decode start ai=%u\r\n", additionalInfo);

  if (additionalInfo == 31) {
    while (true) {
      uint8_t nextByte = 0;
      if (!cborPeekByte(cursor, nextByte)) {
        return false;
      }
      if (nextByte == 0xff) {
        cursor.position++;
        break;
      }

      String key;
      if (!decodeCborTextString(cursor, key)) {
        return false;
      }
      USE_SERIAL.printf("[CBOR] key=%s\r\n", key.c_str());
      object[key] = nullptr;
      JsonVariant child = object[key];
      if (!decodeCborItem(cursor, child)) {
        return false;
      }
    }
    USE_SERIAL.printf("[CBOR] map size=%u\r\n", static_cast<unsigned int>(object.size()));
    return true;
  }

  uint64_t pairCount = 0;
  if (!cborReadUint(cursor, additionalInfo, pairCount)) {
    return false;
  }

  for (uint64_t i = 0; i < pairCount; i++) {
    String key;
    if (!decodeCborTextString(cursor, key)) {
      return false;
    }
    USE_SERIAL.printf("[CBOR] key=%s\r\n", key.c_str());
    object[key] = nullptr;
    JsonVariant child = object[key];
    if (!decodeCborItem(cursor, child)) {
      return false;
    }
  }

  USE_SERIAL.printf("[CBOR] map size=%u\r\n", static_cast<unsigned int>(object.size()));
  return true;
}

bool decodeCborArray(CborCursor& cursor, uint8_t additionalInfo, JsonVariant target) {
  JsonArray array = target.to<JsonArray>();

  if (additionalInfo == 31) {
    while (true) {
      uint8_t nextByte = 0;
      if (!cborPeekByte(cursor, nextByte)) {
        return false;
      }
      if (nextByte == 0xff) {
        cursor.position++;
        break;
      }

      JsonVariant child = array.add();
      if (!decodeCborItem(cursor, child)) {
        return false;
      }
    }
    return true;
  }

  uint64_t itemCount = 0;
  if (!cborReadUint(cursor, additionalInfo, itemCount)) {
    return false;
  }

  for (uint64_t i = 0; i < itemCount; i++) {
    JsonVariant child = array.add();
    if (!decodeCborItem(cursor, child)) {
      return false;
    }
  }

  return true;
}

bool decodeCborItem(CborCursor& cursor, JsonVariant target) {
  uint8_t initialByte = 0;
  if (!cborReadByte(cursor, initialByte)) {
    return false;
  }

  uint8_t majorType = initialByte >> 5;
  uint8_t additionalInfo = initialByte & 0x1f;

  if (majorType == 0) {
    uint64_t value = 0;
    if (!cborReadUint(cursor, additionalInfo, value)) {
      return false;
    }
    target.set(value);
    return true;
  }

  if (majorType == 1) {
    uint64_t encoded = 0;
    if (!cborReadUint(cursor, additionalInfo, encoded)) {
      return false;
    }
    int64_t value = -1 - static_cast<int64_t>(encoded);
    target.set(value);
    return true;
  }

  if (majorType == 2) {
    if (additionalInfo == 31) {
      return false;
    }
    uint64_t length = 0;
    if (!cborReadUint(cursor, additionalInfo, length)) {
      return false;
    }
    if (length > (cursor.length - cursor.position)) {
      return false;
    }
    String hex = "";
    hex.reserve(static_cast<size_t>(length) * 2);
    for (uint64_t i = 0; i < length; i++) {
      char nibble[3];
      sprintf(nibble, "%02X", cursor.data[cursor.position++]);
      hex += nibble;
    }
    target.set(hex);
    return true;
  }

  if (majorType == 3) {
    cursor.position--;
    String text;
    if (!decodeCborTextString(cursor, text)) {
      return false;
    }
    target.set(text);
    return true;
  }

  if (majorType == 4) {
    return decodeCborArray(cursor, additionalInfo, target);
  }

  if (majorType == 5) {
    return decodeCborMap(cursor, additionalInfo, target);
  }

  if (majorType == 6) {
    uint64_t ignoredTag = 0;
    if (!cborReadUint(cursor, additionalInfo, ignoredTag)) {
      return false;
    }
    return decodeCborItem(cursor, target);
  }

  if (majorType == 7) {
    if (additionalInfo == 20) {
      target.set(false);
      return true;
    }
    if (additionalInfo == 21) {
      target.set(true);
      return true;
    }
    if (additionalInfo == 22 || additionalInfo == 23) {
      target.set(nullptr);
      return true;
    }
    if (additionalInfo == 26) {
      uint64_t raw = 0;
      if (!cborReadUint(cursor, additionalInfo, raw)) {
        return false;
      }
      uint32_t raw32 = static_cast<uint32_t>(raw);
      float value = 0.0f;
      memcpy(&value, &raw32, sizeof(value));
      target.set(value);
      return true;
    }
    if (additionalInfo == 27) {
      uint64_t raw = 0;
      if (!cborReadUint(cursor, additionalInfo, raw)) {
        return false;
      }
      double value = 0.0;
      memcpy(&value, &raw, sizeof(value));
      target.set(value);
      return true;
    }
    return false;
  }

  return false;
}

bool cborValueToJson(CborValue* value, JsonVariant target) {
  CborType type = cbor_value_get_type(value);

  if (type == CborTagType) {
    CborError error = cbor_value_skip_tag(value);
    if (error != CborNoError) {
      return false;
    }
    return cborValueToJson(value, target);
  }

  if (type == CborMapType) {
    JsonObject object = target.to<JsonObject>();
    CborValue iterator;
    CborError error = cbor_value_enter_container(value, &iterator);
    if (error != CborNoError) {
      return false;
    }

    while (!cbor_value_at_end(&iterator)) {
      if (!cbor_value_is_text_string(&iterator)) {
        return false;
      }
      String key;
      if (!readCborTextString(&iterator, key)) {
        return false;
      }
      object[key] = nullptr;
      JsonVariant child = object[key];
      if (!cborValueToJson(&iterator, child)) {
        return false;
      }
    }
    return cbor_value_leave_container(value, &iterator) == CborNoError;
  }

  if (type == CborArrayType) {
    JsonArray array = target.to<JsonArray>();
    CborValue iterator;
    CborError error = cbor_value_enter_container(value, &iterator);
    if (error != CborNoError) {
      return false;
    }

    while (!cbor_value_at_end(&iterator)) {
      JsonVariant child = array.add();
      if (!cborValueToJson(&iterator, child)) {
        return false;
      }
    }
    return cbor_value_leave_container(value, &iterator) == CborNoError;
  }

  if (type == CborTextStringType) {
    String text;
    if (!readCborTextString(value, text)) {
      return false;
    }
    target.set(text);
    return true;
  }

  if (type == CborByteStringType) {
    String byteStringAsHex;
    if (!readCborByteStringAsHex(value, byteStringAsHex)) {
      return false;
    }
    target.set(byteStringAsHex);
    return true;
  }

  if (type == CborIntegerType) {
    CborError error = CborNoError;
    if (cbor_value_is_unsigned_integer(value)) {
      uint64_t unsignedValue = 0;
      error = cbor_value_get_uint64(value, &unsignedValue);
      if (error == CborNoError) {
        target.set(unsignedValue);
      }
    } else {
      int64_t signedValue = 0;
      error = cbor_value_get_int64(value, &signedValue);
      if (error == CborNoError) {
        target.set(signedValue);
      }
    }
    if (error != CborNoError) {
      return false;
    }
    return cbor_value_advance(value) == CborNoError;
  }

  if (type == CborBooleanType) {
    bool boolValue = false;
    CborError error = cbor_value_get_boolean(value, &boolValue);
    if (error != CborNoError) {
      return false;
    }
    target.set(boolValue);
    return cbor_value_advance(value) == CborNoError;
  }

  if (type == CborFloatType || type == CborDoubleType) {
    double numberValue = 0.0;
    if (!readCborFloatingPointValue(value, numberValue)) {
      return false;
    }
    target.set(numberValue);
    return cbor_value_advance(value) == CborNoError;
  }

  if (type == CborNullType || type == CborUndefinedType) {
    target.set(nullptr);
    return cbor_value_advance(value) == CborNoError;
  }

  return cbor_value_advance(value) == CborNoError;
}

bool deserializeCborToJsonDocument(const uint8_t* payload, size_t length, DynamicJsonDocument& doc) {
  JsonVariant rootVariant = doc.to<JsonVariant>();
  CborCursor cursor{payload, length, 0};
  return decodeCborItem(cursor, rootVariant);
}

void handleCommandDocument(DynamicJsonDocument& doc) {
  if (doc.containsKey("bodyWeight.powerstate")) {
    getFitbitWeight();
  } else if (doc.containsKey("weightGoal.powerstate")) {
    getFitbitWeightGoal();
  } else if (doc.containsKey("dailyActivities.powerstate")) {
    getFitbitDailyActivities();
  } else if (doc.containsKey("getBody.powerstate")) {
    updateStatus(deviceId, "fitbit.get.body", "");
  } else if (doc.containsKey("currentWeight.powerstate")) {
    getCurrentWeight();
  } else {
    Serial.print("Unknown command. Sending through chain");
    Serial.flush();
    waterHandler.handle(doc);
  }

  context.printStatus(Serial);
  context.printStatus(TelnetStream);
  Serial.println(F("Done"));
  Serial.flush();
  TelnetStream.println(F("Done"));
  TelnetStream.flush();
}

void handleTextCommand(const String& payload, size_t length) {
  DynamicJsonDocument doc(length + INBOUND_DOC_OVERHEAD);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() returned "));
    Serial.println(error.c_str());
    return;
  }
  handleCommandDocument(doc);
}

void handleBinaryCommand(const uint8_t* payload, size_t length) {
  DynamicJsonDocument doc((length * 8) + INBOUND_DOC_OVERHEAD);
  if (!deserializeCborToJsonDocument(payload, length, doc)) {
    Serial.println(F("Failed to deserialize CBOR payload"));
    TelnetStream.println(F("Failed to deserialize CBOR payload"));
    return;
  }
  String jsonDebug;
  serializeJson(doc, jsonDebug);
  USE_SERIAL.printf("[WSc] doc usage=%u/%u overflow=%d\r\n", static_cast<unsigned int>(doc.memoryUsage()), static_cast<unsigned int>(doc.capacity()), doc.overflowed() ? 1 : 0);
  USE_SERIAL.printf("[WSc] cbor as json: %s\r\n", jsonDebug.c_str());
  TelnetStream.printf("[WSc] cbor as json: %s\r\n", jsonDebug.c_str());
  handleCommandDocument(doc);
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      TelnetStream.printf("[WSc] Disconnected!\r\n");
      break;
    case WStype_ERROR:
      USE_SERIAL.printf("[WSc] Error: %s s\r\n", payload);
      TelnetStream.printf("[WSc] Error: %s \r\n", payload);
      break;
    case WStype_CONNECTED:
      {
        USE_SERIAL.printf("[WSc] Connected to url: %s\r\n", payload);
        TelnetStream.printf("[WSc] Connected to url: %s\r\n", payload);
        // send message to server when Connected
        //webSocketClient.sendTXT("powerstate = Off");
        reportIPAddress();
        shouldReport = true;
        reportVersion();
        updateStatus(deviceId, "myVersion", (char*)theVersion);
        timer.setTimeout(500, updateMode);
        // try to update modes. Send the instance of the mode and which mode is active. Easy peasy
        //disco
        //contextDriven
        //off
        timer.setTimeout(15 * 1000, updateMode);

        updateFitbitValues();
      }
      break;
    case WStype_TEXT:
      turnOnLed();

      USE_SERIAL.printf("[WSc] get text: %s\r\n", payload);
      TelnetStream.printf("[WSc] get text: %s\r\n", payload);
      handleTextCommand(String((char*)payload), length);

      break;
    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: %u\r\n", length);
      {
        String payloadHex = bytesToHex(payload, length);
        USE_SERIAL.printf("[WSc] binary hex: %s\r\n", payloadHex.c_str());
      }
      handleBinaryCommand(payload, length);
      break;
    case WStype_PING:
      // pong will be send automatically
      USE_SERIAL.printf("[WSc] get ping\r\n");
      TelnetStream.printf("[WSc] get ping\r\n");
      gotPing = true;
      break;
    case WStype_PONG:
      // answer to a ping we send
      USE_SERIAL.printf("[WSc] get pong\r\n");
      TelnetStream.printf("[WSc] get pong\r\n");
      break;
    default:
      USE_SERIAL.printf("[WSc] default: ");
      //      USE_SERIAL.printf(type);
      USE_SERIAL.printf("\n");
      break;
  }
}


void setup() {
  WiFi.setHostname("fitbit-display");

  Serial.begin(115200);
  delay(75);
  Serial.println("Booting");

  Serial.print(F("Sketch:   " __FILE__ "\n"
                 "Compiled: " __DATE__ " " __TIME__ "\n\n"));


  pinMode(LED_BUILTIN, OUTPUT);
  turnOnLed();
  dht.begin();
  calculateTempHum();


  // Link the chain
  waterHandler.setNext(&caloriesHandler);
  caloriesHandler.setNext(&sleepHandler);
  sleepHandler.setNext(&stepsGoalHandler);
  stepsGoalHandler.setNext(&weightGoalHandler);
  weightGoalHandler.setNext(&ledModeHandler);

#if defined(ESP32)
  WiFi.onEvent(onWifiEvent);
#endif

  setupOTA("FitbitDisplay", mySSID, myPASSWORD);

  webSocketClient.setExtraHeaders(websocketHeaders);
  webSocketClient.onEvent(webSocketEvent);
  // try ever 5000 again if connection has failed
  webSocketClient.setReconnectInterval(8000);
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocketClient.enableHeartbeat(15000, 15000, 2);
  startWebSocketIfNeeded();

  //TODO: Only do this when connection
  timer.setInterval(60 * 1000L, reconnectIfNoPing);
  timer.setInterval(10 * 1000, calculateTempHum);
  timer.setInterval(60 * 1000, updateMode);

  timer.setInterval(5 * 60 * 1000, updateFitbitValues);
  Serial.println("Setup done");
  Serial.flush();
  turnOffLed();

  // WiFi.eventName(arduino_event_id_t id)
}


void loop() {
  //#ifdef defined(ESP32_RTOS) && defined(ESP32)
  //#else // If you do not use FreeRTOS, you have to regulary call the handle method.
  ensureWifiConnected();
  startWebSocketIfNeeded();
  ArduinoOTA.handle();
  //#endif
  webSocketClient.loop();
  timer.run();

  String modeRightNow = ledModeHandler.run();
  if (modeRightNow.compareTo(currentModeAsString) != 0) {
    currentModeAsString = modeRightNow;
    reportMode();
  }

  //  if (TelnetStream.)
  if (TelnetStream.available() > 0) {
    char inChar = TelnetStream.read();
  
    switch (inChar) {
      case 'r':
        TelnetStream.println("Restarting.......");
        ESP.restart();
        break;
      case 'o':
        TelnetStream.print(F("I am the fitbit-display in version "));
        TelnetStream.print(theVersion);
        break;
      case '1':
        turnOnPump();
        break;
      case '2':
        turnOffPump();
        break;
      case '3':
        getFitbitWeight();
        break;
      case '4':
        getFitbitWeightGoal();
        break;
      case '5':
        getFitbitDailyActivities();
        break;
      case '6':
        updateMode();
        break;
      case '7':
        calculateTempHum();
        break;
      case '8':
        updateFitbitValues();
        break;
      case 't':
        turnOnPump();
        timer.setTimeout(1000 * 60 * 3, turnOffPump);
        break;
      case 0xa:
      case 0xd:
        // skip newline and carriage return
        break;
      default:
        TelnetStream.printf("%c typed\n", inChar);
        break;
    }
  }
}


//show:
// - water intake
// - calories burned
// - weight goal
// . sleep score
// if excercised today
