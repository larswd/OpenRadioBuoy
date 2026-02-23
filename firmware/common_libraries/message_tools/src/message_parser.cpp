#include "message_parser.h"
#include "parser_utils.h"

Message_Parser MESSAGE_PARSER;
// The new structure is "Tn(n*s*xxxx)ttttttttE"
temperature_Reading Message_Parser::parse_temperature_message(byte *msg)
{
    temperature_Reading temperature_reading_packet;
    int32_t offset = 1;
    // int32_t temps[max_thermometer] = {0};

    temperature_reading_packet.reading_ID = msg_extract_uint<uint16_t>(msg, offset, true);
    offset += sizeof(uint16_t);

    temperature_reading_packet.num_sensors =  msg[offset];
    offset += sizeof(byte);

    for (int j = 0; j < temperature_reading_packet.num_sensors; j++)
    {
        // int32_t temp = 0;
        // int32_t factor = 1;
        // if (msg[offset] == 'N')
        // {
        //     factor = -1;
        // }
        // offset += 1;
        // uint32_t fac32 = 256 * 256 * 256;
        // for (int i = 0; i < 4; i++)
        // {
        //     temp += msg[offset] * fac32;
        //     fac32 /= 256;
        //     offset += 1;
        // }
        // temps[j] = temp * factor;
        
        int8_t factor = msg[offset] == 'N' ? -1 : 1;
        // offset += sizeof(byte);
        offset += sizeof(byte);
        temperature_reading_packet.temps[j] = msg_extract_uint<int32_t>(msg, offset, true) * factor;
        // offset += sizeof(int32_t);
        offset += sizeof(int32_t);

    }
    // for (int j = 0; j < max_thermometer; j++)
    // {
    //     temperature_reading_packet.temps[j] = temps[j];
    // }

    // time_t timestamp = 0;
    // uint64_t fac64 = 1;
    // for (int i = 0; i < 7; i++)
    // {
    //     fac64 *= 256;
    // }
    // for (int i = 0; i < 8; i++)
    // {
    //     timestamp += msg[offset] * fac64;
    //     fac64 /= 256;
    //     offset += 1;
    // }
    temperature_reading_packet.timestamp = msg_extract_uint<time_t>(msg, offset, true);
    // Serial.println(sizeof(time_t));
    // time_t timestamp = 0;
    // uint64_t fac64 = 1;
    // for (int i = 0; i < 7; i++)
    // {
    //     fac64 *= 256;
    // }
    // for (int i = 0; i < 8; i++)
    // {
    //     timestamp += msg[offset] * fac64;
    //     fac64 /= 256;
    //     offset += 1;
    // }
    // temperature_reading_packet.timestamp = timestamp;
    return temperature_reading_packet;
}

GPS_Reading Message_Parser::parse_gps_message(byte *msg)
{
    GPS_Reading gps_reading_packet;
    uint8_t offset = 1;
    gps_reading_packet.reading_ID = msg_extract_uint<uint16_t>(msg, offset, true);
    offset += sizeof(uint16_t);
    gps_reading_packet.lat = msg_extract_uint<uint32_t>(msg, offset, true);
    offset += sizeof(uint32_t);
    gps_reading_packet.lng = msg_extract_uint<uint32_t>(msg, offset, true);
    offset += sizeof(uint32_t);
    gps_reading_packet.vel = msg_extract_uint<uint32_t>(msg, offset, true);
    offset += sizeof(uint32_t);
    gps_reading_packet.direction = msg_extract_uint<uint32_t>(msg, offset, true);
    offset += sizeof(uint32_t);
    gps_reading_packet.timestamp = msg_extract_uint<time_t>(msg, offset, true);
    // time_t timestamp = 0;
    // uint64_t fac64 = 1;
    // for (int i = 0; i < 7; i++)
    // {
    //     fac64 *= 256;
    // }
    // for (int i = 0; i < 8; i++)
    // {
    //     timestamp += msg[offset] * fac64;
    //     fac64 /= 256;
    //     offset += 1;
    // }
    // gps_reading_packet.timestamp = timestamp;
    return gps_reading_packet;
}

beacon_Reading Message_Parser::parse_beacon_message(byte *msg)
{
    beacon_Reading beacon_reading_packet;
    uint8_t offset = 2;
    beacon_reading_packet.timestamp = msg_extract_uint<time_t>(msg, offset, true);
    offset += sizeof(time_t);
    beacon_reading_packet.lat = msg_extract_uint<int32_t>(msg, offset, true);
    offset += sizeof(int32_t);
    beacon_reading_packet.lng = msg_extract_uint<int32_t>(msg, offset, true);
    offset += sizeof(int32_t);
    beacon_reading_packet.buoy_id = msg_extract_uint<uint32_t>(msg, offset, true);
    return beacon_reading_packet;

}

turbidity_Reading Message_Parser::parse_turbidity_message(byte* msg)
{
    turbidity_Reading turbidity_reading_packet;

    int32_t offset = 1;
    // uint32_t timestamp = 0;
    // uint64_t fac64 = 1;
    // for (int i = 0; i < 7; i++)
    // {
    //     fac64 *= 256;
    // }
    // for (int i = 0; i < 8; i++)
    // {
    //     timestamp += msg[offset] * fac64;
    //     fac64 /= 256;
    //     offset += 1;
    // }

    turbidity_reading_packet.timestamp = msg_extract_uint<time_t>(msg, offset, true);
    offset += sizeof(time_t);
    turbidity_reading_packet.voltage = msg_extract_uint<uint32_t>(msg, offset, true);

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
    int32_t offset = 4;
    buoy_info_packet.listen_time = msg_extract_uint<uint32_t>(msg, offset, true);
    // for (int i = 0; i < 3; i++)
    // {
    //     alpha *= 256;
    // }
    // for (int i = 0; i < 4; i++)
    // {
    //     listen_time += msg[i + offset] * alpha;
    //     alpha /= 256;
    // }
    // buoy_info_packet.listen_time = listen_time;
    buoy_info_packet.crashed = msg[8];

    return buoy_info_packet;
}

buoyInitMessage Message_Parser::parse_buoy_init_message(byte *msg)
{
    buoyInitMessage buoy_init_message;

    if (msg[0] == 'U' && msg[6] == 'E')
    {
        buoy_init_message.buoy_id = msg_extract_uint<uint32_t>(msg, 1, true);
        buoy_init_message.base_station_ID = msg[1+sizeof(uint32_t)];
    }

    return buoy_init_message;
}
