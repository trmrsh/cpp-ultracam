#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>


void timing(char* buffer, unsigned char& day_of_month, unsigned char& month_of_year, unsigned short int& year, int& hour, int& minute, 
	    int& second, int& millisec);

int main(int argc, char* argv[]){

  if(argc != 2){
    std::cerr << "usage: getstart run" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string file(argv[1]);
  file += ".dat";

  std::ifstream fin(file.c_str(), std::ios::binary);
  if(!fin){
    std::cerr << "Could not open open " << file << " for reading" << std::endl;
    exit(EXIT_FAILURE);
  }

  char buffer[24];
  if (!fin.read(buffer, 24)){
    std::cerr << "Error while trying to read first 24 bytes from " << file << std::endl;
    exit(EXIT_FAILURE);
  }

  fin.close();

  unsigned char day_of_month, month_of_year;
  unsigned short int year;
  int hour, minute, second, millisec;
  timing(buffer, day_of_month, month_of_year, year, hour, minute, second, millisec);

  char output[1024];
  sprintf(output, "%s, UT at start = %02d/%02d/%d, %02d:%02d:%02d.%03d\n", argv[1], day_of_month, month_of_year, year, hour, minute, second, millisec);
  std::cout << output;
    
}

void timing(char* buffer, unsigned char& day_of_month, unsigned char& month_of_year, unsigned short int& year, int& hour, int& minute, int& second, int& millisec){

  union IntRead{
    char c[4];
    int  i;
    unsigned short int usi;
    short int si;
  } intread;

  // Number of seconds
  intread.c[0] = buffer[9];
  intread.c[1] = buffer[10];
  intread.c[2] = buffer[11];
  intread.c[3] = buffer[12];
  
  int nsec = intread.i;

  // number of nanoseconds
  intread.c[0] = buffer[13];
  intread.c[1] = buffer[14];
  intread.c[2] = buffer[15];
  intread.c[3] = buffer[16];
  
  int nnanosec = intread.i;

  day_of_month  = buffer[17];
  month_of_year = buffer[18];
  intread.c[0]  = buffer[19];
  intread.c[1]  = buffer[20];
  year = intread.usi;

  second   = nsec % 86400;
  hour     = second / 3600;
  second  -= 3600*hour;
  minute   = second / 60;
  second  -= 60*minute;
  millisec = int(nnanosec / 1.e6 + 0.5);
}


