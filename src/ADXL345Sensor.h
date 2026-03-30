#ifndef ADXL345Sensor_h
#define ADXL345Sensor_h

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ADXL345.h>

class ADXL345Sensor {

    public:
        //========== Hilfsstrukturen ==========//
        struct XYZ {
            float x;
            float y;
            float z;
        };

        struct CalibrationData {
            int offX;
            int offY;
            int offZ;
            int threshActStart;
            bool valid;
        };

        //========== Konstruktoren ==========//

        // Für Kalibrierbetrieb
        ADXL345Sensor(byte SDA_PIN, byte SCL_PIN, byte ADXL_INT_PIN, bool Z_AXIS_POINTS_UP = true);

        // Für Messbetrieb mit vorhandenen Kalibrierwerten
        ADXL345Sensor(byte SDA_PIN, byte SCL_PIN, byte ADXL_INT_PIN,CalibrationData calibrationData, bool Z_AXIS_POINTS_UP = true);

        //========== Public-Funktionsdeklarationen ==========//
        CalibrationData runCalibration(uint16_t offsetSamples = 100, uint16_t offsetDelayMs = 10, uint32_t quietPhaseMs = 10000, uint32_t eventPhaseMs = 15000);
        
        bool beginMeasurementMode(float rateHz = 12.5f, bool lowPower = false, bool activityAcCoupled = true, bool zAxisOnly = true);
        void endMeasurementMode();
        bool activityDetected();
        bool readAccel(int &x, int &y, int &z);
        void clearInterruptSource();
        bool hasValidCalibration() const;

    private:
        //========== Private-Konstanten ==========//
        static constexpr float RAW_COUNTS_PER_G = 256.0f;
        static constexpr float RAW_COUNTS_PER_OFFSET_LSB = 4.0f;
        static constexpr float RAW_COUNTS_PER_ACTIVITY_LSB = 16.0f;

        //========== Private-Variablen ==========//
        ::ADXL345 _adxl;

        const byte _SDA_PIN;
        const byte _SCL_PIN;
        const byte _ADXL_INT_PIN;
        const bool _Z_AXIS_POINTS_UP;

        CalibrationData _calibrationData;
        volatile bool _adxlIrqFlag;

        static ADXL345Sensor* _activeInstance;

        //========== Private-Funktionsdeklarationen ==========//
        static void IRAM_ATTR adxlIsrRouter();

        void initI2C();
        void setupBase(float rateHz, bool lowPower);
        XYZ averageSamples(uint16_t n, uint16_t delayMs = 10);
        float max3(float a, float b, float c);
        int clampOffset(int v);
        void printXYZ(Stream &out, const char *label, const XYZ &v) const;
        bool beginCalibrationMode();
        void printCalibrationData(Stream &out = Serial) const;
};

#endif