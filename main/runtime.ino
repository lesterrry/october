Logger logger(DEBUG);
Relay relay(RELAY_PIN);
Knob knob(ENC_CLK_PIN, ENC_DT_PIN, ENC_SW_PIN);
Display display(LED_ADDRESS, LED_WIDTH, LED_HEIGHT);
ClimateSensor sensorInt(TEMPA_DT_PIN, TEMPA_SCK_PIN);
ClimateSensor sensorExt(TEMPB_DT_PIN, TEMPB_SCK_PIN);

MemoryEntry mem_tempSet(EEPROM_INITIAL_TEMP_SET_ADDRESS);

int8_t tempInt;
int8_t tempExt;
int8_t humidInt;
int8_t humidExt;
int8_t tempSet;
bool tempSetSaved = true;
bool displaySleeping = false;
bool knobDown = false;
bool earlyClick = true;  // hack to disable false clicks on startup
Mode currentMode = Off;

unsigned long timer_updateClimateData;
unsigned long timer_knobDown;

String getFormattedClimateValue(int8_t value, bool isHumidity) {
  int8_t minimum = isHumidity ? 0 : -40;
  
  String str = value <= minimum ? "--" : String(value);

  return str + (isHumidity ? '%' : 'C');
}

void saveSettings() {
  logger.print(F("Saving settings..."));
  mem_tempSet.write(tempSet);
}

void getClimateData(bool showBusy = true) {
  logger.print(F("Getting climate data..."));
  
  if (showBusy) {
    display.clear();
    display.print("....");
  }

  tempInt = sensorInt.getTemp();
  tempExt = EXT_TEMP_ENABLED ? sensorExt.getTemp() : -1;
  humidInt = sensorInt.getHumidity();
  humidExt = EXT_TEMP_ENABLED ? sensorExt.getHumidity() : -1;

  logger.print(getFormattedClimateValue(tempInt, false) + " / " + getFormattedClimateValue(tempExt, false));
  logger.print(getFormattedClimateValue(humidInt, true) + " / " + getFormattedClimateValue(humidExt, true));
}


void render(bool isHumidity = false) {
  display.clear();

  if (EXT_TEMP_ENABLED) {
    display.print("OUT", 4, 0);
    display.print("IN", 4, 1);
    display.print(getFormattedClimateValue(isHumidity ? humidExt : tempExt, isHumidity), 0, 0);
  }

  display.print(getFormattedClimateValue(isHumidity ? humidInt : tempInt, isHumidity), 0, 1);

  if (isHumidity) return;

  display.print(relay.getState() ? '|' : 'O', 15, 0);

  if (currentMode != Auto) return;

  display.print("[", 12, 1);
  display.print("]", 15, 1);
  display.print(String(tempSet), 13, 1);
}

void adjustSetTemp(bool inc) {
  int8_t adjusted;

  if (inc) {
    if (tempSet >= TEMP_SET_MAX) adjusted = TEMP_SET_MAX;
    else adjusted = tempSet + 1;
  } else {
    if (tempSet <= TEMP_SET_MIN) adjusted = TEMP_SET_MIN;
    else adjusted = tempSet - 1;
  }

  tempSet = adjusted;

  setRelay();
}

void switchMode() {
  if (currentMode >= Off) {
    currentMode = Auto;
  } else {
    currentMode = currentMode + 1;
  }

  setRelay();
}

void resolveAutoRelayCommand() {
  if (tempInt >= tempSet + TEMP_GRACE_INC) {
    relay.off();
  } else if (tempInt < tempSet + TEMP_GRACE_DEC) {
    relay.on();
  }
}

void setRelay() {
  switch (currentMode) {
    case On: {
      relay.on();
      break;
    }
    case Off: {
      relay.off();
      break;
    }
    case Auto: {
      resolveAutoRelayCommand();
    }
  }
}

void setup() {
  logger.begin();
  display.begin();

  display.print(F("SUBURBS:"));
  display.print(F("OCTOBER"), 0, 1);
  
  logger.print(F("October initializing..."));

  tempSet = mem_tempSet.read();
  logger.print("EEPROM: " + String(tempSet));

  #ifdef CLEAR_EEPROM
    mem_tempSet.write(0);
  #endif

  getClimateData(false);

  render();

  timer_updateClimateData = millis();
}

void loop() {
  unsigned long current = millis();

  bool anyAction = false;
  bool settingsChanged = false;

  knob.update();

  bool isLeft = knob.isLeft(); bool isRight = knob.isRight(); 

  if (isLeft || isRight) {
    anyAction = true;
    if (!displaySleeping && currentMode == Auto) {
      adjustSetTemp(isRight);
      settingsChanged = true;
    }
  } else if (knob.isClick()) {
    if (earlyClick) {
      logger.print(F("Early click"));
      earlyClick = false;
      return;
    }
    anyAction = true;
    switchMode();
  } else if (knob.isDown()) {
    if (!knobDown) {
      timer_knobDown = current;
      knobDown = true;
    }
  } else if (knobDown) {
    anyAction = true;
    knobDown = false;
  }

  if (anyAction) {
    display.setBacklight();
    displaySleeping = false;
    earlyClick = false;

    render(false);
    
    timer_updateClimateData = current;
  }

  if (settingsChanged) {
    tempSetSaved = false;
  }

  if (current - timer_knobDown >= 1000 && knobDown) {
    render(true);
    knobDown = false;
  }

  if (current - timer_updateClimateData >= 20000) {
    display.setBacklight(false);
    displaySleeping = true;
    
    if (!tempSetSaved) { 
      saveSettings();
      tempSetSaved = true;
    }

    getClimateData();

    setRelay();

    render();

    timer_updateClimateData = current;
  }
}
