#include "ADXL345Sensor.h"
#include <math.h>

//========== Statische Member ==========//
ADXL345Sensor* ADXL345Sensor::_activeInstance = nullptr;

//========== Konstruktoren ==========//
ADXL345Sensor::ADXL345Sensor(byte SDA_PIN, byte SCL_PIN, byte ADXL_INT_PIN, bool Z_AXIS_POINTS_UP)
: _adxl(),
  _SDA_PIN(SDA_PIN),
  _SCL_PIN(SCL_PIN),
  _ADXL_INT_PIN(ADXL_INT_PIN),
  _Z_AXIS_POINTS_UP(Z_AXIS_POINTS_UP),
  _calibrationData{0, 0, 0, 0, false},
  _adxlIrqFlag(false) 
  { }

ADXL345Sensor::ADXL345Sensor(byte SDA_PIN, byte SCL_PIN, byte ADXL_INT_PIN, CalibrationData calibrationData, bool Z_AXIS_POINTS_UP)
: _adxl(),
  _SDA_PIN(SDA_PIN),
  _SCL_PIN(SCL_PIN),
  _ADXL_INT_PIN(ADXL_INT_PIN),
  _Z_AXIS_POINTS_UP(Z_AXIS_POINTS_UP),
  _calibrationData(calibrationData),
  _adxlIrqFlag(false) 
  { }

//========== Private-Funktionsimplementierungen ==========//
void IRAM_ATTR ADXL345Sensor::adxlIsrRouter() {
    if (_activeInstance != nullptr) {
        _activeInstance->_adxlIrqFlag = true;
    }
}

void ADXL345Sensor::initI2C() {
    Wire.begin(_SDA_PIN, _SCL_PIN);
    Wire.setClock(100000);
}

void ADXL345Sensor::setupBase(float rateHz, bool lowPower) {
    initI2C();

    _adxl.powerOn();
    _adxl.setRangeSetting(2);          // +/-2g
    _adxl.setFullResBit(1);            // Full resolution
    _adxl.setInterruptLevelBit(0);     // active HIGH
    _adxl.setRate(rateHz);
    _adxl.setLowPower(lowPower ? 1 : 0);

    if (_calibrationData.valid) {
        _adxl.setAxisOffset(_calibrationData.offX, _calibrationData.offY, _calibrationData.offZ);
    } else {
        _adxl.setAxisOffset(0, 0, 0);
    }
}

ADXL345Sensor::XYZ ADXL345Sensor::averageSamples(uint16_t n, uint16_t delayMs) {
    long sx = 0;
    long sy = 0;
    long sz = 0;

    int x, y, z;

    for (uint16_t i = 0; i < n; i++) {
        _adxl.readAccel(&x, &y, &z);
        sx += x;
        sy += y;
        sz += z;
        delay(delayMs);
    }

    XYZ out;
    out.x = (float)sx / n;
    out.y = (float)sy / n;
    out.z = (float)sz / n;
    return out;
}

float ADXL345Sensor::max3(float a, float b, float c) {
    float m = a;
    if (b > m) m = b;
    if (c > m) m = c;
    return m;
}

int ADXL345Sensor::clampOffset(int v) {
    if (v > 127) return 127;
    if (v < -128) return -128;
    return v;
}

void ADXL345Sensor::printXYZ(Stream &out, const char *label, const XYZ &v) const {
    out.print(label);
    out.print(" X=");
    out.print(v.x, 2);
    out.print("  Y=");
    out.print(v.y, 2);
    out.print("  Z=");
    out.println(v.z, 2);
}

bool ADXL345Sensor::beginCalibrationMode() {
    setupBase(100.0f, true);
    _adxl.setAxisOffset(0, 0, 0);
    _calibrationData = {0, 0, 0, 0, false};
    return true;
}

void ADXL345Sensor::printCalibrationData(Stream &out) const {
    out.println("Diese Werte in den Mess-Sketch uebernehmen:");
    out.print("ADXL345Sensor::CalibrationData calibrationData = {");
    out.print(_calibrationData.offX);
    out.print(", ");
    out.print(_calibrationData.offY);
    out.print(", ");
    out.print(_calibrationData.offZ);
    out.print(", ");
    out.print(_calibrationData.threshActStart);
    out.println(", true};");
}

//========== Public-Funktionsimplementierungen ==========//

ADXL345Sensor::CalibrationData ADXL345Sensor::runCalibration(uint16_t offsetSamples, uint16_t offsetDelayMs, uint32_t quietPhaseMs, uint32_t eventPhaseMs) {

    Serial.println();
    Serial.println("ADXL345 Kalibrierung");
    beginCalibrationMode();
    Serial.println();
    Serial.println("=== Offset-Kalibrierung ===");
    Serial.println("Sensor in finaler Einbaulage ruhig liegen lassen.");
    Serial.println("Plattform leer lassen. Messung startet in 5 Sekunden...");
    delay(5000);

    XYZ meas = averageSamples(offsetSamples, offsetDelayMs);
    printXYZ(Serial, "Mittelwert vor Kalibrierung:", meas);

    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetZ = _Z_AXIS_POINTS_UP ? RAW_COUNTS_PER_G : -RAW_COUNTS_PER_G;

    float errX = meas.x - targetX;
    float errY = meas.y - targetY;
    float errZ = meas.z - targetZ;

    int offX = clampOffset((int)round(-errX / RAW_COUNTS_PER_OFFSET_LSB));
    int offY = clampOffset((int)round(-errY / RAW_COUNTS_PER_OFFSET_LSB));
    int offZ = clampOffset((int)round(-errZ / RAW_COUNTS_PER_OFFSET_LSB));

    Serial.print("Berechnete Offsets: OFFX=");
    Serial.print(offX);
    Serial.print(" OFFY=");
    Serial.print(offY);
    Serial.print(" OFFZ=");
    Serial.println(offZ);

    _adxl.setAxisOffset(offX, offY, offZ);
    delay(50);

    XYZ corrected = averageSamples(offsetSamples, offsetDelayMs);
    printXYZ(Serial, "Mittelwert nach Kalibrierung:", corrected);

    Serial.println();
    Serial.println("=== Charakterisierung fuer Becher-Abstellen ===");
    Serial.println("Phase 1: Plattform leer lassen, Ruhe messen...");

    XYZ baseline = averageSamples(offsetSamples, offsetDelayMs);

    float quietPeak = 0.0f;
    uint32_t t0 = millis();

    while (millis() - t0 < quietPhaseMs) {
        XYZ s = averageSamples(5, 4);

        float dx = fabs(s.x - baseline.x);
        float dy = fabs(s.y - baseline.y);
        float dz = fabs(s.z - baseline.z);

        float peak = max3(dx, dy, dz);
        if (peak > quietPeak) quietPeak = peak;

        delay(15);
    }

    Serial.print("Quiet peak [raw counts] = ");
    Serial.println(quietPeak, 2);

    Serial.println();
    Serial.println("Phase 2: Jetzt Becher mehrfach normal abstellen...");

    float eventPeak = 0.0f;
    uint32_t t1 = millis();

    while (millis() - t1 < eventPhaseMs) {
        XYZ s = averageSamples(5, 4);

        float dx = fabs(s.x - baseline.x);
        float dy = fabs(s.y - baseline.y);
        float dz = fabs(s.z - baseline.z);

        float peak = max3(dx, dy, dz);
        if (peak > eventPeak) eventPeak = peak;

        Serial.print("peak=");
        Serial.println(peak, 1);
        delay(20);
    }

    Serial.println();
    Serial.print("Event peak [raw counts] = ");
    Serial.println(eventPeak, 2);

    int suggested = max(2, (int)ceil((quietPeak * 2.0f) / RAW_COUNTS_PER_ACTIVITY_LSB));

    int maxUseful = (int)floor((eventPeak * 0.7f) / RAW_COUNTS_PER_ACTIVITY_LSB);
    if (maxUseful < 1) maxUseful = 1;
    if (suggested > maxUseful) suggested = maxUseful;

    Serial.println();
    Serial.print("Empfohlener THRESH_ACT Startwert = ");
    Serial.println(suggested);
    Serial.print("Das entspricht ca. ");
    Serial.print(suggested * 62.5f, 1);
    Serial.println(" mg");

    _calibrationData.offX = offX;
    _calibrationData.offY = offY;
    _calibrationData.offZ = offZ;
    _calibrationData.threshActStart = suggested;
    _calibrationData.valid = true;

    Serial.println();
    printCalibrationData(Serial);

    return _calibrationData;
}

bool ADXL345Sensor::beginMeasurementMode(float rateHz, bool lowPower, bool activityAcCoupled, bool zAxisOnly) {
    if (!_calibrationData.valid) {
        return false;
    }

    setupBase(rateHz, lowPower);

    pinMode(_ADXL_INT_PIN, INPUT);

    _adxl.InactivityINT(0);
    _adxl.FreeFallINT(0);
    _adxl.doubleTapINT(0);
    _adxl.singleTapINT(0);

    _adxl.setActivityThreshold(_calibrationData.threshActStart);
    _adxl.setActivityAc(activityAcCoupled);
    _adxl.setActivityXYZ(zAxisOnly ? false : true, zAxisOnly ? false : true, true);

    _adxl.setInterruptMapping(ADXL345_INT_ACTIVITY_BIT, ADXL345_INT1_PIN);
    _adxl.ActivityINT(1);
    _adxl.getInterruptSource();

    _adxlIrqFlag = false;
    _activeInstance = this;
    attachInterrupt(digitalPinToInterrupt(_ADXL_INT_PIN), adxlIsrRouter, RISING);

    return true;
}

void ADXL345Sensor::endMeasurementMode() {
    detachInterrupt(digitalPinToInterrupt(_ADXL_INT_PIN));

    if (_activeInstance == this) {
        _activeInstance = nullptr;
    }

    _adxlIrqFlag = false;
}

bool ADXL345Sensor::activityDetected() {
    if (!_adxlIrqFlag) {
        return false;
    }

    _adxlIrqFlag = false;

    byte interrupts = _adxl.getInterruptSource();
    return _adxl.triggered(interrupts, ADXL345_ACTIVITY);
}

bool ADXL345Sensor::readAccel(int &x, int &y, int &z) {
    _adxl.readAccel(&x, &y, &z);
    return true;
}

void ADXL345Sensor::clearInterruptSource() {
    _adxl.getInterruptSource();
}

bool ADXL345Sensor::hasValidCalibration() const {
    return _calibrationData.valid;
}
