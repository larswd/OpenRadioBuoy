#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H
#include <Arduino.h>
#include "config.h"
#include "STM32LowPower.h"
/* We count the amount  */
static uint32_t measurement_timer;
static uint32_t sleep_cycles_measurement  = 0;
static uint32_t iterations_counter = 0;

uint32_t millis_time_corrected(uint32_t sleep_cycles);

#endif