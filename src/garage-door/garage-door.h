#ifndef TESLA_CHARGE_GARAGE_DOOR_H
#define TESLA_CHARGE_GARAGE_DOOR_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#define SERIAL_DEBUG        /* Serial output (comment to disable) */

#define DEBUG_URL       1   /* Call the debug data HTTP URL */
#define URL             "http://192.168.0.250:55556"
#define URL_DEBUG       "http://192.168.0.250:55556/debug"

#define DOOR_CHECK_DELAY    1000
#define DOOR_CLOSE_TIME     3000

#define JSON_SIZE           192

#ifdef SERIAL_DEBUG
#define s(x)   Serial.print(x)
#define spl(x) Serial.println(x)
#else
#define s(x)
#define spl(x)
#endif

#define DOOR_SENSOR_PIN     14 // Pin D5
#define DOOR_RELAY_PIN      12 // Pin D6

#endif //TESLA_CHARGE_GARAGE_DOOR_H
