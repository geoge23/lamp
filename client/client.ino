#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define NEOPIXEL_PIN 13
#define NEOPIXEL_COUNT 60
#define RECHECK_TIME 60000
#define BUTTON_PIN 2
#define SHORT_PRESS_TIME 500
#define LONG_PRESS_TIME 800

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
WiFiManager wifiManager;
WiFiClient wifiClient;
HTTPClient http;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

String rootUrl = "http://lamp.geoge.co";

int lastServerCheck = 0;
String mac = WiFi.macAddress();
String message = "";
double color[] = {0.0, 0.0, 0.0};
bool nudging = false;
uint8_t brightnessLevel = 3;

int lastState = LOW;
bool longPress = false;
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  wifiManager.autoConnect("awesomelamp");
  Serial.print("ESP Board MAC Address: ");
  Serial.println(mac);

  http.begin(wifiClient, (rootUrl + "/lamp").c_str());

  DynamicJsonDocument doc(200);
  doc["mac"] = mac;
  String buffer;
  serializeJson(doc, buffer);
  Serial.println(buffer);

  http.addHeader("Content-Type", "application/json");
  http.POST(buffer.c_str());

  String payload = http.getString();
  Serial.println(payload.c_str());

  http.end();

  EEPROM.begin(20);
  brightnessLevel = EEPROM.read(0);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void loop() {
  if (millis() - lastServerCheck > RECHECK_TIME || lastServerCheck == 0) {
    Serial.println("Re-checking server");
    http.begin(wifiClient, (rootUrl + "/state?mac=" + mac).c_str());

    http.GET();
    StaticJsonDocument<500> doc;

    String json = http.getString();

    http.end();

    deserializeJson(doc, json);

    bool tap = doc["tap"];
    if (tap) {
      nudging = true;
    }

    double newColor0 = doc["color"][0];
    double newColor1 = doc["color"][1];
    double newColor2 = doc["color"][2];
    color[0] = newColor0;
    color[1] = newColor1;
    color[2] = newColor2;

    message = String(doc["message"]);

    lastServerCheck = millis();

    strip.begin();
    strip.show();
    strip.clear();
  }

  //display message
  displayText(message);

  //check if button is pressed
  //if pressed long, send a nudge
  //if short press, change brightness between 0, 1, 2, 3
  int currentState = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && currentState == LOW) {
    pressedTime = millis();
    longPress = false;
  } else if (lastState == LOW && currentState == HIGH) { // button is released
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if (pressDuration < SHORT_PRESS_TIME) {
      //short press
      Serial.println("short");
      if (brightnessLevel == 3) {
        brightnessLevel = 0;
      } else {
        brightnessLevel++;
      }
      EEPROM.write(0, brightnessLevel);
      EEPROM.commit();
    }
  } else if (currentState == LOW) {
    if (millis() - pressedTime > LONG_PRESS_TIME && longPress == false) {
      longPress = true;
      Serial.println("long");

      http.begin(wifiClient, (rootUrl + "/tap").c_str());

      DynamicJsonDocument doc(200);
      doc["mac"] = mac;
      String buffer;
      serializeJson(doc, buffer);

      http.addHeader("Content-Type", "application/json");

      http.POST(buffer.c_str());
      http.end();

      displayText("Sent nudge <3");
      delay(1000);
    }
  }

  // save the the last state
  lastState = currentState;

  if (nudging) {
    Serial.println("nudging");
    for (int i = 1; i < NEOPIXEL_COUNT; i++) {
      strip.setPixelColor(i, 156, 30, 201);
      strip.setPixelColor(i - 1, 0, 0, 0);
      strip.show();
      delay(50);
    }
    for (int i = 0; i < 100; i++) {
      double lvl = sin(i * (3.1415 / 100)) * 255;
      Serial.println(lvl);
      strip.setBrightness(lvl);
      for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        strip.setPixelColor(i, 156, 30, 201);
      }
      strip.show();
      delay(50);
    }
    delay(1000);
    nudging = false;
  }

  //set neopixel color
  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    strip.setPixelColor(i, color[0], color[1], color[2]);
  }
  strip.setBrightness(brightnessLevel * 85);
  strip.show();
}

void displayText(String msg) {
  display.clearDisplay();
  display.setTextSize(1.5);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setTextWrap(true);

  display.print(msg);

  display.display();
}
