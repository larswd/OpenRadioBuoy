#ifndef CONFIG_H
#define CONFIG_H
#include "Arduino.h"

static constexpr uint8_t BUOY_MODE {0};
static constexpr uint8_t BST_MODE {1};

static constexpr uint8_t WIO_MODE {BST_MODE}; 

// Base station parameters
static constexpr uint8_t base_station_ID            {25};
static constexpr int max_message_length             {64};

// SD card parameters
const int8_t DISABLE_CS_PIN = -1;

// Radio parameters
static constexpr float LoRa_freq_send               {863};


/*
  LoRa_freq_receive is the target frequency to be used to receive
  messages from a buoy. If set to negative, it will be computed pseudorandomly
  otherwise, the fixed value will be used. 
*/
static constexpr float LoRa_freq_receive_min        {864.00};
static           float LoRa_freq_receive            {-1};
static constexpr float LoRa_freq_receive_max        {867.00};

static constexpr uint8_t num_LoRa_channels          {20};
static constexpr float LoRa_bw                      {125.0};
static constexpr int LoRa_sf                        {8};
static constexpr int LoRa_cr                        {6};
static constexpr int LoRa_power                     {15};
static constexpr uint32_t max_radio_fix_look_time   {60000};
static constexpr uint32_t max_radio_wait_time       {40000};
static constexpr uint32_t listen_time               {30000};

// Enable or disable parameters
static constexpr int debug_serial                   {1};
static constexpr int debug_SD                       {1};
static constexpr bool enable_handshake              {true};

// Debugging parameters
static constexpr int serial_baud                    {115200};

// Drifters parameters
static constexpr int max_number_of_thermometres          {8};
static constexpr uint32_t scale_factor              {100000};

// Watchdog parameters
static constexpr bool enable_watchdog               {true};
static constexpr uint32_t watchdog_wait_time        {32000};


// Notecard parameters
static constexpr int output_sync_frequency          {30}; // 30 minutes
static constexpr int input_sync_frequency           {30}; // 30 minutes
static constexpr int health_frequency               {30 * 60 * 1000}; // 30 minutes
static constexpr int reset_frequency                {120 * 60 * 1000}; // 2 h

// Adaptive frequency parameters
static constexpr bool get_frequency_from_notehub            {false}; 
static           uint32_t base_measurement_period           {10*60*1000}; // 10 minutes
static           bool default_update_frequency              {false};
static           bool enable_motion_detection               {false};
static           uint32_t targetReadingDistance             {100};
static           uint32_t motion_treshold                   {2000}; // m/s, divided by 10^4

// Buoy rescue parameters
static constexpr bool enable_rescue_from_notehub            {true}; 
static constexpr float LoRa_freq_beacon                     {868.00};


#endif