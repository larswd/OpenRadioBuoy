#include <Arduino.h>
#include "lora_transceiver.h"
#include "message_parser.h"
#include "etl_error_manager.h"
#include "sd_writer.h"
#include "gsm.h"
#include "IWatchdog.h"

#include <queue>

uint32_t health_time_start;
uint32_t notecard_reset_start;
uint32_t notecard_input_sync_start;

static FrequencyMessage adaptive_frequency_msg = {default_update_frequency, base_measurement_period, enable_motion_detection, target_reading_distance, motion_treshold};

bool rescue_mode = false;
uint32_t rescue_timeout = 0;
uint32_t notecard_rescue_mode_start;

bool SD_fail = true;

void setup()
{
  Serial.begin(serial_baud);
  delay(250);

  Serial.print("setup of base station: ");
  Serial.println(base_station_ID);

  sd_writer.begin();
  // if (!sd_writer.getStatus())
  // {
  //   Serial.println("SD opened");
  // }
  sd_writer.startLogging("boot_info.txt");
  sd_writer.logString("Booting base station");
  sd_writer.closeLog();

  // if (sd_closed)
  // {
  //   Serial.println("SD file closed");
  // }
  LORA.beginRadio(LoRa_freq_receive, LoRa_bw, LoRa_sf, LoRa_cr, LoRa_power);
  LORA.setDefaultSendFrequency(LoRa_freq_send);
  LORA.setBaseStationID(base_station_ID);
  LORA.getWiOID();
  LORA.computeReceptionChannel(num_LoRa_channels, LoRa_freq_receive_min, LoRa_freq_receive_max);
  // set up GSM connection
  //GSM.begin();

  uint32_t startListen = millis();

  health_time_start = millis();
  notecard_reset_start = millis();
  notecard_input_sync_start = millis();

  //GSM.sendMessage("BST is restarted");
  //GSM.syncMessages(false);

  if (enable_watchdog){
    IWatchdog.begin(1000*watchdog_wait_time);
  }
}

void loop()
{

  // reset Notecard
  if ((millis() - notecard_reset_start) > reset_frequency)
  {
    sd_writer.startDebugging(DEBUG_MODE_NOTECARD_RESET);
    IWatchdog.reload();
    sd_writer.debugSerialPrintln("Reset notecard");
    //GSM.reset();
    //GSM.sendMessage("Notecard has been reset.");
    //GSM.syncMessages(false);
    notecard_reset_start = millis();
    sd_writer.closeDebug();
  }

  // BST heartbeat message
  if ((millis() - health_time_start) > health_frequency)
  {
    sd_writer.startDebugging(DEBUG_MODE_BST_HEARTBEAT);
    IWatchdog.reload();
    sd_writer.debugSerialPrintln("send health msg to notehub");
    //GSM.sendMessage("BST is up and running");
    //GSM.syncMessages(false);
    health_time_start = millis();
    sd_writer.closeDebug();
  }

  if (((millis() - notecard_input_sync_start) > (input_sync_frequency * 60 * 1000)))
  {
    if (get_frequency_from_notehub)
    {
          sd_writer.startDebugging(DEBUG_MODE_NOTEHUB_SYNC);
          IWatchdog.reload();
          sd_writer.debugSerialPrintln("Receive measurement frequency:");
          //GSM.syncMessages(true);
          //adaptive_frequency_msg = GSM.receiveMeasurementFrequency();
          sd_writer.closeDebug();
    }
    if (enable_rescue_from_notehub) 
    {
      sd_writer.startDebugging(DEBUG_MODE_NOTEHUB_SYNC);
      sd_writer.debugSerialPrintln("Receive rescue parameter:");
      //GSM.syncMessages(true);
      BeaconIncomingMessage beacon_message = GSM.receiveBeaconMessage(rescue_mode, rescue_timeout);
      // only start timing if rescue mode has been enabled now
      if(!rescue_mode && beacon_message.enable_rescue_mode) 
      {
        notecard_rescue_mode_start = millis();
      }
      rescue_mode = beacon_message.enable_rescue_mode;
      rescue_timeout = beacon_message.timeout_rescue_mode;
      sd_writer.closeDebug();
    }

    notecard_input_sync_start = millis();

  }

  if (rescue_mode) 
  {
    sd_writer.startDebugging(DEBUG_MODE_BUOY_RESCUE);
     if ((millis() - notecard_rescue_mode_start) < rescue_timeout) 
     {
      IWatchdog.reload();

      LORA.changeFrequency(LoRa_freq_beacon);
      uint32_t listen_time_delta = 1000;
      uint32_t startListen = millis();
      std::queue<BeaconOutgoingMessage *> beaconMessageQueue;
      while ((millis() - startListen) < (listen_time + listen_time_delta))
      {
        sd_writer.debugSerialPrint("Remaining listening time: ");
        sd_writer.debugSerialPrintln((listen_time + listen_time_delta) - (millis() - startListen));


        LORA.listenByteArray(10000);
        if (LORA.byte_msg.success)
        {

          sd_writer.debugSerialPrint("Receiving message: ");
          sd_writer.debugSerialPrintln((char)LORA.byte_msg.byteMsg[0]);

          sd_writer.debugSerialPrint("Received message bytes: ");
          sd_writer.debugByteArray(LORA.byte_msg.byteMsg, LORA.byte_msg.numBytes);
          sd_writer.debugSerialPrintln("");
          

          sd_writer.debugSerialPrint("RSSI:\t\t");
          float rssi = LORA.getRSSI();
          sd_writer.debugSerialPrintln(rssi);

          if (LORA.byte_msg.byteMsg[0] == 'U' && LORA.byte_msg.byteMsg[1] == 'R')
          {

            sd_writer.debugSerialPrint("Received beacon message");
            beacon_Reading b = MESSAGE_PARSER.parse_beacon_message(LORA.byte_msg.byteMsg);
            sd_writer.debugSerialPrint("timestamp: ");
            sd_writer.debugSerialPrint(b.timestamp);
            sd_writer.debugSerialPrint(", lat: ");
            sd_writer.debugSerialPrint(b.lat);
            sd_writer.debugSerialPrint(", lng: ");
            sd_writer.debugSerialPrint(b.lng);
            sd_writer.debugSerialPrint(", bouy ID: ");
            sd_writer.debugSerialPrintln(b.buoy_id);
            BeaconOutgoingMessage *beaconMsg = new BeaconOutgoingMessage;
            beaconMsg->buoy_id = b.buoy_id;
            beaconMsg->lat = b.lat;
            beaconMsg->lng = b.lng;
            beaconMsg->timestamp = b.timestamp;
            beaconMsg->rssi = rssi;
            beaconMessageQueue.push(beaconMsg);
          }
        }
      }
      IWatchdog.reload();
      while (!beaconMessageQueue.empty())
      {
        sd_writer.debugSerialPrintln("Send messages to Notehub");
        BeaconOutgoingMessage *beaconMsg = beaconMessageQueue.front();
        beaconMessageQueue.pop();
        //GSM.sendBeaconMessage(beaconMsg);
        //GSM.syncMessages(false);
        delete beaconMsg;
      }
     }
     else 
     {
      rescue_mode = false;
     }
     sd_writer.closeDebug();

  }
  else if (enable_handshake)
  {
    sd_writer.startDebugging(DEBUG_MODE_BUOY_COMM);
    IWatchdog.reload();
    sd_writer.debugSerialPrintln("handshake enabled");
    buoyInfo buoy = LORA.findBuoy(max_radio_fix_look_time);
     sd_writer.debugSerialPrintln("still after searching for buoy");

    // GSM.sendMessage("loop in main");
    if (buoy.inrange)
    {
      IWatchdog.reload();
      uint32_t buoy_ID = buoy.ID;
      int pos = 0;
      uint32_t startListen = millis();

      // IWatchdog.reload();
      sd_writer.debugSerialPrintln("Write to SD card");
      char filename[32];
      // sprintf(filename, "transmissions/packet_%020d.txt", sd_writer.transmissionsCount); // TODO: Change to date queried from notehub
      bool SD_fail = sd_writer.begin();
      // sd_writer.startLogging(filename);

      if (!SD_fail)
      {
        sd_writer.debugSerialPrintln("SD opened");
        // sd_writer.debugSerialPrint("Status code: ");
        // sd_writer.debugSerialPrintln(status);
      }

      std::queue<BuoyMessage *> messageQueue;

      uint32_t listen_time_delta = 1000;

      while ((millis() - startListen) < (listen_time + listen_time_delta))
      {
        sd_writer.debugSerialPrint("Remaining listening time: ");
        sd_writer.debugSerialPrintln((listen_time + listen_time_delta) - (millis() - startListen));
        // LORA.listenByteArray((listen_time + listen_time_delta) - (millis() - startListen));
        
        // This should probably be a variable
        LORA.listenByteArray(10000);
        if (LORA.byte_msg.success)
        {
          sd_writer.debugSerialPrint("Receiving message: ");
          sd_writer.debugSerialPrintln((char)LORA.byte_msg.byteMsg[0]);

          sd_writer.debugSerialPrint("Received message bytes: ");
          sd_writer.debugByteArray(LORA.byte_msg.byteMsg, LORA.byte_msg.numBytes);
          sd_writer.debugSerialPrintln("");
          

          sd_writer.debugSerialPrint("RSSI:\t\t");
          float rssi = LORA.getRSSI();
          sd_writer.debugSerialPrintln(rssi);

          if (LORA.byte_msg.byteMsg[0] == 'G')
          {
            // Receive GPS message
            // GSM.sendByteMessage(LORA.byte_buoy_msg);
            BuoyMessage *qMsg = new BuoyMessage;
            qMsg->byteMsg = new ByteMessage;
            qMsg->rssi = rssi;
            qMsg->byteMsg->numBytes = LORA.byte_msg.numBytes;
            memcpy(qMsg->byteMsg->byteMsg, LORA.byte_msg.byteMsg, LORA.byte_msg.numBytes);
            messageQueue.push(qMsg);

            // GSM.sendMessage("location info");
            sd_writer.debugSerialPrintln("location info");
            GPS_Reading g = MESSAGE_PARSER.parse_gps_message(LORA.byte_msg.byteMsg);
            sd_writer.debugSerialPrint("reading id: ");
            sd_writer.debugSerialPrint(g.reading_ID);
            sd_writer.debugSerialPrint(", lat: ");
            sd_writer.debugSerialPrint(g.lat);
            sd_writer.debugSerialPrint(", lng: ");
            sd_writer.debugSerialPrint(g.lng);
            sd_writer.debugSerialPrint(", vel: ");
            sd_writer.debugSerialPrint(g.vel);
            sd_writer.debugSerialPrint(", direction: ");
            sd_writer.debugSerialPrint(g.direction);
            sd_writer.debugSerialPrint(", timestamp: ");
            sd_writer.debugSerialPrintln(g.timestamp);
          }
          else if (LORA.byte_msg.byteMsg[0] == 'T')
          {
            // GSM.sendMessage("temperature info");
            // GSM.sendByteMessage(LORA.byte_buoy_msg);
            // QueuedMessage qMsg;
            // qMsg.type = 'T';
            // qMsg.message = new byte[LORA.byte_buoy_msg.numBytes];
            // qMsg.size = LORA.byte_buoy_msg.numBytes;
            // memcpy(qMsg.message, LORA.byte_buoy_msg.byteMsg, LORA.byte_buoy_msg.numBytes);
            // messageQueue.push(qMsg);

            BuoyMessage *qMsg = new BuoyMessage;
            qMsg->byteMsg = new ByteMessage;
            qMsg->rssi = rssi;
            qMsg->byteMsg->numBytes = LORA.byte_msg.numBytes;
            memcpy(qMsg->byteMsg->byteMsg, LORA.byte_msg.byteMsg, LORA.byte_msg.numBytes);
            messageQueue.push(qMsg);

            sd_writer.debugSerialPrintln("temperature info");
            // Receive temperature message
            temperature_Reading c = MESSAGE_PARSER.parse_temperature_message(LORA.byte_msg.byteMsg);
            sd_writer.debugSerialPrint("reading id: ");
            sd_writer.debugSerialPrint(c.reading_ID);
            sd_writer.debugSerialPrint(", ");
            for (int i = 0; i < max_number_of_thermometres; i++){
              sd_writer.debugSerialPrint("temperature sensor ");
              sd_writer.debugSerialPrint(i);
              sd_writer.debugSerialPrint(": ");
              sd_writer.debugSerialPrint(c.temps[i]);
              sd_writer.debugSerialPrint(", ");
            }
            sd_writer.debugSerialPrint(", timestamp: ");
            sd_writer.debugSerialPrintln(c.timestamp);
          }
          else if (LORA.byte_msg.byteMsg[0] == 'R')
          {
            turbidity_Reading turbidity_msg = MESSAGE_PARSER.parse_turbidity_message(LORA.byte_msg.byteMsg);
            // Serial.println("turbidity info");
            // Serial.print("voltage: ");
            // Serial.println(turbidity_msg.voltage);
          }
          else if (LORA.byte_msg.byteMsg[0] == 'E' && LORA.byte_msg.byteMsg[1] == 'M')
          {
            sd_writer.debugSerialPrintln("end message");

            buoyInfoReading b = MESSAGE_PARSER.parse_buoy_info_message(LORA.byte_msg.byteMsg);

            sd_writer.debugSerialPrint("Sent packets: ");
            sd_writer.debugSerialPrint(b.sent_packets);
            sd_writer.debugSerialPrint(", left packets: ");
            sd_writer.debugSerialPrint(b.left_packets);
            sd_writer.debugSerialPrint(", listen time: ");
            sd_writer.debugSerialPrint(b.listen_time);
            sd_writer.debugSerialPrint(", crashed: ");
            sd_writer.debugSerialPrintln(b.crashed);
            sd_writer.debugSerialPrintln("End of message");
            LORA.sendFinalMessage(adaptive_frequency_msg);
            // delay(100);

            // Serial.println("Resend message in case of failure");
            // LORA.sendFinalMessage(msg, 14);
            // delay(300);

            // GSM.syncMessages();
            break;
          }
          sd_writer.debugSerialPrintln("");
        }
      }

      if (!messageQueue.empty())
      {
        time_t time = GSM.getDate();
        Serial.print("Current time: ");
        Serial.println(time);
        Serial.println("Create file on SD card");
        char filename[32];
        sprintf(filename, "transmissions/packet_%020d.txt", time);
        sd_writer.startLogging(filename);
      }

      while (!messageQueue.empty())
      {
        IWatchdog.reload();
        sd_writer.debugSerialPrintln("Sending all queued messages via GSM...");
        BuoyMessage *qMsg = messageQueue.front();
        messageQueue.pop();

        // Create a temporary ByteMessage structure
        // ByteMessage tempMsg;
        // tempMsg.numBytes = qMsg->numBytes;
        // memcpy(tempMsg.byteMsg, qMsg->byteMsg, tempMsg.numBytes);

        if (SD_fail == 0)
        {
          sd_writer.logByteArray(qMsg->byteMsg->byteMsg, qMsg->byteMsg->numBytes);
          // sd_writer.logString((char*)LORA.byte_buoy_msg.byteMsg);
        }
        else
        {
          sd_writer.debugSerialPrintln("SD card failed to open");
        }

        //GSM.sendBuoyMessage(qMsg, buoy_ID);
        // delay(100); // Small delay between messages
        IWatchdog.reload();
        delete qMsg->byteMsg;
        delete qMsg;
      }
      // notecard_reset_start = millis();
      IWatchdog.reload();
      //GSM.syncMessages(false);
      sd_writer.closeLog();
      sd_writer.closeDebug();
      // buoy = LORA.initEmptyBuoy();
    }
  }
}
