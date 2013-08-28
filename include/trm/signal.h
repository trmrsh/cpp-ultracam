#ifndef TRM_SIGNAL_H
#define TRM_SIGNAL_H

// Small header to help with ctrl-C handling in rtplot. Unfortunately it
// has to be included in any routine using get_server_frame.cc

#include <csignal>

//! Function for getting data from server
extern "C" {
	extern int global_ctrlc_set;
	void signalproc(int signum);
}

#endif
