#include "sleep_manager.h"

/*
  We count the amount of active time of the buoy using millis()
  but also add the amount of sleep_cycles since last time
*/
uint32_t millis_time_corrected(uint32_t sleep_cycles){
  return millis() + sleep_cycles*sleep_time;
}