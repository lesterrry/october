class ClimateSensor {
  public:
    ClimateSensor(int dtPin, int sckPin)
      : _sensor(dtPin, sckPin) {}

    float getTemp() {
      return _sensor.readTemperatureC();
    }

    float getHumidity() {
      return _sensor.readHumidity();
    }

  private:
    SHT1x _sensor;
};
