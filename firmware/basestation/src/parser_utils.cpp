#include "parser_utils.h"


/*
  Function for extracting a uint32 from a byte array, which starts at index start. 
  Be cautious to stay within the bounds of the array msg. 
  leftIncreasing = true means that the number 1234 is interpreted as
  1 * 256^3 + 2*256^2 + 3*256 + 4
  while leftIncreasing = false means that the number is interpreted as
  4 * 256^3 + 3*256^2 + 2*256 + 1
*/
// template<typename T> T msg_extract_uint(const byte * msg, uint8_t start, bool leftIncreasing){
  
//   // We construct the basis polynomial
//   uint8_t exponent = sizeof(T)-1;
//   uint64_t fac = leftIncreasing ? 1 << (exponent * 8) : 1; 
  
//   // if (leftIncreasing){
//   //   for (int idx = 0; idx < exponent; idx++){
//   //     fac *= 256;
//   //   }
//   // }
  
//   // We convert from base 256 to base 10. 
//   uint32_t result = 0;
//   for (uint8_t idx = start; idx < start + sizeof(T); idx++){
//     result += msg[idx]*fac;
//     if (leftIncreasing){
//       fac >>= 8;
//     } else {
//       fac <<= 8;
//     }
//   }
//   return result;
// }

uint32_t msg_extract_(const uint8_t * msg, uint8_t start, bool leftIncreasing = true){
  
  // We construct the basis polynomial
  uint64_t fac = 1;
  uint8_t exponent = sizeof(uint32_t)-1;
  if (leftIncreasing){
    for (int idx = 0; idx < exponent; idx++){
      fac *= 256;
    }
  }
  
  // We convert from base 256 to base 10. 
  uint32_t result = 0;
  for (uint8_t idx = start; idx < start + sizeof(uint32_t); idx++){
    result += msg[idx]*fac;
    if (leftIncreasing){
      fac /= 256;
    } else {
      fac *= 256;
    }
  }
  return result;
}


uint64_t msg_extract_uint64(const uint8_t * msg, uint8_t start, bool leftIncreasing = true){
  
  // We construct the basis polynomial
  uint64_t fac = 1;
  uint8_t exponent = sizeof(uint64_t)-1;
  if (leftIncreasing){
    for (int idx = 0; idx < exponent; idx++){
      fac *= 256;
    }
  }
  
  // We convert from base 256 to base 10. 
  uint32_t result = 0;
  for (uint8_t idx = start; idx < start + sizeof(uint64_t); idx++){
    result += msg[idx]*fac;
    if (leftIncreasing){
      fac /= 256;
    } else {
      fac *= 256;
    }
  }
  return result;
}
