void dischargeStart() {
  pwmStep = 0;
  mAh = 0;
  Timer1.setPwmDuty(dischargeMosfetGatePin, pwmStep);
  delay(25);
  String string_discharge_start = "Starting PWM discharge, stopping at ";
  Serial.print(string_discharge_start + stop_voltage + "v\n");
  state = DISCHARGING;
}


void dischargeStop() {
  pwmStep = 0;
  Timer1.setPwmDuty(dischargeMosfetGatePin, pwmStep);
  delay(25);
  Serial.write("Disabling mosfet and resetting \n");
  state = IDLE;
}

void dischargeAdjustCurrent() {
  if (Isense > setCurrent) {
    pwmStep -= 4;
  } else {
    pwmStep += 4;
  }
  if (pwmStep > 1023) 
    pwmStep = 1023;
  if (pwmStep < 0) 
    pwmStep = 0;

  Timer1.setPwmDuty(dischargeMosfetGatePin, pwmStep);
  delay(25);
}

bool dischargeTestBatteryLow() {
  return (openClampBatteryValue * adcVoltPerStep) < stop_voltage;
}
