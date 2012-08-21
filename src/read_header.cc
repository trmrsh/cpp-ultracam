#include <deque>
#include "trm_subs.h"
#include "trm_format.h"
#include "trm_constants.h"
#include "trm_date.h"
#include "trm_time.h"
#include "trm_ultracam.h"
#include "trm_constants.h"

// Following are bit masks associated with the Meinberg GPS

/* Bit masks used with both PCPS_TIME_STATUS and PCPS_TIME_STATUS_X */
#define PCPS_FREER     0x01  /* DCF77 clock running on xtal, GPS receiver has not verified its position */
#define PCPS_DL_ENB    0x02  /* daylight saving enabled */
#define PCPS_SYNCD     0x04  /* clock has sync'ed at least once after pwr up */
#define PCPS_DL_ANN    0x08  /* a change in daylight saving is announced */
#define PCPS_UTC       0x10  /* a special UTC firmware is installed */
#define PCPS_LS_ANN    0x20  /* leap second announced, (requires firmware rev. REV_PCPS_LS_ANN_...) */
#define PCPS_IFTM      0x40  /* the current time was set via PC, (requires firmware rev. REV_PCPS_IFTM_...) */
#define PCPS_INVT      0x80  /* invalid time because battery was disconn'd */


/* Bit masks used only with PCPS_TIME_STATUS_X */

#define PCPS_LS_ENB      0x0100  /* current second is leap second */
#define PCPS_ANT_FAIL    0x0200  /* antenna failure */

/* The next two bits are used only if the structure */
/* PCPS_HR_TIME contains a user capture event */
#define PCPS_UCAP_OVERRUN      0x2000  /* events interval too short */
#define PCPS_UCAP_BUFFER_FULL  0x4000  /* events read too slow */

/*
 * Immediately after a clock has been accessed, subsequent accesses
 * are blocked for up to 1.5 msec to give the clock's microprocessor
 * some time to decode the incoming time signal.
 * The flag below is set if a program tries to read the PCPS_HR_TIME
 * during this interval. In this case the read function returns the
 * proper time stamp which is taken if the command byte is written,
 * however, the read function returns with delay.
 * This flag is not supported by all clocks.
 */
#define PCPS_IO_BLOCKED        0x8000

// little structure to save data relevant to the blue co-add option
struct Blue_save { 
    Blue_save(const Subs::Time& time, float expose, bool reliable){
        this->time     = time;
	this->expose   = expose;
	this->reliable = reliable;
    }
    Subs::Time time;
    float expose;
    bool reliable;
};

/**
 * Interpret the ULTRACAM header info. This is the routine that handles all the ULTRACAM
 * timing stuff. It carries out byte swapping depending upon the endian-ness of the machine.
 * \param buffer        pointer to start of header buffer
 * \param serverdata    data from the XML file needed for interpreting the times. This will be slightly
 *                      modified to reflect whether the data is affected by the timestamping bug of Dec 2004
 * \param timing        all the timing info derived from the header (returned)
 */

void Ultracam::read_header(char* buffer, const Ultracam::ServerData& serverdata, Ultracam::TimingInfo& timing){

    // In Feb 2010, format changed. Spot by testing for the version number, issue a warning
    int format;
    if(serverdata.instrument == "ULTRASPEC" && serverdata.version == -1){
        // temporary fix for new ULTRASPEC stuff
        format = 2;
    }else if(serverdata.version == -1 || serverdata.version == 70514 || serverdata.version == 80127){
        format = 1;
    }else if(serverdata.version == 100222 || serverdata.version == 110921 || serverdata.version == 111205 || serverdata.version == 120716 || serverdata.version == 120813){
        format = 2;
    }else{
        std::cerr << "WARNING: unrecognized version number in read_header.cc = " << serverdata.version << std::endl;
        std::cerr << "Program will continue, but there are highly likely to be problems with timing and other aspects." << std::endl;
        std::cerr << "Will assume post-Feb 2010, pre-Sep 2011 format #2" << std::endl;
        format = 2;
    }
    
    // The raw data files are written on a little-endian (linux) machine. Bytes
    // must be swapped if reading on big-endian machines such as Macs
    const bool LITTLE = Subs::is_little_endian();
    
    // Union used for byte swapping
    union IntRead{
	char c[4];
	int32_t i;
	uint32_t ui;
	uint16_t usi;
	int16_t si;
    } intread;

    // Read format-specific info
    bool reliable = true;  // is time reliable?
    std::string reason = "";
    int nsatellite = 0;
    unsigned int nsec = 0, nnanosec = 0, tstamp = 0;
    if(format == 1){

	// Number of seconds
	if(LITTLE){
	    intread.c[0] = buffer[9];
	    intread.c[1] = buffer[10];
	    intread.c[2] = buffer[11];
	    intread.c[3] = buffer[12];
	}else{
	    intread.c[3] = buffer[9];
	    intread.c[2] = buffer[10];
	    intread.c[1] = buffer[11];
	    intread.c[0] = buffer[12];
	}
	nsec = intread.ui;
    
	// number of nanoseconds
	if(LITTLE){
	    intread.c[0] = buffer[13];
	    intread.c[1] = buffer[14];
	    intread.c[2] = buffer[15];
	    intread.c[3] = buffer[16];
	}else{
	    intread.c[3] = buffer[13];
	    intread.c[2] = buffer[14];
	    intread.c[1] = buffer[15];
	    intread.c[0] = buffer[16];
	}    
	nnanosec = intread.i;
	nnanosec = format == 1 ? intread.i : 100*intread.i;

        // number of satellites. -1 indicates no GPS, and thus times generated from
	// when software loaded into kernel. Useful for relative times still.
	if(LITTLE){
	    intread.c[0] = buffer[21];
	    intread.c[1] = buffer[22];
	}else{
	    intread.c[1] = buffer[21];
	    intread.c[0] = buffer[22];
	}
	nsatellite = int(intread.si);
	if(nsatellite <= 2){
	    reason = "too few = " + Subs::str(nsatellite) +  " satellites";
	    std::cerr << "WARNING, time unreliable: " << reason << std::endl;
	    reliable = false;
	}

    }else if(format == 2){

        if(LITTLE){
	    intread.c[0] = buffer[8];
	    intread.c[1] = buffer[9];
	    intread.c[2] = buffer[10];
	    intread.c[3] = buffer[11];
	}else{
	    intread.c[3] = buffer[8];
	    intread.c[2] = buffer[9];
	    intread.c[1] = buffer[10];
	    intread.c[0] = buffer[11];
	}

	if(intread.ui*serverdata.time_units != serverdata.expose_time)
	    std::cerr << "WARNING: XML expose time does not match time in timing header " 
		      << intread.ui*serverdata.time_units << " vs " << serverdata.expose_time << std::endl;

	// Number of seconds
	if(LITTLE){
	    intread.c[0] = buffer[12];
	    intread.c[1] = buffer[13];
	    intread.c[2] = buffer[14];
	    intread.c[3] = buffer[15];
	}else{
	    intread.c[3] = buffer[12];
	    intread.c[2] = buffer[13];
	    intread.c[1] = buffer[14];
	    intread.c[0] = buffer[15];
	}
	nsec = intread.ui;

	// number of nanoseconds
	if(LITTLE){
            intread.c[0] = buffer[16];
	    intread.c[1] = buffer[17];
	    intread.c[2] = buffer[18];
	    intread.c[3] = buffer[19];
	}else{
	    intread.c[3] = buffer[16];
	    intread.c[2] = buffer[17];
	    intread.c[1] = buffer[18];
	    intread.c[0] = buffer[19];
	}    
	nnanosec = 100*intread.i;

	if(LITTLE){
	    intread.c[0] = buffer[24];
	    intread.c[1] = buffer[25];
	}else{
	    intread.c[1] = buffer[24];
	    intread.c[0] = buffer[25];
	}
	tstamp = intread.usi;

	// Report timing information. Report a single problem.
	if(reliable && (tstamp & PCPS_ANT_FAIL)){
	    reason = "GPS antenna failure";
	    std::cerr << "WARNING, time unreliable: " << reason << std::endl;
	    reliable = false;
	}
	if(reliable && (tstamp & PCPS_INVT)){
	    reason = "GPS battery disconnected";
	    std::cerr << "WARNING, time unreliable: " << reason << std::endl;	
	    reliable = false;
	}
	if(reliable && !(tstamp & PCPS_SYNCD)){
	    reason = "GPS clock not yet synced since power up";
	    std::cerr << "WARNING, time unreliable: " << reason << std::endl;
	    reliable = false;
	}
	if(reliable && (tstamp & PCPS_FREER)){
	    reason = "GPS receiver has not verified its position"; 
	    std::cerr << "WARNING, time unreliable: " << reason << std::endl;
	    reliable = false;
	}

	/*
          if(tstamp & PCPS_DL_ENB)   std::cerr << "Daylight saving enabled" << std::endl;
          if(tstamp & PCPS_DL_ANN)   std::cerr << "Change in daylight saving announced" << std::endl;
          if(tstamp & PCPS_UTC)      std::cerr << "Special UTC firmware installed" << std::endl;
          if(tstamp & PCPS_LS_ANN)   std::cerr << "Leap second announced" << std::endl;
          if(tstamp & PCPS_IFTM)     std::cerr << "Current time set via PC" << std::endl;
          if(tstamp & PCPS_LS_ENB)   std::cerr << "Current second is a leap second" << std::endl;
	*/
    }

    // Frame number. First one = 1
    if(LITTLE){
	intread.c[0] = buffer[4];
	intread.c[1] = buffer[5];
	intread.c[2] = buffer[6];
	intread.c[3] = buffer[7];
    }else{
	intread.c[3] = buffer[4];
	intread.c[2] = buffer[5];
	intread.c[1] = buffer[6];
	intread.c[0] = buffer[7];
    }
    int frame_number = int(intread.ui);

    // is the u-band junk data?
    // Changed from 3rd to 4th bit in Feb 2010 (Dave Atkinson)
    bool bad_blue = (serverdata.nblue > 1) && ((format == 1 && (buffer[0] & 1<<3)) || 
					       (format == 2 && (buffer[0] & 1<<4))); 

    // Flag so that some things are only done once
    static bool first = true;
    static Subs::Format form(8);

    // Now translate date info. All a bit complicated owing to various
    // bugs in the system early on. Date has no meaning when nsat=-1
    // in this case, set the date to an impossible one

    Subs::Time gps_timestamp;  // This is the raw gps timestamp
    static Subs::Time old_gps_timestamp;  // This is the raw gps timestamp of the previous frame
    static double vclock_frame = 0;  // Number of seconds taken to shift one row.
    Subs::Time ut_date;   // this will be the time at the centre of the exposure
    static int old_frame_number = -1000;     // frame number stored in previous call
    float exposure_time = 0.f;          // length of exposure

    // Clock board was changed in July 2003 and this resulted in the wrong sense of bit
    // change for the timestamps. Thus the timing code has to change in between this date
    // and the date when it was fixed in early 2005. Basically the timestamps started to
    // occur immediately after readout as opposed to immediately prior to the frame shift
    // into the masked region.

    // This happened again when the GPS changed in March 2010. Extract from e-mail from 
    // Dave Atkinson 11 June 2010:
    //
    // Apps3,4,5b
    // Loop:
    // CLEAR CCD
    // TIME STAMP +ve edge
    // EXPOSE DELAY
    // FRAME TRANSFER
    // READ CCD
    // *TIME STAMP -ve edge
    //
    // Apps5,6,7,8,9
    // CLEAR CCD ONCE
    //
    // Loop:
    // EXPOSE DELAY
    // FRAME TRANSFER
    // TIME STAMP +VE EDGE
    // READ CCD
    // *TIME STAMP -ve EDGE
    //
    // * timestamp captures are happening here on -ve edge
    // rather than the +ve edges as expected
    //

    // Dates to define when change occurred. "Default" time stamps occurred
    // prior to timestamp_change1 and then toggled thereafter.
    const Subs::Time clockboard_change(1,Subs::Date::Aug,2003); 

    const Subs::Time timestamp_change1(1,Subs::Date::Aug,2003); 
    const Subs::Time timestamp_change2(1,Subs::Date::Jan,2005); 
    const Subs::Time timestamp_change3(1,Subs::Date::Mar,2010); 

    const Subs::Time ultraspec_change1(21,Subs::Date::Sep,2011); 

    if(format == 1 && nsatellite == -1){

	gps_timestamp.set(1,Subs::Date::Jan,2000,0,0,0.);
	gps_timestamp.add_second(double(nsec) + double(nnanosec)/1.e9);

	if(first){
	    std::cerr << "WARNING: no satellites, so the date unknown. In this case the timing settings cannot" << std::endl;
	    std::cerr << "be determined. Values for > July 2003 will be used by default. If this is not right" << std::endl;
	    std::cerr << "and timing matters for these data, please contact Vik Dhillon or Tom Marsh." << std::endl;
	}

	if(serverdata.v_ft_clk > 127){
	    vclock_frame = 6.e-9*(40+320*(serverdata.v_ft_clk - 128));
	}else{
	    vclock_frame = 6.e-9*(40+40*serverdata.v_ft_clk);
	}

    }else{

	// For several modes we need to store information from earlier frames to get correct times.
	// We also need to do this to correct May 2002 times.
	if(serverdata.which_run == Ultracam::ServerData::MAY_2002 && format == 1){
      
	    // The first ULTRACAM run in May 2002 did not have date info. Offset from start of week
	    // which was 0 UT on 12 May 2002
	    gps_timestamp.set(12,Subs::Date::May,2002,0,0,0.);
	    gps_timestamp.add_second(double(nsec) + double(nnanosec)/1.e9);
      
	    // For times which run over the next week
	    if(gps_timestamp < Subs::Time(16,Subs::Date::May,2002)) gps_timestamp.add_hour(168.);
      
	    // Correct 10 second error that affected May 2002 run, but only if we are running frame by frame
	    // Cannot fix first whatever; a fairly rare problem luckily
	    if(frame_number == old_frame_number+1 && gps_timestamp < old_gps_timestamp) gps_timestamp.add_second(10.);
      
	    // The first night of the May run had a short vertical clock that caused problems
	    if(gps_timestamp < Subs::Time(17, Subs::Date::May, 2002, 12.)){
		vclock_frame = 10.0e-6; 
	    }else{
		vclock_frame = 24.46e-6; 
	    }
      
	}else{
      
	    // Starting with the second night of the September 2002 run, we have date
	    // information. We try to spot rubbish dates by their silly year      

	    unsigned char day_of_month = 0, month_of_year = 0;
	    unsigned short int year = 0;

	    if(format == 1){
		day_of_month  = buffer[17];
		month_of_year = buffer[18];

		if(LITTLE){
		    intread.c[0] = buffer[19];
		    intread.c[1] = buffer[20];
		}else{
		    intread.c[0] = buffer[20];
		    intread.c[1] = buffer[19];
		}
		year = intread.usi;

	    }else if(format == 2){
		day_of_month  = 1;
		month_of_year = 1;
		year          = 1970;
	    }else{
		std::cerr << "WARNING: could not recognize format = " << format << " when trying to establish date in read_header" << std::endl;
	    }
      
	    // hack for partial fix with day and month ok but not year
	    if(format == 1 && month_of_year == 9 && year == 263) year = 2002;
      
	    if(format == 1 && year < 2002){
		gps_timestamp.set(8,Subs::Date::Sep,2002,0,0,0.);
		gps_timestamp.add_second(double(nsec) + double(nnanosec)/1.e9);

	    }else{
	
		if(format == 1 && month_of_year == 9 && year == 2002){
	  
		    // Yet another special case!! day numbers seem problematic in the
		    // September run, but seem to be correct to within 1 day. So just try 
		    // to use them to indicate which week we are in, refining the final number
		    // using 'nsec'
		    // The problem is that the day number seems to change, but not exactly on
		    // midnight UT, leaving times near midnight in a bit of a mess. We recover
		    // from this using 'nsec', the number of seconds from the start of the week
		    // (saturday/sunday boundary), but then we have to be careful to identify
		    // this correctly. The next bit of code does this.
	  
		    Subs::Time first_week(8,Subs::Date::Sep,2002,0,0,0.);
		    Subs::Time test_time(day_of_month,month_of_year,year,0,0,0.);
		    double secdiff = test_time - first_week;
		    int    nweek   = int(secdiff/Constants::IDAY/7.);
		    double days    = (secdiff - Constants::IDAY*7*nweek)/Constants::IDAY;
	  
		    if(days > 3.5 && nsec < 2*Constants::IDAY){
			// 'days' indicates a date late in the week while nsec indicates
			// early. This means 'nweek' is one too small
			nweek++;
		    }else if(days < 3.5 && nsec > 5*Constants::IDAY){
			// 'days' indicates a date early in the week while nsec indicates
			// late. This means 'nweek' is one too large (not sure this case ever arises,
			// but here for safety).
			nweek--;
		    }
	  
		    // OK, now have correct week. Set gps_time to the start of the week
		    gps_timestamp = first_week;
		    gps_timestamp.add_day(7*nweek);
	  
		    // Now just add in the fraction of the week
		    gps_timestamp.add_second(double(nsec) + double(nnanosec)/1.e9);
	
		}else if(format == 1){ 
 
		    // nsec represents the number of seconds since the start of the week, but
		    // the date is the date of the relevant day therefore we set the date to be the date measured 
		    // and then add the number of seconds modulo 86400. This can lead to an error just after
		    // midnight when the date is taken before midnight while the times are after. This is corrected
		    // down below.
		    gps_timestamp.set(day_of_month,month_of_year,year,0,0,0.);

		    // We have the  right date, now just add in the fraction of the day
		    gps_timestamp.add_second(double(nsec % Constants::IDAY) + double(nnanosec)/1.e9);

		}else if(format == 2){

		    // This format started in Feb 2010 before the NTT run with a new GPS thingy. 
		    // nsec in this case represents the number of seconds from the start of 
		    // "unix time", 1 Jan 1970
		    gps_timestamp.set(day_of_month,month_of_year,year,0,0,0.);
		    gps_timestamp.add_second(double(nsec) + double(nnanosec)/1.e9);

		}else{
		    std::cerr << "WARNING: could not recognize format = " << format << " when trying to establish GPS time in read_header" << std::endl;
		}

		// Set the vertical clock time. Have to account for the change of
		// clock board that occured in July 2003 which altered the conversion
		// formulae.

		if(gps_timestamp > clockboard_change){
		    if(serverdata.v_ft_clk > 127){
			vclock_frame = 6.e-9*(40+320*(serverdata.v_ft_clk - 128));
		    }else{
			vclock_frame = 6.e-9*(40+40*serverdata.v_ft_clk);
		    }
		}else{
		    if(serverdata.v_ft_clk > 127){
			vclock_frame = 6.e-9*(80+160*(serverdata.v_ft_clk - 128));
		    }else{
			vclock_frame = 6.e-9*(80+20*serverdata.v_ft_clk);
		    }
		}
	    }
	}
    }

    // 'midnight bug' corrector. Spot this by working out the day 
    // of the week from the seconds and the date. If they do not match, we add a day to 
    // the time. Extra % 7 added 14/05/2005 to cope with changes made before May 2005 VLT run

    if((gps_timestamp.int_day_of_week() + 1) % 7 == int((nsec / Constants::IDAY) % 7)){
	std::cerr << "WARNING: Midnight bug detected and corrected *****." << std::endl;
	gps_timestamp.add_day(1);
    }

    // We finally have a correct raw timestamp 'gps_timestamp'
    // Now we get onto working out the time at the centre of the exposures
    // which depends upon the mode employed and the particular run to some extent because of
    // of the bug introduced with the clock board change of summer 2003 which we only spotted
    // in Dec 2004. Decide how to fix times, accounting for inverting switch

    bool deftime = gps_timestamp < timestamp_change1 || (gps_timestamp > timestamp_change2 && gps_timestamp < timestamp_change3);
    timing.default_tstamp = (serverdata.timestamp_default  && deftime) || (!serverdata.timestamp_default && !deftime);

    // One-off variables that need saving
    static double clear_time;
    static double readout_time;
    static double frame_transfer;
    static std::deque<Subs::Time> gps_times;

    static std::deque<Blue_save> blue_times;

    // Clear old times and status flags if frame numbers not consecutive
    if(frame_number != old_frame_number + 1){
	gps_times.clear();
	blue_times.clear();
    }
  
    // Push current gps time onto front of deque. This ensures
    // that there will always be at least one time in the deque.
    // For clarity in what follows I consistently use gps_times[n]
    // even when n = 0, for which I could instead use gps_timestamp
    // gps_times[n] is the n-th previous timestamp to the current one
    gps_times.push_front(gps_timestamp);

    // Timing parameters from Vik
    //  const double INVERSION_DELAY = 110.;   // microseconds
    const double VCLOCK_STORAGE  = vclock_frame;   // microseconds
    const double HCLOCK          = 0.48;   // microseconds
    const double CDS_TIME_FDD    = 2.2;    // microseconds
    const double CDS_TIME_FBB    = 4.4;    // microseconds
    const double CDS_TIME_CDD    = 10.;    // microseconds
    const double SWITCH_TIME     = 1.2;    // microseconds

    // Ultraspec timing parameters from Naidu for old version, Vik for post
    // 21/09/2011 version. Frame transfer time is fixed.
    const double USPEC_FT_TIME = gps_timestamp < ultraspec_change1 ? 0.0067196 : 0.0149818;

    double cds_time = 10.;
    if(first){
	if(serverdata.instrument == "ULTRACAM"){
	    if(serverdata.gain_speed == "3293"){
		// 3293 == CDD in hex
		cds_time = CDS_TIME_CDD;
	    }else if(serverdata.gain_speed == "4027"){
		// 4027 == FBB in hex
		cds_time = CDS_TIME_FBB;
	    }else if(serverdata.gain_speed == "4061"){
		// 4061 == FDD in hex
		cds_time = CDS_TIME_FDD;
	    }else{
		std::cerr << "Unrecognised gain speed setting = " << serverdata.gain_speed << std::endl;
		std::cerr << "Recognised values are 3293==CDD, 4027==FBB, 4061==FDD" << std::endl;
		std::cerr << "Will set CDS time = to CDD time, but this may not be right" << std::endl;
		cds_time = CDS_TIME_CDD;
	    }
	}else if(serverdata.instrument == "ULTRASPEC"){
	    std::cerr << "Ultracam::read_header WARNING: timing for ULTRASPEC still to be worked out!!" << std::endl;
	    cds_time = 0.;
	}
    }

    const double VIDEO = SWITCH_TIME + cds_time;

    // OK now start on timing code
    if(serverdata.instrument == "ULTRACAM"  &&
       (serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_CLEAR || 
	serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_OVERSCAN ||
	serverdata.readout_mode == Ultracam::ServerData::WINDOWS_CLEAR)){

	// Never need more than 2 times
	if(gps_times.size() > 2) gps_times.pop_back(); 

	if(first){

	    std::cout << "#" << std::endl;
	    if(timing.default_tstamp)
		std::cout << "# Standard timing mode" << std::endl;
	    else
		std::cout << "# Non-standard timing mode" << std::endl;

	    std::cout << "# Exposure delay      = " << form(serverdata.expose_time) << " seconds" << std::endl;
      
	}

	if(timing.default_tstamp){

	    // Order of events: loop [clear, timestamp, expose, frame transfer, readout]
	    // Timestamp is placed at start of 'expose', so this is easy and accurate
	    ut_date = gps_times[0];
	    ut_date.add_second(serverdata.expose_time/2.);
	    exposure_time = serverdata.expose_time;

	}else{

	    // Order of events: loop [clear, expose, frame transfer, readout, timestamp]
	    // Timestamp in this case is placed between 'readout' and 'clear' so one must 
	    // backtrack to get to mid 'expose'. Reliability of this is unclear to me.

	    if(first){

		// Time taken to clear CCD
		clear_time   = (1033. + 1027)*vclock_frame;

		// Time taken to read CCD (assuming cdd mode) ?? needs generalising ??
		if(serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_CLEAR){
		    readout_time = (1024/serverdata.ybin)*(VCLOCK_STORAGE*serverdata.ybin + 536*HCLOCK + (512/serverdata.xbin+2)*VIDEO)/1.e6;
		}else if(serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_OVERSCAN){
		    readout_time = (1032/serverdata.ybin)*(VCLOCK_STORAGE*serverdata.ybin + 540*HCLOCK + (540/serverdata.xbin+2)*VIDEO)/1.e6;
		}else{
		    const Wind& lwin = serverdata.window[0];
		    const Wind& rwin = serverdata.window[1];
		    int nxu          = serverdata.xbin*rwin.nx;
		    int nxb          = rwin.nx;
		    int nyb          = rwin.ny;
		    int xleft        = lwin.llx;
		    int xright       = rwin.llx + nxu - 1;
		    int diff_shift   = abs(xleft - 1 - (1024 - xright) );
		    int num_hclocks  = (xleft - 1 > 1024 - xright) ? nxu + diff_shift + (1024 - xright) + 8 : nxu + diff_shift + (xleft - 1) + 8;
		    readout_time = nyb*(VCLOCK_STORAGE*serverdata.ybin + num_hclocks*HCLOCK + (nxb+2)*VIDEO)/1.e6;
		}

		// Frame transfer time
		frame_transfer = 1033.*vclock_frame;

		std::cout << "#" << std::endl;
		std::cout << "# Vertical clock time = " << form(vclock_frame) << " seconds" << std::endl;
		std::cout << "# Clear time          = " << form(clear_time)     << " seconds" << std::endl;
		std::cout << "# Frame transfer time = " << form(frame_transfer) << " seconds" << std::endl;
		std::cout << "# Exposure delay      = " << form(serverdata.expose_time) << " seconds" << std::endl;
		std::cout << "# Read time           = " << form(readout_time) << " seconds" << std::endl;

	    }

	    if(gps_times.size() == 1){

		// Case where we have not got a previous timestamp. Hop back over the 
		// readout and frame transfer and half the exposure delay
		ut_date = gps_times[0];
		ut_date.add_second(-frame_transfer-readout_time-serverdata.expose_time/2.);
		if(reliable){
		    reason = "cannot establish an accurate time without previous GPS timestamp";
		    std::cerr << "WARNING, time unreliable: " << reason  << std::endl;
		    reliable = false;
		}

	    }else{

		// Case where we have got previous timestamp is somewhat easier and perhaps
		// more reliable since we merely need to step forward over the clear time and
		// half the exposure time.
		ut_date = gps_times[1];
		ut_date.add_second(clear_time + serverdata.expose_time/2.);

	    }
	    exposure_time = serverdata.expose_time;
      
	}

    }else if(serverdata.instrument == "ULTRACAM" && 
	     (serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_NOCLEAR || 
	      serverdata.readout_mode == Ultracam::ServerData::WINDOWS)){

	// Never need more than 3 times
	if(gps_times.size() > 3) gps_times.pop_back(); 

	if(first){

	    // Time taken to move 1033 rows.
	    frame_transfer = 1033.*vclock_frame;

	    if(serverdata.readout_mode == Ultracam::ServerData::FULLFRAME_NOCLEAR){
		readout_time = (1024/serverdata.ybin)*(VCLOCK_STORAGE*serverdata.ybin + 536*HCLOCK + (512/serverdata.xbin+2)*VIDEO)/1.e6;

	    }else{
 
		readout_time = 0.;
	
		int xbin = serverdata.xbin;
		int ybin = serverdata.xbin;
		for(size_t np=0; np<serverdata.window.size(); np += 2){

		    int nx     = xbin*serverdata.window[np].nx;
		    int ny     = ybin*serverdata.window[np].ny;
			
		    int ystart = serverdata.window[np].lly;
		    int xleft  = serverdata.window[np].llx;
		    int xright = serverdata.window[np+1].llx + nx - 1;
	  
		    int ystart_m = np > 0 ? serverdata.window[np-2].lly : 1;
		    int ny_m     = np > 0 ? ybin*serverdata.window[np-2].ny : 0;
			
		    // Time taken to shift the window next to the storage area
		    double y_shift = np > 0 ? (ystart-ystart_m-ny_m)*VCLOCK_STORAGE : (ystart-1)*VCLOCK_STORAGE;
			
		    // Number of columns to shift whichever window is further from the edge of the readout
		    // to get ready for simultaneous readout.
		    int diff_shift   = abs(xleft - 1 - (1024 - xright) );

		    // Time taken to dump any pixels in a row that come after the ones we want.
		    // The '8' is the number of HCLOCKs needed to open the serial register dump gates
		    // If the left window is further from the left edge than the right window is from the
		    // right edge, then the diffshift will move it to be the same as the right window, and
		    // so we use the right window parameters to determine the number of hclocks needed, and
		    // vice versa.
		    int num_hclocks  = (xleft - 1 > 1024 - xright) ? nx + diff_shift + (1024 - xright) + 8 : nx + diff_shift + (xleft - 1) + 8;
			
		    // Time taken to read one line. The extra 2 is required to fill the video pipeline buffer
		    double line_read = VCLOCK_STORAGE*ybin + num_hclocks*HCLOCK + (nx/xbin+2)*VIDEO;
		
		    readout_time += y_shift + (ny/ybin)*line_read;
		}
		readout_time /= 1.e6;
	    }

	    std::cout << "#" << std::endl;
      
	    if(timing.default_tstamp)
		std::cout << "# Standard timing mode" << std::endl;
	    else
		std::cout << "# Non-standard timing mode" << std::endl;
            
	    std::cout << "# Vertical clock time = " << form(vclock_frame) << " seconds" << std::endl;
	    std::cout << "# Frame transfer time = " << form(frame_transfer) << " seconds" << std::endl;
	    std::cout << "# Exposure delay      = " << form(serverdata.expose_time) << " seconds" << std::endl;
	    std::cout << "# Readout time        = " << form(readout_time) << " seconds" << std::endl;
      
	}

	// For all except first frame, the actual exposure covers [readout+expose]

	if(timing.default_tstamp){

	    // Order of events: loop [expose, frame transfer, time stamp, readout]

	    if(frame_number == 1){
	
		// First frame of all is a special case. It has an exposure part but no
		// preceding read.
		ut_date = gps_times[0];

		ut_date.add_second(-frame_transfer-serverdata.expose_time/2.);
		exposure_time = serverdata.expose_time;
	
	    }else{
	
		if(gps_times.size() > 1){
	  
		    // If the stored parameters result from the previous call then we can get a good time.
		    double texp = gps_times[0] - gps_times[1] - frame_transfer;
		    ut_date = gps_times[1];
		    ut_date.add_second(texp/2.);
		    exposure_time = texp;
	  
		}else{
	  
		    // Do not have an earlier call to fall back on. Do the best that we can which
		    // is to skip back over the frame transfer that took place after the exposure
		    // and half the estimated exposure
		    double texp = readout_time + serverdata.expose_time;
		    ut_date = gps_times[0];

		    ut_date.add_second(-frame_transfer-texp/2.);

		    exposure_time    = texp;
		    if(reliable){
			reason = "cannot establish an accurate time without previous GPS timestamp";
			std::cerr << "WARNING, time unreliable: " << reason << std::endl;
			reliable = false;
		    }
		}
	    }

	}else{

	    // Order of events: loop [expose, frame transfer, readout, timestamp]

	    // Timestamp here is placed between 'read' and 'expose', skipping
	    // the expose at the start. It is more complex than you might think 
	    // owing to the skipped timestamp at the start which means that the correct
	    // timestamps take one cycle longer to come out than you might guess. Read
	    // the PDF document for detailed info. Do not monkey with this code without
	    // fully understanding the issues.
	    if(frame_number == 1){
	
		// First frame of all is a special case and we can't get a reliable value
		// Just try to skip back over the frame transfer and readout
		ut_date = gps_times[0];
		exposure_time = serverdata.expose_time;
		ut_date.add_second(-frame_transfer-readout_time-exposure_time/2.);
		if(reliable){
		    reason = "cannot establish an accurate time for first frame in this mode";
		    std::cerr << "WARNING, time unreliable: " << reason << std::endl;
		    reliable = false;
		}
	
	    }else{

		// We need in this case to backtrack two frames
		if(gps_times.size() > 2){
	  
		    // If the stored parameters result from the previous calls then we can get a good time.
		    double texp = gps_times[1] - gps_times[2] - frame_transfer;
		    ut_date = gps_times[1];
		    ut_date.add_second(serverdata.expose_time-texp/2.);
		    exposure_time = texp;

		}else if(gps_times.size() == 2){

		    // Only one back, use difference of most recent as estimate for one before
		    // probably not too bad, but must call it unreliable
		    double texp = gps_times[0] - gps_times[1] - frame_transfer;
		    ut_date = gps_times[1];
		    ut_date.add_second(serverdata.expose_time-texp/2.);
		    exposure_time = texp;
		    if(reliable){
			reason = "cannot establish an accurate time without at least 2 prior timestamps";
			std::cerr << "WARNING, time unreliable: " << reason  << std::endl;
			reliable = false;
		    }
	  
		}else{
	  
		    // No earlier call. Must rely on estimates
		    double texp = readout_time + serverdata.expose_time;
		    ut_date = gps_times[0];
		    ut_date.add_second(-texp-frame_transfer+serverdata.expose_time-texp/2.);
		    exposure_time = texp;
		    if(reliable){
			reason = "cannot establish an accurate time without at least a prior timestamp";
			std::cerr << "WARNING, time unreliable: " << reason  << std::endl;
			reliable = false;
		    }
		}
	    }
	}

    }else if(serverdata.instrument == "ULTRACAM" && serverdata.readout_mode == Ultracam::ServerData::DRIFT){

	// The trickiest of them all, but essentially boils down to
	// an nwins-1 shifted version of the case above

	static int nwins;

	// Calculate these just once
	if(first){

	    int xbin = serverdata.xbin;
	    int ybin = serverdata.xbin;
      
	    int nx     = xbin*serverdata.window[0].nx;
	    int ny     = ybin*serverdata.window[0].ny;
      
	    int ystart = serverdata.window[0].lly;
	    int xleft  = serverdata.window[0].llx;
	    int xright = serverdata.window[1].llx + nx - 1;

	    // Maximum number of windows in pipeline
	    nwins = int((1033./ny+1.)/2.);

	    double pipe_shift = (int)(1033.-(((2.*nwins)-1.)*ny));

	    // Time taken for (reduced) frame transfer, the main advantage of drift mode
	    frame_transfer = (ny + ystart - 1)*vclock_frame;
	  
	    // Number of columns to shift whichever window is further from the edge of the readout
	    // to get ready for simultaneous readout.
	    int diff_shift   = abs(xleft - 1 - (1024 - xright) );

	    // Time taken to dump any pixels in a row that come after the ones we want.
	    // The '8' is the number of HCLOCKs needed to open the serial register dump gates
	    // If the left window is further from the left edge than the right window is from the
	    // right edge, then the diffshift will move it to be the same as the right window, and
	    // so we use the right window parameters to determine the number of hclocks needed, and
	    // vice versa.
	    int num_hclocks  = (xleft - 1 > 1024 - xright) ? nx + diff_shift + (1024 - xright) + 8 : nx + diff_shift + (xleft - 1) + 8;
			
	    // Time taken to read one line. The extra 2 is required to fill the video pipeline buffer
	    double line_read = VCLOCK_STORAGE*ybin + num_hclocks*HCLOCK + (nx/xbin+2)*VIDEO;
		
	    readout_time = ((ny/ybin)*line_read + pipe_shift*VCLOCK_STORAGE)/1.e6;

	    std::cout << "#" << std::endl;

	    if(timing.default_tstamp)
		std::cout << "# Standard time stamp handling" << std::endl;
	    else
		std::cout << "# Non-standard time stamp handling" << std::endl;

	    std::cout << "# NWIN                         = " << nwins << std::endl;
	    std::cout << "# Vertical clock time          = " << form(vclock_frame)   << " seconds" << std::endl;
	    std::cout << "# Frame transfer time          = " << form(frame_transfer) << " seconds" << std::endl;
	    std::cout << "# Exposure delay               = " << form(serverdata.expose_time) << " seconds" << std::endl;
	    std::cout << "# Mean readout time (inc pipe) = " << form(readout_time)   << " seconds" << std::endl;

	}

	// Never need more than nwins+2 times
	if(int(gps_times.size()) > nwins+2) gps_times.pop_back(); 

	if(timing.default_tstamp){

	    // Pre board change or post-bug fix
	    if(int(gps_times.size()) > nwins){

		double texp = gps_times[nwins-1] - gps_times[nwins] - frame_transfer;
		ut_date = gps_times[nwins];
		ut_date.add_second(texp/2.);
		exposure_time = texp;

	    }else{

		// Set to silly value for easy checking
		ut_date = Subs::Time(1,Subs::Date::Jan,1900);
		exposure_time    = serverdata.expose_time;
		if(reliable){
		    reason = "too few stored timestamps";
		    std::cerr << "WARNING, time unreliable: " << reason << std::endl; 
		    reliable = false;
		}
	    }

	}else{

	    // Non-standard mode

	    if(int(gps_times.size()) > nwins+1){

		double texp = gps_times[nwins] - gps_times[nwins+1] - frame_transfer;
		ut_date = gps_times[nwins];
		ut_date.add_second(serverdata.expose_time-texp/2.);
		exposure_time = texp;
	
	    }else if(int(gps_times.size()) == nwins+1){

		double texp = gps_times[nwins-1] - gps_times[nwins] - frame_transfer;
		ut_date = gps_times[nwins];
		ut_date.add_second(serverdata.expose_time-texp/2.);
		exposure_time = texp;
		if(reliable){
		    reason = "too few stored timestamps";
		    std::cerr << "WARNING, time unreliable: " << reason << std::endl;
		    reliable = false;
		}

	    }else{
	  
		// Set to silly value for easy checking
		ut_date = Subs::Time(1,Subs::Date::Jan,1900);
		exposure_time    = serverdata.expose_time;
		if(reliable){
		    reason = "too few stored timestamps";
		    std::cerr << "WARNING, time unreliable: " << reason << std::endl; 
		    reliable = false;
		}
	    }
	}

    }else if(serverdata.instrument == "ULTRASPEC"){

	// Avoid accumulation of timestamps.
	if(gps_times.size() > 2) gps_times.pop_back(); 

        // 13/07/2012. I believe the readout sequences to be as
        // follows:
        //
        // Clear mode: CLR|EXP|TS|FT|READ|CLR|EXP|TS|FT|READ ..
        // Non-clear:  CLR|EXP|TS|FT|READ|EXP|TS|FT|READ ..
        // 
        // On non-clear mode, the accumulation time is from the start
        // of the read until the end of "exp" (the user-defined exposure delay)
        //
	// At one point I thought that post-21/09/2011 the order of TS and FT had
        // reversed and I had two options. From July 2012, when Dave Atkinson 
        // corrected this, we are back to the simpler code which makes no
        // distinction between pre- and post-21/09/2011. The only difference now
        // is the frame-transfer time.

        ut_date = gps_times[0];

        if(serverdata.l3data.en_clr || frame_number == 1){

            ut_date.add_second(-serverdata.expose_time/2.);
            exposure_time = serverdata.expose_time;
            
        }else if(gps_times.size() > 1){
            
            double texp = gps_times[0] - gps_times[1] - USPEC_FT_TIME;
            ut_date.add_second(-texp/2.);
            exposure_time = texp;
            
        }else{
            
            // Could be improved with an estimate of the read time
            ut_date.add_second(-serverdata.expose_time/2.);
            exposure_time = serverdata.expose_time;
            if(reliable){
                reason = "too few stored timestamps";
                std::cerr << "WARNING, time unreliable: " << reason << std::endl; 
                reliable = false;
            }
        }
        
    }
  
    // Save old values
    old_frame_number     = frame_number;
    old_gps_timestamp    = gps_timestamp;

    // Return some data
    timing.ut_date          = ut_date;
    timing.exposure_time    = exposure_time;
    timing.frame_number     = frame_number;
    timing.gps_time         = gps_timestamp;
    timing.format           = format;
    if(format == 1){
	timing.reliable     = reliable && nsatellite > 2;
	timing.nsatellite   = nsatellite;
    }else if(format == 2){
	timing.reliable      = reliable &&
	    (tstamp & PCPS_SYNCD) && !(tstamp & PCPS_INVT) && !(tstamp & PCPS_ANT_FAIL) && !(tstamp & PCPS_FREER);      
	timing.tstamp_status = tstamp;
    }
    timing.reason            = reason;
    timing.vclock_frame      = vclock_frame;
    timing.blue_is_bad       = bad_blue;

    if(serverdata.nblue > 1){

	// The mid-exposure time for the OK blue frames in this case is computed by averaging the 
	// mid-exposure times of all the contributing frames, if they are available.
	blue_times.push_front(Blue_save(ut_date, exposure_time, reliable));

	if(bad_blue){
	    // just pass through the standard time for the junk frames
	    timing.ut_date_blue       = timing.ut_date;
	    timing.exposure_time_blue = exposure_time;
	    timing.reliable_blue      = timing.reliable;
	}else{

	    // if any of the contributing times is unreliable, then so is the final time. This is
	    // also unreliable if any contributing frame times are missing. Time is calculated
            // as half-way point between start of first and end of last contributing exposure.
	    // Corrections are made if there are too few contributing exposures (even though the
	    // final value will still be flagged as unreliable

	    int    ncont  = std::min(serverdata.nblue, int(blue_times.size()));
	    double start  = blue_times[ncont-1].time.mjd() - blue_times[ncont-1].expose/Constants::DAY/2;
	    double end    = blue_times[0].time.mjd()       + blue_times[0].expose/Constants::DAY/2;
	    double expose = end - start;

	    // correct the times
	    bool ok = (ncont == serverdata.nblue);
	    if(!ok){
		expose *= serverdata.nblue/float(ncont);
		start   = end - expose;
	    }else{
		ok = blue_times[0].reliable && blue_times[ncont-1].reliable;
	    }
	    timing.ut_date_blue       = Subs::Time((start+end)/2.);
	    timing.exposure_time_blue = Constants::DAY*expose;
	    timing.reliable_blue      = ok;
	}

	// Avoid wasting memory storing past times
	if(int(blue_times.size()) > serverdata.nblue) blue_times.pop_back();
	    
    }else{
	timing.ut_date_blue       = timing.ut_date;
	timing.exposure_time_blue = exposure_time;
	timing.reliable_blue      = timing.reliable;
    }	
	
    first = false;
}
