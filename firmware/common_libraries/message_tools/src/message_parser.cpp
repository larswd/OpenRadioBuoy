#include "message_parser.h"
#include "parser_utils.h"

Message_Parser MESSAGE_PARSER;
// The new structure is "Tn(n*s*xxxx)ttttttttE"
temperature_Reading Message_Parser::parse_temperature_message(byte *msg)
{
    temperature_Reading temperature_reading_packet;
    uint8_t offset = 1;

    temperature_reading_packet.reading_ID = msg_extract_uint<uint16_t>(msg, offset, true, offset);

    temperature_reading_packet.num_sensors =  msg[offset];
    offset += sizeof(byte);

    for (int j = 0; j < temperature_reading_packet.num_sensors; j++)
    {
        int8_t factor = msg[offset] == 'N' ? -1 : 1;

        offset += sizeof(byte);
        temperature_reading_packet.temps[j] = msg_extract_uint<int32_t>(msg, offset, true, offset) * factor;

    }
   
    temperature_reading_packet.timestamp = msg_extract_uint<time_t>(msg, offset, true, offset);
   
    return temperature_reading_packet;
}

GPS_Reading Message_Parser::parse_gps_message(byte *msg)
{
    GPS_Reading gps_reading_packet;
    uint8_t offset = 1;
    gps_reading_packet.reading_ID = msg_extract_uint<uint16_t>(msg, offset, true, offset);
    gps_reading_packet.lat = msg_extract_uint<uint32_t>(msg, offset, true, offset);
    gps_reading_packet.lng = msg_extract_uint<uint32_t>(msg, offset, true, offset);
    gps_reading_packet.vel = msg_extract_uint<uint32_t>(msg, offset, true, offset);
    gps_reading_packet.direction = msg_extract_uint<uint32_t>(msg, offset, true, offset);
    gps_reading_packet.timestamp = msg_extract_uint<time_t>(msg, offset, true, offset);
   
    return gps_reading_packet;
}

beacon_Reading Message_Parser::parse_beacon_message(byte *msg)
{
    beacon_Reading beacon_reading_packet;
    uint8_t offset = 2;
    beacon_reading_packet.timestamp = msg_extract_uint<time_t>(msg, offset, true, offset);
    beacon_reading_packet.lat = msg_extract_uint<int32_t>(msg, offset, true, offset);
    beacon_reading_packet.lng = msg_extract_uint<int32_t>(msg, offset, true, offset);
    beacon_reading_packet.buoy_id = msg_extract_uint<uint32_t>(msg, offset, true, offset);
    return beacon_reading_packet;

}

turbidity_Reading Message_Parser::parse_turbidity_message(byte* msg)
{
    turbidity_Reading turbidity_reading_packet;

    uint8_t offset = 1;
    turbidity_reading_packet.timestamp = msg_extract_uint<time_t>(msg, offset, true, offset);
    turbidity_reading_packet.voltage = msg_extract_uint<uint32_t>(msg, offset, true, offset);

    // uint32_t voltage = 0;
    // fac64 = 1;
    // for (int i = 0; i < 3; i++)
    // {
    //     fac64 *= 256;
    // }
    // for (int i = 0; i < 4; i++)
    // {
    //     voltage += msg[offset] * fac64;
    //     fac64 /= 256;
    //     offset += 1;
    // }
    // turbidity_reading_packet.timestamp = timestamp;
    // turbidity_reading_packet.voltage = voltage;
    return turbidity_reading_packet;
}

// Buoy infor structure is EMxytttteE
buoyInfoReading Message_Parser::parse_buoy_info_message(byte *msg)
{
    buoyInfoReading buoy_info_packet;
    buoy_info_packet.sent_packets = msg[2];
    buoy_info_packet.left_packets = msg[3];
    uint8_t offset = 4;
    buoy_info_packet.listen_time = msg_extract_uint<uint32_t>(msg, offset, true, offset);
    buoy_info_packet.crashed = msg[offset];

    return buoy_info_packet;
}

buoyInitMessage Message_Parser::parse_buoy_init_message(byte *msg)
{
    buoyInitMessage buoy_init_message;

    if (msg[0] == 'U' && msg[6] == 'E')
    {
        uint8_t offset = 1;
        buoy_init_message.buoy_id = msg_extract_uint<uint32_t>(msg, offset, true, offset);
        buoy_init_message.base_station_ID = msg[offset];
    }

    return buoy_init_message;
}
