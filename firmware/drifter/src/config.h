#ifndef CONFIG_H
#define CONFIG_H
#include "Arduino.h"

static constexpr uint8_t BUOY_MODE {0};
static constexpr uint8_t BST_MODE {1};

static constexpr uint8_t WIO_MODE {BUOY_MODE}; 

/*
 Useful constants
 Do note, base time unit in arduino is milliseconds.
 These denote where we convert from seconds to milliseconds, or
 minutes to seconds to milliseconds. 
*/
static constexpr uint32_t s_2_ms                         {1000}; 
static constexpr uint32_t min_2_s                          {60};
static constexpr float    mps_2_kmph                       {3.6};


// Radio parameters
static           uint32_t minimal_transmission_period    {60*s_2_ms};
static constexpr float    LoRa_freq_receive              {863};
static constexpr float    LoRa_bw                        {125.0};
static constexpr uint8_t  LoRa_sf                        {8};
static constexpr uint8_t  LoRa_cr                        {6};
static constexpr uint8_t  LoRa_power                     {15};
static constexpr uint8_t  packet_count_send_treshold     {4};
static constexpr int16_t  transmission_grace_period      {5*s_2_ms};
static constexpr uint32_t max_radio_fix_look_time        {90*s_2_ms};
static constexpr uint32_t max_radio_wait_time            {40*s_2_ms};
static constexpr uint8_t  max_message_length             {64};
static constexpr uint32_t beacon_ping_period             {2*min_2_s*s_2_ms};
static constexpr float    LoRa_freq_beacon               {868};

// Sensor reading parameters
static constexpr uint8_t  measurements_per_packet            {15};
static constexpr uint8_t  max_number_of_measurements         {40};
static constexpr uint32_t max_GPS_read_time                  {3*min_2_s*s_2_ms};
static constexpr uint32_t max_sensor_read_time               {40*s_2_ms};
static constexpr float    outlier_discard_tolerance          {2};
static constexpr uint8_t  num_sensor_variables               {6};
static constexpr uint16_t GPS_baud                           {9600};
static constexpr uint8_t  max_number_of_thermometres         {1};
static constexpr uint8_t  thermometre_resolution             {12};
static constexpr uint32_t minimal_measurement_period         {10*s_2_ms};
static           uint32_t base_measurement_period            {15*s_2_ms};
static constexpr uint32_t maximal_measurement_period         {30*min_2_s*s_2_ms};
static constexpr bool     log_every_reading                  {true};
static constexpr bool     resync_RTC_using_GPS               {true};
static constexpr uint32_t scale_factor                       {100000};
static constexpr uint8_t  number_of_different_sensors        {2};
static constexpr uint32_t thermometre_pause_between_readings {300};

// Pin config. Do not change unless you have rewired the OLB
static constexpr uint8_t  GPS_RX_PIN                     {PC0};
static constexpr uint8_t  GPS_TX_PIN                     {PC1};
static constexpr uint8_t  GPS_SLEEP_PIN                  {PA0};


// Enable or disable parameters
static constexpr bool remove_outliers                       {true};
static constexpr bool debug_serial                          {true};
static constexpr bool enable_GPS                            {true};
static constexpr bool enable_watchdog                       {true};
static constexpr int  debug_SD                              {1};
static constexpr int  serial_baud                           {115200};
static           bool enable_motion_detection               {false};
static constexpr bool transmitDeploymentMessage             {false};
static constexpr bool debug_LED_enabled                     {false};
static constexpr bool sleep_GPS                             {true};
static constexpr bool perform_handshake                     {true};
static constexpr bool enable_baseStation_parameter_updates  {false};
static constexpr bool enable_recovery_beacon                {true};


// Motion parameters
static float    motion_treshold                          {0.5};
static uint32_t targetReadingDistance                    {30};

// Watchdog and power parameters
static constexpr uint32_t watchdog_wait_time              {32000};
static constexpr bool     debug_radio                     {false};
static constexpr uint32_t sleep_time                      {12000};
#endif