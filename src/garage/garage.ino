#include "garage.h"

char*           urlTesla;
char*           urlGarage;
char*           urlUpdate;

bool            gotData = false;
bool            doorAutoCloseAuthorized = false;

int8_t*         teslaData;
uint8_t         lastDoorPosition = -1;

unsigned long   doorOpenTime = 0;

enum            shiftState {P, R, D};
enum            doorStatus {DOOR_CLOSED, DOOR_OPEN, DOOR_CLOSING, DOOR_OPENING};
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

    ArduinoOTA.begin();
}

void loop() {
    int apiResult = fetchGarageData();

    if (apiResult < 0) {
        spl(F("Error retrieving garage API data"));
        delay(1000);
        return;
    }

    uint8_t garageDoorState = doorState();

    autoCloseDoor();
    pendingOperations();
}

bool doorAutoCloseCondition () {
    if (! garageStruct.autoCloseEnabled) {
        spl(F("Door auto close disabled"));
        return false;
    }

    if (doorState() == DOOR_CLOSED) {
        spl(F("Door already closed"));
        return false;
    }

    uint8_t* teslaData = fetchTeslaData();

    uint8_t carInGarage     = teslaData[1];
    uint8_t teslaError      = teslaData[5];
    uint8_t teslaFetching   = teslaData[7];

    uint8_t attempts = 0;

    while (teslaError == 1 || teslaFetching == 1) {
        delay(TESLA_API_DELAY);

        teslaData = fetchTeslaData();

        carInGarage     = teslaData[1];
        teslaError      = teslaData[5];
        teslaFetching   = teslaData[7];

        if (attempts == TESLA_API_RETRIES) {
            continue;
        }

        attempts++;
    }

    if (! carInGarage) {
        spl(F("All auto close conditions met (car not in garage)"));
        return true;
    }

    spl(F("Car is in garage"));
    return false;
}

uint8_t doorState () {
    uint8_t doorOpen = ! digitalRead(DOOR_OPEN_PIN);
    uint8_t doorClosed = ! digitalRead(DOOR_CLOSED_PIN);

    uint8_t doorState = lastDoorPosition;

    if (doorOpen) {
        doorState = DOOR_OPEN;
        if (! digitalRead(DOOR_OPEN_LED)) {
            digitalWrite(DOOR_OPEN_LED, HIGH);
        }
    }
    else if (doorClosed) {
        doorState = DOOR_CLOSED;
        if (digitalRead(DOOR_OPEN_LED)) {
            digitalWrite(DOOR_OPEN_LED, LOW);
        }
    }
    else {
        if (lastDoorPosition == DOOR_OPEN || lastDoorPosition == DOOR_CLOSING) {
            doorState = DOOR_CLOSING;
        }
        else if (lastDoorPosition == DOOR_CLOSED || lastDoorPosition == DOOR_OPENING) {
            doorState = DOOR_OPENING;
        }
        else {
            spl(F("Door is in an unknown state"));
            doorState = -1;
        }
    }

    if (doorState != lastDoorPosition) {
        updateData(doorState);

        s(F("doorState: "));
        spl(doorState);
        s(F("lastDoorPosition: "));
        spl(lastDoorPosition);
    }

    lastDoorPosition = doorState;

    return doorState;
}

void autoCloseDoor () {

    uint8_t doorPosition = doorState();

    if (doorPosition == DOOR_OPEN) {
        if (doorOpenTime == 0) {
            doorOpenTime = millis();
        }

        if (millis() - doorOpenTime >= DOOR_CLOSE_TIME) {
            bool canCloseDoor = doorAutoCloseCondition();

            if (canCloseDoor) {
                doorOperate();
            }

            doorOpenTime = 0;
        }
    }
    else {
        if (doorOpenTime > 0) {
            doorOpenTime = 0;
        }
    }
}

void pendingOperations () {
    if (garageStruct.appEnabled) {
        int8_t activity = garageStruct.activity;

        if (activity > 0) {
            // Reset pending activity flag
            garageStruct.activity = 0;
        }

        if (activity == OPERATE_DOOR) {
            spl(F("Manually operating garage door"));
            doorOperate();
        }
    }
    else {
        spl(F("App is disabled; can't perform action"));
    }

    updateData(doorState());
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

    static uint8_t teslaData[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    int httpCode = http.GET();

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        gotData = false;
        teslaData[5] = 1;
        http.end();
        return teslaData;
    }

    StaticJsonDocument<JSON_SIZE> json;
    DeserializationError error = deserializeJson(json, http.getString());

    if (error) {
        gotData = false;
        teslaData[5] = 1;
        http.end();
        return teslaData;
    }

    teslaData[0] = json["online"];
    teslaData[1] = json["garage"];
    teslaData[2] = json["gear"];
    teslaData[3] = json["charge"];
    teslaData[4] = json["charging"];
    teslaData[5] = json["error"];
    teslaData[6] = json["rainbow"];
    teslaData[7] = json["fetching"];

    http.end();

    gotData = true;

    return teslaData;
}

void updateData (uint8_t doorState) {

    http.begin(wifi, urlUpdate);
    http.setTimeout(8000);
    http.addHeader(F("Content-Type"), F("application/json"));

    DynamicJsonDocument jsonDoc(128);

    jsonDoc[F("door_state")] = doorState;
    jsonDoc[F("activity")] = garageStruct.activity;

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

    Serial.print(F("MAC Address: "));
    Serial.println(WiFi.macAddress());

    char ssid[16];
    char ssidPassword[16];

    readEEPROM(0,  16, ssid);
    readEEPROM(16, 16, ssidPassword);

    WiFi.begin(ssid, ssidPassword);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("RSSI: ") + (String) WiFi.RSSI());
        delay(500);
    }

    Serial.println(F("Wifi Connected"));

    Serial.print(F("IP address:\t"));
    Serial.println(WiFi.localIP());

    delay(1000);
}
