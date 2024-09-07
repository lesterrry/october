class MemoryEntry {
  public:
    MemoryEntry(uint16_t address) {
      _address = address;
    }

    void write(uint16_t value) {
      EEPROM.put(_address, value);
    }

    uint16_t read() {
      uint16_t retrievedValue;
      EEPROM.get(_address, retrievedValue);
      return retrievedValue;
    }

  private:
    uint16_t _address;
};
