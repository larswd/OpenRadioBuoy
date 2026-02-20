#ifndef ETL_ERROR_MANAGER_H
#define ETL_ERROR_MANAGER_H
#include <Arduino.h>
#include "config.h"

#define ETL_LOG_ERRORS 1
#define ETL_CHECK_PUSH_POP

#include "etl/vector.h"

void etl_error_func(const etl::exception& e);

#endif