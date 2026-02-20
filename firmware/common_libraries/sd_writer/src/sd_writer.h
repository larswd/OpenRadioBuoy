#ifndef SD_WRITER_H
#define SD_WRITER_H

#include "config.h"
#include "SdFat.h"
#include "IWatchdog.h"
#include "etl/deque.h"
#include "etl_error_manager.h"

#define SD_CS_PIN PB9
#define PIN_SPI_MISO PB14
#define PIN_SPI_MOSI PA10
#define PIN_SPI_SCK  PB13


static constexpr uint8_t OLB_SD_INACTIVE   {0};
static constexpr uint8_t OLB_SD_WRITE_MODE {1};
static constexpr uint8_t OLB_SD_READ_MODE  {2};
static constexpr uint8_t DEBUG_MODE_NOTECARD_RESET {3};
static constexpr uint8_t DEBUG_MODE_BST_HEARTBEAT  {4};
static constexpr uint8_t DEBUG_MODE_NOTEHUB_SYNC   {5};
static constexpr uint8_t DEBUG_MODE_BUOY_COMM      {6};
static constexpr uint8_t DEBUG_MODE_BUOY_RESCUE    {7};



/*
  File reading parameters
  Add computations to separate file
*/

static constexpr uint8_t max_line_length   {128};
static constexpr uint8_t max_file_length   {128};

struct fileLine{
  char line[max_line_length];
  uint8_t length;
};

class SDWriter{
  public:
    // Boot SD card
    bool begin();

    // Open files
    int8_t startLogging(String filename);
    int8_t startLogging(const char * filename);
    int8_t startReading(String filename);
    int8_t startReading(const char * filename);
    int8_t startDebugging(int MODE);
    bool debugOpened = false;

    // File manipulation
    int8_t logString(String msg);
    int8_t logString(const char * msg);
    int8_t logString(const byte * msg, const uint8_t messageSize);
    int8_t logByteArray(byte* array, int length);
    int8_t logSignalInfo(uint32_t RSSI, uint32_t SNR);
    int8_t readFile(void);
    int8_t printFileToSerial(void);
    int8_t deleteFile(const char * filename);
 
    // Shutdown and stop functions
    int8_t closeLog(void);
    int8_t closeRead(void);
    int8_t shutdown(void);

    // Flag checking if SD writer already is in use
    bool active = false;
    
    // File counts
    uint16_t logCount = 0;
    uint32_t transmissionsCount = 0;
    uint32_t debugInfoLogCount[4]  = {0,0,0,0};

    // File reading utilities
    uint16_t numLines = 0;
    etl::deque<fileLine, max_file_length> file_buffer;
    
    // Debugging tools
    void debugSerialPrint(const char * line);
    void debugSerialPrint(float number);
    void debugSerialPrintln(const char * line);
    void debugSerialPrintln(float number);
    int8_t debugByteArray(byte * array, int length);
    void closeDebug(void);
    uint8_t mode = OLB_SD_INACTIVE;
  private:
    bool SD_fail;
    File LogFile;
    File debugFile;
    SdFat SD;
    fileLine line;
    uint32_t countFilesInDirectory(const char * dirName);
};

extern SDWriter sd_writer;
#endif