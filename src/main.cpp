#include <Arduino.h>
#include <Wire.h>
#include <ADXL345Sensor.h>
#include <LoadCell.h>
#include <RC522Reader.h>
#include <esp_sleep.h>

//========== ADXL345 ==========//
constexpr byte SDA_PIN = 5;          // XIAO D4
constexpr byte SCL_PIN = 6;          // XIAO D5
constexpr byte ADXL_INT_PIN = 4;    // XIAO D3
constexpr bool Z_AXIS_POINTS_UP = true;

// Kalibrierung ADXL345--> adxlSensor.runCalibration() im Setup aufrufen
//ADXL345Sensor adxlSensor(SDA_PIN, SCL_PIN, ADXL_INT_PIN, Z_AXIS_POINTS_UP);

// Messbetrieb ADXL345
ADXL345Sensor::CalibrationData calibrationData = {-7, 67, 72, 6, true};
ADXL345Sensor adxlSensor(SDA_PIN, SCL_PIN, ADXL_INT_PIN, calibrationData, Z_AXIS_POINTS_UP);


//========== Load Cell ==========//
constexpr byte CLOCK_PIN = 2;
constexpr byte DATA_PIN = 3;

LoadCell loadCell(DATA_PIN, CLOCK_PIN);

//========== RC522-RFID_Reader ==========//
constexpr byte RC522_SS_PIN = 43;
constexpr byte RC522_RST_PIN = 44;
constexpr byte RC522_SCK_PIN = 7;
constexpr byte RC522_MISO_PIN = 8;
constexpr byte RC522_MOSI_PIN = 9;
constexpr byte GATE_PIN = 1;

RC522Reader rfidReader(RC522_SS_PIN, RC522_RST_PIN, RC522_SCK_PIN, RC522_MISO_PIN, RC522_MOSI_PIN, GATE_PIN);

RC522Reader::CardData myCard;


//========== Funktionsdeklarationen ==========//
void goToDeepSleep();
void printWakeupReason();

//========== Setup ==========//
void setup() {
  Serial.begin(115200);
  delay(1500);
  
  printWakeupReason();

  if (!adxlSensor.beginMeasurementMode(12.5f, true, true, true)) {
    Serial.println("Messbetrieb konnte nicht gestartet werden.");
    while (true) {
      delay(1000);
    }
  }

  rfidReader.begin();
  loadCell.powerOn(100);

  Serial.println("Setup done");
  
}


//========== Loop ==========//
void loop() {
  /*
  long wheight = loadCell.getRawWheight();
  Serial.println();
  Serial.print("Gewicht = ");
  Serial.println(wheight);
*/
  if (rfidReader.readCard()) {
    myCard = rfidReader.getCardData();
    rfidReader.printCardData(&myCard);
    rfidReader.endCommunication();  
  }
  
  //goToDeepSleep();
  delay(50);
}

//========== Funktionsimplementierungen ==========//

void goToDeepSleep() {
  Serial.println("Power off");
  rfidReader.hardSleep();
  rfidReader.powerOff();
  loadCell.powerOff();

  // Alte/latchte ADXL-Interruptquelle löschen,
  // sonst kann der ESP sofort wieder aufwachen
  adxlSensor.clearInterruptSource();
  delay(5);

  // ADXL345 INT ist active HIGH -> Wakeup auf HIGH
  esp_sleep_enable_ext0_wakeup((gpio_num_t)ADXL_INT_PIN, 1);

  // Optional zusätzlich Timer als Backup:
  // esp_sleep_enable_timer_wakeup(20ULL * 1000000ULL);

  Serial.println("Gehe in Deep Sleep, Wakeup per ADXL345...");
  Serial.flush();
  //esp_deep_sleep_start();
}

void printWakeupReason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  Serial.print("Wakeup cause: ");
  switch (cause) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("ADXL345 / EXT0");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("TIMER");
      break;
    default:
      Serial.println((int)cause);
      break;
  }
}

