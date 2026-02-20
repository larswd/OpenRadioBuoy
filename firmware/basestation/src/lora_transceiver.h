#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include "RadioLib.h"
#include "etl/deque.h"
#include "etl/string.h"
#include "messages.h"
#include "config.h"
#include "sd_writer.h"
#include "parser_utils.h"
#include "IWatchdog.h"

//This function is difficult to weave into a class structure
void setFlag(void);

static volatile bool operationDone = false;
static char buoy_ID[12];
  
struct StringMessage{
  String msg; // TODO: replace more meaningful struct
  bool success;
};

struct buoyInfo{
  uint32_t ID;
  bool inrange;
};

class LoRa_Transceiver{
  public:
    void computeReceptionChannel(void);
    void beginRadio(double freq, double bw, int sf, int cr, int power);
    void transmit(String);
    void transmit(const char *);
    void transmitB(byte *, uint8_t msgSize);
    void sendFinalMessage(FrequencyMessage frequency_message);
    void waitUntilReady();
    void listen(uint32_t max_wait_time);
    void listenByteArray(uint32_t max_wait_time);
    float getRSSI();
    uint32_t create_update_message(byte* msg, FrequencyMessage frequency_message);
    int16_t state;
    int16_t packet_count;
    buoyInfo initEmptyBuoy();
    buoyInfo findBuoy(uint32_t);
    bool available = false;
    StringMessage string_buoy_msg;
    ByteMessage byte_buoy_msg;
    bool listening = false;
    void changeFrequency(double);
    private:
    STM32WLx radio = new STM32WLx_Module();
    const uint32_t rfswitch_pins[5] = {PA4, PA5, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC};
    const Module::RfSwitchMode_t rfswitch_table[4] = {
      {STM32WLx::MODE_IDLE,  {LOW,  LOW}},
      {STM32WLx::MODE_RX,    {HIGH, LOW}},
      {STM32WLx::MODE_TX_HP, {LOW, HIGH}},
      END_OF_MODE_TABLE,
    };
    
    int16_t msgCounter = 0;
    buoyInfo buoy;
};

extern LoRa_Transceiver LORA;
#endif