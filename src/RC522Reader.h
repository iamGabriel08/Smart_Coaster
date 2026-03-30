#ifndef RC522Reader_h
#define RC522Reader_h

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

class RC522Reader {

    public:
        //========== Hilfsstrukturen ==========//
        struct CardData {
            byte uid[10];
            byte uidSize;
            MFRC522::PICC_Type piccType;
            bool valid;
        };

        //========== Konstruktor ==========//
        RC522Reader(byte RC522_SS_PIN, byte RC522_RST_PIN, byte RC522_SCK_PIN, byte RC522_MISO_PIN, byte RC522_MOSI_PIN, byte GATE_PIN);

        //========== Public-Funktionsdeklarationen ==========//
        void begin();
        void powerOn();
        void powerOff();
        bool readCard();
        void printCardData() const;
        void endCommunication();
        void hardSleep();
        void hardWake();
        CardData getCardData() const;
        bool hasValidCard() const;

    private:
        //========== Private-Variablen ==========//
        const byte _RC522_SS_PIN;
        const byte _RC522_RST_PIN;
        const byte _RC522_SCK_PIN;
        const byte _RC522_MISO_PIN;
        const byte _RC522_MOSI_PIN;
        const byte _GATE_PIN;

        MFRC522 _rfid;
        CardData _cardData;

        //========== Private-Funktionsdeklarationen ==========//
        void printHex(byte *buffer, byte bufferSize) const;
        void printDec(byte *buffer, byte bufferSize) const;
};

#endif