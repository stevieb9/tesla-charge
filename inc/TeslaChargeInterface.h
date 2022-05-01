#ifndef TESLA_CHARGE_TESLACHARGE_INTERFACE_H
#define TESLA_CHARGE_TESLACHARGE_INTERFACE_H

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include "SSD1306Wire.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaChargeFont.h"

#define DEBUG_MAGNET    0   // REED switch on at all times
#define DEBUG_MOTION    0   // PIR on at all times

#define REED_PIN        2   // Pin D4
#define PIR_PIN         12  // Pin D6
#define ALARM_PIN       13  // Pin D7
#define WIFI_CONFIG_PIN 14  // Pin D5
#define SDA_PIN         4   // Pin D2
#define SCL_PIN         5   // Pin D1

#define JSON_SIZE       224  // Maximum size of JSON data
#define CHARGE_MAX      101  // Maximum battery level +1
#define CHARGE_MIN      0    // Minimum battery level
#define ALARM_ON_TIME   100  // Alarm cycle on time (milliseconds)
#define ALARM_OFF_TIME  3000 // Alarm cycle off time (milliseconds)
#define ALARM_CHARGE    70   // Below this battery percent, audible alarm sounds

#define DATA_DELAY      500  // Wait between API calls (milliseconds)

char apiURL[64];             // Maximum length of the API URL
char apiToken[86];           // Maximum length of API token (don't change this!)

#endif