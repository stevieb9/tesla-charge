#ifndef TESLA_CHARGE_TESLACHARGE_H
#define TESLA_CHARGE_TESLACHARGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "SSD1306Wire.h"
#include "FastLED.h"
#include "myFont.h"

#define SERIAL_DEBUG        /* Serial output (comment to disable) */

#define DEBUG_DEVEL     0   /* Testing on a non production unit */
#define DEBUG_MAGNET    0   /* REED switch on at all times*/
#define DEBUG_MOTION    0   /* PIR on at all times */
#define DEBUG_URL       0   /* Call the debug data HTTP URL */

//#define URL         "http://192.168.1.252:55556"
//#define URL_DEBUG   "http://192.168.1.252:55556/debug"

#define URL         "http://10.0.0.50:55556"
#define URL_DEBUG   "http://10.0.0.50:55556/debug"

#ifdef SERIAL_DEBUG
#define s(x)   Serial.print(x)
#define spl(x) Serial.println(x)
#else
#define s(x)
#define spl(x)
#endif

#define REED            2  // Pin D4
#define PIR             12 // Pin D6
#define ALARM           13 // Pin D7
#define LED             14 // Pin D5

#define NUM_LEDS        6 // LED strip length
#define LED_STATUS      5 // LED strip top
#define LED_STATE       0 // LED strip bottom

#define JSON_SIZE       192
#define CHARGE_MAX      101
#define CHARGE_MIN      0
#define ALARM_ON_TIME   100
#define ALARM_OFF_TIME  3000
#define ALARM_CHARGE    70

#define DATA_DELAY          500
#define FETCH_BLINK_DELAY   250

#endif //TESLA_CHARGE_TESLACHARGE_H
