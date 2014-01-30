#include <cstdlib>
#include <string.h>
#include "trm/ultracam.h"

/** Call back function for getting data from server with cURL.
 * This handles reallocation of memory as the buffer size grows and also
 * allows pre-allocation for efficiency when you have an idea of how much
 * data will be passed back.
 */

extern "C" {

  size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *stream){
    register size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *) stream;

    // Only reallocate when necessary
    if(mem->posn + realsize > mem->size)
      mem->memory = (char *)realloc(mem->memory, mem->size + realsize);

    // Copy data starting at posn
    if(mem->memory) {
      memcpy(&(mem->memory[mem->posn]), ptr, realsize);
      mem->size += realsize;
      mem->posn += realsize;
    }
    return realsize;
  }
}



