#include "lora_transceiver.h"


LoRa_Transceiver LORA;

void setFlag(void){
  // Function called by RadioLib to mark completed operation
  operationDone = true;
}


// Read Buoy ID from wio device
void LoRa_Transceiver::getWiOID(void){
  // Only UID0 seems to be necessary 
  WiO_ID = HAL_GetUIDw0();
  if (debug_serial){
    Serial.print("Buoy ID: ");
    Serial.print(WiO_ID);
  }
}


void LoRa_Transceiver::computeReceptionChannel(uint8_t num_LoRa_channels, double fmin, double fmax){
  if (LoRa_freq_receive < 0){
    if (WiO_ID ==  0)
      getWiOID(); //In case we forgot to aquire ID
    
    // We use the modulo operator to get a "random" channel number
    uint8_t channelNo = WiO_ID % num_LoRa_channels;

    // We partition the domain into num_LoRa_channels equidistant channels, and choose the appropriate one
    LoRa_freq_receive = fmin + (fmax - fmin)*( (float) channelNo)/num_LoRa_channels;
  }
}


void LoRa_Transceiver::beginRadio(double freq, double bw, int sf, int cr, int power){
  /*
    Method to be called once, here we set the transmission parameters
    and set the appropriate flags. If you need to restart the radio
    use the changeFrequency method.
  */
  radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);
  state = radio.begin(freq, bw, sf, cr, (0x12),power, 8,1.7, false);
  radio.setTCXO(1.7);
  radio.setDio1Action(setFlag);
  packet_count = 0;
  listening = false;
  delay(200);
  IWatchdog.reload();
}


/*
  Three message sending method
  the two former are for sending strings
  The latter for sending byte arrays.
  We recommend transmitting byte arrays 
  for efficiency
*/

void LoRa_Transceiver::transmit(String msg){
  transmit(msg.c_str());
}

void LoRa_Transceiver::transmit(const char * msg){
  if (debug_serial){
    Serial.println(msg);
  }
  radio.startTransmit(msg);
}


void LoRa_Transceiver::transmitB(byte * msg, uint8_t msgSize){
  radio.startTransmit(msg, msgSize);
}


float LoRa_Transceiver::getRSSI()
{
  return radio.getRSSI();
}

// Method called after the end of transmission to send the updated parameters
void LoRa_Transceiver::sendFinalMessage(FrequencyMessage frequency_message)
{
  create_update_message(frequency_message);
  sd_writer.debugSerialPrint("Sending end message ");
  sd_writer.debugByteArray(end_message, end_message_size);
  sd_writer.debugSerialPrintln("");

  changeFrequency(send_frequency);
  radio.startTransmit(end_message, end_message_size);
  msgCounter++;
}


void LoRa_Transceiver::changeFrequency(double f){
  /*
    This method restarts the radio with a new given frequency. 
  */
  state = radio.begin(f, LoRa_bw, LoRa_sf, LoRa_cr, (0x12), LoRa_power, 8,1.7, false);
  radio.setTCXO(1.7);
  radio.setDio1Action(setFlag);
  listening = false;
  delay(200);
  IWatchdog.reload();

   if (debug_serial){
    Serial.print("Radio restarted at frequency: ");
    Serial.println(f);
  }
}

void LoRa_Transceiver::setDefaultSendFrequency(double freq){
  send_frequency = freq;
}

void LoRa_Transceiver::setBaseStationID(uint8_t id){
  baseStationID = id;
}

void LoRa_Transceiver::listenByteArray(uint32_t max_wait_time)
{
  /*
    Start listening for message
    Wait up to max_wait_time (in ms) for a message
    return the message if successful
  */
  if (operationDone){
    operationDone = false;
  }
  
  if (!listening)
  {
    radio.startReceive();
    listening = true;
  }
  uint32_t listenTime = millis();
  while (!operationDone && (millis() - listenTime) < max_wait_time)
  {
    delay(100);
    if ((millis() - listenTime) % 10000 > 9500)
    {
      IWatchdog.reload();
    }
  }
  if (operationDone)
  {
    operationDone = false;
    // TODO: this might only work for temp
    int numBytes = radio.getPacketLength();
    state = radio.readData(byte_msg.byteMsg, numBytes);
    if (state == RADIOLIB_ERR_NONE)
    {
      byte_msg.success = true;
      byte_msg.numBytes = numBytes;
    }
    else
    {
      byte_msg.success = false;
      byte_msg.numBytes = 0;
    }
  }
  else
  {
    byte_msg.success = false;
    byte_msg.numBytes = 0;
  }

  if (debug_serial)
  {
    Serial.println("Stopped listening");
  }
}


void LoRa_Transceiver::listen(uint32_t max_wait_time){
  /*
    Start listening for message 
    Wait up to max_wait_time for a message
    return the message if succesful
  */

  if (operationDone)
  {
    operationDone = false;
  }


  if (!listening){
    radio.startReceive();
    listening = true;
  }
  uint32_t listenTime = millis();
  while (!operationDone && millis() - listenTime < max_wait_time){
    delay(100);
    if ((millis() - listenTime) % 10000 > 9500)
    {
      IWatchdog.reload();
    }
  }

  if (operationDone){
    operationDone = false;
    state = radio.readData(string_msg.msg);
    if (state == RADIOLIB_ERR_NONE){
      string_msg.success = true;
    } else{
      string_msg.success = false;
    }
  } else {
    string_msg.success = false;
  }

  if (debug_serial)
  {
    Serial.println("Stopped listening");
  }
}

void LoRa_Transceiver::findBaseStation(uint32_t max_wait_time){
  /*
    We look for a base station for a set amount of time
    Returning base station ID
  */
  changeFrequency(LoRa_freq_receive);
  uint32_t wait_time_start = millis();
  while ((baseStationID < 1) && millis() - wait_time_start < max_wait_time){
    listenByteArray(max_wait_time + wait_time_start - millis());
    char msg1 = (char) byte_msg.byteMsg[0];
    char msg2 = (char) byte_msg.byteMsg[1];
    char msg3 = (char) byte_msg.byteMsg[2];
    IWatchdog.reload();

    /*
      First message is 7 bytes. 
      1 B
      1 ID
      4 transmission frequency 
      1 E
    */
    if (byte_msg.byteMsg[0] == 'B' && byte_msg.numBytes == 7 && byte_msg.byteMsg[6] == 'E'){
      baseStationID = byte_msg.byteMsg[1];
      send_frequency = (float) msg_extract_uint<uint32_t>(byte_msg.byteMsg, 2, true);
      
      //We scale it down to the correct value
      send_frequency /= scale_factor;
    } else {
      baseStationID = 0;
    }
  }

  if (debug_serial){
    Serial.print("Base station ID now: ");
    Serial.println(baseStationID);
    delay(100);
  }
  if (operationDone){
    operationDone = false;
  }
}


bool LoRa_Transceiver::handshake(uint32_t max_wait_time){
  /* 
    Handshake method for greenlighting communication between buoy and base station
    We have gotten a signal from a base station, and now we transmit back
    A buoy ID and the base station ID to the base station at the base station's specified 
    reception frequency. 
    If we get a confirmation back from the BST, we flag the handshake as a success 
    and commence data transmission.
  */
  
  changeFrequency(send_frequency);


  // Create handshake message
  byte handshake[2 + sizeof(baseStationID) + sizeof(WiO_ID)];
  uint8_t offset = 0;
  handshake[offset++] = 'U';
  msg_insert_uint(handshake, WiO_ID, offset, sizeof(handshake), offset, true);
  handshake[offset++] = baseStationID;
  handshake[offset++] = 'E';

  // Transmit handshake message and wait for confirmation
  radio.startTransmit(handshake, sizeof(handshake));
  if (debug_serial){
    Serial.println("Handshake sent to base station!");
  }
  waitUntilReady();
  
  changeFrequency(LoRa_freq_receive);

  // Listen for confirmation signal.
  uint32_t final_listen = millis();

  
  while (!available && (millis() - final_listen < max_wait_time)){
    listenByteArray(max_wait_time);
    if (byte_msg.success){
      if (debug_serial){
        Serial.println("Message received!");
        Serial.print("Message is: ");
        Serial.print(byte_msg.numBytes);
        Serial.println(" bytes large!");
      }
      IWatchdog.reload();
      // Last format B + 4*id + BST_ID + 4*listenTime + E
      if (byte_msg.byteMsg[0] == 'B' && byte_msg.numBytes == 3 + 2*sizeof(uint32_t)){
        uint32_t WiO_ID_received = msg_extract_uint<uint32_t>(byte_msg.byteMsg, 1,true);
        bool matching_Bid = (baseStationID == byte_msg.byteMsg[1 + sizeof(WiO_ID)]);
        bool matching_Uid = (WiO_ID == WiO_ID_received); 
        if (matching_Bid && matching_Uid){
  
          if (debug_serial){
            Serial.println("Base station found!");
          }

          listenTime = msg_extract_uint<uint32_t>(byte_msg.byteMsg, 1 + sizeof(WiO_ID) + sizeof(baseStationID),true);
          if (debug_serial){
            Serial.print("Listen time: ");
            Serial.println(listenTime);
          }
          available = true;
          msgCounter++;
        }
      }
    }
    
    IWatchdog.reload();
  }
  changeFrequency(send_frequency);
  operationDone = false;
  return available;
}


Message_Data LoRa_Transceiver::sendData(byte * message, uint8_t msgSize,  uint32_t message_send_time){
  /*
    Method which sends a message byte array with size msgSize
    and update the surrounding metadata like remaining listenTime
    and signal strength of the message. 
  */
  uint32_t start_send = millis();
  transmitB(message, msgSize);
  Message_Data message_data;
  while (!operationDone && (millis() - start_send) < message_send_time){
    delay(50);
    IWatchdog.reload();
  }
  if (operationDone){
    operationDone = false;
    message_data.RSSI = (uint32_t) 1e4*radio.getRSSI();
    message_data.SNR  = (uint32_t) 1e4*radio.getSNR();
  }
  listenTime -= millis() - start_send;
  return message_data;
}

void LoRa_Transceiver::transmitFinished(uint8_t packetsLeft){
  /*
    Send done signal to base station, switch to listen mode for set time period 
    for additional instructions. Otherwise switch to default or adaptive transmission frequency
    Format for end message: 
    EMxyzzzzeE
    Here, capital letters indicate hard coded letters
    x is the amount of messages sent
    y is the amount of packets left
    zzzz is the amount of time the buoy will await instructions from the base station
    e indicates that the buoy has crashed since last transmission
  */

  byte endmessage[10];
  uint8_t offset=0;
  endmessage[offset++] = 'E';
  endmessage[offset++] = 'M';
  endmessage[offset++] = msgCounter;
  endmessage[offset++] = packetsLeft;

  uint32_t wait_time = max_radio_wait_time;
  msg_insert_uint(endmessage, max_radio_wait_time, offset, 10, offset, true);

  IWatchdog.reload();
  if (IWatchdog.isReset(true)){
    endmessage[offset++] = 1;
  } else {
    endmessage[offset++] = 0;
  }
  endmessage[offset++] = 'E';

  transmitB(endmessage, 10);
  baseStationID = 0;
  available = false;
  IWatchdog.reload();
}

void LoRa_Transceiver::receiveDesiredMeasurementIDs(void){
  /*
    We wait for a dismissed signal from the base station,
    which might update measurement frequency and transmission frequency.
    Format of update string:
    B
    mmmm - base measurement period
    fa   - adaptive measurement period
    llll - Target distance
    uuuu - Velocity treshold
    E
    Total Length 
  */
  waitUntilReady();
  changeFrequency(LoRa_freq_receive);
  uint32_t start_listen_time = millis();
  bool cleared = false;
  while (!cleared && (millis() - start_listen_time < max_radio_wait_time)){
    listenByteArray(max_radio_wait_time + start_listen_time - millis());
    char msg1 = byte_msg.byteMsg[0];
    char msg2 = byte_msg.byteMsg[1];
    IWatchdog.reload();

    if ((msg1 == 'B') && (byte_msg.numBytes == 15)){
      cleared = true;
      base_measurement_period = msg_extract_uint<uint32_t>(byte_msg.byteMsg,1,true);
      IWatchdog.reload();

      if (byte_msg.byteMsg[5] == 1){
        enable_motion_detection = true;
        target_reading_distance = msg_extract_uint<uint32_t>(byte_msg.byteMsg,6,true);
        motion_treshold       = msg_extract_uint<uint32_t>(byte_msg.byteMsg, 10, true);
      } else {
        enable_motion_detection = false;
      }

    }
  }
}
void LoRa_Transceiver::receiveInstructions(void){
  /*
    We wait for a dismissed signal from the base station,
    which might update measurement frequency and transmission frequency.
    Format of update string:
    B
    mmmm - base measurement period
    fa   - adaptive measurement period
    llll - Target distance
    uuuu - Velocity treshold
    E
    Total Length 
  */
  waitUntilReady();
  changeFrequency(LoRa_freq_receive);
  uint32_t start_listen_time = millis();
  bool cleared = false;
  while (!cleared && (millis() - start_listen_time < max_radio_wait_time)){
    listenByteArray(max_radio_wait_time + start_listen_time - millis());
    char msg1 = byte_msg.byteMsg[0];
    char msg2 = byte_msg.byteMsg[1];
    IWatchdog.reload();
    if ((msg1 == 'B') && (byte_msg.numBytes == 15)){
      cleared = true;
      base_measurement_period = msg_extract_uint<uint32_t>(byte_msg.byteMsg,1,true);
      IWatchdog.reload();

      if (byte_msg.byteMsg[5] == 1){
        enable_motion_detection = true;
        target_reading_distance = msg_extract_uint<uint32_t>(byte_msg.byteMsg,6,true);
        motion_treshold       = msg_extract_uint<uint32_t>(byte_msg.byteMsg, 10, true);
      } else {
        enable_motion_detection = false;
      }

    }
  }
}

void LoRa_Transceiver::updateMeasurementFrequency(uint32_t velocity, uint32_t max_period, uint32_t min_period){
  /*
    Method for updating the measurement frequency. Checks if motion detection is on, otherwise uses
    default measurement frequency. Formula for adaptive measurement frequency is
      mf = distance/velocity. 
    We divide by 100 times velocity as velocity is given as 10000 times the double value,
    and time unit is milliseconds. Hence, 1e-3*1e4 = 1e2
  */
  float ms_2_s {1e-3};
  if (enable_motion_detection && (velocity > scale_factor*motion_treshold)){
    measurement_period = min((uint32_t) (target_reading_distance*scale_factor/(ms_2_s*velocity)), max_period);
    measurement_period = max(measurement_period, min_period);
  } else {
    measurement_period = base_measurement_period;
  }
  IWatchdog.reload();
}

void LoRa_Transceiver::waitUntilReady(){
  /*
    Method which lets the radio wait until a message is successfully sent
  */
  uint32_t start_wait = millis();
  if (debug_serial){
    Serial.println("Waiting for successful sending");
  }
  while (!operationDone){
    delay(100);
    IWatchdog.reload();
  }
  operationDone = false;
}

/*
  Sleep mode disables the radio to save power. 
  wakeUp needs to be called to get the radio out of sleep mode
*/
void LoRa_Transceiver::sleep(void){
  if (!sleeping){
    radio.sleep();
    sleeping = true;
  }
}

void LoRa_Transceiver::wakeUp(void){
  if (sleeping){
    radio.standby();
    sleeping = false;
  }
}

void LoRa_Transceiver::transmitBeaconMessage(byte * beaconMsg, uint8_t msgSize){
 /*
  Send the last measured position every five minutes at the Beacon frequency 
  fB
 */ 
  changeFrequency(LoRa_freq_beacon);
  for (uint8_t i = 0; i < 5; i++){
    transmitB(beaconMsg, msgSize);
    waitUntilReady();
    delay(500);
  }
}

/*
Todo: Use parser utils
*/
void LoRa_Transceiver::create_update_message(FrequencyMessage frequency_message)
{
  uint8_t offset = 0;
  end_message[offset++] = 'B';
  end_message[offset++] = frequency_message.update_frequency;
  msg_insert_uint<uint32_t>(end_message, frequency_message.measurement_frequency, offset, end_message_size, offset);
  end_message[offset++] = frequency_message.adaptive_frequency;
  msg_insert_uint<uint32_t>(end_message, frequency_message.measurement_frequency, offset, end_message_size, offset);
  msg_insert_uint<uint32_t>(end_message, frequency_message.target_length, offset, end_message_size, offset);
  msg_insert_uint<uint32_t>(end_message, frequency_message.threshold_velocity, offset, end_message_size, offset);
  end_message[offset++] = 'E';
}

//BST Utils

buoyInfo LoRa_Transceiver::initEmptyBuoy()
{
  buoyInfo buoy;
  buoy.ID = 0;
  buoy.inrange = false;
  return buoy;
}



buoyInfo LoRa_Transceiver::findBuoy(uint32_t max_wait_time)
{
  /*
    We look for a base station for a set amount of time
    Returning base station ID
  */

  buoy = initEmptyBuoy();

  // Set radio in send mode
  changeFrequency(send_frequency);

  // Create transmit message
  byte baseStationIDMsg[7];
  uint8_t offset = 0;
  baseStationIDMsg[offset++] = 'B';
  baseStationIDMsg[offset++] = baseStationID;
  uint32_t freq = (uint32_t) (LoRa_freq_receive*scale_factor);
  msg_insert_uint(baseStationIDMsg, freq, offset, 7, offset, true);
  baseStationIDMsg[offset++] = 'E';

  // Transmit message and wait until ready
  IWatchdog.reload();
  transmitB(baseStationIDMsg, sizeof(baseStationIDMsg));
  IWatchdog.reload();
  waitUntilReady();

  // Start radio in listen mode and wait for response
  uint32_t startLooking = millis();
  LoRa_freq_receive = (float) ((float) freq/ (float) scale_factor);

  changeFrequency(LoRa_freq_receive);
  // listen(max_wait_time);
  IWatchdog.reload();
  listenByteArray(max_wait_time);
  buoyInitMessage buoyIDMessage;
  if (byte_msg.success)
  {
    buoyIDMessage = MESSAGE_PARSER.parse_buoy_init_message(byte_msg.byteMsg);

    sd_writer.debugSerialPrint("Bouy ID : ");
    sd_writer.debugSerialPrint(buoyIDMessage.buoy_id);
    sd_writer.debugSerialPrint(", ");


    sd_writer.debugSerialPrint("Base station ID : ");
    sd_writer.debugSerialPrint(buoyIDMessage.base_station_ID);
    sd_writer.debugSerialPrintln("");
  }

  delay(100);

  if (baseStationID == buoyIDMessage.base_station_ID)
  {
    buoy.inrange = true;
  }

  // If no response, exit
  if (!buoy.inrange)
  {
    sd_writer.debugSerialPrintln("No buoy found");
    return buoy;
  }
    
  IWatchdog.reload();
  buoy.ID = buoyIDMessage.buoy_id;
  // Else, send go ahead signal to buoy before switching back to receive mode
  changeFrequency(send_frequency);

  // Create message
  byte final_handshake_msg[1 + sizeof(buoy.ID) + sizeof(baseStationID) + sizeof(listenTime) + 1];
  size_t msgSize = sizeof(final_handshake_msg);
  offset = 0;
  final_handshake_msg[offset++] = 'B';
  msg_insert_uint(final_handshake_msg, buoy.ID, offset, msgSize, offset, true);
  final_handshake_msg[offset++] = baseStationID;
  msg_insert_uint(final_handshake_msg, listenTime, offset, msgSize, offset, true);
  final_handshake_msg[offset++] = 'E';

  sd_writer.debugSerialPrintln("sending last message now");


  for (uint8_t repetitions = 0; repetitions < 3; repetitions++){
    transmitB(final_handshake_msg, sizeof(final_handshake_msg));
    delay(300);
    waitUntilReady();
  }
  
  changeFrequency(LoRa_freq_receive);
  delay(100);
  return buoy;
}