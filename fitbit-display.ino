//DOIT ESP32 DEVKIT V1 worked
#define ESP32 123

#include "OTA.h"
#include <credentials.h>
#include <WebSocketsClient.h>
#include <SimpleTimer.h>


//#include <ESP8266httpUpdate.h>
//#include <ESP8266HTTPClient.h>
#include "buildTime.h"
#include "version.h"


WiFiClient http;

const int ledPin = LED_BUILTIN;  // the number of the LED pin
int ledState = LOW;  // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;  // will store last time LED was updated

WebSocketsClient webSocketClient;
//WiFiClient client;


//for prod
//char host[] = "www.offbeat-iot.com";
char host[] = "soeren.herokuapp.com";

char user[] = "px403";
char socketPassword[] = "Rxxy4cH9";
char path[] = "/ws?device=CjvJ39w8";
char deviceId[] = "CjvJ39w8";

#define USE_SERIAL Serial

SimpleTimer timer;

void updateStatus(char* deviceId, char* state, char* newState) {
  if (webSocketClient.isConnected()) {
    char powerstate[4];
    strcpy(powerstate, newState);
    char s[256];
    sprintf(s, "%s=%s,deviceId=%s", state, newState, deviceId);
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


boolean shouldReport = false;
boolean pumpOn = false;
void turnOnPump() {
  digitalWrite(ledPin, HIGH);
 // digitalWrite(relayPin, HIGH);
  pumpOn = true;
  updateStatus(deviceId, "powerState", "On");
}

void turnOffPump() {
  digitalWrite(ledPin, LOW);
  //digitalWrite(relayPin, LOW);
  pumpOn = false;
  updateStatus(deviceId, "powerState", "Off");
}
// a function to be executed periodically

void repeatMe() {
  Serial.print("Uptime (s): ");
  Serial.println(millis() / 1000);
}

boolean gotPing = false;
void reconnectIfNoPing() {

  if (!gotPing) {
    TelnetStream.println("No ping received, reconnecting");
    TelnetStream.print("Is connected?: ");
    TelnetStream.println(gotPing);
    webSocketClient.disconnect();
    ESP.restart();
  } else {
    gotPing = false;
  }
}


void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  char* deviceIdFromMessage;
  char* command;
  char* commandValue;
  switch (type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
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
      }
      break;
    case WStype_TEXT:
      USE_SERIAL.printf("[WSc] get text: % s\n", payload);
      //14:58:50.193 -> [WSc] get text: CjvJ39w8,powerstate,TurnOn
      deviceIdFromMessage = strtok((char*)payload, ", ");
      command = strtok(NULL, ", ");
      commandValue = strtok(NULL, ", ");
      Serial.print("Command: ");
      Serial.println(command);
      Serial.print("Value: ");
      Serial.println(commandValue);

      TelnetStream.print("Command: ");
      TelnetStream.println(command);
      TelnetStream.print("Value: ");
      TelnetStream.println(commandValue);

      if (strcmp(command, "powerstate") == 0) {
        if (strcmp(commandValue, "TurnOn") == 0) {
          turnOnPump();
        } else if (strcmp(commandValue, "TurnOff") == 0) {
          turnOffPump();
        }
        shouldReport = true;
      } else if (strcmp(command, "nextVersion") == 0) {
        Serial.println(F("Should do OTA213"));
        TelnetStream.println(F("Should do ota?"));
      } else {
        Serial.print("Unknown command");
      }
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
  Serial.println("Booting");

  setupOTA("FitbitDevice", mySSID, myPASSWORD);

//  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Connecting to websocket");

  webSocketClient.begin(host, 80, path);
  webSocketClient.setAuthorization(user, socketPassword);
  webSocketClient.onEvent(webSocketEvent);
  // try ever 5000 again if connection has failed
  webSocketClient.setReconnectInterval(5000);
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocketClient.enableHeartbeat(15000, 15000, 2);

  WiFi.setHostname("fitbit-device");

  timer.setInterval(60 * 1000L, reconnectIfNoPing);
}


void loop() {
  //#ifdef defined(ESP32_RTOS) && defined(ESP32)
  //#else // If you do not use FreeRTOS, you have to regulary call the handle method.
  ArduinoOTA.handle();
  //#endif
  webSocketClient.loop();
  timer.run();

  // Your code here
  if (shouldReport) {
    shouldReport = false;
    Serial.println("Pump state changed. Reporting");
    char powerstate[4];
    if (pumpOn) {
      strcpy(powerstate, "On");
      Serial.println("Reporting pump on");
    } else {
      strcpy(powerstate, "Off");
      Serial.println("Reporting pump off");
    }

    char s[256];
    sprintf(s, "powerstate = % s, deviceId = % s", powerstate, deviceId);
    Serial.print("Sending text: ");
    Serial.println(s);
    webSocketClient.sendTXT(s);
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
