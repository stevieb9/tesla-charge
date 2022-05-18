#ifndef TESLA_CHARGE_GARAGE_H
#define TESLA_CHARGE_GARAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#define SERIAL_DEBUG        /* Serial output (comment to disable) */

#define LOOP_CYCLE_DELAY    500     // Delays the loop cycle
#define DOOR_CLOSE_TIME     20000   // Millieconds door must be open before auto-close
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
#define WIFI_CONFIG_PIN     5  // Pin D1

class garageData {
    public :
        int8_t garageDoorState;
        int8_t teslaInGarage;
        int8_t activity;
        int8_t relayEnabled;
        int8_t appEnabled;
        int8_t autoCloseEnabled;
};

char garageURL[64];             // Maximum length of the garage URL
char updateURL[64];             // Maximum length of the garage update URL

#endif //TESLA_CHARGE_GARAGE_H
