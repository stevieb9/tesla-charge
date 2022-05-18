#ifndef TESLA_CHARGE_TESLACHARGE_COMMON_H
#define TESLA_CHARGE_TESLACHARGE_COMMON_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <espnow.h>
#include <WiFiManager.h>

#define SERIAL_DEBUG        // Serial output (comment to disable)
#define CONFIG_RESET    0   // Enable AP mode to reset WiFi settings

#ifdef SERIAL_DEBUG
#define sp(x)   Serial.print(x)
#define spl(x) Serial.println(x)
#else
#define sp(x)
#define spl(x)
#endif

// MAC addresses of the controllers for the ESP-NOW protocol

uint8_t MacInterface[] = {0x60, 0x01, 0x94, 0x71, 0xFF, 0xB0};
uint8_t MacController[] = {0x5C, 0xCF, 0x7F, 0xCC, 0xA1, 0x8E};

// Access Point names when the controllers are in config mode

const char* apNameInterface = "TeslaInterface";
const char* apNameController = "TeslaController";

typedef struct VehicleData {
    uint8_t state;
    uint8_t charge;
} VehicleData;

char apiURL[64];             // Maximum length of the API URL
char apiToken[86];           // Maximum length of API token (don't change this!)

#endif