#ifndef TESLA_CHARGE_TESLACHARGE_COMMON_H
#define TESLA_CHARGE_TESLACHARGE_COMMON_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoOTA.h>

#define SERIAL_DEBUG        /* Serial output (comment to disable) */

//#define URL         "http://192.168.1.252:55556"
//#define URL_DEBUG   "http://192.168.1.252:55556/debug"

#define URL         "http://10.0.0.50:55556"
#define URL_DEBUG   "http://10.0.0.50:55556/debug"

#ifdef SERIAL_DEBUG
#define sp(x)   Serial.print(x)
#define spl(x) Serial.println(x)
#else
#define sp(x)
#define spl(x)
#endif

uint8_t MacController[] = {0x5C, 0xCF, 0x7F, 0xCC, 0xA1, 0x8E}; // Female
uint8_t MacInterface[] = {0xC8, 0x2B, 0x96, 0x08, 0x65, 0x4E};  // Male

typedef struct VehicleData {
    uint8_t state;
    uint8_t charge;
} VehicleData;

#endif