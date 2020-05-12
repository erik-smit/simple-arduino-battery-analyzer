bool chargeTestConstantCurrent() {
  if (Isense > Imax) {
    CCtrip = true;
    return true;
  } else {
    return false;
  }
}

bool chargeTestConstantVoltage() {
  if (Vcurr > Vmax) {
    CVtrip = true;
    return true;
  } else {
    return false;
  }
}

bool chargeTestBatteryFull() {
  return Vbatt > Vbattmax;
}

void chargeDecreasePwm() {
  if (chargePWM > 0) {
    chargePWM -= 4;
  }
  Timer1.setPwmDuty(chargeMosfetGatePin, chargePWM);
  delay(25);
}

void chargeIncreasePwm() {
  if (chargePWM <= 1020) {
    chargePWM += 4;
  }
  Timer1.setPwmDuty(chargeMosfetGatePin, chargePWM);
  delay(25);
}


void chargeStart() {
  chargePWM = 1020;
  mAh = 0;
  Timer1.setPwmDuty(chargeMosfetGatePin, chargePWM);
  delay(25);
  String string_charge_start = "Starting charge";
  Serial.print(string_charge_start + "\n");
  state = CHARGING;
}
