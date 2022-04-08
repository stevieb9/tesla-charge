#ifndef TESLA_CHARGE_TESLACHARGE_INTERFACE_H
#define TESLA_CHARGE_TESLACHARGE_INTERFACE_H

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "SSD1306Wire.h"
#include "/Users/steve/repos/tesla-charge/inc/TeslaChargeFont.h"

#define DEBUG_URL       0   /* Call the debug data HTTP URL */
#define DEBUG_DEVEL     0   /* Testing on a non production unit */
#define DEBUG_MAGNET    0   /* REED switch on at all times*/
#define DEBUG_MOTION    0   /* PIR on at all times */

//#define URL         "http://192.168.1.252:55556"
//#define URL_DEBUG   "http://192.168.1.252:55556/debug"

#define URL         "http://10.0.0.50:55556"
#define URL_DEBUG   "http://10.0.0.50:55556/debug"

#define REED            2  // Pin D4
#define PIR             12 // Pin D6
#define ALARM           13 // Pin D7

#define JSON_SIZE       224
#define CHARGE_MAX      101
#define CHARGE_MIN      0
#define ALARM_ON_TIME   100
#define ALARM_OFF_TIME  3000
#define ALARM_CHARGE    70

#define DATA_DELAY          500 // ms to wait between API calls
#define FETCH_BLINK_DELAY   250 // ms to leave fetch LED on

#endif