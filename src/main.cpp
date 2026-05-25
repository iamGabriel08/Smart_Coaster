#include <Arduino.h>
#include <Wire.h>
#include <ADXL345Sensor.h>
#include <LoadCell.h>
#include <RC522Reader.h>
#include <esp_sleep.h>


//========== TCA6408A I/O Expander ==========//
constexpr uint8_t TCA6408A_ADDR = 0x21;  // Standard-Adresse (A0=A1=GND) 0x21 wenn A0=HIGH

// Register des TCA6408A
constexpr uint8_t TCA6408A_INPUT_PORT   = 0x00;  // Eingangsport lesen
constexpr uint8_t TCA6408A_OUTPUT_PORT  = 0x01;  // Ausgangsport schreiben
constexpr uint8_t TCA6408A_POLARITY     = 0x02;  // Polaritätsinversion
constexpr uint8_t TCA6408A_CONFIG       = 0x03;  // Richtungskonfiguration --> 1=Input, 0=Output

constexpr uint8_t LED_PIN = 0;
constexpr uint8_t LOAD_CELL_PIN = 1;
constexpr uint8_t RFID_PIN = 2;


//========== Measuring Mode ==========//
unsigned long timeStamp = 0;
unsigned long deltaT = 20000;


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
constexpr byte CLOCK_PIN = 3; // ACHtung unbedingt phyisich umstecken
constexpr byte DATA_PIN = 2;

LoadCell loadCell(DATA_PIN, CLOCK_PIN);

//========== RC522-RFID_Reader ==========//
constexpr byte RC522_SS_PIN = 43;
constexpr byte RC522_RST_PIN = 44;
constexpr byte RC522_SCK_PIN = 7;
constexpr byte RC522_MISO_PIN = 8;
constexpr byte RC522_MOSI_PIN = 9;


RC522Reader rfidReader(RC522_SS_PIN, RC522_RST_PIN, RC522_SCK_PIN, RC522_MISO_PIN, RC522_MOSI_PIN);
RC522Reader::CardData myCard;

//========== Battery ==========//
constexpr uint8_t BAT_VOLTAGE_PIN = 1;


//========== Funktionsdeklarationen ==========//

// Deep Sleep
void goToDeepSleep();
void printWakeupReason();

// TCA6408A I/O Expander
void tca6408a_writeRegister(uint8_t reg, uint8_t value);
uint8_t tca6408a_readRegister(uint8_t reg);
void tca6408a_pinMode(uint8_t pin, uint8_t mode); // Pin-Operationen 
void tca6408a_digitalWrite(uint8_t pin, uint8_t value);
uint8_t tca6408a_digitalRead(uint8_t pin);
void tca6408a_begin(); // Initialisierung
void tca6408a_init_pins();
void ON_OFF_Sensors(uint8_t state);
void ON_OFF_LED(uint8_t state);

// Battery
uint32_t readBatteryVoltage_mV();

//========== Setup ==========//
void setup() {

  // Achtung wire.begin() kommt schon in Klasse vor --> NUR EINMAL MACHEN
  Serial.begin(115200);
  delay(1500);
  printWakeupReason();

  pinMode(BAT_VOLTAGE_PIN, INPUT); // Batterie Spannungs Pin initialsierung

  // Wire.begin() hier enthalten --> immer vor allen I2C Geräte
  if (!adxlSensor.beginMeasurementMode(12.5f, true, true, true)) {
    Serial.println("Messbetrieb konnte nicht gestartet werden.");
    while (true) {
      delay(1000);
    }
  }

  tca6408a_begin();
  tca6408a_init_pins();

  ON_OFF_Sensors(true);
  delay(10);
  ON_OFF_LED(true);
  rfidReader.begin();
  loadCell.powerOn(100);
  
  Serial.println("Setup done");
}


//========== Loop ==========//
void loop() {

    
  long weight = loadCell.getRawWheight();

  Serial.println();
  Serial.print("Gewicht = ");
  Serial.println(weight);

  if (rfidReader.readCard()) {
    myCard = rfidReader.getCardData();
    rfidReader.printCardData(&myCard);
    //rfidReader.endCommunication();  
  }
  
  Serial.println(readBatteryVoltage_mV());
  if(adxlSensor.activityDetected()) Serial.println("EVENT");
  delay(200);
}

//========== Funktionsimplementierungen ==========//

// Battery
uint32_t readBatteryVoltage_mV(){
  uint32_t mVolts = analogReadMilliVolts(BAT_VOLTAGE_PIN);
  return 2 * mVolts;
}

// Deep Sleep
void goToDeepSleep() {
  Serial.println("Power off");
  rfidReader.hardSleep();
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
  esp_deep_sleep_start();
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


// // TCA6408A I/O Expander
void tca6408a_writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(TCA6408A_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t tca6408a_readRegister(uint8_t reg) {
    Wire.beginTransmission(TCA6408A_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);  // Repeated start
    
    Wire.requestFrom(TCA6408A_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;  // Fehler
}

void tca6408a_pinMode(uint8_t pin, uint8_t mode) {
    if (pin > 7) return;
    
    uint8_t config = tca6408a_readRegister(TCA6408A_CONFIG);
    
    if (mode == INPUT || mode == INPUT_PULLUP) {
        config |= (1 << pin);   // Bit setzen = Input
    } else {
        config &= ~(1 << pin);  // Bit löschen = Output
    }
    
    tca6408a_writeRegister(TCA6408A_CONFIG, config);
}

void tca6408a_digitalWrite(uint8_t pin, uint8_t value) {
    if (pin > 7) return;
    
    uint8_t output = tca6408a_readRegister(TCA6408A_OUTPUT_PORT);
    
    if (value == HIGH) {
        output |= (1 << pin);
    } else {
        output &= ~(1 << pin);
    }
    
    tca6408a_writeRegister(TCA6408A_OUTPUT_PORT, output);
}

uint8_t tca6408a_digitalRead(uint8_t pin) {
    if (pin > 7) return LOW;
    
    uint8_t input = tca6408a_readRegister(TCA6408A_INPUT_PORT);
    return (input & (1 << pin)) ? HIGH : LOW;
}

void tca6408a_begin() {
    // Alle Pins als Input konfigurieren (Standardzustand)
    tca6408a_writeRegister(TCA6408A_CONFIG, 0xFF);
    
    // Keine Polaritätsinversion
    tca6408a_writeRegister(TCA6408A_POLARITY, 0x00);
    
    // Ausgänge auf LOW setzen
    tca6408a_writeRegister(TCA6408A_OUTPUT_PORT, 0x00);
    
    Serial.println("TCA6408A initialisiert.");
}

void tca6408a_init_pins(){
  tca6408a_pinMode(LED_PIN, OUTPUT);
  tca6408a_pinMode(LOAD_CELL_PIN, OUTPUT);
  tca6408a_pinMode(RFID_PIN, OUTPUT);
  tca6408a_pinMode(3, INPUT);
  tca6408a_pinMode(4, INPUT);
  tca6408a_pinMode(5, INPUT);
  tca6408a_pinMode(6, INPUT);
  tca6408a_pinMode(7, INPUT);
}

void ON_OFF_Sensors(uint8_t state){
  if(state){
    //loadCell.powerOn();
    tca6408a_digitalWrite(LOAD_CELL_PIN, HIGH);
    tca6408a_digitalWrite(RFID_PIN,HIGH);
  }
  else{
    tca6408a_digitalWrite(LOAD_CELL_PIN, LOW);
    tca6408a_digitalWrite(RFID_PIN,LOW);
    //loadCell.powerOff();
  }
  
}

void ON_OFF_LED(uint8_t state){
  // Achtung inversere Logik aufgrund von Schaltung der LED
  if(state){
    tca6408a_digitalWrite(LED_PIN, LOW); // ON
  }
  else{
    tca6408a_digitalWrite(LED_PIN, HIGH); // OFF
  }
}