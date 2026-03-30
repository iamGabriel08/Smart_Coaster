#include "RC522Reader.h"

//========== Konstruktor ==========//
RC522Reader::RC522Reader(byte RC522_SS_PIN, byte RC522_RST_PIN, byte RC522_SCK_PIN, byte RC522_MISO_PIN, byte RC522_MOSI_PIN, byte GATE_PIN)
: _RC522_SS_PIN(RC522_SS_PIN),
  _RC522_RST_PIN(RC522_RST_PIN),
  _RC522_SCK_PIN(RC522_SCK_PIN),
  _RC522_MISO_PIN(RC522_MISO_PIN),
  _RC522_MOSI_PIN(RC522_MOSI_PIN),
  _GATE_PIN(GATE_PIN),
  _rfid(RC522_SS_PIN, RC522_RST_PIN),
  _cardData{{0}, 0, MFRC522::PICC_TYPE_UNKNOWN, false}
  { }

//========== Public-Funktionsimplementierungen ==========//
void RC522Reader::begin() {
    pinMode(_GATE_PIN, OUTPUT);
    powerOn();
    delay(1000);

    SPI.begin(_RC522_SCK_PIN, _RC522_MISO_PIN, _RC522_MOSI_PIN, _RC522_SS_PIN);
    _rfid.PCD_Init();

    delay(50);

    Serial.println();
    Serial.println("RC522 initialisiert.");
    Serial.println("Halte einen RFID-Tag an den Leser...");
    Serial.println();
}

void RC522Reader::powerOn() {
    digitalWrite(_GATE_PIN, HIGH);
}

void RC522Reader::powerOff() {
    digitalWrite(_GATE_PIN, LOW);
}

bool RC522Reader::readCard() {
    _cardData.valid = false;
    _cardData.uidSize = 0;
    _cardData.piccType = MFRC522::PICC_TYPE_UNKNOWN;

    if (!_rfid.PICC_IsNewCardPresent()) {
        return false;
    }

    if (!_rfid.PICC_ReadCardSerial()) {
        return false;
    }

    _cardData.uidSize = _rfid.uid.size;
    _cardData.piccType = _rfid.PICC_GetType(_rfid.uid.sak);
    _cardData.valid = true;

    for (byte i = 0; i < _rfid.uid.size && i < sizeof(_cardData.uid); i++) {
        _cardData.uid[i] = _rfid.uid.uidByte[i];
    }

    return true;
}

void RC522Reader::printCardData() const {
    if (!_cardData.valid) {
        Serial.println("Keine gueltige Karte vorhanden.");
        return;
    }

    Serial.println("=== Karte erkannt ===");

    Serial.print("Typ: ");
    Serial.println(_rfid.PICC_GetTypeName(_cardData.piccType));

    Serial.print("UID (HEX): ");
    printHex((byte*)_cardData.uid, _cardData.uidSize);
    Serial.println();

    Serial.print("UID (DEC): ");
    printDec((byte*)_cardData.uid, _cardData.uidSize);
    Serial.println();

    Serial.print("UID-Laenge: ");
    Serial.print(_cardData.uidSize);
    Serial.println(" Byte");
    Serial.println();
}

void RC522Reader::endCommunication() {
    _rfid.PICC_HaltA();
    _rfid.PCD_StopCrypto1();
}

void RC522Reader::hardSleep() {
    endCommunication();
    _rfid.PCD_AntennaOff();
    SPI.end();

    pinMode(_RC522_SS_PIN, OUTPUT);
    digitalWrite(_RC522_SS_PIN, HIGH);

    pinMode(_RC522_SCK_PIN, OUTPUT);
    digitalWrite(_RC522_SCK_PIN, LOW);

    pinMode(_RC522_MOSI_PIN, OUTPUT);
    digitalWrite(_RC522_MOSI_PIN, LOW);

    pinMode(_RC522_MISO_PIN, INPUT);

    pinMode(_RC522_RST_PIN, OUTPUT);
    digitalWrite(_RC522_RST_PIN, LOW);
}

void RC522Reader::hardWake() {
    digitalWrite(_RC522_RST_PIN, HIGH);
    delay(5);
    SPI.begin(_RC522_SCK_PIN, _RC522_MISO_PIN, _RC522_MOSI_PIN, _RC522_SS_PIN);
    _rfid.PCD_Init();
}

RC522Reader::CardData RC522Reader::getCardData() const {
    return _cardData;
}

bool RC522Reader::hasValidCard() const {
    return _cardData.valid;
}

//========== Private-Funktionsimplementierungen ==========//
void RC522Reader::printHex(byte *buffer, byte bufferSize) const {
    for (byte i = 0; i < bufferSize; i++) {
        if (buffer[i] < 0x10) Serial.print("0");
        Serial.print(buffer[i], HEX);
        if (i < bufferSize - 1) Serial.print(":");
    }
}

void RC522Reader::printDec(byte *buffer, byte bufferSize) const {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i], DEC);
        if (i < bufferSize - 1) Serial.print(".");
    }
}