#ifndef THERMO_MANAGER_H
#define THERMO_MANAGER_H

#include "OneWire.h"
#include "etl/vector.h"
#include "etl/deque.h"
#include "etl/string.h"
#include "config.h"
#include "stats.h"
#include "sd_writer.h"
#include "TimeLib.h"
#include "parser_utils.h"

using Address = uint8_t[8];

// T + E + numSensors + 5 byte per thermometre + 9 byte for timestamp
static constexpr uint8_t thermo_message_size {3 + sizeof(uint16_t) + 5*max_number_of_thermometres + 1 + sizeof(time_t)};

struct temperatureReading{
  int32_t temps[max_number_of_thermometres];
  time_t timestamp;
  uint16_t readingID;
};

class Thermo_Manager{
  public:
    etl::vector<uint64_t, max_number_of_thermometres> thermometres;
    etl::deque<temperatureReading, max_number_of_measurements> temperatures;

    /*
      Starts reading temperatures from pin with pin ID Dpin
    */
    void begin(uint16_t dataPin, uint16_t powerPin);
    void get_thermometre_identifications(void);
    void request_start_thermistors_conversions(void);
    uint8_t collect_thermistors_conversions(time_t timestamp);
    uint32_t remaining_conversion_time(void);
    uint8_t takeReadings(uint32_t maxReadTime, time_t timestamp, bool logAllReadings);
    void processReadings(void);
    size_t updateTransmitMessage(void);
    byte msgB[thermo_message_size];
    uint8_t logReading(temperatureReading tempData);
    void wake(void);
    void sleep(void);
    void getMeasurementFromFile(void);

  private:
    uint32_t readingStartTime;
    uint16_t dPin;
    uint16_t pPin;
    uint8_t numSensors;
    OneWire TWire;
    uint8_t numPacketReadings=0;
    etl::deque<temperatureReading, measurements_per_packet> packet;
    temperatureReading reading;
    etl::deque<temperatureReading, max_number_of_measurements> tmp_storage;
    
    void cleanUp(void);
};

extern Thermo_Manager thermo_manager;
#endif