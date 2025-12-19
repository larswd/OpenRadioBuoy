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

class SD_Writer{
  public:
    bool begin(int8_t cs_pin, bool countLogFiles);
    int8_t startLogging(String filename);
    int8_t startLogging(const char * filename);
    int8_t startReading(String filename);
    int8_t startReading(const char * filename);
    int8_t logString(String msg);
    int8_t logString(const char * msg);
    int8_t logString(const byte * msg, const uint8_t messageSize);
    int8_t logByteArray(byte* array, int length);
    int8_t logSignalInfo(uint32_t RSSI, uint32_t SNR);
    int8_t readFile(void);
    int8_t printFile(void);
    int8_t stopLogging(void);
    int8_t stopReading(void);
    int8_t shutdown(void);
    int8_t deleteFile(const char * filename);
    bool active = false;
    uint16_t logCount = 0;
    uint16_t numLines = 0;
    etl::deque<fileLine, max_file_length> file_buffer;
    
  private:
    bool SD_fail;
    uint8_t mode = OLB_SD_INACTIVE;
    File LogFile;
    SdFat SD;
    fileLine line;
    uint16_t countFiles(const char * directory);
};

extern SD_Writer sd_writer;
#endif