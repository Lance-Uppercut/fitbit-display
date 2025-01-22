//DOIT ESP32 DEVKIT V1 worked
#define ESP32 123

#include "OTA.h"
#include <credentials.h>
#include <WebSocketsClient.h>
#include <SimpleTimer.h>
#include <ArduinoJson.h>
#include "WaterGoalHandler.cpp"
#include "SleepGoalHandler.cpp"
#include "CaloriesGoalHandler.cpp"
#include "WeightGoalHandler.cpp"
#include "LedModeHandler.cpp"
#include "Context.h"
//#include <ESP8266httpUpdate.h>
//#include <ESP8266HTTPClient.h>
#include "buildTime.h"
#include "version.h"
#include <DHT.h>


#define DHTPIN 32       //(rød gpio0, pin D3) (3,3 volt)
#define DHTTYPE DHT11   // DHT 11
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
char host[] = "soeren.herokuapp.com";

char user[] = "px403";
char socketPassword[] = "Rxxy4cH9";
char path[] = "/ws?device=CjvJ39w8";
char deviceId[] = "CjvJ39w8";

#define USE_SERIAL Serial

// Create the context
Context context;

// Create handlers
WaterGoalHandler waterHandler(context);
CaloriesGoalHandler caloriesHandler(context);
SleepGoalHandler sleepHandler(context);
WeightGoalHandler weightGoalHandler(context);
LedModeHandler ledModeHandler(context);
SimpleTimer timer;


String currentModeAsString = String("off");

void turnOnLed() {
  digitalWrite(LED_BUILTIN, HIGH);
}
void turnOffLed() {
  digitalWrite(LED_BUILTIN, LOW);
}


void updateStatus(char* deviceId, char* state, char* newState) {
  if (webSocketClient.isConnected()) {
    char powerstate[4];
    strcpy(powerstate, newState);
    //char s[256];
    //sprintf(s, "%s=%s,deviceId=%s", state, newState, deviceId);
    char s[256];
    sprintf(s, "{\"%s\":\"% s\",\"deviceId\":\"% s\"}", state, newState, deviceId);

    Serial.print("Sending text: ");
    Serial.println(s);
    TelnetStream.print("Sending text: ");
    TelnetStream.println(s);
    webSocketClient.sendTXT(s);
  } else {
    Serial.println("Not connected when updating status");
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

void getWater(){
  //{"endpointId":"${endpointId}","fitbit.get.water":{"water":0.0}}      
    updateStatus(deviceId, "fitbit.get.water", "");
}

void getWaterGoal(){
  //{"endpointId":"${endpointId}","fitbit.get.water.goal":{"goal":24,"startDate":"2019-03-21"}}
    updateStatus(deviceId, "fitbit.get.water.goal", "");

}

void getFitbitWeight() {
  Serial.println("Getting fitbit weight");
  //taking the easy road of just sending strings
  webSocketClient.sendTXT("{\"fitbit.get.weight\":\"\"}");
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
    webSocketClient.disconnect();
    ESP.restart();
  } else {
    gotPing = false;
  }
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


void handleCommand(const String& payload, size_t length) {
  const char* deviceIdFromMessage;
  const char* command;
  const char* commandValue;
  //  const size_t capacity = JSON_OBJECT_SIZE(2) + 20;  //Memory pool
  //{"bodyWeight.powerstate":"TurnOn","endpointId":"CjvJ39w8"}
  // {"endpointId":"CjvJ39w8","powerstate":"TurnOn"}
  DynamicJsonDocument doc(length + 4);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() returned "));
    Serial.println(error.c_str());
    return;
  }
  if (doc.containsKey("powerstate")) {
    commandValue = doc["powerstate"];
    Serial.print(F("Handling powerstate: "));
    Serial.println(commandValue);
    if (strcmp(commandValue, "TurnOn") == 0) {
      turnOnPump();
    } else if (strcmp(commandValue, "TurnOff") == 0) {
      turnOffPump();
    }
    shouldReport = true;
  } else if (doc.containsKey("bodyWeight.powerstate")) {
    getFitbitWeight();
  } else if (doc.containsKey("weightGoal.powerstate")) {
    //{"endpointId":"CjvJ39w8","weightGoal.powerstate":"TurnOff"}
    getFitbitWeightGoal();
  } else if (doc.containsKey("dailyActivities.powerstate")) {
    //{"endpointId":"CjvJ39w8","weightGoal.powerstate":"TurnOff"}
    getFitbitDailyActivities();
  } else if (doc.containsKey("getBody.powerstate")) {
    //{"endpointId":"CjvJ39w8","weightGoal.powerstate":"TurnOff"}
    updateStatus(deviceId, "fitbit.get.body", "");
  } else if (doc.containsKey("caloriesOut")) {
    //{"endpointId":"CjvJ39w8","caloriesOut":178,"activityCalories":0,"steps":0,"veryActiveMinutes":0,"caloriesBMR":173,"sedentaryMinutes":129}
    int caloriesOut = doc["caloriesOut"];
    Serial.printf("caloriesOut: %4d\n", caloriesOut);
    int activityCalories = doc["activityCalories"];
    Serial.printf("activityCalories: %4d\n", caloriesOut);

    //{"endpointId":"CjvJ39w8","weightGoal.powerstate":"TurnOff"}
    //    updateStatus(deviceId, "fitbit.get.body", "");
  } else if (doc.containsKey("currentWeight.powerstate")) {
    getCurrentWeight();
  } else {
    Serial.print("Unknown command");
    Serial.flush();
    waterHandler.handle(doc);
  }

  context.printStatus();
  Serial.println(F("Done"));
  Serial.flush();
}

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  char* deviceIdFromMessage;
  char* command;
  char* commandValue;
  switch (type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;
    case WStype_ERROR:
      USE_SERIAL.printf("[WSc] Error: %s s\n", payload);
      break;
    case WStype_CONNECTED:
      {
        USE_SERIAL.printf("[WSc] Connected to url: % s\n", payload);

        // send message to server when Connected
        //webSocketClient.sendTXT("powerstate = Off");
        reportIPAddress();
        shouldReport = true;
        reportVersion();
        updateStatus(deviceId, "myVersion", (char*)theVersion);
        // try to update modes. Send the instance of the mode and which mode is active. Easy peasy
        //disco
        //contextDriven
        //off
        reportMode();
        //TODO: perhaps it could make sense to add an asycnt method
        getFitbitWeight();
        getFitbitWeightGoal();
        getFitbitDailyActivities();
        getCurrentWeight();
        getWater();
        getWaterGoal();
      }
      break;
    case WStype_TEXT:
      turnOnLed();
      USE_SERIAL.printf("[WSc] get text: % s\n", payload);
      handleCommand(String((char*)payload), length);

      break;
    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: % u\n", length);
      //hexdump(payload, length);

      // send data to server
      // webSocketClient.sendBIN(payload, length);
      break;
    case WStype_PING:
      // pong will be send automatically
      USE_SERIAL.printf("[WSc] get ping\n");
      gotPing = true;
      break;
    case WStype_PONG:
      // answer to a ping we send
      USE_SERIAL.printf("[WSc] get pong\n");
      break;
    default:
      USE_SERIAL.printf("[WSc] default: ");
      //      USE_SERIAL.printf(type);
      USE_SERIAL.printf("\n");
      break;
  }
}


void setup() {
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
  sleepHandler.setNext(&weightGoalHandler);
  weightGoalHandler.setNext(&ledModeHandler);

  setupOTA("FitbitDisplay", mySSID, myPASSWORD);

  Serial.println("Connecting to websocket");

  webSocketClient.begin(host, 80, path);
  webSocketClient.setExtraHeaders("Accept: application/json");
  webSocketClient.setAuthorization(user, socketPassword);
  webSocketClient.onEvent(webSocketEvent);
  // try ever 5000 again if connection has failed
  webSocketClient.setReconnectInterval(8000);
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocketClient.enableHeartbeat(15000, 15000, 2);

  WiFi.setHostname("fitbit-display");
  //TODO: Only do this when connection
  timer.setInterval(60 * 1000L, reconnectIfNoPing);
  timer.setInterval(10 * 1000, calculateTempHum);

  Serial.println("Setup done");
  Serial.flush();
  turnOffLed();
}


void loop() {
  //#ifdef defined(ESP32_RTOS) && defined(ESP32)
  //#else // If you do not use FreeRTOS, you have to regulary call the handle method.
  ArduinoOTA.handle();
  //#endif
  webSocketClient.loop();

  String modeRightNow = ledModeHandler.run();
  if (modeRightNow.compareTo(currentModeAsString) != 0) {
    currentModeAsString = modeRightNow;
    reportMode();
  }

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