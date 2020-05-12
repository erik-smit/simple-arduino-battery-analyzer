// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimerOne.h>

// settings
// Pins
#define chargeMosfetGatePin 9
#define dischargeMosfetGatePin 10

#define batteryInputPin A1
#define resistorDropPin A0
#define ONE_WIRE_BUS 2

#define stop_voltage_lion 2.7
#define stop_voltage_nimh 0.7

const float resistorOhm = 0.15;
float adcVoltPerStep = 5.0 / 1024.0;

// constant voltage
const float Vmax = 4.2f;
// constant current
const float Imax = 1.0f;

// battery voltage stop
const float VBATmax = 4.15f;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// global variables
long millisSinceLastSummary = 0;
float setCurrent = 2.0f;
int pwmStep = 0;
float mAh = 0;
float stop_voltage = 5;
int beginMillis = 0;

bool CCtrip = false;
bool CVtrip = false;

// sensor data
float batteryValue = 0;
int openClampBatteryValue = 0;
float resistorDropValue = 0;
float resistorDrop = 0;

float Isense = 0;
float tempC = 0;

// FSM
enum state_t {
  IDLE,
  DISCHARGING,
  CHARGING
};

state_t state;
void setup() {
  pinMode(dischargeMosfetGatePin, OUTPUT);
  pinMode(chargeMosfetGatePin, OUTPUT);
  Timer1.initialize(32);
  Timer1.pwm(dischargeMosfetGatePin, 0);
  Timer1.pwm(chargeMosfetGatePin, 0);
  pinMode(batteryInputPin, INPUT);
  Serial.begin(115200);
  dischargeStop();
}

void testBatteryType() {
  if ((openClampBatteryValue * adcVoltPerStep) > 2.0) {
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
  resistorDrop = batteryValue - resistorDropValue;
  if (resistorDrop < 0) {
    resistorDrop = 0;
  }
  Isense = ((resistorDrop / resistorOhm) * adcVoltPerStep);

  sensors.requestTemperatures(); // Send the command to get temperatures
  tempC = sensors.getTempCByIndex(0);
}

void readOpenClampSensor() {
  Timer1.setPwmDuty(dischargeMosfetGatePin, 0);
  delay(25);
  openClampBatteryValue = analogRead(batteryInputPin);
  Timer1.setPwmDuty(dischargeMosfetGatePin, pwmStep);
  delay(25);
}

void printSummary() {
  Serial.write("battery: ");
  Serial.print(openClampBatteryValue * adcVoltPerStep * 1000);
  Serial.write("mV");
  
  Serial.write(" - current: ");
  Serial.print(Isense * 1000);
  Serial.write("mA");

  Serial.write(" - temp: ");
  tempC != DEVICE_DISCONNECTED_C ? Serial.print(tempC) : Serial.write ("ERR");
  
  Serial.write("c - capacity: ");
  Serial.print(mAh);
  Serial.write("mAh");

  Serial.write(" - pwm: ");
  Serial.print(pwmStep);

  Serial.write("\n");

}

void readSerial () {
  if (Serial.available() > 0) {
    // read the incoming byte:
    int incomingByte = Serial.read();

    if (incomingByte == 121) {
      dischargeStart();
    } 
    if (incomingByte == 110) {
      everythingStop();
    } 
  }
}

void everythingStop() {
  pwmStep = 0;
  Timer1.setPwmDuty(dischargeMosfetGatePin, pwmStep);
  Timer1.setPwmDuty(chargeMosfetGatePin, pwmStep);
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
  testBatteryType();

  if (millis() - millisSinceLastSummary > 1000) {
    if (state == DISCHARGING) {
      mAh += (Isense*0.95*1000) / 3600;
      if (dischargeTestBatteryLow()) {
        String string_battery_stop = "Battery < ";
      
        Serial.print(string_battery_stop + stop_voltage + "v, ");
        dischargeStop();
      }
    }
    readOpenClampSensor();
    printSummary();
    millisSinceLastSummary = millis();
  }

 }
