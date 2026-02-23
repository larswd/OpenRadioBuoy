#ifndef GSM_MANAGER_H
#define GSM_MANAGER_H

#include <Notecard.h>
#include "messages.h"
#include <TimeLib.h>

#include "sd_writer.h"

static const uint32_t char_message_size = max_message_length;

// This is the unique Product Identifier for our device
#ifndef PRODUCT_UID
#define PRODUCT_UID "no.uio.ifi.andreapf:oslofjord_drifter_basestation" 
#endif
#define myProductID PRODUCT_UID

class GSM_module
{
public:
    void begin();
    void end();
    void reset();
    void init_aux();
    long getTime();
    FrequencyMessage receiveMeasurementFrequency();
    void sendMessage(const char* msg);
    void sendBuoyMessage(BuoyMessage* msg, uint32_t id);
    void sendBeaconMessage(BeaconOutgoingMessage* msg);
    void syncMessages(bool sync_input);
    BeaconIncomingMessage receiveBeaconMessage(bool rescue_mode, time_t timeout);
    time_t getDate();

private:
    Notecard notecard;
    uint8_t collected_messages = 0;
};

extern GSM_module GSM;
#endif