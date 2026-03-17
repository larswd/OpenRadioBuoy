#include <Arduino.h>
#include "config.h"
#include "sd_writer.h"
#include "thermo_manager.h"
/*
  This sketch showcase  all the functionality of the thermistor library
  Do note, this is more or less a port of the excellent thermistor library of
  the OpenMetBuoy by Jean Rabault, and can be found here:
  https://github.com/jerabaul29/OpenMetBuoy-v2021a/blob/main/legacy_firmware/firmware/plain_gps_drifter/thermistors_manager.h
*/

static uint32_t measurement_timer;
static constexpr uint8_t thermistorDataPin  {PA9};
static constexpr uint8_t thermistorPowerPin {PB10};

void setup() {
  
  // We start serial output at 115200 baudrate
  Serial.begin(serial_baud);
  Serial.println("Serial initialized!");
  delay(100);

  // We try to enable rtc timing
  setSyncInterval(1);


  // Some thermistor functionality interface with the SD writer
  // So we have to initialize the SD card writer as well
  bool SD_fail = sd_writer.begin();

  // We initialize the thermometres at the desired pins
  thermo_manager.begin(thermistorDataPin, thermistorPowerPin);
  measurement_timer = millis();
}

void loop() {
  // Measurement loop. 
  if (millis() - measurement_timer > base_measurement_period){
    
    thermo_manager.wake();

    //The SD library counts the amount of measurements taken
    char logname[25];
    sprintf(logname, "readings/reading%05d.txt", sd_writer.logCount);

    sd_writer.startLogging(logname);

    // The last argument specifies if we want to log each thermistor reading or
    // Just the filtered end value
    thermo_manager.takeReadings(max_sensor_read_time, millis(), true);
    thermo_manager.processReadings();
    
    // We should turn off the thermistors to save power
    thermo_manager.sleep();
    // Update clock variables
    measurement_timer = millis();
    sd_writer.closeLog();
  }
  delay(150);
}

