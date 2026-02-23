#ifndef MESSAGE_PARSER_H
#define MESSAGE_PARSER_H
#include <TimeLib.h>
#include <Arduino.h>
#include "readings.h"

class Message_Parser{
    public:
        temperature_Reading parse_temperature_message(byte* msg);
        GPS_Reading parse_gps_message(byte* msg);
        turbidity_Reading parse_turbidity_message(byte* msg);
        buoyInfoReading parse_buoy_info_message(byte* msg);
        buoyInitMessage parse_buoy_init_message(byte* msg);
        beacon_Reading parse_beacon_message(byte *msg);
};

extern Message_Parser MESSAGE_PARSER;
#endif