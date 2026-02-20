#include "etl_error_manager.h"

/*
  We check for errors related to etl-memory management
*/
void etl_error_func(const etl::exception& e){
  char emsg[128];
  sprintf(emsg, "The Error was %s in %s at %d", e.what(), e.file_name(), e.line_number());
  if (debug_serial){
    Serial.print(emsg);
  }
}