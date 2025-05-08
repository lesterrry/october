class SerialCommunicator {
  public:
    SerialCommunicator(bool active, unsigned long baudRate = 9600, unsigned long timeout = 2000) {
      _active = active;
      _baudRate = baudRate;
      _timeout = timeout;
      _isConnected = false;
      _inputBuffer = "";
    }

    void begin() {
      Serial.begin(_baudRate);
      
      unsigned long startTime = millis();
      while (!Serial && (millis() - startTime < _timeout)) {
        delay(10);
      }
      
      _isConnected = Serial;
      
      if (_isConnected) {
        Serial.println(F("READY"));
      }
    }
    
    bool isConnected() const {
      return _isConnected;
    }
    
    String getCommand() {
      if (_isConnected && Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        return command;
      }
      
      return "";
    }

    void respond(String command) {
      if (_isConnected) Serial.println(command);
    }
  
  private:
    bool _active;
    bool _isConnected;
    unsigned long _baudRate;
    unsigned long _timeout;
    String _inputBuffer;
};
