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

    private:

        //========== Funktions-Prototypen Privat ==========//
        double calcWeight(long analogVal);

        //========== Variablen  Privat ==========//
        const byte _dataPin;
        const byte _clockPin;
        HX711 _scale;

        bool   _tareActive = false;
        double _tareOffset_g = 0.0;   // gespeicherter Nullpunkt in Gramm           

};

#endif