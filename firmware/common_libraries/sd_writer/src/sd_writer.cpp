#include "sd_writer.h"


SDWriter sd_writer;

/*
  Initialising SDWriter, and file opening functions. 

  File functions return the following error codes
  Error codes:
    0 - Everything works as intended
    1 - SD card reader failed to open
    2 - User tries to either open an already opened file, or 
        write to nonexisting file
*/

uint32_t SDWriter::countFilesInDirectory(const char * dirName){
  // Reads all files in given directory
  uint16_t fileCount = 0;
  File directory;
  directory.open(dirName);
  while (LogFile.openNext(&directory, O_RDONLY)) {
    if (!LogFile.isHidden()) {
      fileCount++;
    }
  }
  LogFile.close();
  directory.close();
  return fileCount;
}

bool SDWriter::begin(void){
  /*
    Start up the SD writer. Returns 0 if everything works as intended. 
  */
  if (debug_serial)
    Serial.println("Initializing SD card!");

  SD_fail = !SD.begin(SD_CS_PIN);
 
  if (!SD_fail && ((WIO_MODE == BUOY_MODE) || (WIO_MODE == MOORED_MODE))){
    active = true;

    // We create the necessary directories if not already present
    if (!SD.exists("readings")){
      SD.mkdir("readings");
    } else if (SD.exists("readings")) {
      //If readings directory is present, we count 
      // the files in said directory to avoid writing to 
      // the same file twice
      if (debug_serial){
        Serial.println("Counting files");
      }
      
      logCount = countFilesInDirectory("readings/");
      //Count amount of files in reading directory to ensure readability
      if (debug_serial){
        Serial.print("Num files: ");
        Serial.println(logCount);
        Serial.println("\n");
      }
    } 
    if (!SD.exists("messages")){
      SD.mkdir("messages");
    }

  } else if (!SD_fail && (WIO_MODE == BST_MODE)){
    if (!SD.exists("transmissions")){
      SD.mkdir("transmissions");
    } else {
      // We only need to count if the directory is already present
      transmissionsCount = countFilesInDirectory("transmissions/");
    }
    if (!SD.exists("debugFiles")){
      SD.mkdir("debugFiles");
      SD.mkdir("debugFiles/NotecardResets");
      SD.mkdir("debugFiles/BSTHeartbeats");
      SD.mkdir("debugFiles/NotehubSyncs");
      SD.mkdir("debugFiles/buoyComms");
      SD.mkdir("debugFiles/buoyRescue");
    } else {
      debugInfoLogCount[DEBUG_MODE_NOTECARD_RESET] = countFilesInDirectory("debugFiles/NotecardResets/");
      debugInfoLogCount[DEBUG_MODE_BST_HEARTBEAT]  = countFilesInDirectory("debugFiles/BSTHeartbeats/");
      debugInfoLogCount[DEBUG_MODE_NOTEHUB_SYNC]   = countFilesInDirectory("debugFiles/NotehubSyncs/");            
      debugInfoLogCount[DEBUG_MODE_BUOY_COMM]      = countFilesInDirectory("debugFiles/buoyComms/");    
      debugInfoLogCount[DEBUG_MODE_BUOY_RESCUE]    = countFilesInDirectory("debugFiles/buoyRescue/");          
    }
    //Count amount of files in reading directory to ensure readability
  }

  delay(100);
  return SD_fail;
}

int8_t SDWriter::startDebugging(int MODE){
  if (debug_SD){
    if (!SD_fail && !debugOpened){
      debugOpened = true;
      char debugFileName[128];
      if (MODE == DEBUG_MODE_NOTECARD_RESET){
        sprintf(debugFileName, "debugFiles/NotecardResets/log%09d.txt", debugInfoLogCount[DEBUG_MODE_NOTECARD_RESET]++);
      } else if (MODE == DEBUG_MODE_BST_HEARTBEAT){
        sprintf(debugFileName, "debugFiles/BSTHeartbeats/log%09d.txt", debugInfoLogCount[DEBUG_MODE_BST_HEARTBEAT]++);
      } else if (MODE == DEBUG_MODE_NOTEHUB_SYNC){
        sprintf(debugFileName, "debugFiles/NotehubSyncs/log%09d.txt", debugInfoLogCount[DEBUG_MODE_NOTEHUB_SYNC]++);
      } else if (MODE == DEBUG_MODE_BUOY_COMM){
        sprintf(debugFileName, "debugFiles/buoyComms/log%09d.txt", debugInfoLogCount[DEBUG_MODE_BUOY_COMM]++);
      } else if (MODE == DEBUG_MODE_BUOY_RESCUE){
        sprintf(debugFileName, "debugFiles/buoyRescue/log%09d.txt", debugInfoLogCount[DEBUG_MODE_BUOY_RESCUE]++);
      } else {
        //Unsupported mode
        return 3;
      }

      debugFile = SD.open(debugFileName, FILE_WRITE);

    } else if (SD_fail){
      return 1;
    } else {
      return 2;
    }
  }
  
  return 0;
}


// We avoid using Arduino strings for most operations
int8_t SDWriter::startLogging(String filename){
  int8_t state = startLogging(filename.c_str());
  return state;
}

int8_t SDWriter::startLogging(const char * filename){
  /*
    We check if the SD writer is in the correct mode to start new operations
  */
  if ((mode == OLB_SD_INACTIVE) && (!SD_fail)){
    mode   = OLB_SD_WRITE_MODE;
    LogFile = SD.open(filename, FILE_WRITE);

    logCount++;
    return 0;

  } else if (SD_fail) {
    return 1; 
  } else {
    return 2;
  }

}

int8_t SDWriter::startReading(const char * filename){
  /*
    If the SD writer is inactive, start reading operation. Otherwise pass with error code
  */
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

int8_t SDWriter::startReading(String filename){
  int8_t state = startReading(filename.c_str());
  return state;
}


// File writing functions
int8_t SDWriter::logString(const char * line){
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

int8_t SDWriter::logString(String line){
  int8_t state = logString(line.c_str());
  return state;
}

int8_t SDWriter::logString(const byte * line, const uint8_t messageSize){
  char _line[messageSize];
  memcpy(_line, line, messageSize);
  int8_t state = logString(_line);
  return state;
}

// RSSI/SNR only works on received signals
int8_t SDWriter::logSignalInfo(uint32_t RSSI, uint32_t SNR){
  char msg[32];
  sprintf(msg, "RSSI: %ld SNR: %ld\n", RSSI, SNR);
  int8_t ecode = logString(msg);
  return ecode;
}

// The 'b' separator is included to indicate a new byte as to ensure unambigous log files
int8_t SDWriter::logByteArray(byte* array, int length){
  if ((mode == OLB_SD_WRITE_MODE) && (!SD_fail)){
    if (debug_serial)
      Serial.println("Writing the following byte array to file:");
    for (int i = 0; i < length; i++){
      if (debug_serial){
        Serial.print(array[i]);
        Serial.print('b');
      }
      LogFile.print(array[i]);
      LogFile.print('b');
    }
    if (debug_serial)
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

int8_t SDWriter::readFile(void){
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


int8_t SDWriter::printFileToSerial(void){
  for (uint8_t line_num = 0; line_num < numLines; line_num++ ){
    Serial.println(file_buffer.at(line_num).line);
    delay(500);
    IWatchdog.reload();
  }
  return 0;
}

// Functions to stop writing/reading a file. Important to call before opening a new file. 

int8_t SDWriter::closeLog(void){
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


int8_t SDWriter::closeRead(void){
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



void SDWriter::closeDebug() {
    if (debugFile) {
        debugFile.close();
        debugOpened = false;
    }
}

// Boot down to preserve power
int8_t SDWriter::shutdown(void){
  if (active){
    SD.end();
    active = false;
  }
  return 0;
}


// File cleaning
int8_t SDWriter::deleteFile(const char * filename){
  if (SD.exists(filename)){
    SD.remove(filename);
    return 0;
  } else {
    return 1;
  }
}


/*
debugSerialPrint is an alias for Serial print 
which also dumps to debug file if open
Many, likely redundant overloads
*/

void SDWriter::debugSerialPrint(const char * line){
  if (debug_serial){
    Serial.print(line);
  }
  if (debug_SD && debugOpened){
    debugFile.print(line);
    debugFile.sync();
  }
}

void SDWriter::debugSerialPrintln(const char * line){
  if (debug_serial){
    Serial.println(line);
  }
  if (debug_SD && debugOpened){
    debugFile.println(line);
    debugFile.sync();
  }
}


void SDWriter::debugSerialPrint(float number){
  if (debug_serial){
    Serial.print(number);
  }
  if (debug_SD && debugOpened){
    debugFile.print(number);
    debugFile.sync();
  }
}

void SDWriter::debugSerialPrintln(float number){
  if (debug_serial){
    Serial.println(number);
  }
  if (debug_SD && debugOpened){
    debugFile.println(number);
    debugFile.sync();
  }
}


int8_t SDWriter::debugByteArray(byte* array, int length){
  if (debugOpened && !SD_fail){
    debugSerialPrintln("Writing the following byte array to file:");
    for (int i = 0; i < length; i++){
      if (debug_serial)
        Serial.print(array[i]);
      debugFile.print('b');
      debugFile.print(array[i]);
    }
    if (debug_serial)
      Serial.println();
    debugFile.println();
    debugFile.sync();
    return 0;
  } else if (SD_fail) {
    return 1;
  } else {
    return 2;
  }
}