#include "gsm.h"
#include "base64.hpp"

GSM_module GSM;

void GSM_module::begin()
{
    notecard.begin();

    // Configure the productUID, and instruct the Notecard to stay connected to
    // the service.
    J *req = notecard.newRequest("hub.set");
    if (myProductID[0])
    {
        JAddStringToObject(req, "product", myProductID);
    }

    // periodically send
    JAddStringToObject(req, "mode", "periodic");
    JAddNumberToObject(req, "outbound", output_sync_frequency);
    JAddNumberToObject(req, "inbound", input_sync_frequency);
    notecard.sendRequestWithRetry(req, 5); // 5 seconds

    init_aux();
}

void GSM_module::end()
{
    notecard.end();
}

void GSM_module::reset()
{
    end();
    begin();
}

void GSM_module::init_aux()
{
    J *req = NoteNewRequest("card.aux");
    JAddStringToObject(req, "mode", "gpio");

    J *pins = JAddArrayToObject(req, "usage");
    JAddItemToArray(pins, JCreateString("high")); // AUX1
    JAddItemToArray(pins, JCreateString("off"));  // AUX2
    JAddItemToArray(pins, JCreateString("off"));  // AUX3
    JAddItemToArray(pins, JCreateString("off"));  // AUX4

    J *rsp = notecard.requestAndResponse(req);
    const char *err = JGetString(rsp, "err");
    if (err != "")
    {
        SD_CARD.debugSerialPrint("Error: ");
        SD_CARD.debugSerialPrintln(err);
    }
}

long GSM_module::getTime()
{
    long time = 0;
    J *rsp = notecard.requestAndResponse(notecard.newRequest("card.time"));
    if (rsp != NULL)
    {
        time = JGetNumber(rsp, "time");
        notecard.deleteResponse(rsp);
    }
    return time;
}

FrequencyMessage GSM_module::receiveMeasurementFrequency()
{
    // inbound communication
    FrequencyMessage frequency_msg = {default_update_frequency, default_measurement_frequency, default_adaptive_frequency, default_target_length, default_threshold_velocity};
   

    // TODO: replace by note.get, see https://dev.blues.io/notecard/notecard-walkthrough/inbound-requests-and-shared-data/
    SD_CARD.debugSerialPrintln("Receive measurement frequency function");
    J *req = NoteNewRequest("note.get");
    JAddStringToObject(req, "file", "setup.qi");
    JAddBoolToObject(req, "delete", true);
    J *rsp = NoteRequestResponse(req);
    if (notecard.responseError(rsp))
    {
        SD_CARD.debugSerialPrintln("No notes available");
    }
    else
    {
        J *body = JGetObject(rsp, "body");
        frequency_msg.measurement_frequency = JGetInt(body, "measurement_frequency");
        frequency_msg.target_length = JGetInt(body, "target_length");
        frequency_msg.threshold_velocity = JGetInt(body, "threshold_velocity");
        frequency_msg.adaptive_frequency = JGetBool(body, "adaptive_frequency");
        SD_CARD.debugSerialPrint("Measurement frequency: ");
        SD_CARD.debugSerialPrintln(frequency_msg.measurement_frequency);
        SD_CARD.debugSerialPrint("Target length: ");
        SD_CARD.debugSerialPrintln(frequency_msg.target_length);
        SD_CARD.debugSerialPrint("Threshold velocity: ");
        SD_CARD.debugSerialPrintln(frequency_msg.threshold_velocity);
        SD_CARD.debugSerialPrint("Adaptive frequency enabled: ");
        SD_CARD.debugSerialPrintln(frequency_msg.adaptive_frequency);

    }
    notecard.deleteResponse(rsp);
    return frequency_msg;
}

void GSM_module::sendBuoyMessage(BuoyMessage *msg, uint32_t buoy_id)
{

    byte msg_buffer[byte_message_size];
    memcpy(msg_buffer, msg->byteMsg, msg->byteMsg->numBytes); // remove memcpy since it is unnecessary

    unsigned char msg_buffer_enc[encode_base64_length(msg->byteMsg->numBytes)];
    encode_base64(msg_buffer, msg->byteMsg->numBytes, msg_buffer_enc);

    if (J *req = notecard.newRequest("note.add"))
    {
        // JAddBoolToObject(req, "sync", false);
        JAddStringToObject(req, "file", "buoy.qo");

        J *body = JAddObjectToObject(req, "body");
        if (body != NULL)
        {
            JAddIntToObject(body, "id", buoy_id);
            JAddStringToObject(body, "payload", reinterpret_cast<char *>(msg_buffer_enc));
            JAddIntToObject(body, "rssi", int(msg->rssi));
        }

        J *rsp = notecard.requestAndResponse(req);
        const char *err = JGetString(rsp, "err");
        if (err != "")
        {
            SD_CARD.debugSerialPrint("Error in notecard communication: ");
            SD_CARD.debugSerialPrintln(err);
            reset();
        }
    }
}

void GSM_module::sendMessage(const char *msg)
{
    J *req = notecard.newRequest("note.add");
    if (req != NULL)
    {
        // JAddBoolToObject(req, "sync", false);
        JAddStringToObject(req, "file", "bst.qo");
        J *body = JAddObjectToObject(req, "body");
        if (body != NULL)
        {
            JAddStringToObject(body, "message", msg);
        }
        bool success = notecard.sendRequest(req);
        if (!success)
        {
            SD_CARD.debugSerialPrintln("Error on notecard communication");
            reset();
        }
    }
}

void GSM_module::syncMessages(boolean sync_input)
{
    J *sync_req = NoteNewRequest("hub.sync");
    JAddBoolToObject(sync_req, "in", sync_input);
    NoteRequest(sync_req);
}

BeaconIncomingMessage GSM_module::receiveBeaconMessage(bool rescue_mode, time_t timeout)
{
    SD_CARD.debugSerialPrintln("Receive beacon rescue function");
    BeaconIncomingMessage beacon_message = {rescue_mode, timeout};
    J *req = NoteNewRequest("note.get");
    JAddStringToObject(req, "file", "beacon.qi");
    JAddBoolToObject(req, "delete", true);
    J *rsp = NoteRequestResponse(req);
    if (notecard.responseError(rsp))
    {
        SD_CARD.debugSerialPrintln("No notes available");
    }
    else
    {
        J *body = JGetObject(rsp, "body");
        beacon_message.enable_rescue_mode = JGetBool(body, "enable");
        beacon_message.timeout_rescue_mode = JGetInt(body, "timeout");
        if (beacon_message.enable_rescue_mode)
        {
            SD_CARD.debugSerialPrintln("Rescue mode enabled");
        } 
        else 
        {
            SD_CARD.debugSerialPrintln("Rescue mode disabled");
        }
        SD_CARD.debugSerialPrint("Timeout for rescue mode: ");
        SD_CARD.debugSerialPrintln(beacon_message.timeout_rescue_mode);
    }

    return beacon_message;
}

void GSM_module::sendBeaconMessage(BeaconOutgoingMessage* msg)
{


    if (J *req = notecard.newRequest("note.add"))
    {
        // JAddBoolToObject(req, "sync", false);
        JAddStringToObject(req, "file", "beacon.qo");

        J *body = JAddObjectToObject(req, "body");
        if (body != NULL)
        {
            JAddIntToObject(body, "id", msg->buoy_id);
            JAddIntToObject(body, "lat", msg->lat);
            JAddIntToObject(body, "lng", msg->lng);
            JAddIntToObject(body, "rssi", int(msg->rssi));
            JAddIntToObject(body, "timestamp", msg->timestamp);
        }

        J *rsp = notecard.requestAndResponse(req);
        const char *err = JGetString(rsp, "err");
        if (err != "")
        {
            SD_CARD.debugSerialPrint("Error in notecard communication: ");
            SD_CARD.debugSerialPrintln(err);
            reset();
        }
    }
}

time_t GSM_module::getDate()
{
    J *req = NoteNewRequest("card.location");
    J *rsp = NoteRequestResponse(req);
    time_t time = 0;
    if (notecard.responseError(rsp))
    {
        Serial.println("No notes available");
    }
    else
    {
        time = JGetInt(rsp, "time");
    }
    return time;
}
