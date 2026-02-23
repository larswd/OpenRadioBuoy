#include "lora_transceiver.h"
#include "message_parser.h"

LoRa_Transceiver LORA;


void setFlag(void){
  // Function called by RadioLib to mark completed operation
  operationDone = true;
}

void LoRa_Transceiver::computeReceptionChannel(void){
  if (LoRa_freq_receive < 0){
    //Only first part of serial number is likely to be necessary
    uint32_t UID = HAL_GetUIDw0();

    // We use the modulo operator to get a "random" channel number
    uint8_t channelNo = UID % num_LoRa_channels;

    // We partition the domain into num_LoRa_channels equidistant channels, and choose the appropriate one
    LoRa_freq_receive = LoRa_freq_receive_min + (LoRa_freq_receive_max - LoRa_freq_receive_min)*( (float) channelNo)/num_LoRa_channels;
  }
}

void LoRa_Transceiver::beginRadio(double freq, double bw, int sf, int cr, int power)
{
  radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);
  state = radio.begin(freq, bw, sf, cr, (0x12), power, 8, 1.7, false);
  radio.setTCXO(1.7);
  radio.setDio1Action(setFlag);
  listening = false;
  packet_count = 0;
  delay(200);
}

buoyInfo LoRa_Transceiver::initEmptyBuoy()
{
  buoyInfo buoy;
  buoy.ID = 0;
  buoy.inrange = false;
  return buoy;
}

void LoRa_Transceiver::transmit(String msg)
{
  transmit(msg.c_str());
}

float LoRa_Transceiver::getRSSI()
{
  return radio.getRSSI();
}

void LoRa_Transceiver::transmitB(byte *msg, uint8_t msgSize)
{
  radio.startTransmit(msg, msgSize);
  msgCounter++;
}

// Method called after the end of transmission to send the updated parameters
void LoRa_Transceiver::sendFinalMessage(FrequencyMessage frequency_message)
{
  byte msg[15];
  uint32_t msgSize = create_update_message(msg, frequency_message);
  sd_writer.debugSerialPrint("Sending end message ");
  sd_writer.debugByteArray(msg, msgSize);
  sd_writer.debugSerialPrintln("");

  changeFrequency(LoRa_freq_send);
  radio.startTransmit(msg, msgSize);
  msgCounter++;
}

void LoRa_Transceiver::transmit(const char *msg)
{
  radio.startTransmit(msg);
  msgCounter++;
}

void LoRa_Transceiver::changeFrequency(double f)
{
  state = radio.begin(f, LoRa_bw, LoRa_sf, LoRa_cr, (0x12), LoRa_power, 8, 1.7, false);
  radio.setTCXO(1.7);
  radio.setDio1Action(setFlag);
  listening = false;
  delay(200);
}

void LoRa_Transceiver::listenByteArray(uint32_t max_wait_time)
{
  /*
    Start listening for message
    Wait up to max_wait_time (in ms) for a message
    return the message if successful
  */
  // if (operationDone){
  //   operationDone = false;
  // }
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
    state = radio.readData(byte_buoy_msg.byteMsg, numBytes);
    if (state == RADIOLIB_ERR_NONE)
    {
      byte_buoy_msg.success = true;
      byte_buoy_msg.numBytes = numBytes;
    }
    else
    {
      byte_buoy_msg.success = false;
      byte_buoy_msg.numBytes = 0;
    }
  }
  else
  {
    byte_buoy_msg.success = false;
    byte_buoy_msg.numBytes = 0;
  }
}

void LoRa_Transceiver::listen(uint32_t max_wait_time)
{
  /*
    Start listening for message
    Wait up to max_wait_time (in ms) for a message
    return the message if succesful
  */
  if (operationDone)
  {
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
  }
  
  if (operationDone)
  {
    operationDone = false;
    state = radio.readData(string_buoy_msg.msg);
    if (state == RADIOLIB_ERR_NONE)
    {
      string_buoy_msg.success = true;
    }
    else
    {
      string_buoy_msg.success = false;
    }
  }
  else
  {
    string_buoy_msg.success = false;
  }
}

buoyInfo LoRa_Transceiver::findBuoy(uint32_t max_wait_time)
{
  /*
    We look for a base station for a set amount of time
    Returning base station ID
  */

  // buoyInfo buoy;

  // Set radio in send mode
  changeFrequency(LoRa_freq_send);

  buoy.inrange = false;

  // Thransmit message and wait until ready

  char msg_out[32];
  // sprintf(msg_out,"Base station ID: %d", baseStationID);
  byte base_station_id_msg[7];
  base_station_id_msg[0] = 'B';
  base_station_id_msg[1] = base_station_ID;
  
  uint32_t freq = (uint32_t) (LoRa_freq_receive*scale_factor);
  msg_insert_uint(base_station_id_msg, freq, 2, 7, true);
  base_station_id_msg[6] = 'E';
  IWatchdog.reload();
  transmitB(base_station_id_msg, sizeof(base_station_id_msg));
  IWatchdog.reload();
  waitUntilReady();

  // Start radio in listen mode and wait for response
  uint32_t startLooking = millis();
  LoRa_freq_receive = (float) ((float) freq/ (float) scale_factor);

  changeFrequency(LoRa_freq_receive);
  // listen(max_wait_time);
  IWatchdog.reload();
  listenByteArray(max_wait_time);
  buoyInitMessage buoy_id_message;
  if (byte_buoy_msg.success)
  {

    buoy_id_message = MESSAGE_PARSER.parse_buoy_init_message(byte_buoy_msg.byteMsg);

    // sscanf(string_buoy_msg.msg.c_str(), "%s %d", buoy_ID, &baseStationID2);
    // Serial.println(string_buoy_msg.msg);
    sd_writer.debugSerialPrint("Bouy ID : ");
    sd_writer.debugSerialPrint(buoy_id_message.buoy_id);
    sd_writer.debugSerialPrint(", ");


    sd_writer.debugSerialPrint("Base station ID : ");
    sd_writer.debugSerialPrint(buoy_id_message.base_station_ID);
    sd_writer.debugSerialPrintln("");
  }
  // Serial.print(baseStationID);
  // Serial.print(" Received: ");
  // Serial.println(baseStationID2);
  // Serial.print("Equal? ");
  // Serial.println(baseStationID == baseStationID2);
  // Serial.println("Buoy ID");
  // Serial.println(buoy_ID);
  delay(100);

  if (base_station_ID == buoy_id_message.base_station_ID)
  {
    buoy.inrange = true;
  }

  // If no response, exit
  if (!buoy.inrange)
  {
    sd_writer.debugSerialPrintln("No buoy found");
    buoy.ID = 0;
    return buoy;
  }
  else
  {
    IWatchdog.reload();
    buoy.ID = buoy_id_message.buoy_id;
    // Else, send go ahead signal to buoy before switching back to receive mode
    changeFrequency(LoRa_freq_send);

    byte final_handshake_msg[1 + sizeof(buoy.ID) + sizeof(base_station_ID) + sizeof(listen_time) + 1];
    final_handshake_msg[0] = 'B';
    sd_writer.debugSerialPrintln("sending last message");
    msg_insert_uint(final_handshake_msg, buoy.ID, 1, sizeof(final_handshake_msg), true);
    final_handshake_msg[1 +sizeof(buoy.ID)] = base_station_ID;
    msg_insert_uint(final_handshake_msg, listen_time, 1 + sizeof(buoy.ID) + sizeof(base_station_ID), sizeof(final_handshake_msg), true);

    sd_writer.debugSerialPrintln("sending last message now");
    final_handshake_msg[14] = 'E';


    for (uint8_t repetitions = 0; repetitions < 3; repetitions++){
      transmitB(final_handshake_msg, sizeof(final_handshake_msg));
      delay(300);
      waitUntilReady();
    }
  }

  changeFrequency(LoRa_freq_receive);
  delay(100);
  return buoy;
}

void LoRa_Transceiver::waitUntilReady()
{
  uint32_t start_wait = millis();
  if (debug_serial)
  {
    sd_writer.debugSerialPrintln("Waiting for successful sending");
  }
  while (!operationDone)
  {
    delay(100);
  }
  IWatchdog.reload();
  operationDone = false;
}

uint32_t LoRa_Transceiver::create_update_message(byte *end_message, FrequencyMessage frequency_message)
{
  end_message[0] = 'B';
  // end_message[1] = 'E';
  // end_message[2] = change_freq;

  uint64_t alpha = 1;
  int32_t offset = 1;

  end_message[offset] = frequency_message.update_frequency;
  offset += 1;
  for (int i = 0; i < 3; i++)
  {
    alpha *= 256;
  }
  int32_t freq_tmp = frequency_message.measurement_frequency;
  for (int i = 0; i < 4; i++)
  {
    end_message[offset] = freq_tmp / alpha;
    freq_tmp = freq_tmp % alpha;
    alpha /= 256;
    offset++;
  }

  end_message[offset] = frequency_message.adaptive_frequency;
  offset++;

  alpha = 1;
  for (int i = 0; i < 3; i++)
  {
    alpha *= 256;
  }
  int32_t period_tmp = frequency_message.measurement_frequency;
  for (int i = 0; i < 4; i++)
  {
    end_message[offset] = period_tmp / alpha;
    period_tmp = period_tmp % alpha;
    alpha /= 256;
    offset++;
  }

  alpha = 1;
  for (int i = 0; i < 3; i++)
  {
    alpha *= 256;
  }
  int32_t target_length_tmp = frequency_message.target_length;
  for (int i = 0; i < 4; i++)
  {
    end_message[offset] = period_tmp / alpha;
    period_tmp = period_tmp % alpha;
    alpha /= 256;
    offset++;
  }

  alpha = 1;
  for (int i = 0; i < 3; i++)
  {
    alpha *= 256;
  }
  int32_t threshold_velocity_tmp = frequency_message.threshold_velocity;
  for (int i = 0; i < 4; i++)
  {
    end_message[offset] = period_tmp / alpha;
    period_tmp = period_tmp % alpha;
    alpha /= 256;
    offset++;
  }
  assert(offset == 14);
  end_message[offset] = 'E';
  return 15;
}
