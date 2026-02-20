#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include "config.h"
#include "parser_utils.h"
#include "RadioLib.h"
#include "etl/string.h"
#include "etl/deque.h"
#include "etl/vector.h"
#include "IWatchdog.h"


//This function is difficult to weave into a class structure
void setFlag(void);

static volatile bool operationDone = false;

struct Message{
  byte msg[max_message_length];
  uint8_t msgSize;
  bool success;
};

struct Message_Data{
  uint32_t RSSI;
  uint32_t SNR;
};

class LoRa_Transceiver{
  public:
    uint32_t WiO_ID;
    uint32_t listenTime = 0;
    uint8_t baseStationID = 0;
    uint32_t startup_timestamp = 0;
    bool available = false;
    int16_t state;
    int16_t packet_count;
    uint32_t lastTransmission;
    // This variable is a LORA variable as we might give 
    // instructions to the buoy in terms of measurement frequency
    uint32_t measurement_period = base_measurement_period;
    void getBuoyID(void);
    void beginRadio(double freq, double bw, int sf, int cr, int power);
    void transmit(String msg);
    void transmit(const char * msg);
    void transmitB(byte * msg, uint8_t msgSize);
    void waitUntilReady();
    Message listen(uint32_t max_wait_time);
    void findBaseStation(uint32_t max_wait_time);
    bool handshake(uint32_t maxListenTime);
    Message_Data sendData(byte * message, uint8_t msgSize, uint32_t message_send_time);
    void transmitFinished(uint8_t packetsLeft);
    void receiveInstructions(void);
    void receiveDesiredMeasurementIDs(void);
    void updateMeasurementFrequency(uint32_t velocity);
    uint16_t msgCounter = 0;
    void sleep(void);
    void wakeUp(void);
    void transmitBeaconMessage(byte * beaconMsg, uint8_t msgSize);

  private:
    STM32WLx radio = new STM32WLx_Module();
    const uint32_t rfswitch_pins[5] = {PA4, PA5, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC};
    const Module::RfSwitchMode_t rfswitch_table[4] = {
      {STM32WLx::MODE_IDLE,  {LOW,  LOW}},
      {STM32WLx::MODE_RX,    {HIGH, LOW}},
      {STM32WLx::MODE_TX_HP, {LOW, HIGH}},
      END_OF_MODE_TABLE,
    };
    void changeFrequency(double);
    int16_t transmissionState = RADIOLIB_ERR_NONE;

    Message receivedMessage;
    bool listening = false;
    bool sleeping = false;
    float LoRa_freq_send;

};

extern LoRa_Transceiver LORA;
#endif