#ifndef TESLA_CHARGE_TESLACHARGE_INTERFACE_H
#define TESLA_CHARGE_TESLACHARGE_INTERFACE_H

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "SSD1306Wire.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaChargeFont.h"

#define DEBUG_URL       0   // Call the debug data HTTP URL
#define DEBUG_DEVEL     0   // Testing on a non production unit
#define DEBUG_MAGNET    0   // REED switch on at all times
#define DEBUG_MOTION    0   // PIR on at all times

#define URL         "http://192.168.1.252:55556"
//#define URL         "http://10.0.0.50:55556"

#define REED_PIN        2  // Pin D4
#define PIR_PIN         12 // Pin D6
#define ALARM_PIN       13 // Pin D7
#define SDA_PIN         4  // Pin D2
#define SCL_PIN         5  // Pin D1

#define JSON_SIZE       224  // Maximum size of JSON data
#define CHARGE_MAX      101  // Maximum battery level +1
#define CHARGE_MIN      0    // Minimum battery level
#define ALARM_ON_TIME   100  // Alarm cycle on time
#define ALARM_OFF_TIME  3000 // Alarm cycle off time
#define ALARM_CHARGE    70   // Below this, audible alarm sounds

#define DATA_DELAY      500 // ms to wait between API calls

#endif