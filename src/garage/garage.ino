#include "garage.h"

unsigned long   doorCheckTime;
bool            gotData = false;
int8_t*         data;
char*           urlTesla;
char*           urlGarage;
char*           urlUpdate;
uint8_t         lastDoorPosition;
enum            shiftState {P, R, D};
enum            doorStatus {DOOR_CLOSED, DOOR_OPEN, DOOR_OPENING, DOOR_CLOSING};
enum            operation {NONE, OPERATE_DOOR};

garageData garageStruct;

HTTPClient http;
WiFiClient wifi;

void setup() {
    pinMode(DOOR_RELAY_PIN, OUTPUT);
    digitalWrite(DOOR_RELAY_PIN, LOW);

    pinMode(DOOR_OPEN_PIN, INPUT_PULLUP);
    pinMode(DOOR_CLOSED_PIN, INPUT_PULLUP);

    pinMode(DOOR_OPEN_LED, OUTPUT);
    digitalWrite(DOOR_OPEN_LED, LOW);

    Serial.begin(9600);
    wifiSetup();

    if (DEBUG_URL) {
        urlTesla  = URL_TESLA;
        urlGarage = URL_GARAGE;
        urlUpdate = URL_UPDATE;
    }
    else {
        urlTesla  = URL_TESLA;
        urlGarage = URL_GARAGE;
        urlUpdate = URL_UPDATE;
    }

    doorCheckTime = millis();

    ArduinoOTA.begin();
}

void loop() {
    int apiResult = fetchGarageData();

    if (apiResult < 0) {
        spl("Error retrieving API data");
        delay(1000);
        return;
    }

    uint8_t garageDoorState = doorState();

    spl(garageDoorState);

    // autoCloseDoor()

    // pendingOperations()
}

bool doorAutoCloseCondition () {

    // get / (tesla) data separately (separate out from /garage_data)

    if (! garageStruct.autoCloseEnabled) {
        spl("Door auto close disabled");
        return false;
    }

    if (doorState() == DOOR_OPEN) {
        spl("Door already open");
        return false;
    }

    uint8_t* teslaData = fetchTeslaData();

    uint8_t carInGarage     = data[1];
    uint8_t teslaError      = data[5];
    uint8_t teslaFetching   = data[7];

    uint8_t attempts = 0;

    while (teslaError == 1 || teslaFetching == 1) {
        delay(TESLA_API_DELAY);

        teslaData = fetchTeslaData();

        carInGarage     = data[1];
        teslaError      = data[5];
        teslaFetching   = data[7];

        if (attempts == TESLA_API_RETRIES) {
            continue;
        }

        attempts++;
    }

    if (! carInGarage) {
        spl("All auto close conditions met (car in garage)");
        return true;
    }

    return false;
}

uint8_t doorState () {
    uint8_t doorOpen = ! digitalRead(DOOR_OPEN_PIN);
    uint8_t doorClosed = ! digitalRead(DOOR_CLOSED_PIN);

    uint8_t doorState;

    if (doorOpen) {
        doorState = DOOR_OPEN;
        spl(F("Door open"));
        lastDoorPosition = DOOR_OPEN;
        digitalWrite(DOOR_OPEN_LED, HIGH);
    }
    else if (doorClosed) {
        doorState = DOOR_CLOSED;
        spl(F("Door closed"));
        lastDoorPosition = DOOR_CLOSED;
        if (digitalRead(DOOR_OPEN_LED)) {
            digitalWrite(DOOR_OPEN_LED, LOW);
        }
    }
    else {
        if (lastDoorPosition == DOOR_OPEN) {
            spl(F("Door closing"));
            doorState = DOOR_CLOSING;
        }
        else if (lastDoorPosition == DOOR_CLOSED) {
            spl(F("Door opening"));
            doorState = DOOR_OPENING;
        }
        else {
            spl(F("Door is in an unknown state"));
            doorState = -1;
        }
    }

    updateData(doorState);
    return doorState;
}

void autoCloseDoor (uint8_t doorState) {

    if (doorState == DOOR_OPEN) {
        // - save door open time, increment each loop

        // - if door open time > 5 mins

        // bool canCloseDoor = doorAutoCloseCondition();
        //
        // if (canCloseDoor) {
        //     doorOperate();
        // }

    }
    else if (doorState == DOOR_CLOSED){
        // - if door open time > 0, reset it to 0
    }
}

void pendingOperations () {
    if (garageStruct.appEnabled) {
        int8_t activity = garageStruct.activity;

        if (activity == OPERATE_DOOR) {
            spl("Manually operating garage door");
            doorOperate();
        }
    }
    else {
        spl("App is disabled; can't perform action");
    }
}

void doorOperate () {
    uint8_t door = doorState();

    if (door == DOOR_OPEN) {
        spl(F("Closing door"));
        doorActivate();
    }
    else if (door == DOOR_CLOSED) {
        spl(F("Opening door"));
        doorActivate();
    }
}

void doorActivate () {
    if (garageStruct.relayEnabled) {
        digitalWrite(DOOR_RELAY_PIN, HIGH);
        delay(250);
        digitalWrite(DOOR_RELAY_PIN, LOW);
    }
}

int fetchGarageData () {

    http.begin(wifi, urlGarage);
    http.setTimeout(8000);

    int httpCode = http.GET();

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        http.end();
        return httpCode;
    }

    StaticJsonDocument<JSON_SIZE> json;
    DeserializationError error = deserializeJson(json, http.getString());

    if (error) {
        s(F("Error: "));
        spl(error.c_str());
        http.end();
        return -1;
    }

    garageStruct.garageDoorState    = json["garage_door_state"];
    garageStruct.teslaInGarage      = json["tesla_in_garage"];
    garageStruct.activity           = json["activity"];
    garageStruct.relayEnabled       = json["relay_enabled"];
    garageStruct.appEnabled         = json["app_enabled"];
    garageStruct.autoCloseEnabled   = json["auto_close_enabled"];

    http.end();

    return 0;
}

uint8_t* fetchTeslaData () {

    http.begin(wifi, urlTesla);
    http.setTimeout(8000);

    static uint8_t data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    int httpCode = http.GET();

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        gotData = false;
        data[5] = 1;
        http.end();
        return data;
    }

    StaticJsonDocument<JSON_SIZE> json;
    DeserializationError error = deserializeJson(json, http.getString());

    if (error) {
        gotData = false;
        data[5] = 1;
        http.end();
        return data;
    }

    data[0] = json["online"];
    data[1] = json["garage"];
    data[2] = json["gear"];
    data[3] = json["charge"];
    data[4] = json["charging"];
    data[5] = json["error"];
    data[6] = json["rainbow"];
    data[7] = json["fetching"];

    http.end();

    gotData = true;

    return data;
}

void updateData (uint8_t doorState) {

    http.begin(wifi, urlUpdate);
    http.setTimeout(8000);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument jsonDoc(128);

    jsonDoc["door_state"] = doorState;

    char jsonData[192];
    serializeJson(jsonDoc, jsonData);

    int httpCode = http.POST(jsonData);

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        http.end();
    }

    http.end();
}

void readEEPROM(int startAdr, int maxLength, char* dest) {
    EEPROM.begin(512);
    delay(10);
    for (int i = 0; i < maxLength; i++) {
        dest[i] = char(EEPROM.read(startAdr + i));
    }
    EEPROM.end();
}

void wifiSetup () {

    s(F("MAC Address: "));
    spl(WiFi.macAddress());

    char ssid[16];
    char ssidPassword[16];

    readEEPROM(0,  16, ssid);
    readEEPROM(16, 16, ssidPassword);

    WiFi.begin(ssid, ssidPassword);

    while (WiFi.status() != WL_CONNECTED) {
        spl("RSSI: " + (String) WiFi.RSSI());
        delay(500);
    }
    spl(F("Wifi Connected"));

    delay(1000);
}
