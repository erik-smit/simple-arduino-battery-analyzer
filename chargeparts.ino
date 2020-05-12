bool chargeTestConstantCurrent() {
  if (Isense > Imax) {
    CCtrip = true;
    return true;
  } else {
    return false;
  }
}

bool chargeTestConstantVoltage() {
  if (chargingVoltage > Vmax) {
    CVtrip = true;
    return true;
  } else {
    return false;
  }
}

bool chargeTestBatteryFull() {
  return openClampVoltage > VBATmax;
}

void chargeDecreasePwm() {
  if (pwmStep > 0) {
    pwmStep -= 4;
  }
  Timer1.setPwmDuty(chargeMosfetGatePin, pwmStep);
  delay(25);
}

void chargeIncreasePwm() {
  if (pwmStep <= 252) {
    pwmStep += 4;
  }
  Timer1.setPwmDuty(chargeMosfetGatePin, pwmStep);
  delay(25);
}


void chargeStart() {
  pwmStep = 0;
  mAh = 0;
  Timer1.setPwmDuty(chargeMosfetGatePin, pwmStep);
  delay(25);
  state = CHARGING;
}

void chargeStop() {
  pwmStep = 0;
  CCtrip = 0;
  CVtrip = 0;
  fastPWMdac.analogWrite8bit(pwmStep);
  state = IDLE;
}
