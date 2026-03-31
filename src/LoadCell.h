#ifndef LoadCell_h
#define LoadCell_h
#include <Arduino.h>
#include "HX711.h"
#include <stdint.h>

class LoadCell{

    public:
        //========== Konstruktor ==========//
        LoadCell(const byte dataPin, const byte clockPin);

        //========== Funktions-Prototypen Public ==========//
        double getMeanWheight(const byte NUM_SAMPLES);
        double getRawWheight();
        bool tare(byte samples = 10);
        void clearTare();

        //========== Neu: Power-Management ==========//
        void powerOn(uint16_t settleMs = 100);
        void powerOff();
        bool isPowered() const;

    private:

        //========== Funktions-Prototypen Privat ==========//
        double calcWeight(long analogVal);

        //========== Variablen  Privat ==========//
        const byte _dataPin;
        const byte _clockPin;
        HX711 _scale;

        // Neu
        byte _powerPin = 0xFF;              // 0xFF = kein externer Power-Pin verwendet
        bool _powerPinActiveHigh = true;
        bool _powered = true;

        bool   _tareActive = false;
        double _tareOffset_g = 0.0;   // gespeicherter Nullpunkt in Gramm
};

#endif