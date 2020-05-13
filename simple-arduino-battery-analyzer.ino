#include <TimerOne.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

// settings
// Screen pins
#define TFT_CS   7
#define TFT_DC   6
#define TFT_RST  8

// Joystick pins
#define VRxPin A2
#define VRyPin A3
#define SWPin 0

// Charger pins
#define chargeMosfetGatePin 10
#define dischargeMosfetGatePin 9
#define batteryInputPin A5
#define resistorDropPin A4

#define stop_voltage_lion 2.7
#define stop_voltage_nimh 0.7

const float resistorOhm = 0.15;
float adcVoltPerStep = 5.0 / 1024.0;

// constant voltage
const float Vmax = 4.2f;
// constant current
const float Imax = 2.0f;

// battery voltage stop
const float Vbattmax = 4.15f;

// global variables
long millisSinceLastSummary = 0;
float setCurrent = 2.0f;
int dischargePWM = 0;
int chargePWM = 1023;
float stop_voltage = 5;
int beginMillis = 0;

bool CCtrip = false;
bool CVtrip = false;

// buttonvalues
int VRxValue = 0;
int VRyValue = 0;
bool SWValue = 0;

// sensor data
float batteryValue = 0;
int openClampBatteryValue = 0;
float resistorDropValue = 0;
float resistorDrop = 0;

float Isense = 0;
char Isensechar[11];
float Vcurr = 0;
char Vcurrchar[11];
float Vbatt = 0;
char Vbattchar[11];
float mAh = 0;
char mAhchar[11];

// FSM
enum state_t {
  IDLE,
  DISCHARGING,
  CHARGING
};

state_t state;

// FSM
enum menu_t {
  MENU_IDLE,
  MENU_DISCHARGE,
  MENU_CHARGE
};

menu_t menu = MENU_IDLE;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // initialize the screen
  tft.initG();
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  // initialize charger circuit
  pinMode(dischargeMosfetGatePin, OUTPUT);
  pinMode(chargeMosfetGatePin, OUTPUT);
  Timer1.initialize(32);
  Timer1.pwm(dischargeMosfetGatePin, 0);
  Timer1.pwm(chargeMosfetGatePin, 1020);
  pinMode(batteryInputPin, INPUT);
  Serial.begin(115200);
  everythingStop();
}

void testBatteryType() {
  if (Vbatt > 2.0) {
    if (stop_voltage != stop_voltage_lion) {
      String string_lion_detected = "Li-on detected, setting stop voltage to ";
      stop_voltage = stop_voltage_lion;
      Serial.print(string_lion_detected + stop_voltage + "v\n");
    }
  } else {
    if (stop_voltage != stop_voltage_nimh) {
      String string_nimh_detected = "NiMH detected, setting stop voltage to ";
      stop_voltage = stop_voltage_nimh;
      Serial.print(string_nimh_detected + stop_voltage + "v\n");
    }
  }
}

void readSensors() {
  batteryValue = analogRead(batteryInputPin);

  resistorDropValue = analogRead(resistorDropPin);

  if (state == DISCHARGING) {
    resistorDrop = batteryValue - resistorDropValue;
  } else {
    resistorDrop = resistorDropValue - batteryValue;
  }
//  if (resistorDrop < 0) {
//    resistorDrop = 0;
//  }
  Isense = ((resistorDrop / resistorOhm) * adcVoltPerStep);
  Vcurr = ((batteryValue) * adcVoltPerStep);
}

void readSensorSummary() {
  if(state == CHARGING) {
    Timer1.setPwmDuty(chargeMosfetGatePin, 1020);
  }
  if(state == DISCHARGING) {
    Timer1.setPwmDuty(dischargeMosfetGatePin, 0);
  }
  delay(25);
  openClampBatteryValue = analogRead(batteryInputPin);
  if(state == CHARGING) {
    Timer1.setPwmDuty(chargeMosfetGatePin, chargePWM);
  }
  if(state == DISCHARGING) {
    Timer1.setPwmDuty(dischargeMosfetGatePin, dischargePWM);
  }
  delay(25);

  Vbatt = openClampBatteryValue * adcVoltPerStep;

  VRxValue = analogRead(VRxPin);
  VRyValue = analogRead(VRyPin);
}

void testVRx() {
  if (VRxValue < 200) {
    menu = (int)menu + 1;
    menu = menu > MENU_CHARGE ? MENU_IDLE : menu;
  }
  if (VRxValue > 800) {
    menu = (int)menu - 1;
    menu = menu < MENU_IDLE ? MENU_CHARGE : menu;
  }
}

void printSummary() {
  String VbattS =  String("Vbatt:    " + String(Vbatt * 1000)) + "mV";
  String VcurrS =  String("Vcurr:    " + String(Vcurr * 1000)) + "mV";
  String IsenseS = String("Isense:   " + String(Isense * 1000)) + "mA";
  String mAhS =    String("capacity: " + String(mAh)) + "mAh";

  // write serial output
  Serial.write(String(VbattS + " - ").c_str());
  Serial.write(String(VcurrS + " - ").c_str());
  Serial.write(String(IsenseS + " - ").c_str());

  Serial.write(String("CC: " + String(CCtrip, BIN) + " - ").c_str());
  CCtrip = false;
  Serial.write(String("CV: " + String(CVtrip, BIN) + " - ").c_str());
  CVtrip = false;

  Serial.write(String(mAhS + " - ").c_str());

  Serial.write(String("cpwm: " + String(chargePWM, DEC) + " - ").c_str());
  Serial.write(String("dpwm: " + String(dischargePWM, DEC) + " - ").c_str());

  Serial.write("\n");

  // write tft output
  tft.setCursor(0,0);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.println(VbattS);
  tft.println(VcurrS);
  tft.println(IsenseS);
  tft.println(mAhS);
  tft.println();

  // print menu
  tft.setTextColor(ST77XX_WHITE, menu == MENU_IDLE ? ST77XX_BLUE : ST77XX_BLACK);
  tft.println("IDLE     ");
  tft.setTextColor(ST77XX_WHITE, menu == MENU_CHARGE ? ST77XX_BLUE : ST77XX_BLACK);
  tft.println("CHARGE   ");
  tft.setTextColor(ST77XX_WHITE, menu == MENU_DISCHARGE ? ST77XX_BLUE : ST77XX_BLACK);
  tft.println("DISCHARGE");
}

void readSerial () {
  if (Serial.available() > 0) {
    // read the incoming byte:
    int incomingByte = Serial.read();

    if (incomingByte == 100) { // d
      dischargeStart();
    } 
    if (incomingByte == 99) { // c
      chargeStart();
    } 
    if (incomingByte == 110) { // s
      everythingStop();
    } 
  }
}

void everythingStop() {
  dischargePWM = 0;
  chargePWM = 1023;
  Timer1.setPwmDuty(dischargeMosfetGatePin, dischargePWM);
  Timer1.setPwmDuty(chargeMosfetGatePin, chargePWM);
  delay(25);
  Serial.write("Stopping \n");
  state = IDLE;
}

void loop()
{
  readSerial();
  readSensors();
  if (state == DISCHARGING) {
    dischargeAdjustCurrent();
  }
  if (state == CHARGING) {
    if (!chargeTestConstantCurrent() && !chargeTestConstantVoltage()) {
      chargeDecreasePwm();
    } else {
      chargeIncreasePwm();
    }
  }
  testBatteryType();

  if (millis() - millisSinceLastSummary > 1000) {
    readSensorSummary();
    testVRx();
    printSummary();

    if (state == DISCHARGING || state == CHARGING) {
      mAh += (Isense*0.95*1000) / 3600;
    }
    if (state == DISCHARGING) {
      if (dischargeTestBatteryLow()) {
        String string_battery_stop = "Battery < ";
      
        Serial.print(string_battery_stop + stop_voltage + "v, ");
        dischargeStop();
      }
    }
    if (state == CHARGING) {
      delay(10);
      if (chargeTestBatteryFull()) {
        String string_battery_stop = "Battery > ";
      
        Serial.print(string_battery_stop + Vbattmax + "v, ");
        everythingStop();
      }
    }
    millisSinceLastSummary = millis();
  }

 }
