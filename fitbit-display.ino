
//DOIT ESP32 DEVKIT V1 worked
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
#include "OTA.h"
#include <credentials.h>
#include <WebSocketsClient.h>
#include <SimpleTimer.h>


#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include "buildTime.h"
#include "version.h"
#include <MQ135.h>

#define PIN_MQ135 A0  // MQ135 Analog Input Pin

#include <DHT.h>;


#define DHTPIN D6          // pin 1
#define DHTTYPE DHT11      // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);  //// Initialize DHT sensor for normal 16mhz Arduino


WiFiClient http;
//Measured by running quite a while
MQ135 mq135_sensor(PIN_MQ135, 2005.789673);

// constants won't change. Used here to set a pin number:
const int ledPin = LED_BUILTIN;  // the number of the LED pin

// Variables will change:
int ledState = LOW;  // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;  // will store last time LED was updated

// constants won't change:
const long interval = 2000;  // interval at which to blink (milliseconds)
WebSocketsClient webSocketClient;
//WiFiClient client;


//for prod
//char host[] = "www.offbeat-iot.com";
char host[] = "soeren.herokuapp.com";

char user[] = "px403";
char socketPassword[] = "Rxxy4cH9";
char path[] = "/ws?device=CjvJ39w8";
char deviceId[] = "CjvJ39w8";
char sceneId[] = "82fc3aaa-2af5-4bb0-a3cb-1547891f2a6c";

//char host[] = "952d-193-161-31-161.ngrok.io";
//char user[] = "q1641212572561";
//char password[] = "beebd23a-6620-4d72-8d4a-2d313358d8e1";
//char path[] = "/name?device=ecbcde82-0b1b-4a62-b3e8-d37a99bc602e";
//char deviceId[]="ecbcde82-0b1b-4a62-b3e8-d37a99bc602e";
//char path[] = "/name";


#define USE_SERIAL Serial

//const int trigPin = 14;
//const int echoPin = 27;
//const int relayPin = 25;

const int trigPin = D8;  //gul
const int echoPin = D7;  //oragne
const int relayPin = D5;

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;
float distanceInch;

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

void updateStatus(char* deviceId, char* state, float newState) {
  char buf[8];
  sprintf(buf, "%02f", newState);
  updateStatus(deviceId, state, buf);
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

  updateStatus(sceneId, "ipaddress", ipaddressString);
  updateStatus(deviceId, "ipaddress", ipaddressString);
}

char* fileName;
int lastError;
const char* lastErrorString;
char error[50];

void doOTAUpdate() {

  char buf[100];
  sprintf(buf, "http://%s/api/devices/%s/images", host, deviceId);
  Serial.print(F("Doingupdate for "));
  Serial.println(fileName);
  Serial.print("On url");
  Serial.println(buf);
  Serial.flush();
  TelnetStream.println("Doing OTA update");
  TelnetStream.flush();
  String fwv = String(fileName);
  String url = String(buf);

  ESPhttpUpdate.setAuthorization(user, socketPassword);
  t_httpUpdate_return ret = ESPhttpUpdate.update(http, url, fwv);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      lastError = ESPhttpUpdate.getLastError();
      lastErrorString = ESPhttpUpdate.getLastErrorString().c_str();
      sprintf(error, "[update] Update failed (%d): %s", lastError, lastErrorString);
      Serial.println(error);
      TelnetStream.println(error);
      TelnetStream.flush();
      updateStatus(deviceId, "log", error);
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("[update] Update no Update."));
      TelnetStream.println(F("[update] Update no Update."));
      TelnetStream.flush();

      updateStatus(deviceId, "log", "no ota updates found");
      break;
    case HTTP_UPDATE_OK:
      Serial.println(F("[update] Update ok."));

      TelnetStream.println(F("[update] Update update ok."));
      TelnetStream.flush();
      updateStatus(deviceId, "log", "[update] Update ok.");
      break;
  }
}


boolean shouldReport = false;
boolean pumpOn = false;
void turnOnPump() {
  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, HIGH);
  pumpOn = true;
  updateStatus(deviceId, "powerState", "On");
}

void turnOffPump() {
  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, LOW);
  pumpOn = false;
  updateStatus(deviceId, "powerState", "Off");
  updateStatus(sceneId, "powerstate", "Off");
}
// a function to be executed periodically

void repeatMe() {
  Serial.print("Uptime (s): ");
  Serial.println(millis() / 1000);
}


float hum;       //Stores humidity value
float temp;      //Stores temperature value
float prevHum;   //Stores humidity value
float prevTemp;  //Stores temperature value

void updateTempAndHum() {

  //Read data and store it to variables hum and temp
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  //Serial.print(temp);
  //Serial.print(" ");
  //Serial.println(hum);
  char buf[100];
  sprintf(buf, "%.2f", hum);
  updateStatus(deviceId, "humidity", buf);
  sprintf(buf, "%.2f", temp);
  updateStatus(deviceId, "temperature", buf);
}

float previousDistanceCm;
float percent;
float distanceInCm;
float previousPercent = 0;
float maxLengthInBucketCm = 23;
void measureDistance() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance
  distanceCm = duration * SOUND_VELOCITY / 2.0;

  float distance;
  float speed_sound;

  speed_sound = 331.3 + 0.606 * (float)temp;
  speed_sound = speed_sound / 10000;  // convert to centimeters per microsecond
  distance = duration * speed_sound / 2;

  // Convert to inches
  distanceInch = distanceCm * CM_TO_INCH;

  updateStatus(deviceId, "distanceCmTempAdjust", distance);

  percent = (distanceCm / maxLengthInBucketCm) * 100.0;
  shouldReport |= round(percent) != round(previousPercent);
  previousPercent = percent;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  Serial.print("Distance (inch): ");
  Serial.println(distanceInch);
  Serial.print("Distance (percent): ");
  Serial.println(percent);

  updateStatus(deviceId, "distanceInCm", distanceInCm);
}

//https://github.com/Phoenix1747/MQ135/blob/master/examples/MQ135/MQ135.ino
void reportAirQuality() {
  float rzero = mq135_sensor.getRZero();
  float correctedRZero = mq135_sensor.getCorrectedRZero(temp, hum);
  float resistance = mq135_sensor.getResistance();
  float ppm = mq135_sensor.getPPM();
  float correctedPPM = mq135_sensor.getCorrectedPPM(temp, hum);

  updateStatus(deviceId, "rzero", rzero);
  updateStatus(deviceId, "correctedRZero", correctedRZero);
  updateStatus(deviceId, "resistance", rzero);
  updateStatus(deviceId, "correctedPPM", correctedPPM);
  updateStatus(deviceId, "ppm", ppm);
}

boolean gotPing = false;
void reconnectIfNoPing() {

  if (!gotPing) {
    TelnetStream.println("No ping received, reconnecting");
    TelnetStream.print("Is connected?: ");
    TelnetStream.println(gotPing);
    webSocketClient.disconnect();
    ESP.reset();
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
        updateStatus(sceneId, "powerstate", "Off");
        shouldReport = true;
        reportVersion();
        updateStatus(deviceId, "myVersion", (char*)theVersion);
        updateTempAndHum();
        reportAirQuality();
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

      if (strcmp(command, "powerstate") == 0 || strcmp(command, "scenestate") == 0) {
        if (strcmp(commandValue, "TurnOn") == 0) {
          turnOnPump();
          if (strcmp(deviceIdFromMessage, sceneId) == 0) {
            updateStatus(sceneId, "powerstate", "On");
          }
          timer.setTimeout(1000 * 60 * 3, turnOffPump);
          //timer.setTimeout(1000 * 60 * 2, updateStatus(sceneId, "powerstate", "Off"));
        } else if (strcmp(commandValue, "TurnOff") == 0) {
          if (strcmp(deviceIdFromMessage, sceneId) == 0) {
            updateStatus(sceneId, "powerstate", "Off");
          }
          turnOffPump();
        }
        shouldReport = true;
      } else if (strcmp(command, "nextVersion") == 0) {
        fileName = commandValue;
        Serial.println(F("Should do OTA213"));
        TelnetStream.println(F("Should do ota?"));
        doOTAUpdate();
      } else {
        Serial.print("Unknown command");
      }

      // send message to server
      // webSocketClient.sendTXT("message here");
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

  dht.begin();

  setupOTA("FitbitDevice", mySSID, myPASSWORD);
  // Your setup code

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(trigPin, OUTPUT);   // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);    // Sets the echoPin as an Input
  pinMode(relayPin, OUTPUT);  // Sets the trigPin as an Output

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
        updateStatus(sceneId, "powerstate", "On");
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
