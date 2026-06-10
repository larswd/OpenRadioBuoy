#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "config.h"
#include "sd_writer.h"
#include "parser_utils.h"
#include "stats.h"
#include "etl/deque.h"
#include "etl/vector.h"
#include "etl/string.h"
#include "IWatchdog.h"
#include "TimeLib.h"

struct GPS_Data{
  time_t timestamp;
  uint32_t lat;
  uint32_t lng;
  uint32_t vel;
  uint32_t direction;
  uint16_t readingID;
};

struct DateInfo{
  uint8_t day;
  uint8_t month;
  uint16_t year;
  bool valid;
};

// Pin config. Do not change unless you have rewired the OLB
static constexpr uint8_t  GPS_RX_PIN                     {PC0};
static constexpr uint8_t  GPS_TX_PIN                     {PC1};
static constexpr uint8_t  GPS_SLEEP_PIN                  {PA0};



static constexpr uint32_t beaconMsgSize  {3 + sizeof(time_t) + 3*sizeof(uint32_t)};
static constexpr uint8_t GPS_message_size {2+sizeof(time_t)+4*sizeof(uint32_t) + sizeof(uint16_t)};
static constexpr uint8_t deployment_message_size {2 + sizeof(uint32_t) + sizeof(time_t) + 2*sizeof(uint32_t) + 1};
class GPS_Manager{
  public:
    GPS_Manager(uint32_t rxpin, uint32_t txpin) : ss(rxpin, txpin) {};
    bool fix = false;
    void begin(float);
    DateInfo date = {0,0,0,false};
    etl::deque<GPS_Data, max_number_of_measurements> GPSReadings;
    
    uint8_t setTimeFromGps();
    uint8_t updateTimestamp(uint32_t max_wait_time, bool refreshGPStime);
    void getDeploymentMessage(uint32_t buoy_ID);
    time_t timestamp = 0;
    uint8_t performNReadings(uint8_t N, uint32_t max_wait_time, bool logEveryReading);
    void shutdownGPS(void);
    void processReadings(bool);
    uint32_t iterations = 0;

    size_t updateTransmitMessage(void);
    byte msgB[GPS_message_size];
    byte deploymentMessage[deployment_message_size];
    uint8_t logReading(GPS_Data & reading);
    uint32_t current_buoy_velocity = 0;
    void getMeasurementFromFile(void);
    GPS_Data currentPosition;
    void updateBeaconMsg(uint32_t WiO_ID);
    byte beaconMsg[beaconMsgSize];
  private:
    uint8_t getGPSData(uint32_t max_wait_time);
    
    etl::vector<GPS_Data, readings_per_measurement> packet;
    TinyGPSPlus gps;
    HardwareSerial ss;
    bool initialized = false;
    uint64_t initial_timestamp = 0;
    GPS_Data mean_values;
};

extern GPS_Manager gps_manager;
#endif