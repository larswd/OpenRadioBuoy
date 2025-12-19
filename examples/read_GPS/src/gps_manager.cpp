#include "gps_manager.h"



void GPS_Manager::begin(float baudrate){
  /*
    We initialize the GPS at a set baudrate,
    and supply power to the GPS enable pin to 
    power it up. 
  */
  pinMode(GPS_SLEEP_PIN, OUTPUT);
  digitalWrite(GPS_SLEEP_PIN, HIGH);
  delay(200);
  ss.begin(baudrate);
  initialized = true;
  delay(100);
  currentPosition = {0,0,0,0,0,0};
}

uint8_t GPS_Manager::updateTimestamp(uint32_t max_wait_time, bool refreshGPStime){
    /*
      Read date from GPS. Error codes are returned if the 
      GPS is called while not initialized. 
      Returns 2 if no fix was found
    */
  if (refreshGPStime){
    uint8_t hour, minute, second;
    if (!enable_GPS){
      date.year  = 2025;
      date.month = 1;
      date.day   = 17;
      hour = 14;
      minute = 21;
      second = 0;
      date.valid = true;
    } else {
      if (!initialized){
        return 1;
      }
      uint32_t searchStart = millis();
      // Error code 1 removed for debugging purposes
      uint16_t iteration_counter = 0;
      
      // We try to get a fix to get RTC
      while ((gps.date.isValid() == false) || (gps.time.isValid() == false) && (millis() - searchStart < max_GPS_read_time)){
        while(ss.available() > 0){
          gps.encode(ss.read());
        }
        if (iteration_counter > 2000){
          IWatchdog.reload();
          iteration_counter = 0;
        }
        iteration_counter++;
        if ((millis() - searchStart > max_GPS_read_time) && gps.time.isValid() == false){
          return 2;
        }
      }
      hour = gps.time.hour();
      minute = gps.time.minute();
      second = gps.time.second();
      date.year  = gps.date.year();
      date.month = gps.date.month();
      date.day   = gps.date.day();
      date.valid = true;
    }
    // We set the RTC using the GPS measurements
    setTime(hour, minute, second, date.day, date.month, date.year);
  }
  // Then update the timestamp
  timestamp = now();
  return 0;
}

uint8_t GPS_Manager::performNReadings(uint8_t N, uint32_t max_wait_time, bool logEveryReading){
  /*
    We read our position from the GPS N times, and push each 
    to the private packet vector. User then need to call the processReadings method
    to push the data to the public GPSReadings array
  */
 
  int8_t counter = 0;
  uint32_t start = millis();
  if (N > measurements_per_packet){
    if (debug_serial){
      Serial.println(F("N is larger than max value. Setting N equal to max value"));
    }
  }

  
  if (logEveryReading){
    sd_writer.logString("t:lat:lng:vel:dir");
  }
  
  uint8_t numReadings = min(N, measurements_per_packet);
  while( (counter < numReadings) && (millis() - start < max_wait_time)){
    updateTimestamp(max_wait_time, false);
    getGPSData(min(watchdog_wait_time/2, max_wait_time - (millis() - start)));
    if (logEveryReading){
      GPS_Data latestReading = packet.back();
      logReading(latestReading);
    }
    counter++;
    IWatchdog.reload();
  }

  iterations++;

  if (counter == N){
    return 0;
  } else {
    return 1;
  }
}

uint8_t GPS_Manager::getGPSData(uint32_t max_wait_time){
  /*
    Read a single full measurement packet from the GPS,
    or return a dummy value in case the GPS is disabled for debugging in config.h
    If the GPS spends more than max_wait_time milliseconds, then a reading filled with zeros is returned
  */
  GPS_Data reading = {0,0,0,0,0};
  if (!enable_GPS){
    reading.timestamp = timestamp;
    reading.lat       = scale_factor*4;
    reading.lng       = scale_factor*5;
    reading.vel       = scale_factor*6;
    reading.direction = scale_factor*7;
    date.year = 2025;
    date.month = 1;
    date.day = 16;
  } else {
    if (!initialized){
      return 2;
    }
    uint32_t searchStart = millis();
    int counter = 0;
    while ((gps.location.isUpdated() == false) && (millis() - searchStart < max_wait_time)){
      while(ss.available() > 0){
        gps.encode(ss.read());
      }
      //No need to pet the dog every cycle
      counter++;
      if (counter > 100){
        IWatchdog.reload();
      }
    }
    
    reading.timestamp = timestamp;
    reading.lat       = (uint32_t) (scale_factor*gps.location.lat());
    reading.lng       = (uint32_t) (scale_factor*gps.location.lng());
    reading.vel       = (uint32_t) (scale_factor*gps.speed.mps());
    reading.direction = (uint32_t) (scale_factor*gps.course.deg());

    if (gps.date.isUpdated() || iterations < 1){
      date.year  = gps.date.year();
      date.month = gps.date.month();
      date.day   = gps.date.day();
    }

  }
  packet.push_back(reading);
  return 0;
}

void GPS_Manager::getDeploymentMessage(uint32_t buoy_ID){
  /*
    Deployment message format:
    UIzzzzttttttttyyyyxxxxE
    Where
    z is the 4 byte buoy ID
    t is the 8 byte timestamp
    y is the 4 byte latitude
    x is the 4 byte longitude

    Total size: 23
  */
  struct GPS_Data initial_fix = GPSReadings.back();
  
  if (debug_serial){
    Serial.println("Writing deployment message!");
    delay(100);
  }
  deploymentMessage[0] = 'U';
  deploymentMessage[1] = 'I';
  
  deploymentMessage[deployment_message_size-1] = 'E';
  msg_insert_uint(deploymentMessage, buoy_ID, 2, deployment_message_size, true);
  msg_insert_uint(deploymentMessage, timestamp,2 + sizeof(buoy_ID), deployment_message_size, true);
  msg_insert_uint(deploymentMessage, initial_fix.lat, 2 + sizeof(buoy_ID) + sizeof(timestamp), deployment_message_size, true);
  msg_insert_uint(deploymentMessage, initial_fix.lng, 2 + sizeof(buoy_ID) + sizeof(timestamp) + sizeof(initial_fix.lat), deployment_message_size, true);

  if (debug_serial){
    Serial.println("Removing deployment data");
    delay(100);
  }
  gps_manager.GPSReadings.pop_back();
  
}

void GPS_Manager::processReadings(bool fullProcessingToggle){
  /*
    Method which processes the measured data packet and,
    if fullProcessingToggle is true, filters extreme values before computing 
    an average. Pushes the averaged GPS reading to the GPSReadings deque.
  */

  // Storage variables
  etl::vector<uint32_t, measurements_per_packet> int32vals;

  // As it is, afaik, not possible to iterate over struct variables
  // We instead have to perform the iteration though code repetition
  for (int i = 0; i < packet.size(); i++){
    int32vals.push_back(packet[i].lat);
  }
  if (fullProcessingToggle){
    mean_values.lat = filter_vector(int32vals);
  } else {
    etl::mean<uint32_t, double> mean_int32vals(int32vals.begin(), int32vals.end());
    mean_values.lat = (uint32_t) mean_int32vals;
  }
  mean_values.timestamp = timestamp;
  int32vals.clear();

  delay(50);
  IWatchdog.reload();

  for (int i = 0; i < packet.size(); i++){
    int32vals.push_back(packet[i].lng);
  }


  if (fullProcessingToggle){
    mean_values.lng = filter_vector(int32vals);
  } else {
    etl::mean<uint32_t, double> mean_int32vals(int32vals.begin(), int32vals.end());
    mean_values.lng = (uint32_t) mean_int32vals;
  }
  int32vals.clear();
  delay(50);
  
  for (int i = 0; i < packet.size(); i++){
    int32vals.push_back(packet[i].vel);
  }

  if (fullProcessingToggle){
    mean_values.vel = filter_vector(int32vals);
  } else {
    etl::mean<uint32_t, double> mean_int32vals(int32vals.begin(), int32vals.end());
    mean_values.vel = (uint32_t) (mean_int32vals);
  }
  current_buoy_velocity = mean_values.vel;
  IWatchdog.reload();
  int32vals.clear();
  delay(50);
  
  for (int i = 0; i < packet.size(); i++){
    int32vals.push_back(packet[i].direction);
  }
  if (fullProcessingToggle){
    mean_values.direction = filter_vector(int32vals);
  } else {
    etl::mean<uint32_t, double> mean_int32vals(int32vals.begin(), int32vals.end());
    mean_values.direction = (uint32_t) mean_int32vals;
  }

  IWatchdog.reload();
  int32vals.clear();
  delay(50);
  

  if (GPSReadings.size() == max_number_of_measurements){
    GPSReadings.pop_front();
  }

  // Minus one to match file name and ID number
  mean_values.readingID = sd_writer.logCount > 0 ?  sd_writer.logCount - 1 : 0;
  if (debug_serial){
    Serial.print("Reading IDs: ");
    Serial.println(mean_values.readingID);
  }

  currentPosition = mean_values;
  GPSReadings.push_back(mean_values);

  sd_writer.logString("Filtered:");
  logReading(mean_values);
}

void GPS_Manager::shutdownGPS(void){
  /*
    We shut the GPS down to save power. 
  */
  if (sleep_GPS){
    pinMode(GPS_SLEEP_PIN, INPUT);
  }  
  packet.clear();
  ss.end();
}

size_t GPS_Manager::updateTransmitMessage(){
  /*
    We cast the oldest GPS reading to a byte array for transmission
    then remove the reading from the deque. 
  */
  GPS_Data gpsdata = GPSReadings.front();
  msgB[0] = 'G';
  msg_insert_uint(msgB, gpsdata.readingID, 1, GPS_message_size, true);
  msg_insert_uint(msgB, gpsdata.lat, 1 + sizeof(gpsdata.readingID), GPS_message_size, true);
  msg_insert_uint(msgB, gpsdata.lng, 1 + sizeof(gpsdata.readingID) + sizeof(gpsdata.lat), GPS_message_size, true);
  msg_insert_uint(msgB, gpsdata.vel, 1 + sizeof(gpsdata.readingID) + 2*sizeof(gpsdata.lat), GPS_message_size, true);
  msg_insert_uint(msgB, gpsdata.direction, 1 + sizeof(gpsdata.readingID) + 3*sizeof(gpsdata.lat), GPS_message_size, true);
  msg_insert_uint(msgB, gpsdata.timestamp, 1 + sizeof(gpsdata.readingID) + 4*sizeof(gpsdata.lat), GPS_message_size, true);  
  msgB[GPS_message_size-1] = 'E';
  GPSReadings.pop_front();
  return GPS_message_size;
}

uint8_t GPS_Manager::logReading(GPS_Data & data){
  /*
    The latest reading is written to an SD file. 
  */

  byte data_reading[1 + sizeof(data.readingID) + 4*sizeof(data.lat) + sizeof(data.timestamp) + 1];
  data_reading[0] = 'G';
  msg_insert_uint(data_reading, data.readingID, 1, sizeof(data_reading));
  msg_insert_uint(data_reading, data.lat, 1 + sizeof(data.readingID) + 0*sizeof(data.lat), sizeof(data_reading), true);
  msg_insert_uint(data_reading, data.lng, 1 + sizeof(data.readingID) + 1*sizeof(data.lat), sizeof(data_reading), true);
  msg_insert_uint(data_reading, data.vel, 1 + sizeof(data.readingID) + 2*sizeof(data.lat), sizeof(data_reading), true);
  msg_insert_uint(data_reading, data.direction, 1 + sizeof(data.readingID) + 3*sizeof(data.lat), sizeof(data_reading), true);
  msg_insert_uint(data_reading, data.timestamp, 1 + sizeof(data.readingID) + 4*sizeof(data.lat), sizeof(data_reading), true);
  data_reading[-1] = 'E';
  
  uint8_t state = sd_writer.logByteArray(data_reading, sizeof(data_reading));
  return state;
}


/*
  Method which reads the filtered measurement from 
  a fil ein the file buffer, and appends it to the 
  front of the GPSReadings deque. 

*/
void GPS_Manager::getMeasurementFromFile(void){
  GPS_Data readData;
  if (sd_writer.numLines > 0){
    uint8_t lineNo = 1;
    fileLine currentLine = {"",0}, prevLine = {"",0};
    while(lineNo < sd_writer.file_buffer.size()){
      // We check for the filtered value
      prevLine = currentLine;
      currentLine = {"",0};
      currentLine = sd_writer.file_buffer.at(lineNo);
      

      if ((strncmp(prevLine.line, "Filtered:", 4) == 0) \
          && (strncmp(currentLine.line, "71b", 3) == 0)){

            byte filteredData[GPS_message_size];

            // Make a copy
            char tmpHolder[max_line_length];
            sscanf(currentLine.line, "%s", tmpHolder);
            
            // Filter for data
            for (uint8_t index = 0; index < GPS_message_size; index++){
              sscanf(tmpHolder, "%db%s", &filteredData[index], tmpHolder);
            }
            readData.readingID = msg_extract_uint<uint16_t>(filteredData, 1, true);
            readData.lat       = msg_extract_uint<uint32_t>(filteredData, 1 + sizeof(readData.readingID) + 0*sizeof(readData.lat), true);
            readData.lng       = msg_extract_uint<uint32_t>(filteredData, 1 + sizeof(readData.readingID) + 1*sizeof(readData.lat), true);
            readData.vel       = msg_extract_uint<uint32_t>(filteredData, 1 + sizeof(readData.readingID) + 2*sizeof(readData.lat), true);
            readData.direction = msg_extract_uint<uint32_t>(filteredData, 1 + sizeof(readData.readingID) + 3*sizeof(readData.lat), true);
            readData.timestamp = msg_extract_uint<uint64_t>(filteredData, 1 + sizeof(readData.readingID) + 4*sizeof(readData.lat), true); 


            if (debug_serial){
              delay(300);
              Serial.println(sd_writer.file_buffer.at(lineNo).line);
              Serial.println("Recovered data:\n---------------\n");
              Serial.print("ID = ");
              Serial.println(readData.readingID);
              Serial.print("Lat = ");
              Serial.println(readData.lat);
              Serial.print("Lon = ");
              Serial.println(readData.lng);
              Serial.print("Vel = ");
              Serial.println(readData.vel);
              Serial.print("Dir = ");
              Serial.println(readData.direction);
              Serial.print("t = ");
              Serial.println(readData.timestamp);
              Serial.println("---------------\n");
              delay(200);
            }
            
            // New measurements take precedence in memory over old measurements
            if (GPSReadings.full()){
              GPSReadings.pop_front();
            }
            GPSReadings.push_back(readData);
            break;
      }
      lineNo++;

      IWatchdog.reload();
    }
  }
}
// Default GPS manager on RX1/TX1
GPS_Manager gps_manager(GPS_RX_PIN, GPS_TX_PIN);