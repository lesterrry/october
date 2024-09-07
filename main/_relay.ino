class Relay {
  public:
    Relay(int pin) {
      pinMode(pin, OUTPUT);
      _pin = pin;
      _state = LOW;
      _writeState();
    }

    void on() {
      _state = HIGH;
      _writeState();
    }

    void off() {
      _state = LOW;
      _writeState();
    }

    void toggle() {
      _state = !_state;
      _writeState();
    }

    int getState() {
      return _state;
    }

  private:
    int _pin;
    int _state;

    void _writeState() {
      digitalWrite(_pin, _state);
    }
};
