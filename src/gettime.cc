// Make sure that we can access > 2^31 bytes

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <iostream>
#include <istream>
#include <sstream>
#include <fstream>
#include <string>
#include <ctime>

/*
!!begin
!!title   Gets the start and end times of an ULTRACAM file
!!author  T.R. Marsh
!!created May 2005
!!revised 05 May 2006
!!root    gettime
!!index   gettime
!!descr   gets the start and end times of an ULTRACAM file
!!css   style.css
!!class   Programs
!!class   Information
!!head1   gettime - gets the start and end times of an ULTRACAM file

!!emph{gettime} is a simple program to extract the start and end times from an ULTRACAM file.
It is a simple program that makes no use of the standard ULTRACAM libraries so that it can be linked 
standalone (e.g. g++ -o gettime gettime.cc) so that it can be used without requiring the full pipeline
to be installed so that it can be used for generation of log files. This means it does not use the standard pipeline 
input and default saving. This program can handle >2GB files but this will need g++ 3.4 series or higher.

!!head2 Invocation

gettime run

!!head2 Command line arguments

!!table

!!arg{run}{The name of a run as in 'run012'. The equivalent .dat and .xml files must be available or the program will die.}

!!table

!!end

*/

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

bool timing(char* buffer, bool may2002, int format, 
	    unsigned char& day_of_month, unsigned char& month_of_year, unsigned short int& year, 
	    int& hour, int& minute, int& second, int& millisec, int& nsatellite);

int main(int argc, char* argv[]){

    if(argc != 2){
	std::cerr << "usage: gettime run" << std::endl;
	exit(EXIT_FAILURE);
    }
    
    // Read the XML
    std::string xml(argv[1]);
    xml += ".xml";
    
    std::ifstream xin(xml.c_str(), std::ios::binary);
    if(!xin){
	std::cerr << "Could not open open " << xml << " for reading" << std::endl;
	exit(EXIT_FAILURE);
    }
    
    int n = 1;
    std::string line;
    off_t framesize, hwords, edelay;
    bool may2002 = true;
    std::size_t ipos;
    int format = 1;
    float time_units = 0.001;
    while(getline(xin, line)){
	ipos = line.find("framesize");
	if(ipos != std::string::npos){
	    std::string sframesize = line.substr(ipos+11);
	    ipos = sframesize.find("\"");
	    sframesize = sframesize.substr(0,ipos);
	    std::istringstream istr(sframesize);
	    istr >> framesize;
	    if(!istr){
		std::cerr << "Failed to read the framesize so cannot find last timestamp" << std::endl;
		exit(EXIT_FAILURE);
	    }
	}
	ipos = line.find("headerwords");
	if(ipos != std::string::npos){
	    std::string shwords = line.substr(ipos+13);
	    ipos = shwords.find("\"");
	    shwords = shwords.substr(0,ipos);

	    std::istringstream istr(shwords);
	    istr >> hwords;
	    if(!istr){
		std::cerr << "Failed to read headerwords" << std::endl;
		exit(EXIT_FAILURE);
	    }
	    if(hwords == 16) {
		may2002 = false;
		format = 2;
	    }
	}

	ipos = line.find("EXPOSE_TIME");
	if(ipos != std::string::npos){
	    ipos = line.find("value=");
	    if(ipos != std::string::npos){
		std::string val = line.substr(ipos+7);
		ipos = val.find("\"");
		val = val.substr(0,ipos);

		std::istringstream istr(val);
		istr >> edelay;
		if(!istr){
		    std::cerr << "Failed to read exposure delay" << std::endl;
		    exit(EXIT_FAILURE);
		}
	    }
	}

	ipos = line.find("<user>");
	if(ipos != std::string::npos) time_units = 0.0001;
	ipos = line.find("VERSION");
	if(ipos != std::string::npos) may2002 = false;
	ipos = line.find("V_FT_CLK");
	if(ipos != std::string::npos) may2002 = false;
    } 
    xin.close();

    if(may2002)
	std::cout << "These data are from May 2002" << std::endl;
    else
	std::cout << "These data are not from May 2002" << std::endl;

    if(format == 2)
	std::cout << "These data are post-March 2010" << std::endl;
    else
	std::cout << "These data are pre-March 2010" << std::endl;

    std::cout << "Exposure delay = " << time_units*edelay << " seconds." << std::endl;

    // Now the data
    std::string data(argv[1]);
    data += ".dat";

    std::ifstream fin(data.c_str(), std::ios::binary);
    if(!fin){
	std::cerr << "Could not open open " << data << " for reading" << std::endl;
	exit(EXIT_FAILURE);
    }

    int nread = format == 1 ? 24 : 32;
    char buffer[32];
    if (!fin.read(buffer, nread)){
	std::cerr << "Error while trying to read first " << nread << " timing bytes from " << data << std::endl;
	fin.close();
	exit(EXIT_FAILURE);
    }

    unsigned char day_of_month1, month_of_year1;
    unsigned short int year1;
    int hour1, minute1, second1, millisec1, nsatellite;
    bool first_time_ok = timing(buffer, may2002, format, day_of_month1, month_of_year1, year1, hour1, minute1, second1, millisec1, nsatellite);

    // Now find the end of the file and work out the number of frames
    fin.seekg(0, std::ios::end);
    if(!fin){
	std::cerr << "Failed to find the end of the data file" << std::endl;
	fin.close();
	exit(EXIT_FAILURE);
    }
    off_t  nfile = fin.tellg() / framesize;
    off_t  ngood = nfile;
    std::cout << "Number of frames = " << nfile << std::endl;

    int ntime = 1;
    double save;
    while(!first_time_ok && ntime < nfile){
	ntime++;
	fin.seekg(framesize*(ntime-1));

	if (!fin.read(buffer, 24)){
	    std::cerr << "Error while trying to read first 24 timing bytes from frame " << ntime << " of " << data << std::endl;
	    fin.close();
	    exit(EXIT_FAILURE);
	}

	if(!(first_time_ok = timing(buffer, may2002, format, day_of_month1, month_of_year1, year1, hour1, minute1, second1, millisec1, nsatellite)) && ntime == 2)
	    save = 3600.*hour1 + 60.*minute1 + second1 + millisec1/1000.; 
    }

	
    if(!first_time_ok){
	std::cerr << "Could not get a valid start time for " << data << std::endl;
	if(nfile > 1){
	    double rtime = (3600.*hour1 + 60.*minute1 + second1 + millisec1/1000. - save);
	    std::cout << "Run length = " << rtime << " seconds, sample time = " << rtime/(nfile-1) << " seconds/frame" << std::endl;
	    std::cout << "Reliability of this estimate unknown" << std::endl;
	}else{
	    std::cerr << "Only 1 frame; cannot estimate cycle time or a meaningful run length" << std::endl;
	} 
	
	fin.close();
	exit(EXIT_FAILURE);
    }
    double dhour1 = hour1 + minute1/60. + second1/3600 + millisec1/3600000;

    std::cout << "Run = " << argv[1] << std::endl;
    char output[1024];
    sprintf(output, "UT at start = %02d/%02d/%d, %02d:%02d:%02d.%03d\n", day_of_month1, month_of_year1, year1, hour1, minute1, second1, millisec1);
    std::cout << output;

    if(nfile < 1){
	std::cerr << "No valid data in " << data << std::endl;
	fin.close();
	exit(EXIT_FAILURE);
    }

    off_t last;
    unsigned char day_of_month2, month_of_year2;
    unsigned short int year2;
    int hour2, minute2, second2, millisec2;
 
    // Look for last time
    fin.seekg(framesize*(ngood-1));
    if (!fin.read(buffer, 24)){
	std::cerr << "Error while trying to read 24 timing bytes of frame " << ngood << " from " << data << std::endl;
	fin.close();
	exit(EXIT_FAILURE);
    }

    // check it    
    bool good = false;
    if((good = timing(buffer, may2002, format, day_of_month2, month_of_year2, year2, hour2, minute2, second2, millisec2, nsatellite))){
      
	double dhour2 = hour2 + minute2/60. + second2/3600 + millisec2/3600000;
	
	good = !(
	    (year2 < year1) || 
	    (year2 == year1 && month_of_year2 < month_of_year1) ||
	    (year2 == year1 && month_of_year2 == month_of_year1 && day_of_month2 < day_of_month1) ||
	    (year2 == year1 && month_of_year2 == month_of_year1 && day_of_month2 == day_of_month1 && dhour2 < dhour1)
	    );
	
    }

    // some runs, especially fast ones, have failures in the timestamps towards
    // the end. This part checks for this and then attempts to track down a good 
    // timestamp by binary chopping as a linear search can be very slow.

    if(!good){
	off_t  n1 = ntime;
	off_t  n2 = ngood;
	off_t  n  = (n1+n2)/2;

	while(n > n1 && !good){

	    fin.seekg(framesize*(n-1));
	    if (!fin.read(buffer, 24)){
		std::cerr << "Error while trying to read 24 timing bytes of frame " << n << " from " << data << std::endl;
		fin.close();
		exit(EXIT_FAILURE);
	    }
    
	    if((good = timing(buffer, may2002, format, day_of_month2, month_of_year2, year2, hour2, minute2, second2, millisec2, nsatellite))){
      
		double dhour2 = hour2 + minute2/60. + second2/3600 + millisec2/3600000;
		
		good = !(
		    (year2 < year1) || 
		    (year2 == year1 && month_of_year2 < month_of_year1) ||
		    (year2 == year1 && month_of_year2 == month_of_year1 && day_of_month2 < day_of_month1) ||
		    (year2 == year1 && month_of_year2 == month_of_year1 && day_of_month2 == day_of_month1 && dhour2 < dhour1)
		    );
		
	    }

	    if(good)
		n1 = n;
	    else
		n2 = n;
	    n = (n1+n2)/2;
	}
	ngood = n - (ntime - 1);
    }else{
	ngood -= (ntime - 1);
    }

    sprintf(output, "UT at end   = %02d/%02d/%d, %02d:%02d:%02d.%03d\n", day_of_month2, month_of_year2, year2, hour2, minute2, second2, millisec2);
    std::cout << output;

    double length = 3600.*(hour2-hour1) + 60.*(minute2-minute1) + (second2-second1) + 1.e-3*(millisec2-millisec1);
    if(day_of_month1 != day_of_month2)
	length += 86400;
  
    std::cout << "Number of good frames = " << ngood << std::endl;
    if(ngood > 1)
	std::cout << "OK run length = " << length << " seconds, sample time = " << length/(ngood-1) << " seconds/frame " << std::endl;
    std::cout << "Number of bad frames  = " << nfile - ngood << std::endl;

    fin.close();

    exit(EXIT_SUCCESS);
}

bool timing(char* buffer, bool may2002, int format, unsigned char& day_of_month, unsigned char& month_of_year, unsigned short int& year, 
	    int& hour, int& minute, int& second, int& millisec, int& nsatellite){

    const int SECONDS_IN_A_DAY = 86400;

    union IntRead{
	char c[4];
	int32_t  i;
	uint16_t usi;
	int16_t si;
    } intread;

    // Number of seconds
    int nsec = 0, nnanosec = 0;
    bool reliable = false;
    if(format == 1){
	intread.c[0] = buffer[9];
	intread.c[1] = buffer[10];
	intread.c[2] = buffer[11];
	intread.c[3] = buffer[12];
	nsec = intread.i;

	intread.c[0] = buffer[13];
	intread.c[1] = buffer[14];
	intread.c[2] = buffer[15];
	intread.c[3] = buffer[16];
	nnanosec = intread.i;

	// number of satellites. -1 indicates no GPS
	intread.c[0] = buffer[21];
	intread.c[1] = buffer[22];
	nsatellite = int(intread.si);
	reliable = nsatellite > 2;

	if(may2002){
	    day_of_month    = 12;
	    month_of_year   = 5;
	    year            = 2002;
	    int nday        = nsec / 86400;
	    if(nday < 4)
		day_of_month += nday+7;
	    else
		day_of_month += nday;
	    nsec -= 86400*nday;
	    
	}else{    
	    day_of_month  = buffer[17];
	    month_of_year = buffer[18];
	    intread.c[0]  = buffer[19];
	    intread.c[1]  = buffer[20];
	    year = intread.usi;
	}

	// Fixes for timing problems
	if(month_of_year == 9 && year == 263) year = 2002;
	
	if(year < 2002){
	    
	    day_of_month  = 8 + nsec / SECONDS_IN_A_DAY;
	    month_of_year = 9;
	    year          = 2002;
	    
	}else{
	    
	    if(month_of_year == 9 && year == 2002){
		int nweek = (day_of_month - 8)/7;
		int days  = day_of_month - 8 - 7*nweek;
		if(days > 3 && nsec < 2*SECONDS_IN_A_DAY){
		    nweek++;
		}else if(days < 4 && nsec > 5*SECONDS_IN_A_DAY){
		    nweek--;
		}
		day_of_month = 8 + 7*nweek + nsec / SECONDS_IN_A_DAY;
	    }
	}

	second   = nsec % 86400;
	hour     = second / 3600;
	second  -= 3600*hour;
	minute   = second / 60;
	second  -= 60*minute;
	millisec = int(nnanosec / 1.e6 + 0.5);

    }else{
	intread.c[0] = buffer[12];
	intread.c[1] = buffer[13];
	intread.c[2] = buffer[14];
	intread.c[3] = buffer[15];
	nsec = intread.i;

	intread.c[0] = buffer[16];
	intread.c[1] = buffer[17];
	intread.c[2] = buffer[18];
	intread.c[3] = buffer[19];
	nnanosec = 100*intread.i;

	nsatellite = 0;

	time_t epoch = nsec;
	tm *time = gmtime(&epoch);
	second = time->tm_sec;
	minute = time->tm_min;
	hour   = time->tm_hour;
	day_of_month  = time->tm_mday;
	month_of_year  = time->tm_mon + 1;
	year = 1900 + time->tm_year;
	millisec = int(nnanosec / 1.e6 + 0.5);

	intread.c[0] = buffer[24];
	intread.c[1] = buffer[25];

	uint32_t tstamp = intread.usi;
	std::cerr << "tstamp           = " << tstamp << std::endl;

	// Report timing information
	// Report timing information. Report a single problem.
	reliable = true;
	std::string reason = "";
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
    }
    
    return (reliable && year > 2001 && year < 2040 && month_of_year > 0 && month_of_year < 13 &&
	    day_of_month > 0 && day_of_month < 32 && hour >= 0 && hour < 24 &&
	    minute >= 0 && minute < 60 && second >= 0 && second < 60);
}

