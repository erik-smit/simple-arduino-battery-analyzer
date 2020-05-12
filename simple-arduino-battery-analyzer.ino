// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Pins
#define mosfetGatePin PD2
#define batteryInputPin A5
#define resistorDropPin A4
#define ONE_WIRE_BUS PD3

// offsets
#define DELAY_OFFSET 129 // how many ms the measuring takes to ensure every cycle takes a second

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


int cycles = 0;
bool running = false;
int previous_cycles = 0;
float mAh = 0;
float previous_mAh = 0;

void setup() {
  pinMode(mosfetGatePin, OUTPUT);
  pinMode(batteryInputPin, INPUT);
  Serial.begin(115200);
  stopDischarge();

}

void startDischarge() {
  digitalWrite(mosfetGatePin, HIGH);
  Serial.write("Starting discharge\n");
  running = true;
}

void stopDischarge() {
  digitalWrite(mosfetGatePin, LOW);
  Serial.write("Disabling mosfet and resetting \n");
  previous_cycles = cycles;
  cycles = 0;
  previous_mAh = mAh;
  mAh = 0;
  running = false;
}

void loop() {
  float adcVoltPerStep = 5.0 / 1024.0;
  
  int batteryValue = analogRead(batteryInputPin);
  float batteryVoltage = batteryValue * adcVoltPerStep;

  int resistorDropValue = analogRead(resistorDropPin);
  int resistorDrop = batteryValue - resistorDropValue;
  if (resistorDrop < 0) {
    resistorDrop = 0;
  }
  float resistorOhm = 1; 
  float resistorDropCurrent = ((resistorDrop / resistorOhm) * adcVoltPerStep);

  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0);
  
  if (Serial.available() > 0) {
    // read the incoming byte:
    int incomingByte = Serial.read();

    if (incomingByte == 121) {
      startDischarge();
    } 
    if (incomingByte == 110) {
      stopDischarge();
    } 
  }
//  Serial.write("batteryValue: ");
//  Serial.print(batteryValue);
//  Serial.write(" - ");
//  Serial.write("resistorDropValue: ");
//  Serial.print(resistorDropValue);
//  Serial.write(" - ");
  Serial.write("battery: ");
  Serial.print(batteryVoltage * 1000);
  Serial.write("mV");
  
  Serial.write(" - current: ");
  Serial.print(resistorDropCurrent * 1000);
  Serial.write("mA");

  Serial.write(" - temp: ");
  tempC != DEVICE_DISCONNECTED_C ? Serial.print(tempC) : Serial.write ("ERR");
  
  Serial.write(" - capacity: ");
  Serial.print(mAh);
  Serial.write("mAh");
  
  Serial.write(" - cycles: ");
  Serial.print(cycles);
  
  Serial.write(" - previous capacity: ");
  Serial.print(previous_mAh);
  Serial.write("mAh");
  
  Serial.write(" - previous cycles: ");
  Serial.print(previous_cycles);
  Serial.write("\n");


  if (running) {
    cycles++;
    mAh += (resistorDropCurrent*1000) / 3600;
    if (batteryVoltage < 0.7) {
      Serial.write("Battery < 0.7v, ");
      stopDischarge();
    }
  }

  delay(1000 - DELAY_OFFSET);
 }
