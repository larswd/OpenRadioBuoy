#include "sd_writer.h"
#include "config.h"

SDWriter SD_CARD;

/*
  File counter, which counts all files in a given directory dirName
  Remember to add the / to the end of the directory name
*/
uint32_t SDWriter::countFilesInDirectory(const char * dirName){
  File directory;
  directory.open(dirName);
  uint32_t fileCount = 0;
  while (LogFile.openNext(&directory, O_RDONLY)) {
    if (!LogFile.isHidden()) {
      fileCount++;
    }
  }
  LogFile.close();
  directory.close();
  return fileCount;
}

bool SDWriter::setup() {
  if (debug_serial)
    Serial.println("Initializing SD card!");
  SD_fail = !SD.begin(SD_CS_PIN);
  if (!SD_fail){
  // We create the necessary directories if not already present
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


/*
  Start writing to file if no file is already opened
  Error code 1 means the SD writer is broken
  Error code 2 means there already is an active file in the relevant category
*/
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
      return 0;
    } else if (SD_fail){
      return 1;
    } else {
      return 2;
    }
  }
}

void SDWriter::startLogging(String filename)
{
  // Check if static assert does this correctly
  if (!SD_fail)
  {
    LogFile = SD.open(filename, FILE_WRITE);
    opened = true;
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

void SDWriter::logString(const char * line){
  if (opened && !SD_fail){
    LogFile.println(line);
    LogFile.sync();
  }
}

void SDWriter::logString(String line)
{
  if (!SD_fail)
  {
    logString(line.c_str());
  }
}

void SDWriter::logByteArray(byte* array, int length){
  if (opened && !SD_fail){
    for (int i = 0; i < length; i++){
      LogFile.print('b');
      LogFile.print(array[i]);
    }
    LogFile.println();
    LogFile.sync();
  }
}


void SDWriter::closeLog() {
    if (LogFile) {
        LogFile.close();
        opened = false;
    }
}

void SDWriter::closeDebug() {
    if (debugFile) {
        debugFile.close();
        debugOpened = false;
    }
}