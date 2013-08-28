#include <cstdlib>
#include "trm/ultracam.h"

// Call back function for getting data from server

size_t WriteFunction(void *ptr, size_t size, size_t nmemb, void *data){
  register int realsize = size * nmemb;
  MemStruct *mem = (MemStruct *) data;
  memcpy(&(mem->memory[mem->sofar]), ptr, realsize);
  mem->sofar += realsize;
  mem->memory[mem->sofar] = 0;
  return realsize;
}

