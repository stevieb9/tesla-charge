#include "garage-door.h"

unsigned long   doorCheckTime;
bool            doorClosing = false;
bool            gotData = false;
int8_t*         data;
char*           url;
enum            shiftState {P, R, D};
enum            doorStatus {DOOR_CLOSED, DOOR_OPEN, DOOR_MOVING};

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
        url = URL_DEBUG;
    }
    else {
        url = URL;
    }

    doorCheckTime = millis();

    ArduinoOTA.begin();
}

void loop() {
    uint8_t doorPosition = doorState();
    
    // autoCloseDoor(doorPosition);
}

bool doorAutoCloseCondition () {
    data = fetchData();
    int8_t carInGarage = data[0];

    if (! carInGarage) {
        return true;
    }
    else {
        return false;
    }
}

uint8_t doorState () {
    uint8_t doorOpen = ! digitalRead(DOOR_OPEN_PIN);
    uint8_t doorClosed = ! digitalRead(DOOR_CLOSED_PIN);

    uint8_t doorState;

    if (doorOpen) {
        doorState = DOOR_OPEN;
        spl(F("Door open"));
        digitalWrite(DOOR_OPEN_LED, HIGH);

    }
    else if (doorClosed) {
        doorState = DOOR_CLOSED;
        spl(F("Door closed"));
        if (digitalRead(DOOR_OPEN_LED)) {
            digitalWrite(DOOR_OPEN_LED, LOW);
        }
    }
    else {
        doorState = DOOR_MOVING;
        spl(F("Door moving"));
    }

    updateData(doorState);
    return doorState;
}

void autoCloseDoor (uint8_t doorState) {
    unsigned long currentTime = millis();

    if (currentTime - doorCheckTime >= DOOR_CHECK_DELAY) {
        
        bool canCloseDoor = doorAutoCloseCondition();

        if (canCloseDoor) {
            doorClose();
        }
        
        doorCheckTime = currentTime;
    }
}

void doorClose () {
    uint8_t door = doorState();

    if (door == DOOR_OPEN && ! doorClosing) {
        spl(F("Closing door"));
        doorClosing = true;
        doorActivate();
    }
    else if (door == DOOR_CLOSED) {
        doorClosing = false;
    }
}

void doorActivate () {
    digitalWrite(DOOR_RELAY_PIN, HIGH);
    delay(250);
    digitalWrite(DOOR_RELAY_PIN, LOW);
}

int8_t* fetchData () {

    http.begin(wifi, url);
    http.setTimeout(8000);

    static int8_t data[1] = { -1};

    int httpCode = http.GET();

    if (httpCode < 0) {
        s(F("HTTP Error Code: "));
        spl(httpCode);
        http.end();
        return data;
    }

    StaticJsonDocument<JSON_SIZE> json;
    DeserializationError error = deserializeJson(json, http.getString());

    if (error) {
        s(F("Error: "));
        spl(error.c_str());
        http.end();
        return data;
    }

    data[0] = json["garage"];

    http.end();

    return data;
}

void updateData (bool doorState) {

    http.begin(wifi, url);
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
