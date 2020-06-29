#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <EasyButton.h>
#include <RBDdimmer.h>

const unsigned int LED_CONTROL = 2;
const unsigned int POTENTIOMETER = 5;
const unsigned int PHOTORESISTOR = 4;
const unsigned int ANALOG_PIN = A0;
const unsigned int AC_CONTROL = 16;
const unsigned int ZC_DETECT = 14;
const unsigned int FLASH_BUTTON = 0;
const unsigned int MODE_SWITCH1 = 12;
const unsigned int MODE_SWITCH2 = 13;

const unsigned int MODE_MANUAL = 1;
const unsigned int MODE_AUTO = 2;
const unsigned int MODE_REMOTE = 0;
const unsigned int MODE_REMOTE_AUTO = 3;

int mode = MODE_REMOTE;
int intensity = 0;

EasyButton button(FLASH_BUTTON);
dimmerLamp dimmer(AC_CONTROL, ZC_DETECT);

Ticker ticker;

ESP8266WebServer server(80);

WiFiManager wifiManager;

void tick()
{
  int state = digitalRead(LED_CONTROL);  
  digitalWrite(LED_CONTROL, !state);    
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach(0.2, tick);
}

void wifiReset() {
  wifiManager.resetSettings();
  Serial.println("wifi config mode setted");
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  dimmer.begin(NORMAL_MODE, ON);
  
  pinMode(LED_CONTROL, OUTPUT);
  pinMode(POTENTIOMETER, OUTPUT);
  pinMode(PHOTORESISTOR, OUTPUT);

  pinMode(MODE_SWITCH1, INPUT_PULLUP);
  pinMode(MODE_SWITCH2, INPUT_PULLUP);

  digitalWrite(POTENTIOMETER, LOW);
  digitalWrite(PHOTORESISTOR, LOW);
  button.begin();

  button.onPressedFor(5000, wifiReset);
  Serial.println(digitalRead(MODE_SWITCH1));
   Serial.println(digitalRead(MODE_SWITCH2));
     Serial.println("test");
  if(digitalRead(MODE_SWITCH1) == HIGH && digitalRead(MODE_SWITCH2) == HIGH) {
    ticker.attach(0.6, tick);
  
    wifiManager.setAPCallback(configModeCallback);
  
    if (!wifiManager.autoConnect()) {
      Serial.println("failed to connect and hit timeout");
      ESP.reset();
      delay(1000);
    }
  
    Serial.println("connected...yeey :)");
  }
  server.on("/set", handleSetRequest); 
  server.on("/set-auto", handleSetAutoRequest); 
  server.on("/get-state", handleGetStateRequest); 
  server.on("/wifi-disconntect", wifiReset); 
  server.onNotFound(handleNotFound);

  server.begin();  
  Serial.println("{Server listening");   
  ticker.attach(1, tick);
}

void loop() {
  button.read();
  server.handleClient(); 
  
  checkMode();
  delay(500);
  if(mode == MODE_MANUAL) { 
    intensity = getLightIntenistyBySensor(POTENTIOMETER);
  } else if(mode == MODE_AUTO || mode == MODE_REMOTE_AUTO) {
    intensity = 1024 - getLightIntenistyBySensor(PHOTORESISTOR);
  } 

  setACIntenisty(intensity);
}

void handleNotFound()
{
    if (server.method() == HTTP_OPTIONS)
    {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Max-Age", "10000");
        server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "*");
        server.send(204);
    }
    else
    {
        server.send(404, "text/plain", "");
    }
}
void handleSetRequest() {
  intensity = server.arg("intensity").toInt(); 
  server.sendHeader("Access-Control-Allow-Origin", "*");

  server.send(200, "text/json", "{\"success\":true}");
}

void handleSetAutoRequest() {
  if(server.arg("on").toInt() == 1) {
     mode = MODE_REMOTE_AUTO;
  } else  {
    mode = MODE_REMOTE;
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/json", "{\"success\":true}");
}

void handleGetStateRequest() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/json", "{\"mode\":" + String(mode) + ", \"intensity\": " + String(intensity) + "}");
}

void checkMode() {
  static const unsigned long REFRESH_INTERVAL = 500;
  static unsigned long lastRefreshTime = 0;


  int modeSwitchState1 = !digitalRead(MODE_SWITCH1);
  int modeSwitchState2 = !digitalRead(MODE_SWITCH2);

  if (millis() - lastRefreshTime >= REFRESH_INTERVAL) {
    if (modeSwitchState1 == HIGH) {
      mode = MODE_MANUAL;
    }
    else if (modeSwitchState2 == HIGH) {
      mode = MODE_AUTO;
    } else {
      if(mode == MODE_REMOTE_AUTO) {
        mode = MODE_REMOTE_AUTO;
      } else {
        mode = MODE_REMOTE;
      }
    }
  }
}

int unsigned getLightIntenistyBySensor(int sensor) {
  int sensorValue = 0;

  if (POTENTIOMETER == sensor) {
    digitalWrite(POTENTIOMETER, HIGH);
    digitalWrite(PHOTORESISTOR, LOW);

    delay(300);

    sensorValue = analogRead(ANALOG_PIN);

    delay(300);

    digitalWrite(POTENTIOMETER, LOW);
  } else if (PHOTORESISTOR == sensor) {
    digitalWrite(POTENTIOMETER, LOW);
    digitalWrite(PHOTORESISTOR, HIGH);

    delay(300);

    sensorValue = analogRead(ANALOG_PIN);

    delay(300);

    digitalWrite(PHOTORESISTOR, LOW);
  } else {
    exit(-1);
  }

  return sensorValue;
}

void setACIntenisty(int intensity) {

  int outVal = map(intensity, 1, 1024, 0, 100); 
  dimmer.setPower(outVal);
  
}
