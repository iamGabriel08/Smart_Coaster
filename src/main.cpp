#include <Arduino.h>
#include <Wire.h>
#include <ADXL345Sensor.h>
#include <LoadCell.h>
#include <RC522Reader.h>

//========== ADXL345 ==========//
constexpr byte SDA_PIN = 5;          // XIAO D4
constexpr byte SCL_PIN = 6;          // XIAO D5
constexpr byte ADXL_INT_PIN = 4;    // XIAO D3
constexpr bool Z_AXIS_POINTS_UP = true;

// Kalibrierung ADXL345--> adxlSensor.runCalibration() im Setup aufrufen
//ADXL345Sensor adxlSensor(SDA_PIN, SCL_PIN, ADXL_INT_PIN, Z_AXIS_POINTS_UP);

// Messbetrieb ADXL345
ADXL345Sensor::CalibrationData calibrationData = {34, 53, 70, 4, true};
ADXL345Sensor adxlSensor(SDA_PIN, SCL_PIN, ADXL_INT_PIN, calibrationData, Z_AXIS_POINTS_UP);


//========== Load Cell ==========//
constexpr byte CLOCK_PIN = 1;
constexpr byte DATA_PIN = 1;

LoadCell loadCell(DATA_PIN, CLOCK_PIN);

//========== RC522-RFID_Reader ==========//
constexpr byte RC522_SS_PIN = 10;
constexpr byte RC522_RST_PIN = 4;
constexpr byte RC522_SCK_PIN = 6;
constexpr byte RC522_MISO_PIN = 5;
constexpr byte RC522_MOSI_PIN = 7;
constexpr byte GATE_PIN = 8;

RC522Reader rfidReader(RC522_SS_PIN, RC522_RST_PIN, RC522_SCK_PIN, RC522_MISO_PIN, RC522_MOSI_PIN, GATE_PIN);


void setup() {
  Serial.begin(115200);
  delay(1500);

  //adxlSensor.runCalibration();
  
  Serial.println();
  Serial.println("ADXL345 Messbetrieb");

  if (!adxlSensor.beginMeasurementMode(12.5f, true, true, true)) {
      Serial.println("Messbetrieb konnte nicht gestartet werden.");
      while (true) {
           delay(1000);
      }
  }

   rfidReader.begin();
   Serial.println("Setup done");
  
}

void loop() {
  
  if (adxlSensor.activityDetected()) {
    int x, y, z;
    adxlSensor.readAccel(x, y, z);
    Serial.println();
    Serial.println("*** ACTIVITY erkannt ***");
    Serial.print("raw X=");
    Serial.print(x);
    Serial.print("  Y=");
    Serial.print(y);
    Serial.print("  Z=");
    Serial.println(z);
  }

  long wheight = loadCell.getRawWheight();
  Serial.println();
  Serial.print("Gewicht = ");
  Serial.println(wheight);  
  delay(50);

  if (rfidReader.readCard()) {
    rfidReader.printCardData();
    rfidReader.endCommunication();

    Serial.println("Power off");
    rfidReader.hardSleep();
    rfidReader.powerOff();

    Serial.println("Gehe fuer 10 Sekunden in Deep Sleep...");
    Serial.flush();
    esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);
    esp_deep_sleep_start();
  }
  
}


