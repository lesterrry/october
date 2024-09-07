class ClimateSensor {
  public:
    ClimateSensor(int dtPin, int sckPin)
      : _sensor(dtPin, sckPin) {}

    int8_t getTemp() {
      return _sensor.readTemperatureC();
    }

    int8_t getHumidity() {
      return _sensor.readHumidity();
    }

  private:
    SHT1x _sensor;
};
