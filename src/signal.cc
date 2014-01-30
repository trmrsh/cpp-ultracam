#include "trm/signal.h"

//! Function for getting data from server
extern "C" {
    int global_ctrlc_set = 0;
    void signalproc(int signum){
        global_ctrlc_set = 1;
    }
}

