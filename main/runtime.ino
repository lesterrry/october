Logger logger(DEBUG);
SerialCommunicator serial(!DEBUG);
Relay relay(RELAY_PIN);
Knob knob(ENC_CLK_PIN, ENC_DT_PIN, ENC_SW_PIN);
Display display(LED_ADDRESS, LED_WIDTH, LED_HEIGHT);
ClimateSensor sensorInt(TEMPA_DT_PIN, TEMPA_SCK_PIN);
ClimateSensor sensorExt(TEMPB_DT_PIN, TEMPB_SCK_PIN);

MemoryEntry mem_tempSet(EEPROM_INITIAL_TEMP_SET_ADDRESS);
MemoryEntry mem_currentMode(EEPROM_INITIAL_MODE_ADDRESS);

float tempInt;
float tempExt;
float humidInt;
float humidExt;
int8_t tempSet;
bool settingsSaved = true;
bool displaySleeping = false;
bool knobDown = false;
bool earlyClick = true;  // hack to disable false clicks on startup
Mode currentMode = Auto;

unsigned long timer_updateClimateData;
unsigned long timer_knobDown;

const char SEP = ';';

void(* reset) (void) = 0;

String getFormattedClimateValue(float value, bool isHumidity) {
  int8_t minimum = isHumidity ? 0 : -40;
  
  String str = value <= minimum ? "--" : String(value);

  return str + (isHumidity ? '%' : 'C');
}

void saveSettings() {
  logger.print(F("Saving settings..."));
  mem_tempSet.write(tempSet);
  mem_currentMode.write(currentMode);
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
    display.print("OUT", 7, 0);
    display.print("IN", 7, 1);
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

String getParseableState() {
  return String(currentMode) + SEP + String(tempSet) + SEP + String(tempInt) + SEP + String(tempExt) + SEP + String(humidInt) + SEP + String(humidExt);
}

bool resolveSerialCommand(String command) {
  if (command.length() == 0) {
    serial.respond(F("ERROR: CMD EMPTY"));
    return false;
  }

  if (command == "S") {
    serial.respond(getParseableState());
    return false;
  } else if (command == "R") {
    reset();
  }
  
  int separatorIndex = command.indexOf(';');

  if (separatorIndex == -1) {
    serial.respond(F("ERROR: CMD CORRUPT"));
    return false;
  }
  
  String modeStr = command.substring(0, separatorIndex);
  String tempStr = command.substring(separatorIndex + 1);
  
  modeStr.trim();
  tempStr.trim();
  
  int newMode = modeStr.toInt();
  if (modeStr != String(newMode) || newMode < Mode::Auto || newMode > Mode::Off) {
    serial.respond(F("ERROR: MODE CORRUPT"));
    return false;
  }
  
  int newTemp = tempStr.toInt();
  if (tempStr != String(newTemp) || newTemp < TEMP_SET_MIN || newTemp > TEMP_SET_MAX) {
    serial.respond(F("ERROR: TEMP CORRUPT"));
    return false;
  }
  
  currentMode = newMode;
  tempSet = newTemp;

  serial.respond(getParseableState());

  setRelay();

  render();

  return true;
}

void setup() {
  logger.begin();
  serial.begin();
  display.begin();

  display.print(F("SUBURBS:"));
  display.print(F("OCTOBER"), 0, 1);
  
  logger.print(F("October initializing..."));

  tempSet = mem_tempSet.read();
  currentMode = mem_currentMode.read();
  logger.print("EEPROM TEMP: " + String(tempSet));
  logger.print("EEPROM MODE: " + String(currentMode));

  #ifdef CLEAR_EEPROM
    mem_tempSet.write(0);
  #endif

  getClimateData(false);

  setRelay();

  render();

  timer_updateClimateData = millis();
}

void loop() {
  knob.update();

  unsigned long current = millis();
  bool anyAction = false;
  bool isLeft = knob.isLeft(); bool isRight = knob.isRight(); 

  String command = serial.getCommand();

  if (command != "") {
    anyAction = resolveSerialCommand(command);
  }

  if (isLeft || isRight) {
    anyAction = true;
    if (!displaySleeping && currentMode == Auto) {
      adjustSetTemp(isRight);
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
    settingsSaved = false;

    render(false);
    
    timer_updateClimateData = current;
  }

  if (current - timer_knobDown >= 1000 && knobDown) {
    render(true);
    knobDown = false;
  }

  if (current - timer_updateClimateData >= 20000) {
    display.setBacklight(false);
    displaySleeping = true;

    if (!settingsSaved) { 
      saveSettings();
      settingsSaved = true;
    }

    getClimateData();

    setRelay();

    render();

    timer_updateClimateData = current;
  }
}
