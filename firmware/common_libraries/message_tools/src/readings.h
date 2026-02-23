// #include "etl/deque.h"
#ifndef READINGS_H
#define READINGS_H

#include <Arduino.h>
#include "config.h"

// typedef byte BuoyID[8];

struct temperature_Reading
{
    uint16_t reading_ID;
    uint8_t num_sensors;
    int32_t temps[max_number_of_thermometres] = {0};
    time_t timestamp;
};

struct GPS_Reading
{   
    uint16_t reading_ID;
    int32_t lat;
    int32_t lng;
    int32_t vel;
    int32_t direction;
    time_t timestamp;
};

struct beacon_Reading
{   
    time_t timestamp;
    int32_t lat;
    int32_t lng;
    uint32_t buoy_id;
};

struct turbidity_Reading
{
    uint32_t voltage;
    time_t timestamp;
};

struct buoyInfoReading
{
    int32_t sent_packets;
    int32_t left_packets;
    uint32_t listen_time;
    bool crashed;
};

struct buoyInitMessage
{
    uint32_t buoy_id;
    uint8_t base_station_ID;
};

#endif