#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <espnow.h>

#include <Adafruit_Sensor.h>

// Female
//uint8_t peerMAC[] = {0xC8, 0x2B, 0x96, 0x08, 0x58, 0x47};

// Male
uint8_t peerMAC[] = {0xC8, 0x2B, 0x96, 0x08, 0x65, 0x4E};

typedef struct struct_message {
    uint8_t state;
    uint8_t settings;
} struct_message;

struct_message dataOut;
struct_message dataIn;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
    Serial.print("Last Packet Send Status: ");
    if (sendStatus == 0){
        Serial.println("Delivery success");
    }
    else{
        Serial.println("Delivery fail");
    }
}

void OnDataRecv(uint8_t * mac, uint8_t *dataInbound, uint8_t len) {
    memcpy(&dataIn, dataInbound, sizeof(dataIn));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print(F("State: "));
    Serial.println(dataIn.state);
    Serial.print(F("Settings: "));
    Serial.println(dataIn.settings);
}

void setup() {
    Serial.begin(9600);

    wifiSetup();

    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    esp_now_add_peer(peerMAC, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
}

void loop() {

    dataOut.state = 3;
    dataOut.settings = 66;

    esp_now_send(peerMAC, (uint8_t *) &dataOut, sizeof(dataOut));

    delay(1000);
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
        Serial.println("RSSI: " + (String) WiFi.RSSI());
        delay(500);
    }
    Serial.println(F("Wifi Connected"));

    delay(1000);
}
void readEEPROM(int startAdr, int maxLength, char* dest) {
    EEPROM.begin(512);
    delay(10);
    for (int i = 0; i < maxLength; i++) {
        dest[i] = char(EEPROM.read(startAdr + i));
    }
    EEPROM.end();
}