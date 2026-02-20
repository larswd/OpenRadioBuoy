#include <Arduino.h>
#include "lora_transceiver.h"
#include "message_parser.h"
#include "sd_writer.h"
#include "gsm.h"
#include "IWatchdog.h"

#include <queue>

uint32_t health_time_start;
uint32_t notecard_reset_start;
uint32_t notecard_input_sync_start;

static FrequencyMessage adaptive_frequency_msg = {default_update_frequency, default_measurement_frequency, default_adaptive_frequency, default_target_length, default_threshold_velocity};

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

  SD_CARD.setup();
  // if (!SD_CARD.getStatus())
  // {
  //   Serial.println("SD opened");
  // }
  SD_CARD.startLogging("boot_info.txt");
  SD_CARD.logString("Booting base station");
  SD_CARD.closeLog();

  // if (sd_closed)
  // {
  //   Serial.println("SD file closed");
  // }

  LORA.beginRadio(LoRa_freq_receive, LoRa_bw, LoRa_sf, LoRa_cr, LoRa_power);
  LORA.computeReceptionChannel();
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
    SD_CARD.startDebugging(DEBUG_MODE_NOTECARD_RESET);
    IWatchdog.reload();
    SD_CARD.debugSerialPrintln("Reset notecard");
    //GSM.reset();
    //GSM.sendMessage("Notecard has been reset.");
    //GSM.syncMessages(false);
    notecard_reset_start = millis();
    SD_CARD.closeDebug();
  }

  // BST heartbeat message
  if ((millis() - health_time_start) > health_frequency)
  {
    SD_CARD.startDebugging(DEBUG_MODE_BST_HEARTBEAT);
    IWatchdog.reload();
    SD_CARD.debugSerialPrintln("send health msg to notehub");
    //GSM.sendMessage("BST is up and running");
    //GSM.syncMessages(false);
    health_time_start = millis();
    SD_CARD.closeDebug();
  }

  if (((millis() - notecard_input_sync_start) > (input_sync_frequency * 60 * 1000)))
  {
    if (get_frequency_from_notehub)
    {
          SD_CARD.startDebugging(DEBUG_MODE_NOTEHUB_SYNC);
          IWatchdog.reload();
          SD_CARD.debugSerialPrintln("Receive measurement frequency:");
          //GSM.syncMessages(true);
          //adaptive_frequency_msg = GSM.receiveMeasurementFrequency();
          SD_CARD.closeDebug();
    }
    if (enable_rescue_from_notehub) 
    {
      SD_CARD.startDebugging(DEBUG_MODE_NOTEHUB_SYNC);
      SD_CARD.debugSerialPrintln("Receive rescue parameter:");
      //GSM.syncMessages(true);
      BeaconIncomingMessage beacon_message = GSM.receiveBeaconMessage(rescue_mode, rescue_timeout);
      // only start timing if rescue mode has been enabled now
      if(!rescue_mode && beacon_message.enable_rescue_mode) 
      {
        notecard_rescue_mode_start = millis();
      }
      rescue_mode = beacon_message.enable_rescue_mode;
      rescue_timeout = beacon_message.timeout_rescue_mode;
      SD_CARD.closeDebug();
    }

    notecard_input_sync_start = millis();

  }

  if (rescue_mode) 
  {
    SD_CARD.startDebugging(DEBUG_MODE_BUOY_RESCUE);
     if ((millis() - notecard_rescue_mode_start) < rescue_timeout) 
     {
      IWatchdog.reload();

      LORA.changeFrequency(LoRa_freq_rescue);
      uint32_t listen_time_delta = 1000;
      uint32_t startListen = millis();
      std::queue<BeaconOutgoingMessage *> beaconMessageQueue;
      while ((millis() - startListen) < (listen_time + listen_time_delta))
      {
        SD_CARD.debugSerialPrint("Remaining listening time: ");
        SD_CARD.debugSerialPrintln((listen_time + listen_time_delta) - (millis() - startListen));


        LORA.listenByteArray(10000);
        if (LORA.byte_buoy_msg.success)
        {

          SD_CARD.debugSerialPrint("Receiving message: ");
          SD_CARD.debugSerialPrintln((char)LORA.byte_buoy_msg.byteMsg[0]);

          SD_CARD.debugSerialPrint("Received message bytes: ");
          SD_CARD.debugByteArray(LORA.byte_buoy_msg.byteMsg, LORA.byte_buoy_msg.numBytes);
          SD_CARD.debugSerialPrintln("");
          

          SD_CARD.debugSerialPrint("RSSI:\t\t");
          float rssi = LORA.getRSSI();
          SD_CARD.debugSerialPrintln(rssi);

          if (LORA.byte_buoy_msg.byteMsg[0] == 'U' && LORA.byte_buoy_msg.byteMsg[1] == 'R')
          {

            SD_CARD.debugSerialPrint("Received beacon message");
            beacon_Reading b = MESSAGE_PARSER.parse_beacon_message(LORA.byte_buoy_msg.byteMsg);
            SD_CARD.debugSerialPrint("timestamp: ");
            SD_CARD.debugSerialPrint(b.timestamp);
            SD_CARD.debugSerialPrint(", lat: ");
            SD_CARD.debugSerialPrint(b.lat);
            SD_CARD.debugSerialPrint(", lng: ");
            SD_CARD.debugSerialPrint(b.lng);
            SD_CARD.debugSerialPrint(", bouy ID: ");
            SD_CARD.debugSerialPrintln(b.buoy_id);
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
        SD_CARD.debugSerialPrintln("Send messages to Notehub");
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
     SD_CARD.closeDebug();

  }
  else if (enable_handshake)
  {
    SD_CARD.startDebugging(DEBUG_MODE_BUOY_COMM);
    IWatchdog.reload();
    SD_CARD.debugSerialPrintln("handshake enabled");
    buoyInfo buoy = LORA.findBuoy(max_radio_fix_look_time);
     SD_CARD.debugSerialPrintln("still after searching for buoy");

    // GSM.sendMessage("loop in main");
    if (buoy.inrange)
    {
      IWatchdog.reload();
      uint32_t buoy_ID = buoy.ID;
      int pos = 0;
      uint32_t startListen = millis();

      // IWatchdog.reload();
      SD_CARD.debugSerialPrintln("Write to SD card");
      char filename[32];
      // sprintf(filename, "transmissions/packet_%020d.txt", SD_CARD.transmissionsCount); // TODO: Change to date queried from notehub
      bool SD_fail = SD_CARD.setup();
      // SD_CARD.startLogging(filename);

      if (!SD_fail)
      {
        SD_CARD.debugSerialPrintln("SD opened");
        // SD_CARD.debugSerialPrint("Status code: ");
        // SD_CARD.debugSerialPrintln(status);
      }

      std::queue<BuoyMessage *> messageQueue;

      uint32_t listen_time_delta = 1000;

      while ((millis() - startListen) < (listen_time + listen_time_delta))
      {
        SD_CARD.debugSerialPrint("Remaining listening time: ");
        SD_CARD.debugSerialPrintln((listen_time + listen_time_delta) - (millis() - startListen));
        // LORA.listenByteArray((listen_time + listen_time_delta) - (millis() - startListen));
        
        // This should probably be a variable
        LORA.listenByteArray(10000);
        if (LORA.byte_buoy_msg.success)
        {
          SD_CARD.debugSerialPrint("Receiving message: ");
          SD_CARD.debugSerialPrintln((char)LORA.byte_buoy_msg.byteMsg[0]);

          SD_CARD.debugSerialPrint("Received message bytes: ");
          SD_CARD.debugByteArray(LORA.byte_buoy_msg.byteMsg, LORA.byte_buoy_msg.numBytes);
          SD_CARD.debugSerialPrintln("");
          

          SD_CARD.debugSerialPrint("RSSI:\t\t");
          float rssi = LORA.getRSSI();
          SD_CARD.debugSerialPrintln(rssi);

          if (LORA.byte_buoy_msg.byteMsg[0] == 'G')
          {
            // Receive GPS message
            // GSM.sendByteMessage(LORA.byte_buoy_msg);
            BuoyMessage *qMsg = new BuoyMessage;
            qMsg->byteMsg = new ByteMessage;
            qMsg->rssi = rssi;
            qMsg->byteMsg->numBytes = LORA.byte_buoy_msg.numBytes;
            memcpy(qMsg->byteMsg->byteMsg, LORA.byte_buoy_msg.byteMsg, LORA.byte_buoy_msg.numBytes);
            messageQueue.push(qMsg);

            // GSM.sendMessage("location info");
            SD_CARD.debugSerialPrintln("location info");
            GPS_Reading g = MESSAGE_PARSER.parse_gps_message(LORA.byte_buoy_msg.byteMsg);
            SD_CARD.debugSerialPrint("reading id: ");
            SD_CARD.debugSerialPrint(g.reading_ID);
            SD_CARD.debugSerialPrint(", lat: ");
            SD_CARD.debugSerialPrint(g.lat);
            SD_CARD.debugSerialPrint(", lng: ");
            SD_CARD.debugSerialPrint(g.lng);
            SD_CARD.debugSerialPrint(", vel: ");
            SD_CARD.debugSerialPrint(g.vel);
            SD_CARD.debugSerialPrint(", direction: ");
            SD_CARD.debugSerialPrint(g.direction);
            SD_CARD.debugSerialPrint(", timestamp: ");
            SD_CARD.debugSerialPrintln(g.timestamp);
          }
          else if (LORA.byte_buoy_msg.byteMsg[0] == 'T')
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
            qMsg->byteMsg->numBytes = LORA.byte_buoy_msg.numBytes;
            memcpy(qMsg->byteMsg->byteMsg, LORA.byte_buoy_msg.byteMsg, LORA.byte_buoy_msg.numBytes);
            messageQueue.push(qMsg);

            SD_CARD.debugSerialPrintln("temperature info");
            // Receive temperature message
            temperature_Reading c = MESSAGE_PARSER.parse_temperature_message(LORA.byte_buoy_msg.byteMsg);
            SD_CARD.debugSerialPrint("reading id: ");
            SD_CARD.debugSerialPrint(c.reading_ID);
            SD_CARD.debugSerialPrint(", ");
            for (int i = 0; i < max_thermometer; i++){
              SD_CARD.debugSerialPrint("temperature sensor ");
              SD_CARD.debugSerialPrint(i);
              SD_CARD.debugSerialPrint(": ");
              SD_CARD.debugSerialPrint(c.temps[i]);
              SD_CARD.debugSerialPrint(", ");
            }
            SD_CARD.debugSerialPrint(", timestamp: ");
            SD_CARD.debugSerialPrintln(c.timestamp);
          }
          else if (LORA.byte_buoy_msg.byteMsg[0] == 'R')
          {
            turbidity_Reading turbidity_msg = MESSAGE_PARSER.parse_turbidity_message(LORA.byte_buoy_msg.byteMsg);
            // Serial.println("turbidity info");
            // Serial.print("voltage: ");
            // Serial.println(turbidity_msg.voltage);
          }
          else if (LORA.byte_buoy_msg.byteMsg[0] == 'E' && LORA.byte_buoy_msg.byteMsg[1] == 'M')
          {
            SD_CARD.debugSerialPrintln("end message");

            buoyInfoReading b = MESSAGE_PARSER.parse_buoy_info_message(LORA.byte_buoy_msg.byteMsg);

            SD_CARD.debugSerialPrint("Sent packets: ");
            SD_CARD.debugSerialPrint(b.sent_packets);
            SD_CARD.debugSerialPrint(", left packets: ");
            SD_CARD.debugSerialPrint(b.left_packets);
            SD_CARD.debugSerialPrint(", listen time: ");
            SD_CARD.debugSerialPrint(b.listen_time);
            SD_CARD.debugSerialPrint(", crashed: ");
            SD_CARD.debugSerialPrintln(b.crashed);
            SD_CARD.debugSerialPrintln("End of message");
            LORA.sendFinalMessage(adaptive_frequency_msg);
            // delay(100);

            // Serial.println("Resend message in case of failure");
            // LORA.sendFinalMessage(msg, 14);
            // delay(300);

            // GSM.syncMessages();
            break;
          }
          SD_CARD.debugSerialPrintln("");
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
        SD_CARD.startLogging(filename);
      }

      while (!messageQueue.empty())
      {
        IWatchdog.reload();
        SD_CARD.debugSerialPrintln("Sending all queued messages via GSM...");
        BuoyMessage *qMsg = messageQueue.front();
        messageQueue.pop();

        // Create a temporary ByteMessage structure
        // ByteMessage tempMsg;
        // tempMsg.numBytes = qMsg->numBytes;
        // memcpy(tempMsg.byteMsg, qMsg->byteMsg, tempMsg.numBytes);

        if (SD_fail == 0)
        {
          SD_CARD.logByteArray(qMsg->byteMsg->byteMsg, qMsg->byteMsg->numBytes);
          // SD_CARD.logString((char*)LORA.byte_buoy_msg.byteMsg);
        }
        else
        {
          SD_CARD.debugSerialPrintln("SD card failed to open");
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
      SD_CARD.closeLog();
      SD_CARD.closeDebug();
      // buoy = LORA.initEmptyBuoy();
    }
  }
}
