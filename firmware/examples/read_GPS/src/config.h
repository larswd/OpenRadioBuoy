#ifndef CONFIG_H
#define CONFIG_H
#include "Arduino.h"

static constexpr uint8_t BUOY_MODE {0};
static constexpr uint8_t BST_MODE {1};

static constexpr uint8_t WIO_MODE {BUOY_MODE}; 

// Useful constants
static constexpr uint32_t s_2_ms                         {1000};
static constexpr uint32_t min_2_s                          {60};
static constexpr float    mps_2_kmph                       {3.6};


// Sensor reading parameters
static constexpr uint8_t  readings_per_measurement            {15};
static constexpr uint8_t  max_number_of_measurements         {80};
static constexpr uint32_t max_GPS_read_time                  {3*min_2_s*s_2_ms};
static constexpr uint32_t max_sensor_read_time               {40*s_2_ms};
static constexpr float    outlier_discard_tolerance          {2};
static constexpr uint16_t GPS_baud                           {9600};
static constexpr uint32_t minimal_measurement_period         {10*s_2_ms};
static           uint32_t base_measurement_period            {5*min_2_s*s_2_ms};
static constexpr uint32_t maximal_measurement_period         {30*min_2_s*s_2_ms};
static constexpr bool     resync_RTC_using_GPS               {true};
static constexpr uint32_t scale_factor                       {100000};

// Pin config. Do not change unless you have rewired the OLB
static constexpr uint8_t  GPS_RX_PIN                     {PC0};
static constexpr uint8_t  GPS_TX_PIN                     {PC1};
static constexpr uint8_t  GPS_SLEEP_PIN                  {PA0};

// Enable or disable parameters
static constexpr bool remove_outliers           {true};
static constexpr bool debug_serial              {true};
static constexpr bool enable_GPS                {true};
static constexpr bool enable_watchdog           {true};
static constexpr int  serial_baud               {115200};
static           bool enable_motion_detection   {false};
static constexpr bool sleep_GPS                 {true};
static constexpr bool debug_SD                  {false};
// Motion parameters
static float    motion_treshold                          {0.5};
static uint32_t target_reading_distance                    {30};

// Watchdog and power parameters
static constexpr uint32_t watchdog_wait_time              {32000};
static constexpr uint32_t sleep_time                      {12000};
#endif