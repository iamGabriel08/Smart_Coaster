#include "LoadCell.h"

//========== Konstruktor ==========//
LoadCell::LoadCell(const byte dataPin, const byte clockPin)
: _dataPin(dataPin), _clockPin(clockPin) {

    _scale.begin(_dataPin, _clockPin);
}

//========== Öffentliche Funktions-Implementierungen  ==========//

double LoadCell::getMeanWheight(const byte NUM_SAMPLES){
    static int64_t sumAdc = 0;
    static uint8_t count  = 0;

    static double lastWeight = 0.0;
    static bool hasMean = false;

    if (!_powered) {
        return hasMean ? lastWeight : 0.0;
    }

    if (_scale.is_ready()) {
        long raw = _scale.read();
        sumAdc += raw;
        count++;

        if (count >= NUM_SAMPLES) {
            int64_t meanAdc = sumAdc / count;
            lastWeight = calcWeight((long)meanAdc);
            hasMean = true;

            sumAdc = 0;
            count  = 0;
            return lastWeight;
        }
    }

    return hasMean ? lastWeight : 0.0;
}

double LoadCell::getRawWheight(){
    static double lastWeight = 0.0;
    static bool hasRaw = false;

    if (!_powered) {
        return hasRaw ? lastWeight : 0.0;
    }

    if (_scale.is_ready()) {
        long raw = _scale.read();
        double weight = calcWeight(raw);

        lastWeight = weight;
        hasRaw = true;

        return weight;
    }

    return hasRaw ? lastWeight : 0.0;
}

bool LoadCell::tare(byte numSamples) {
    if (!_powered) return false;

    int64_t sum = 0;
    uint8_t cnt = 0;

    unsigned long t0 = millis();
    while (cnt < numSamples && (millis() - t0) < 1000) {
        if (_scale.is_ready()) {
            sum += _scale.read();
            cnt++;
        }
        delay(1);
    }

    if (cnt == 0) return false;

    long meanAdc = (long)(sum / cnt);

    bool oldActive = _tareActive;
    _tareActive = false;
    _tareOffset_g = calcWeight(meanAdc);
    _tareActive = oldActive;

    _tareActive = true;
    return true;
}

void LoadCell::clearTare() {
    _tareActive = false;
    _tareOffset_g = 0.0;
}

//========== Neu: Power-Management ==========//


void LoadCell::powerOff() {
    // HX711 intern in Power-Down
    _scale.power_down();
    delayMicroseconds(80);

    // GPIOs hochohmig machen, damit keine Rückspeisung entsteht
    pinMode(_dataPin, INPUT);
    pinMode(_clockPin, INPUT);

    _powered = false;
}

void LoadCell::powerOn(uint16_t settleMs) {

    // HX711 wieder initialisieren
    _scale.begin(_dataPin, _clockPin);
    _scale.power_up();

    // etwas Zeit für HX711 + Wägezelle zum Einschwingen
    delay(settleMs);

    _powered = true;
}

bool LoadCell::isPowered() const {
    return _powered;
}

//========== Private Funktions-Implementierungen  ==========//

double LoadCell::calcWeight(long analogVal){
    double w = 0.010008 * analogVal - 1838.82;
    if (_tareActive) w -= _tareOffset_g;
    return w;
}