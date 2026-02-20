#ifndef SD_WRITER_H
#define SD_WRITER_H

#include <SdFat.h>
#include <config.h>

#define SD_CS_PIN PB9
#define PIN_SPI_MISO PB14
#define PIN_SPI_MOSI PA10
#define PIN_SPI_SCK PB13
// #define SD_FAT_TYPE 1


// Debug modes
static constexpr int8_t DEBUG_MODE_NOTECARD_RESET {0};
static constexpr int8_t DEBUG_MODE_BST_HEARTBEAT  {1};
static constexpr int8_t DEBUG_MODE_NOTEHUB_SYNC   {2};
static constexpr int8_t DEBUG_MODE_BUOY_COMM      {3};
static constexpr int8_t DEBUG_MODE_BUOY_RESCUE    {4};

class SDWriter {
public:
    bool setup();
    void startLogging(String filename);
    void logString(const char * line);
    void logString(String line);
    void logByteArray(byte* array, int length);
    void writeLog(const char *message);
    void closeLog();
    bool opened = false;
    uint32_t transmissionsCount = 0;

    //Debug methods
    uint32_t debugInfoLogCount[4]  = {0,0,0,0};
    int8_t startDebugging(int MODE);
    bool debugOpened = false;
    void debugSerialPrint(const char * line);
    void debugSerialPrint(float number);
    void debugSerialPrintln(const char * line);
    void debugSerialPrintln(float number);
    int8_t debugByteArray(byte * array, int length);
    void closeDebug(void);
private:
    uint32_t countFilesInDirectory(const char * dirName);
    SdFat SD;
    File LogFile;
    File debugFile;
    bool SD_fail;
};

extern SDWriter SD_CARD;
#endif // SD_WRITER_H