#include <Arduino.h>

#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <MQTT.h>

#include <GxEPD2_BW.h>

#include <LineProtocol.h>

#include <map>

#include <Fonts/FreeSansBold9pt7b.h>
#include "FreeSans7pt7b.h"

#include "setting.h"

#define every(t) for (static uint16_t _lasttime; (uint16_t)((uint16_t) millis() - _lasttime) >= (t); _lasttime = millis())

#define MAX_DISPLAY_BUFFER_SIZE 800 
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

MQTTClient mqtt;

GxEPD2_BW<GxEPD2_213_B73, GxEPD2_213_B73::HEIGHT> display(GxEPD2_213_B73(/*CS=5*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEH0213B73

std::map<String, std::map<String, String>> state;

void dump_state() {
  for(auto sensor: state) {
      Serial.print(sensor.first + " ");

      for(auto room: sensor.second) {
        Serial.print(room.first + "=" + room.second + " ");
      }

      Serial.println();
  }
}

void draw_state() {
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(0.75);
    display.setFullWindow();

    display.firstPage();
    display.fillScreen(GxEPD_WHITE);

    display.setCursor(0, 12);

    display.print("Temperatures");

    display.setFont(&FreeSans7pt7b);

    int i = 30;

    for(auto room: state["temperature"]) {
      display.setCursor(0, i);
      display.print(room.first + " " + String(room.second.toFloat(), 1));
      i += 12;
    }

    display.nextPage();
}

void setup_display() {
    display.init();
    display.setRotation(0);

    display.setFullWindow();
    display.firstPage();
    display.fillScreen(GxEPD_WHITE);
    display.nextPage();
}

void setup_wifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) delay(500);
}

void loop_wifi() {
}

void callback_mqtt(String &topic, String &payload) {
    struct line_protocol message;

    if(line_protocol_parse(message, payload)) {
        return;
    }

    state[message.measurement][message.tags["room"]] = message.fields["value"];
}

void setup_mqtt() {
    static WiFiClient wificlient;
    mqtt.begin("192.168.1.10", 1883, wificlient);
    mqtt.onMessage(callback_mqtt);

    while(!mqtt.connect("")) delay(500);

    mqtt.subscribe("/sensor/temperature");
    mqtt.subscribe("/esp8266/temperature");
}

void loop_mqtt() {
    mqtt.loop();
}

void setup() {
    Serial.begin(115200);

    setup_display();
    setup_wifi();
    setup_mqtt();
}

void loop() {
    loop_wifi();
    loop_mqtt();

    dump_state();

    every(10000) draw_state();
};