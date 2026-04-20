#include "thermo_manager.h"

//--------------------------------------------------------------------------------
// Library largely ported from OMB with minor modifications

void address_to_uint64_t(Address &addr_in, uint64_t &uint64_result) {
  uint64_result = 0;
  for (int crrt_byte_index = 0; crrt_byte_index < 8; crrt_byte_index++) {
    uint64_result += static_cast<uint64_t>(addr_in[crrt_byte_index])
                     << (8 * (7 - crrt_byte_index));
  }
}

void uint64_t_to_address(uint64_t const &uint64_in, Address &addr_out) {
  for (int crrt_byte_index = 0; crrt_byte_index < 8; crrt_byte_index++) {
    addr_out[crrt_byte_index] =
        static_cast<byte>(uint64_in >> (8 * (7 - crrt_byte_index)));
  }
}

void print_uint64(uint64_t const &num) {
  uint32_t low = num % 0xFFFFFFFF;
  uint32_t high = (num >> 32) % 0xFFFFFFFF;

  Serial.print(high);
  Serial.print(low);
}

void print_address(Address const &addr) {
  for (int i = 0; i < 8; i++) {
    Serial.print(addr[i], HEX);
    Serial.write(' ');
  }
}

void uint64_t_to_6bits_int_id(uint64_t const &uint64_in,
                              uint8_t &uint8_6bits_id) {
  Address crrt_address;
  uint64_t_to_address(uint64_in, crrt_address);

  // we know that:
  // the first byte is the "kind", should be 0x28
  // the next 6 bytes are the serial number
  // the LS bytes are first; then end is still 0x00 0x00 (as production has not
  // gone so far) the last byte is the CRC so, use the 6 LSBs of the LS byte of
  // the serial number (these are the ones that "rotate" fastest)

  byte byte_to_use = crrt_address[1];
  byte byte_result = byte_to_use & 0b00111111;
  uint8_6bits_id = byte_result;
}

// function to order thermistors by 6 lower bits
// need to be equivalent to >, NOT to >=, see C++ standard
// https://en.cppreference.com/w/cpp/utility/functional/greater
bool greater_6bits_id(uint64_t const &id_1, uint64_t const &id_2) {
  uint8_t id6_1;
  uint8_t id6_2;
  uint64_t_to_6bits_int_id(id_1, id6_1);
  uint64_t_to_6bits_int_id(id_2, id6_2);
  return id6_1 > id6_2;
}

void Thermo_Manager::get_thermometre_identifications(void){
  if (debug_serial)  
    Serial.println(F("get list of ordered thermistors IDs"));

  // Clear previous buffer
  thermometres.clear();

  // get all IDs
  Address crrt_addr;
  uint64_t crrt_id;
  
  while (true){
    IWatchdog.reload();
      
    if ( !TWire.search(crrt_addr) ) {
      if (debug_serial)
        Serial.println("No more addresses.");
      TWire.reset_search();
      delay(250);
  
      break;
    }
    if (debug_serial){
      Serial.println(F("found new address..."));
      Serial.print("ROM =");
      print_address(crrt_addr);
      Serial.println();
    }

    address_to_uint64_t(crrt_addr, crrt_id);
    if (debug_serial)
      Serial.print(F("ID = ")); print_uint64(crrt_id); Serial.println();
    uint8_t crrt_6_bits_id {0};
    uint64_t_to_6bits_int_id(crrt_id, crrt_6_bits_id);
    if (debug_serial)
      Serial.print(F("6 bits ID = ")); Serial.print(crrt_6_bits_id); Serial.println();
  
    if (OneWire::crc8(crrt_addr, 7) != crrt_addr[7]) {
      if (debug_serial)
        Serial.println("WARNING: CRC is not valid!");
      continue;
    } else if (debug_serial) {
      Serial.println(F("CRC is valid"));
    }
  
    // the first ROM byte indicates which chip
    switch (crrt_addr[0]) {
        case 0x10:
        if (debug_serial)
          Serial.println("  WARNING: Chip = DS18S20");  // or old DS1820
        break;
      case 0x28:
        if (debug_serial)
          Serial.println("  CORRECT: Chip = DS18B20");
        thermometres.push_back(crrt_id);
        break;
      case 0x22:
        if (debug_serial)
          Serial.println("  WARNING: Chip = DS1822");
        break;
      default:
        if (debug_serial)
          Serial.println("ERROR: Device is not a DS18x20 family device.");
        continue;
    } 
  }
  
  // we sort greater first; i.e., the sensors with greater IDs are sorted first;
  // i.e., if the thermistor string have greater IDs higher up, then the sensors are sorted from higher up to lower down
  // we actually need to order by reduced ID, as these are the ones we are transmitting...
  // etl::sort(vector_of_ids.begin(), vector_of_ids.end(), std::greater<uint64_t>());  // order by normal ID; not what we need
  etl::sort(thermometres.begin(), thermometres.end(), greater_6bits_id);
  numSensors = thermometres.size();
  if (debug_serial){
    Serial.println(F("sorted list of IDs"));

    for (auto const & crrt_elem : thermometres){
      print_uint64(crrt_elem); Serial.println();
      uint64_t_to_address(crrt_elem, crrt_addr);
      print_address(crrt_addr); Serial.println();
    }
  }

  return;
}

void Thermo_Manager::request_start_thermistors_conversions(void) {
  IWatchdog.reload();
  // Check if millis is correct. 
  readingStartTime = millis();

  Address crrt_address;

  // ask each sensor to start new measurement
  if (debug_serial)
    Serial.println(F("ask to start conversion..."));
  for (auto &crrt_id : thermometres) {
    uint64_t_to_address(crrt_id, crrt_address);
    if(debug_serial){
      print_address(crrt_address);
      Serial.println();
    }

    TWire.reset();
    TWire.select(crrt_address);
    TWire.write(0x44, 1); // start conversion, with parasite power on at the end
  }

  return;
}

uint32_t Thermo_Manager::remaining_conversion_time(void){
    if (millis() -  readingStartTime < thermometre_pause_between_readings) {
        uint32_t result = thermometre_pause_between_readings - (millis() - readingStartTime);
        if (debug_serial)
          Serial.print(F("conversion not over yet; there are ")); Serial.print(result); Serial.println(F(" ms left"));
        return result;
    }
    else{
        return 0UL;
    }
}

uint8_t Thermo_Manager::collect_thermistors_conversions(time_t timestamp) {
  IWatchdog.reload();
  Address crrt_address;
  byte present = 0;
  byte data[12];
  float celsius;
  byte crc;

  // wait until conversion is ready
  delay(remaining_conversion_time());
  

  // collect the output of each sensor
  if (debug_serial)
    Serial.println(F("collect results..."));
  
  int j_ = 0;
  for (auto &crrt_id : thermometres) {
    uint64_t_to_address(crrt_id, crrt_address);
    if (debug_serial){
      print_address(crrt_address);
      Serial.println();
    }
    uint8_t crrt_6bits_id {0};
    uint64_t_to_6bits_int_id(crrt_id, crrt_6bits_id);
    if (debug_serial){
      Serial.print(crrt_6bits_id);
      Serial.println();
    }

    present = TWire.reset();
    TWire.select(crrt_address);
    TWire.write(0xBE); // Read Scratchpad

    for (int i = 0; i < 9; i++) { // we need 9 bytes
      data[i] = TWire.read();
    }

    crc = OneWire::crc8(data, 8);

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    byte cfg = (data[4] & 0x60);

    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) {
      raw = raw & ~7;
      if (debug_serial)
        Serial.println(F("  WARNING: 9 bit res"));
    } // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) {
      raw = raw & ~3;
      if (debug_serial)
        Serial.println(F("  WARNING: 10 bit res"));
    } // 10 bit res, 187.5 ms
    else if (cfg == 0x40) {
      raw = raw & ~1;
      if (debug_serial)
        Serial.println(F("  WARNING: 11 bit res"));
    } // 11 bit res, 375 ms
    else if (cfg == 0x60) {
      if (debug_serial)
        Serial.println(F("  OK: 12 bits resolution"));
    } else {
      if (debug_serial)
        Serial.println(F("  ERROR: unknown resolution config!"));
    }

    celsius = (float)raw / 16.0;
    if (debug_serial){
      Serial.print("  Temperature = ");
      Serial.print(celsius);
      Serial.print(" Celsius");
      Serial.println();
    }
    if (celsius < 84){
      reading.temps[j_] = (int32_t) scale_factor*celsius;
    } else {
      return 1;
    }
    j_++;
  }

  numPacketReadings++;
  reading.timestamp = timestamp;

  if (packet.full()){
    packet.pop_front();
  }

  packet.push_back(reading);
  IWatchdog.reload();
  return 0;
}


void Thermo_Manager::begin(uint16_t dataPin, uint16_t powerPin){
  /*
    Start a thermometre at digital pin Dpin. 
    Read the amount of sensors on the pin. 
    Get the address and index of each thermometre
    Create buoy id from the address of thermometre 0
    Also wakes up the thermometre.
  */
  dPin = dataPin;
  pPin = powerPin;
  TWire.begin(dataPin);
  wake();
  get_thermometre_identifications();
  delay(100);
}

void Thermo_Manager::wake(void){
  pinMode(pPin, OUTPUT);
  digitalWrite(pPin, HIGH);
  delay(100);
}

void Thermo_Manager::sleep(void){
  //WIRE PIN INPUT
  pinMode(pPin, INPUT);
  delay(100);
}


uint8_t Thermo_Manager::takeReadings(uint32_t maxReadTime, time_t timestamp, bool logAllReadings){
  /*
    Public method which constructs a data packet of temperature readings
    up until either the cap readings_per_measurement
    or until the time runs out.
    If logReading is set to true, we dump each reading to te current active logfile
  */
  uint32_t start = millis();
  packet.clear();

  while ((numPacketReadings < readings_per_measurement) && (millis() - start < maxReadTime)){
    request_start_thermistors_conversions();
    delay(400);
    collect_thermistors_conversions(timestamp);

    if (logAllReadings){
      temperatureReading latest = packet.back();
      logReading(latest);
    }
  }

  return 0;
}

void Thermo_Manager::processReadings(){
  /*
    For each thermometre:
      - Create an array with all temperature measurements. 
      - filter the vector (either take the average, or take the ¨
        average of all nonextreme values). 
    Then:
      Create a temperatureReading instance for the filtered
      value of each thermometre
    Then: 
      Append filtered reacing to temperatures deque
    Then:
      Flush packet array for next reading
  */

  // Vectors for post processing each thermometre individually
  etl::vector<int32_t, readings_per_measurement> temp_array;
  
  uint8_t oldSize = packet.size();
  uint8_t newSize = oldSize;
  
  // Average each thermometre individually, 
  // then possibly remove extreme values from average
  for (int j = 0; j < thermometres.size(); j++){
    for (int i = 0; i < packet.size(); i++){
      temp_array.push_back(packet[i].temps[j]);
    }
    int32_t mean_value = filter_vector(temp_array);

    reading.temps[j] = mean_value;
    temp_array.clear();
  }

  reading.timestamp = packet.front().timestamp;

  if (temperatures.size() == max_number_of_measurements){
    temperatures.pop_front();
  }
  if (debug_serial){
    Serial.print("Temperatures T:");
    for (int i = 0; i < numSensors; i++){
      Serial.print(reading.temps[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
  // Minus one to get ID to match logFile number
  reading.readingID = sd_writer.logCount - 1;
  sd_writer.logString("Filtered value");
  logReading(reading);
  temperatures.push_back(reading);
  cleanUp();
}

void Thermo_Manager::cleanUp(){
  /*
    Reset temporary variables to prepare
    for next iteration
  */
  if (debug_serial){
    Serial.println(F("Cleaning packet array"));
    delay(50);
  }
  packet.clear();

  numPacketReadings = 0;
  if (debug_serial){
    Serial.println(F("Cleaned"));
    delay(50);
  }
}

size_t Thermo_Manager::updateTransmitMessage(void){
  /*
    Fetches front of temperatures array and 
    formats it for transmission. It then removes the
    data from the temperatures array

  */

  temperatureReading reading = temperatures.front();

  // Create message
  uint8_t offset = 0;
  msgB[offset++] = 'T';
  msg_insert_uint(msgB, reading.readingID, offset, thermo_message_size, offset, true);
  msgB[offset++] = numSensors;
  
  for (uint8_t sensor = 0; sensor < numSensors; sensor++){
    msg_insert_int(msgB, reading.temps[sensor], offset, thermo_message_size, offset, true);
  }

  msg_insert_uint(msgB, reading.timestamp, offset, thermo_message_size, offset, true);
  msgB[offset++] = 'E';
  
  // Remove reading from queue and return message size
  temperatures.pop_front();
  return offset;
}

uint8_t Thermo_Manager::logReading(temperatureReading tempData){
  /*
    Format of log string
    TR[10t][10x]*numsensorsEnd

  */
  uint8_t offset = 0;
  byte reading[2 + sizeof(tempData.readingID) + sizeof(tempData.timestamp) + max_number_of_thermometres*(1+sizeof(tempData.temps[0]))+ 1]; 
  size_t msg_size = sizeof(reading);

  reading[offset++] = 'T';
  reading[offset++] = 'R';
  msg_insert_uint(reading, tempData.readingID, offset, msg_size, offset, true);
  msg_insert_uint(reading, tempData.timestamp, offset, msg_size, offset, true);
  
  for (int i = 0; i < numSensors; i++){
    msg_insert_int(reading, tempData.temps[i], offset, msg_size, offset, true);
  }
  reading[offset++] = 'E';
  uint8_t state = sd_writer.logByteArray(reading, msg_size);
  return state;
}

void Thermo_Manager::getMeasurementFromFile(void){
  temperatureReading readData;
  if (sd_writer.numLines > 0){
    uint8_t lineNo = 1;
    fileLine currentLine = {"",0}, prevLine = {"",0};
    while(lineNo < sd_writer.file_buffer.size()){
      
      prevLine = currentLine;
      currentLine = sd_writer.file_buffer.at(lineNo);

      // We check for the filtered value 
      if ((strncmp(prevLine.line, "Filtered:", 4) == 0) \
          && (strncmp(currentLine.line, "84b", 3) == 0)){
            //We first do a bytewise extraction
            char tmpHolder[max_line_length];
            byte filteredData[thermo_message_size];
            sscanf(currentLine.line, "%s", tmpHolder);
            
            for (uint8_t index = 0; index < thermo_message_size; index++){
              sscanf(tmpHolder, "%db%s", &filteredData[index], tmpHolder);
            }
            
            readData.readingID = msg_extract_uint<uint16_t>(filteredData, 2, true);
            readData.timestamp = msg_extract_uint<uint64_t>(filteredData, 2 + sizeof(readData.readingID), true);
            
            for (uint8_t thermistor_idx = 0; thermistor_idx < numSensors; thermistor_idx++){
              // If the sign character is a P, assign a positive number, otherwise assign a negative number
              uint8_t sign_index = 2 + sizeof(readData.readingID) + sizeof(readData.timestamp) + (1 + sizeof(readData.temps[0]))*thermistor_idx;
              byte sign = filteredData[sign_index];
              uint32_t extracted_temp =  msg_extract_uint<uint32_t>(filteredData, sign_index + 1, true);
              readData.temps[thermistor_idx] = (sign == 80) ? extracted_temp : - extracted_temp; 
            }

            if (debug_serial){
              Serial.println(lineNo);
              Serial.println(sd_writer.file_buffer[lineNo].line);
              Serial.println("Recovered data:\n---------------\n");
              Serial.print("ID = ");
              Serial.println(readData.readingID);
              for (uint8_t i = 0; i < numSensors; i++){
                Serial.print("Temp[");
                Serial.print(i);
                Serial.print("] = ");
                Serial.println(readData.temps[i]);
              }
              Serial.print("t = ");
              Serial.println(readData.timestamp);
              Serial.println("---------------\n");
              delay(200);
            }
            
            // New measurements take precedence in memory over old measurements
            if (temperatures.full()){
              temperatures.pop_front();
            }
            temperatures.push_back(readData);
            break;
      }
      lineNo++;
      IWatchdog.reload();
    }
  }
}

Thermo_Manager thermo_manager;