void dischargeStart() {
  dischargePWM = 0;
  mAh = 0;
  Timer1.setPwmDuty(dischargeMosfetGatePin, dischargePWM);
  delay(25);
  String string_discharge_start = "Starting PWM discharge, stopping at ";
  Serial.print(string_discharge_start + stop_voltage + "v\n");
  state = DISCHARGING;
}


void dischargeStop() {
  dischargePWM  = 0;
  Timer1.setPwmDuty(dischargeMosfetGatePin, dischargePWM);
  delay(25);
  Serial.write("Disabling mosfet and resetting \n");
  state = IDLE;
}

void dischargeAdjustCurrent() {
  if (Isense > setCurrent) {
    dischargePWM -= 4;
  } else {
    dischargePWM += 4;
  }
  if (dischargePWM > 1023) 
    dischargePWM = 1023;
  if (dischargePWM < 0) 
    dischargePWM = 0;

  Timer1.setPwmDuty(dischargeMosfetGatePin, dischargePWM);
  delay(25);
}

bool dischargeTestBatteryLow() {
  return Vbatt < stop_voltage;
}
