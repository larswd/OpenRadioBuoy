#include "SD_writer.h"

/*
  Initialising SD_writer, and file opening functions. 

  File functions return the following error codes
  Error codes:
    0 - Everything works as intended
    1 - SD card reader failed to open
    2 - User tries to either open an already opened file, or 
        write to nonexisting file
*/
uint16_t SD_Writer::countFiles(const char * directory){
  // Reads all files in given directory
  uint16_t fileCount = 0;
  File readingDir;
  readingDir.open(directory);
  while (LogFile.openNext(&readingDir, O_RDONLY)) {
    if (!LogFile.isHidden()) {
      fileCount++;
    }
    LogFile.close();
  }
  readingDir.close();
  return fileCount;
}

bool SD_Writer::begin(int8_t cs_pin, bool countLogFiles = false){
  /*
    Start up the SD writer. Returns 0 if everything works as intended. 
  */

  SD_fail = !SD.begin(cs_pin);
 
  if (!SD_fail){
    active = true;

    // We create the necessary directories if not already present
    if (!SD.exists("readings")){
      SD.mkdir("readings");
    } else if (SD.exists("readings") && countLogFiles) {
      //If readings directory is present, we count 
      // the files in said directory to avoid writing to 
      // the same file twice
      if (debug_serial){
        Serial.println("Counting files");
      }
      
      logCount = countFiles("readings/");
      //Count amount of files in reading directory to ensure readability
      if (debug_serial){
        Serial.print("Num files: ");
        Serial.println(logCount);
        Serial.println("\n");
      }
    } else {
    }
    if (!SD.exists("messages")){
      SD.mkdir("messages");
    }

  }
  delay(100);
  return SD_fail;
}

// We avoid using Arduino strings for most operations
int8_t SD_Writer::startLogging(String filename){
  int8_t state = startLogging(filename.c_str());
  return state;
}

int8_t SD_Writer::startLogging(const char * filename){
  /*
    We check if the SD writer is in the correct mode to start new operations
  */
  if ((mode == OLB_SD_INACTIVE) && (!SD_fail)){
    mode   = OLB_SD_WRITE_MODE;
    LogFile = SD.open(filename, FILE_WRITE);

    // Index 10 is skipped for some reason.
    if (logCount % 10 == 0){
      Serial.println("Logcount divisible by 10");
      Serial.println(filename);
      Serial.println("----------------\n");
      delay(2000);
    }
    logCount++;
    return 0;
  } else if (SD_fail) {
    return 1; 
  } else {
    return 2;
  }

}

int8_t SD_Writer::startReading(const char * filename){
  if ((mode == OLB_SD_INACTIVE) && (!SD_fail)){
    mode   = OLB_SD_READ_MODE;
    LogFile = SD.open(filename, FILE_READ);
    Serial.println("FILE OPENED");
    numLines = 0;
    return 0;
  } else if (SD_fail) {
    return 1; 
  } else {
    return 2;
  }
}

int8_t SD_Writer::startReading(String filename){
  int8_t state = startReading(filename.c_str());
  return state;
}


// File writing functions
int8_t SD_Writer::logString(const char * line){
  if ((mode == OLB_SD_WRITE_MODE) && (!SD_fail)){
    if (debug_serial){
      Serial.println("Writing the following line to file:");
      Serial.println(line);
    }
    LogFile.println(line);
    LogFile.sync();
    return 0;
  } else if (SD_fail) {
    return 1;
  } else {
    return 2;
  }
}

int8_t SD_Writer::logString(String line){
  int8_t state = logString(line.c_str());
  return state;
}

int8_t SD_Writer::logString(const byte * line, const uint8_t messageSize){
  char _line[messageSize];
  memcpy(_line, line, messageSize);
  int8_t state = logString(_line);
  return state;
}


int8_t SD_Writer::logSignalInfo(uint32_t RSSI, uint32_t SNR){
  char msg[32];
  sprintf(msg, "RSSI: %ld SNR: %ld\n", RSSI, SNR);
  int8_t ecode = logString(msg);
  return ecode;
}

int8_t SD_Writer::logByteArray(byte* array, int length){
  if ((mode == OLB_SD_WRITE_MODE) && (!SD_fail)){
    Serial.println("Writing the following byte array to file:");
    for (int i = 0; i < length; i++){
      Serial.print(array[i]);
      Serial.print('b');
      LogFile.print(array[i]);
      LogFile.print('b');
    }
    Serial.println();
    LogFile.println();
    LogFile.sync();
    IWatchdog.reload();
    // LogFile.write(array, length);
    return 0;
  } else if (SD_fail) {
    return 1;
  } else {
    return 2;
  }
}

// File reading function

int8_t SD_Writer::readFile(void){
  /*
    Read logfile (binary encoding)
    Store all lines to file buffer
    First character indicates if the line is a raw measurement (R)
    or aggregated measurement (A)
  */
  if ((mode == OLB_SD_READ_MODE) && (!SD_fail)){
    char str_line[max_line_length];
    while (LogFile.available()){
      line = {"",0}; 
      String nextline = LogFile.readStringUntil('\n');

      sprintf(line.line, nextline.c_str());
      line.length = nextline.length();
      file_buffer.push_back(line);

      numLines++;
    }
    IWatchdog.reload();
    return 0;
  } else {
    return 1;
  }
}


int8_t SD_Writer::printFile(void){
  for (uint8_t line_num = 0; line_num < numLines; line_num++ ){
    Serial.println(file_buffer.at(line_num).line);
    delay(500);
    IWatchdog.reload();
  }
  return 0;
}

int8_t SD_Writer::stopLogging(void){
  if ((mode == OLB_SD_WRITE_MODE) && (!SD_fail)){
    LogFile.close();
    mode = OLB_SD_INACTIVE;
    numLines = 0;
    return 0;
  } else if (SD_fail) {
    return 1;
  } else {
    return 2; 
  }
}


int8_t SD_Writer::stopReading(void){
  if ((mode == OLB_SD_READ_MODE) && (!SD_fail)){
    LogFile.close();
    file_buffer.clear();
    mode = OLB_SD_INACTIVE;
    return 0;
  } else if (SD_fail) {
    return 1;
  } else {
    return 2;
  }
}

int8_t SD_Writer::shutdown(void){
  if (active){
    SD.end();
    active = false;
  }
  return 0;
}

int8_t SD_Writer::deleteFile(const char * filename){
  if (SD.exists(filename)){
    SD.remove(filename);
    return 0;
  } else {
    return 1;
  }
}

SD_Writer sd_writer;