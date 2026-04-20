#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H
#include "Arduino.h"
#include "etl/vector.h"

/*
  Templates for inserting and extracting integers from byte arrays. 
*/

template <typename T>
T msg_extract_uint(const byte *msg, uint8_t start, bool leftOrdering)
{

  // We construct the basis polynomial
  uint8_t exponent = sizeof(T) - 1;
  // uint64_t fac = leftOrdering ? 1 << (exponent * 8) : 1;
  uint64_t fac = 1;
  if (leftOrdering){
    for (int idx = 0; idx < exponent; idx++){
      fac *= 256;
    }
  }


  // We convert from base 256 to base 10.
  T result = 0;
  for (uint8_t idx = start; idx < start + sizeof(T); idx++)
  {
    result += msg[idx] * fac;
    if (leftOrdering)
    {
      fac /= 256;
    }
    else
    {
      fac *= 256;
    }
  }
  return result;
}

template<typename T, std::size_t S> uint8_t msg_insert_uint(byte (&msg)[S], T number, uint8_t start, uint8_t msgSize, uint8_t& next_offset, bool leftOrdering = true){
  /*
    Error codes:
      0 - Number inserted correctly
      1 - Inserted number would exceed message buffer
  */

  // We construct the basis polynomial
  uint64_t fac = 1;
  
  if (start + sizeof(T) > msgSize){
    return 1;
  }
  for (int idx = 1; idx < sizeof(T); idx++){
    fac *= 256;
  }
  
  for (int idx = start; idx < start + sizeof(T); idx++){
    uint8_t beta = number/fac;
    number = number % fac;
    fac /= 256;
    if (leftOrdering){
      msg[idx] = beta;
    } else {
      msg[(start + sizeof(T)) - (idx - start)-1] = beta;
    }
  }

  // Update next offset
  next_offset = start + sizeof(T);

  return 0;
}

/*
 ints are like uint in space requirements, but do need an additional char to store the sign
 So, assuming left ordering:
    P1234 = 1*256^3 + 2*256^2 + 3*256 + 4
  while:
    N1234 = -(1*256^3 + 2*256^2 + 3*256 + 4)
*/
template<typename T, std::size_t S> uint8_t msg_insert_int(byte (&msg)[S], T number, uint8_t start, uint8_t msgSize, uint8_t& next_offset, bool leftOrdering = true){
  /*
    Error codes:
      0 - Number inserted correctly
      1 - Inserted number would exceed message buffer
  */

  // We construct the basis polynomial
  uint8_t state;
  if (start + sizeof(T) +1 > msgSize){
    return 1;
  }

  next_offset=start+1; // We reserve one byte for the sign

  if (number > 0){
    msg[start] = 'P';
    state = msg_insert_uint(msg, number, next_offset, msgSize, next_offset, leftOrdering);
  } else if (number < 0){
    msg[start] = 'N';
    state = msg_insert_uint(msg, abs(number), next_offset, msgSize, next_offset, leftOrdering);
  } else {
    msg[start] = 'P';
    for (uint8_t idx = 0; idx < sizeof(T); idx++){
      msg[start+idx+1] = 0;
    }
    next_offset += sizeof(T);
    return 0;
  }

  return state;
}

#endif