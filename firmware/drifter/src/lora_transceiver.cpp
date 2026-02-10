#include "lora_transceiver.h"


LoRa_Transceiver LORA;

void setFlag(void){
  // Function called by RadioLib to mark completed operation
  operationDone = true;
}


// Read Buoy ID from wio device
void LoRa_Transceiver::getBuoyID(void){
  // Only UID0 seems to be necessary 
  buoy_ID = HAL_GetUIDw0();
  if (debug_serial){
    Serial.print("Buoy ID: ");
    Serial.print(buoy_ID);
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



void LoRa_Transceiver::changeFrequency(double f){
  /*
    This method restarts the radio with a new given frequency. 
    This is done as successive calls to 
  */
  state = radio.begin(f, LoRa_bw, LoRa_sf, LoRa_cr, (0x12), LoRa_power, 8,1.7, false);
  radio.setTCXO(1.7);
  radio.setDio1Action(setFlag);
  listening = false;
  delay(200);
  IWatchdog.reload();
}

Message LoRa_Transceiver::listen(uint32_t max_wait_time){
  /*
    Start listening for message 
    Wait up to max_wait_time for a message
    return the message if succesful
  */
  if (!listening){
    radio.startReceive();
    listening = true;
  }
  uint32_t listenTime = millis();
  while (!operationDone && millis() - listenTime < max_wait_time){
    delay(100);
    IWatchdog.reload();
  }
  if (operationDone){
    operationDone = false;
    receivedMessage.msgSize = radio.getPacketLength();
    state = radio.readData(receivedMessage.msg, receivedMessage.msgSize);
    if (state == RADIOLIB_ERR_NONE){
      receivedMessage.success = true;
    } else{
      receivedMessage.success = false;
    }
  } else {
    receivedMessage.msgSize = 0;
    receivedMessage.success = false;
  }
  return receivedMessage;
}

void LoRa_Transceiver::findBaseStation(uint32_t max_wait_time){
  /*
    We look for a base station for a set amount of time
    Returning base station ID
  */
  changeFrequency(LoRa_freq_receive);
  uint32_t wait_time_start = millis();
  while ((baseStationID < 1) && millis() - wait_time_start < max_wait_time){
    Message msg = listen(max_wait_time + wait_time_start - millis());
    char msg1 = (char) msg.msg[0];
    char msg2 = (char) msg.msg[1];
    char msg3 = (char) msg.msg[2];
    IWatchdog.reload();

    /*
      First message is 7 bytes. 
      1 B
      1 ID
      4 transmission frequency 
      1 E
    */
    if (msg.msg[0] == 'B' && msg.msgSize == 7 && msg.msg[6] == 'E'){
      baseStationID = msg.msg[1];
      LoRa_freq_send = (float) msg_extract_uint<uint32_t>(msg.msg, 2, true);
      
      //We scale it down to the correct value
      LoRa_freq_send /= scale_factor;
    } else {
      baseStationID = 0;
    }
  }

  if (debug_serial){
    Serial.print("Base station ID now: ");
    Serial.println(baseStationID);
    delay(200);
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
  
  changeFrequency(LoRa_freq_send);
  byte handshake[2 + sizeof(baseStationID) + sizeof(uint32_t)];
  handshake[0] = 'U';
  msg_insert_uint(handshake, buoy_ID, 1, 2 + sizeof(baseStationID) + sizeof(buoy_ID), true);
  handshake[sizeof(handshake) - 2] = baseStationID;
  handshake[sizeof(handshake) - 1] = 'E';
  radio.startTransmit(handshake, sizeof(handshake));
  sd_writer.logString("Handshake sent!\n");
  sd_writer.logByteArray(handshake, 7);
  if (debug_serial){
    Serial.println("Handshake sent to base station!");
  }
  waitUntilReady();
  
  changeFrequency(LoRa_freq_receive);

  // Listen for confirmation signal.
  uint32_t final_listen = millis();

  
  while (!available && (millis() - final_listen < max_wait_time)){
    Message buoy_signal = listen(max_wait_time);
    if (buoy_signal.success){
      if (debug_serial){
        Serial.println("Message received!");
        Serial.print("Message is: ");
        Serial.print(buoy_signal.msgSize);
        Serial.println(" bytes large!");
      }
      IWatchdog.reload();
      // Last format B + 4*id + BST_ID + 4*listenTime + E
      if (buoy_signal.msg[0] == 'B' && buoy_signal.msgSize == 3 + 2*sizeof(uint32_t)){
        uint32_t buoy_ID_received = msg_extract_uint<uint32_t>(buoy_signal.msg, 1,true);
        bool matching_Bid = (baseStationID == buoy_signal.msg[1 + sizeof(buoy_ID)]);
        bool matching_Uid = (buoy_ID == buoy_ID_received); 
        if (matching_Bid && matching_Uid){
  
          if (debug_serial){
            Serial.println("Base station found!");
          }

          listenTime = msg_extract_uint<uint32_t>(buoy_signal.msg, 1 + sizeof(buoy_ID) + sizeof(baseStationID),true);
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
  changeFrequency(LoRa_freq_send);
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
  endmessage[0] = 'E';
  endmessage[1] = 'M';
  endmessage[2] = msgCounter;
  endmessage[3] = packetsLeft;

  uint32_t wait_time = max_radio_wait_time;
  msg_insert_uint(endmessage, max_radio_wait_time, 4,10,true);
  IWatchdog.reload();
  if (IWatchdog.isReset(true)){
    endmessage[8] = 1;
  } else {
    endmessage[8] = 0;
  }
  endmessage[9] = 'E';
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
    Message msg = listen(max_radio_wait_time + start_listen_time - millis());
    char msg1 = msg.msg[0];
    char msg2 = msg.msg[1];
    IWatchdog.reload();
    if ((msg1 == 'B') && (msg.msgSize == 15)){
      cleared = true;
      base_measurement_period = msg_extract_uint<uint32_t>(msg.msg,1,true);
      IWatchdog.reload();

      if (msg.msg[5] == 1){
        enable_motion_detection = true;
        targetReadingDistance = msg_extract_uint<uint32_t>(msg.msg,6,true);
        motion_treshold       = msg_extract_uint<uint32_t>(msg.msg, 10, true);
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
    Message msg = listen(max_radio_wait_time + start_listen_time - millis());
    char msg1 = msg.msg[0];
    char msg2 = msg.msg[1];
    IWatchdog.reload();
    if ((msg1 == 'B') && (msg.msgSize == 15)){
      cleared = true;
      base_measurement_period = msg_extract_uint<uint32_t>(msg.msg,1,true);
      IWatchdog.reload();

      if (msg.msg[5] == 1){
        enable_motion_detection = true;
        targetReadingDistance = msg_extract_uint<uint32_t>(msg.msg,6,true);
        motion_treshold       = msg_extract_uint<uint32_t>(msg.msg, 10, true);
      } else {
        enable_motion_detection = false;
      }

    }
  }
}

void LoRa_Transceiver::updateMeasurementFrequency(uint32_t velocity){
  /*
    Method for updating the measurement frequency. Checks if motion detection is on, otherwise uses
    default measurement frequency. Formula for adaptive measurement frequency is
      mf = distance/velocity. 
    We divide by 100 times velocity as velocity is given as 10000 times the double value,
    and time unit is milliseconds. Hence, 1e-3*1e4 = 1e2
  */
  float ms_2_s {1e-3};
  if (enable_motion_detection && (velocity > scale_factor*motion_treshold)){
    measurement_period = min((uint32_t) (targetReadingDistance*scale_factor/(ms_2_s*velocity)), maximal_measurement_period);
    measurement_period = max(measurement_period, minimal_measurement_period);
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

void LoRa_Transceiver::transmitBeaconMessage(GPS_Data position){
 /*
  Send the last measured position every five minutes at the Beacon frequency 
  fB
 */ 
  changeFrequency(LoRa_freq_beacon);
  byte beaconMsg[beaconMsgSize];
  beaconMsg[0] = 'U';
  beaconMsg[1] = 'R';
  msg_insert_uint(beaconMsg, position.timestamp, 2, beaconMsgSize, true);
  msg_insert_uint(beaconMsg, position.lat, 2 + sizeof(time_t), beaconMsgSize, true);
  msg_insert_uint(beaconMsg, position.lng, 2 + sizeof(time_t) + sizeof(uint32_t), beaconMsgSize, true);
  msg_insert_uint(beaconMsg, buoy_ID, 2 + sizeof(time_t) + 2*sizeof(uint32_t), beaconMsgSize, true);
  beaconMsg[beaconMsgSize -1] = 'E';
  for (uint8_t i = 0; i < 5; i++){
    transmitB(beaconMsg, beaconMsgSize);
    waitUntilReady();
    delay(500);
  }
}