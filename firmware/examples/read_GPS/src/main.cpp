#include <Arduino.h>
#include "config.h"
#include "sd_writer.h"
#include "gps_manager.h"
/*
  This sketch showcase  all the functionality of the GPS library

*/

static uint32_t measurement_timer;

void setup() {
  
  // We start serial output at 115200 baudrate
  Serial.begin(serial_baud);
  Serial.println("Serial initialized!");
  delay(100);

  // We try to enable rtc timing
  setSyncInterval(1);


  // Some GPS functionality interface with the SD writer
  // So we have to initialize the SD card writer as well
  bool SD_fail = sd_writer.begin();

  measurement_timer = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Measurement loop. The millis_time_corrected is a function to account
  // for the Arduino millis()-function not working during a sleep cycle
  if (millis() - measurement_timer > base_measurement_period){
    
    // We begin the GPS at the specified baud rate
    gps_manager.begin(GPS_baud);

    //The SD library counts the amount of measurements taken
    char logname[25];
    sprintf(logname, "readings/reading%05d.txt", sd_writer.logCount);

    sd_writer.startLogging(logname);
    
    // success_gps_read is 0 if the buoy got a fix
    uint8_t success_gps_read = gps_manager.updateTimestamp(max_GPS_read_time, resync_RTC_using_GPS);
    if (success_gps_read == 0){
      // The last argument specifies if we want to log each GPS reading or
      // Just the filtered end value
      gps_manager.performNReadings(readings_per_measurement,max_GPS_read_time, true);
      gps_manager.processReadings(true);

      // We should turn off the GPS post reading due to power consumption
      gps_manager.shutdownGPS();
    } else {
      // If no fix was found, we skip this measurement and hope for better luck the next time around. 
      gps_manager.shutdownGPS();
    }
    // Update clock variables
    measurement_timer = millis();
    sd_writer.closeLog();
  }
  delay(150);
}

