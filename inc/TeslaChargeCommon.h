#ifndef TESLA_CHARGE_TESLACHARGE_COMMON_H
#define TESLA_CHARGE_TESLACHARGE_COMMON_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoOTA.h>

#define SERIAL_DEBUG // Serial output (comment to disable)

#ifdef SERIAL_DEBUG
#define sp(x)   Serial.print(x)
#define spl(x) Serial.println(x)
#else
#define sp(x)
#define spl(x)
#endif

uint8_t MacController[] = {0x5C, 0xCF, 0x7F, 0xCC, 0xA1, 0x8E};
uint8_t MacInterface[] = {0x60, 0x01, 0x94, 0x71, 0xFF, 0xB0};

typedef struct VehicleData {
    uint8_t state;
    uint8_t charge;
} VehicleData;

#endif