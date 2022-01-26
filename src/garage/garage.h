#ifndef TESLA_CHARGE_GARAGE_H
#define TESLA_CHARGE_GARAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

//#define SERIAL_DEBUG        /* Serial output (comment to disable) */

#define DEBUG_TESLA_URL       0   /* Call the Tesla debug data HTTP URL */
#define DEBUG_GARAGE_URL      0   /* Call the garage debug data HTTP URL */

#define URL_TESLA   "http://192.168.1.5:55556/"
#define URL_GARAGE  "http://192.168.1.5:55556/garage_data"
#define URL_UPDATE  "http://192.168.1.5:55556/garage_update"

#define URL_DEBUG_TESLA     "http://192.168.1.5:55556/debug"
#define URL_DEBUG_GARAGE    "http://192.168.1.5:55556/debug_garage"

#define DOOR_CHECK_DELAY    1000    // UNUSED - loop() delay timer
#define DOOR_CLOSE_TIME     3000    // Seconds door must be open before auto-close
#define TESLA_API_DELAY     2000    // Delay to retry the Tesla API call
#define TESLA_API_RETRIES   3       // Retries to attempt Tesla API call

#define JSON_SIZE           192

#ifdef SERIAL_DEBUG
#define s(x)   Serial.print(x)
#define spl(x) Serial.println(x)
#else
#define s(x)
#define spl(x)
#endif

#define DOOR_OPEN_PIN       4  // Pin D2
#define DOOR_CLOSED_PIN     14 // Pin D5
#define DOOR_RELAY_PIN      12 // Pin D6
#define DOOR_OPEN_LED       13 // Pin D7

class garageData {
    public :
        int8_t garageDoorState;
        int8_t teslaInGarage;
        int8_t activity;
        int8_t relayEnabled;
        int8_t appEnabled;
        int8_t autoCloseEnabled;
};
#endif //TESLA_CHARGE_GARAGE_H
