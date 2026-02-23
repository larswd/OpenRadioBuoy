#include <Arduino.h>
#include "config.h"
#include "etl_error_manager.h"
#include "messages.h"
#include "lora_transceiver.h"
#include "gps_manager.h"
#include "sd_writer.h"
#include "thermo_manager.h"
#include "IWatchdog.h"
#include "STM32LowPower.h"
#include "TimeLib.h"
/*
  TODO: 
  Get a good algorithm for getting a date stamp from GPS.
  This feature is currently disabled. 
*/


/* Workaround for millis not increasing during sleep cycles */
uint32_t measurement_timer;
uint32_t beacon_timer = 0;
static uint32_t sleep_cycles_measurement  = 0;
static uint32_t sleep_cycles_transmission = 0;
static uint32_t sleep_cycles_beacon       = 0;
static uint32_t iterations_counter = 0;


uint32_t millis_time_corrected(uint32_t sleep_cycles){
  return millis() + sleep_cycles*watchdog_wait_time/2;
}

void setup() {

  // Serial debugging can be turned on or off with the debug_serial flag
  if (debug_serial){
    Serial.begin(serial_baud);
    Serial.println("Serial initialized!");
    delay(100);
  }

  // We try to enable rtc timing
  setSyncInterval(1);

  // Watchdog begin
  if (enable_watchdog){
    IWatchdog.begin(1000*watchdog_wait_time);
  }
  /*
    Note, watchdog has .isReset member which allows us to resume operations in case of 
    shutdown. Look into implementation of restore from SD/RAM
  */

  // Start SD
  bool SD_fail = sd_writer.begin();  
  int8_t status = sd_writer.startLogging("boot_info.txt");
  if (!SD_fail && debug_serial){
    Serial.println("SD opened");
    Serial.print("Status code: ");
    Serial.println(status);
  }
  IWatchdog.reload();


  /*
    Buoy will not start without working radio.
  */
  LORA.beginRadio(LoRa_freq_receive, LoRa_bw, LoRa_sf, LoRa_cr, LoRa_power);
  LORA.getWiOID();
  if (LORA.state == RADIOLIB_ERR_NONE){
    if (debug_serial){
      Serial.println(F("Radio functional!!!")); 
    }
    sd_writer.logString("Radio functional");
  } else {
    if (debug_serial){
      Serial.println(F("Radio failed to start. Freezing"));
    }
    sd_writer.logString("Radio failed to start. Freezing sensor.");
    while(1);
  }

  if (SD_fail){
    if (debug_serial){
      Serial.println(F("SD card mount failed. Proceeding with Radio only"));
    }
    delay(100);
  }
  
  /*
    We monitor possible overflow in ETL storage containers
  */
  etl::error_handler::set_callback<etl_error_func>();


  if (debug_serial){
    Serial.println(F("Beginning gps"));
    sd_writer.logString("Beginning GPS");
  }
  IWatchdog.reload();
  gps_manager.begin(GPS_baud);

  int8_t  nofix = 1;

  /*
    Buoy will not advance until it has gotten a fix
  */
  while (nofix){
    gps_manager.updateTimestamp(10*max_GPS_read_time, true);
    nofix = gps_manager.performNReadings(1,10*max_GPS_read_time, true);
    delay(100);
    IWatchdog.reload();
  }

  gps_manager.processReadings(false);

  /*
    We need to read the attached thermistors for a buoy ID
  */
  thermo_manager.begin(PA9,PB10);

  gps_manager.getDeploymentMessage(LORA.WiO_ID);
  LORA.startup_timestamp = gps_manager.timestamp;
  IWatchdog.reload();
  if (debug_serial){
    Serial.println("Sending deployment message");
  }
  if (IWatchdog.isReset() == false && transmitDeploymentMessage){
    /*
      We wait until we get a fix from a base station before proceeding.
      Allows us to make sure the buoy works before tossing it into the water. 
    */
    while (LORA.available = false){
      LORA.handshake(max_radio_fix_look_time);
      delay(300);
      IWatchdog.reload();
    }
    LORA.transmitB(gps_manager.deploymentMessage, deployment_message_size);
    sd_writer.logString(gps_manager.deploymentMessage, deployment_message_size);
    LORA.waitUntilReady();
    //String firmware_ID = String(REPO_COMMIT_ID);
    //LORA.transmit("Running firmware version: ");
    //LORA.waitUntilReady();
    //LORA.transmit(firmware_ID.c_str());
  }
  
  LORA.lastTransmission = millis();
  if (IWatchdog.isReset() == true){
    //We check to see if the buoy crashed
    sd_writer.logString("Watchdog crashed during execution of loop. This is a reboot.");
  }


  measurement_timer = millis();
  if (enable_recovery_beacon){
    beacon_timer = millis();
  }
  /*
    Start low power management. As low power requires all connections 
    to be off, we turn them off here, before
    enabling them again in the loop. 
  */ 
  LowPower.begin();
  gps_manager.shutdownGPS();
  sd_writer.closeLog();
  thermo_manager.sleep();
  if (debug_LED_enabled){
    digitalWrite(LED_BUILTIN, LOW);
  }
  LORA.sleep();

  
  // We try to recover a GPS measurement:
  //char targetFile[64];
  //sprintf(targetFile, "readings/reading%05d.txt", 3);
  //sd_writer.startReading(targetFile);
  //sd_writer.readFile();
  //gps_manager.getMeasurementFromFile();
  //thermo_manager.getMeasurementFromFile();
  //sd_writer.closeRead();


  if (debug_serial){
    Serial.println("Setup complete, loop beginning");
    delay(100);
    Serial.end();
  }


}


void loop() {
  /*
    We start by checking if we should switch on each sensor
  */
  IWatchdog.reload();
  bool disable_sensors = false;

  
  if (debug_serial){
    Serial.begin(serial_baud);
    delay(100);
    Serial.println("Booting up!");

  }

  // Measurement loop
  if (millis_time_corrected(sleep_cycles_measurement) - measurement_timer > LORA.measurement_period){
    if (debug_LED_enabled){
      digitalWrite(LED_BUILTIN, HIGH);
    }
    gps_manager.begin(GPS_baud);
    thermo_manager.wake();
    IWatchdog.reload();
    char logname[25];
    sprintf(logname, "readings/reading%05d.txt", sd_writer.logCount);
    
    if (!sd_writer.active){
      sd_writer.begin();
    }
    sd_writer.startLogging(logname);
    IWatchdog.reload();

    // We read each sensor
    if (debug_serial){
      Serial.println("Reading sensors");
      Serial.println(gps_manager.iterations);
      delay(100);
    }
    
    // success_gps_read is 0 if the buoy got a fix
    uint8_t success_gps_read = gps_manager.updateTimestamp(max_GPS_read_time, resync_RTC_using_GPS);
    if (success_gps_read == 0){
      gps_manager.performNReadings(measurements_per_packet,max_GPS_read_time, log_every_reading);
      gps_manager.processReadings(true);

      // We should turn off the GPS post reading due to power consumption
      gps_manager.shutdownGPS();

      thermo_manager.takeReadings(max_sensor_read_time, gps_manager.timestamp, log_every_reading);
      thermo_manager.processReadings();
 

      IWatchdog.reload();
      // We update the measurements period to reflect the new motion paradigme
      if (enable_motion_detection){
        LORA.updateMeasurementFrequency(gps_manager.current_buoy_velocity, maximal_measurement_period, minimal_measurement_period);
      }
    } else {
      // If no fix was found, we skip this measurement and hope for better luck the next time around. 
      gps_manager.shutdownGPS();
    }
    // Turning off remaining sensors
    sleep_cycles_measurement = 0;
    measurement_timer = millis_time_corrected(sleep_cycles_measurement);
    thermo_manager.sleep();
    sd_writer.closeLog();
    IWatchdog.reload();
  }
  
  // Transmission protocol
  if ((millis_time_corrected(sleep_cycles_transmission) - LORA.lastTransmission > minimal_transmission_period) && 
      (gps_manager.GPSReadings.size() > packet_count_send_treshold)) {  
    if (debug_serial){
      Serial.println("Looking for base station");
    }
    if (debug_LED_enabled){
      for (int i = 0; i < 3; i++){
        digitalWrite(LED_BUILTIN, LOW);
        delay(800);
        digitalWrite(LED_BUILTIN, HIGH);
        IWatchdog.reload();
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    char logname[30];
    sprintf(logname, "messages/transmission%05d.txt", LORA.msgCounter);

    if (!sd_writer.active){
      sd_writer.begin();
    }

    sd_writer.startLogging(logname);
    IWatchdog.reload();
    LORA.wakeUp();
    sd_writer.logString("Looking for base station");
    if (perform_handshake){
      LORA.findBaseStation(max_radio_fix_look_time);
      delay(200);
      if (LORA.baseStationID > 0){
        LORA.handshake(max_radio_wait_time);
      }
      if (debug_serial){
        Serial.println("Hands shaked");
      }
      sd_writer.logString("Hands shaked");
      delay(300);
    } else {
      // We do not check if a base station is present
      LORA.available = true;
      LORA.listenTime = 60*s_2_ms;
    }

    Message_Data message_data;
    sd_writer.logString("Transmitting data\n--------\n");

    // We dump as much information as possible to the base station
    // In the time period alloted to us
    while (LORA.available && (thermo_manager.temperatures.size() > 0)){
      gps_manager.updateTransmitMessage();
      message_data = LORA.sendData(gps_manager.msgB, GPS_message_size, 10000);
      delay(500);

      sd_writer.logByteArray(gps_manager.msgB, GPS_message_size);
      sd_writer.logSignalInfo(message_data.RSSI, message_data.SNR);

      thermo_manager.updateTransmitMessage();
      message_data = LORA.sendData(thermo_manager.msgB, thermo_message_size, 10000);
      IWatchdog.reload();

      delay(500);
      sd_writer.logByteArray(thermo_manager.msgB, thermo_message_size);
      sd_writer.logSignalInfo(message_data.RSSI, message_data.SNR);

      delay(200);
      LORA.packet_count++;
      IWatchdog.reload();
      if (debug_serial){
        Serial.print("Available: ");
        Serial.println(LORA.available);
        Serial.print("Num packets left: ");
        Serial.println(thermo_manager.temperatures.size());
      }

      delay(500);
    }
    
    // Wrap up transmission, wait for base station instructions
    if (LORA.available){
      LORA.transmitFinished(thermo_manager.temperatures.size());
      if (enable_baseStation_parameter_updates){
        // We listen for new parameters
        LORA.receiveInstructions();

        // Then we listen for which measurements to add back to memory from 
        // SD card - TBD
        //LORA.receiveDesiredMeasrements();
        //sd_writer.fetchRequestedMeasurements(LORA.measurementTargets);
      }
      LORA.updateMeasurementFrequency(gps_manager.current_buoy_velocity, maximal_measurement_period, minimal_measurement_period);

    }
    
    if (debug_serial){
      Serial.println("Transmission done");
    }
    sd_writer.logString("Transmission done");
    sd_writer.closeLog();

    sleep_cycles_transmission = 0;
    measurement_timer = millis();
    LORA.sleep();
 
  }

  Serial.println("\n Succesfull iteration, starting to log\n");


  // Recovery protocol
  if ((millis_time_corrected(sleep_cycles_beacon) - beacon_timer > beacon_ping_period) && (enable_recovery_beacon)){
    LORA.wakeUp();
    gps_manager.updateBeaconMsg(LORA.WiO_ID);
    LORA.transmitBeaconMessage(gps_manager.beaconMsg, beaconMsgSize);
    sleep_cycles_beacon = 0;
    beacon_timer        = millis();
    LORA.sleep();
  }
  Serial.println("Logging finished");

  if (debug_serial){
    Serial.println(F("Shutting down"));
    delay(100);
    Serial.end();
  }
  
  // We sleep for half the watch dog wait time before waking up to refresh the watchdog
  IWatchdog.reload();
  sleep_cycles_measurement++;
  sleep_cycles_transmission++;
  if (sd_writer.active){
    sd_writer.shutdown();
  }
  delay(150);
  LowPower.sleep(sleep_time);
}
