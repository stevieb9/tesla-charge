#include "garage-door.h"

unsigned long   doorCheckTime;
bool            doorClosing = false;
bool            gotData = false;
int8_t*         data;
char*           url;
enum            shiftState {P, R, D};
enum            doorStatus {DOOR_CLOSED, DOOR_OPEN};

HTTPClient http;

void setup() {
    pinMode(DOOR_RELAY_PIN, OUTPUT);
    digitalWrite(DOOR_RELAY_PIN, LOW);

    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
    
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
    unsigned long currentTime = millis();

    if (currentTime - doorCheckTime >= DOOR_CHECK_DELAY) {
        uint8_t door = doorState();

        bool canCloseDoor = false;

        if (doorClosing) {
            canCloseDoor = true;
        }
        else if (door == DOOR_OPEN) {
            digitalWrite(DOOR_OPEN_LED, HIGH);
            canCloseDoor = doorCloseCondition();
        }
        else {
            digitalWrite(DOOR_OPEN_LED, LOW);
        }

        s(F("canCloseDoor: "));
        spl(canCloseDoor);
        
        if (canCloseDoor) {
            doorClose();
        }

        doorCheckTime = currentTime;        
    }
}

bool doorCloseCondition () {
    data = fetchData();
    int8_t carInGarage = data[0];
    int8_t gear        = data[1];

    if (! carInGarage && gear == P) {
        return true;
    }
    else {
        return false;
    }
}

uint8_t doorState () {
    uint8_t doorState = digitalRead(DOOR_SENSOR_PIN);
    updateData(doorState);
    return doorState;
}

void doorClose () {
    uint8_t door = doorState();

    if (door == DOOR_OPEN && ! doorClosing) {
        spl(F("Open"));
        doorClosing = true;
        doorActivate();
    }
    else if (door == DOOR_OPEN && doorClosing) {
        spl(F("Closing"));
    }
    else {
        spl(F("Closed"));
        doorClosing = false;
    }
}

void doorActivate () {
    digitalWrite(DOOR_RELAY_PIN, HIGH);
    delay(250);
    digitalWrite(DOOR_RELAY_PIN, LOW);
}

int8_t* fetchData () {

    http.begin(url);
    http.setTimeout(8000);

    static int8_t data[2] = {-1, -1};

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
    data[1] = json["gear"];

    http.end();

    return data;
}

void updateData (bool doorOpen) {

    http.begin(url);
    http.setTimeout(8000);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument jsonDoc(128);

    jsonDoc["open"] = doorOpen;

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
