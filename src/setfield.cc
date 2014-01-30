/*

!!begin
!!title   Star field setup program
!!author  T.R. Marsh
!!created 21 February 2001
!!revised 03 October 2005
!!root    setfield
!!index   setfield
!!descr   sets up a star field
!!css     style.css
!!class   Programs
!!class   Testing
!!head1   setfield for setting up star fields

!!emph{setfield} is an interactive program for defining a field of
stars.  This then allows fake Ultracam data to be generated. The stars are modelled as
Moffat profiles, i.e. they have profiles of the form (1+r**2)**(-beta). beta must be > 1.
The r**2 radius term is actually axx*x*x + 2*axy*x*y + ayy*y*y to allow elliptical profiles.
If you want a near-gaussian profile use a large value of beta. The height is set by specifying
the total counts in the profile.

!!head2 Invocation

setfield newfile field (numccd) nccd plotdata (data [device] xleft xright ylow yhigh
iset ilow ihigh plow phigh)

!!head2 Command line arguments

!!table
!!arg{newfile}{true/false to indicate whether the star field file is new or not}
!!arg{field}{Name of the star field. Will be overwritten if it already exists.}
!!arg{numccd}{Number of CCDs if a new field. This number must
agree with the number of CCDs in any data file read in.}
!!arg{nccd}{Which CCD to set field for (and plot if -d)}
!!arg{plotdata}{true/false to plot data}
!!arg{device}{Plot device}
!!arg{data}{Name of data file}
!!arg{xleft xright}{X range to plot}
!!arg{ylow yhigh}{Y range to plot}
!!arg{ilow ihigh}{the intensity range is i1 to i2}
!!arg{plow phigh}{-p is set, p1 to p2 are percentiles to set
the intensity}
!!table

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include "cpgplot.h"
#include "trm/input.h"
#include "trm/plot.h"
#include "trm/subs.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/target.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

  using Ultracam::Input_Error;
  using Ultracam::Ultracam_Error;

  try{

    // Construct Input object

    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // sign-in input variables
    input.sign_in("newfile", Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("field",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("numccd",  Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("nccd",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("plotdata",Subs::Input::LOCAL,  Subs::Input::PROMPT);
    input.sign_in("device",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("data",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xleft",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("xright",  Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ylow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("yhigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("iset",    Subs::Input::GLOBAL,  Subs::Input::PROMPT);
    input.sign_in("ilow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("ihigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("plow",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
    input.sign_in("phigh",   Subs::Input::GLOBAL, Subs::Input::PROMPT);

    // Get inputs

    bool newfile;
    input.get_value("newfile", newfile, true, "do you want to open a new star field file?");
    std::string fname;
    input.get_value("field", fname, "field", "star field file name");
    Ultracam::Mtarget field;
    if(newfile){
      int numccd;
      input.get_value("numccd",numccd,int(3),int(1),int(10),"number of CCDs");
      field = Ultracam::Mtarget(numccd);
    }else{
      field.rasc(fname);
    }
    int nccd;
    input.get_value("nccd", nccd, int(1), int(1), int(field.size()),
            "CCD number to set field for");
    nccd--;
    bool plotdata;
    input.get_value("plotdata", plotdata, true, "plot a data file for guidance?");

    std::string device, name;
    Ultracam::Frame data;
    float x1, x2, y1, y2, x, y;
    char iset;
    float ilow, ihigh, plow, phigh;
    if(plotdata){
      input.get_value("device", device, "/xs", "plot device");
      input.get_value("data", name, "run001", "file or file list to plot");
      data.read(name);
      if(field.size() != data.size())
    throw Ultracam_Error("Numbers of CCDs in star field and data do not match");
      x2 = data[nccd].nxtot()+0.5;
      y2 = data[nccd].nytot()+0.5;
      input.get_value("xleft",  x1, 0.5f, 0.5f, x2, "left X limit of plot");
      input.get_value("xright", x2, x2,   0.5f, x2, "right X limit of plot");
      input.get_value("ylow",   y1, 0.5f, 0.5f, y2, "lower Y limit of plot");
      input.get_value("yhigh",  y2, y2, 0.5f, y2, "upper Y limit of plot");
      input.get_value("iset", iset, 'a', "aAdDpP",
              "set intensity a(utomatically), d(irectly) or with p(ercentiles)?");
      iset = toupper(iset);
      if(iset == 'D'){
    input.get_value("ilow",   ilow,  0.f, -FLT_MAX, FLT_MAX, "lower intensity limit");
    input.get_value("ihigh",  ihigh, 1000.f, -FLT_MAX, FLT_MAX, "upper intensity limit");
      }else if(iset == 'P'){
    input.get_value("plow",   plow,  1.f, 0.f, 100.f,  "lower intensity limit percentile");
    input.get_value("phigh",  phigh, 99.f, 0.f, 100.f, "upper intensity limit percentile");
    plow  /= 100.;
    phigh /= 100.;
      }
      x = (x1+x2)/2.;
      y = (y1+y2)/2.;
    }

    float counts = 1000.;
    float axx = 0.02, axy = 0.0, ayy = 0.02;
    double beta = 3.;
    if(plotdata){
      Subs::Plot plot(device);
      cpgsch(1.5);
      cpgscf(2);

      Ultracam::plot_images(data,x1,x2,y1,y2,false,'X',iset,ilow,ihigh,plow,phigh,
                true,name,nccd,false);

      cpgsci(Subs::WHITE);
      char reply = 'X';

      // Plot stars already set
      pgline(field[nccd]);

      std::cout << "Position cursor at a star position (to add one) "
       << "or near a star\n"
       << "to delete it and hit the appropriate letter.\n"
       << std::endl;

      while(reply != 'Q'){
    reply = 'X';
    std::cout << "Position cursor then hit A(dd), R(emove) or Q(uit)" << std::endl;
    if(!cpgcurs(&x,&y,&reply)) throw Ultracam_Error("Cursor error");

    reply = toupper(reply);

    if(reply == 'A'){

      std::cout << "Enter counts, axx, axy, ayy, beta [" << counts << ","
           << axx << "," << axy << ","<< ayy << "," << beta << "]: " << std::flush;
      std::string entry;
      getline(std::cin,entry);
      if(entry != ""){
        std::istringstream istr(entry);
        istr >> counts >> axx >> axy >> ayy >> beta;
      }
      if(axx > 0. && axx*ayy > Subs::sqr(axy) && beta > 1.){
        field[nccd].push_back(Ultracam::Target(x,y,counts,axx,axy,ayy,beta));
        pgline(field[nccd][field[nccd].size()-1]);
        pgptxt(field[nccd][field[nccd].size()-1], Subs::str(field[nccd].size()));

      }else if(beta <= 1.){
        std::cerr << "beta = " << beta << " is <= 1!" <<std::endl;
        std::cerr << "Try again." << std::endl;

      }else{
        std::cerr << "axx,axy,ayy = "
         << axx << ", " << axy << ", " << ayy
         << "are not positive-definite!" <<std::endl;
        std::cerr << "Try again." << std::endl;
      }

    }else if(reply == 'R'){

      Ultracam::Target s;
      if(field[nccd].del_obj(x,y,s)){
        cpgsci(Subs::RED);
        pgline(s);
        cpgsci(Subs::WHITE);
      }
    }else if(reply != 'Q'){
      std::cout << "Position = " << x << ", " << y << std::endl;
    }
      }

    }else{

      char reply = 'X';
      while(reply != 'Q'){
    std::cout << "A(dd), R(emove), Q(uit): ";
    std::cin  >> reply;
    reply = toupper(reply);

    if(reply == 'A'){
      std::cout << "Enter x, y, counts, axx, axy, ayy, beta [" << x << ","
           << y << "," << counts << "," << axx << "," << axy
           << ","<< ayy << "," << beta << "]: " << std::flush;
      std::cin >> x >> y >> counts >> axx >> axy >> ayy >> beta;

      if(axx > 0. && axx*ayy > Subs::sqr(axy) && beta > 1.){
        field[nccd].push_back(Ultracam::Target(x,y,counts,axx,axy,ayy,beta));
      }else if(beta <= 1.){
        std::cerr << "beta = " << beta << " is <= 1!" <<std::endl;
        std::cerr << "Try again." << std::endl;
      }else{
        std::cerr << "axx,axy,ayy = "
         << axx << ", " << axy << ", " << ayy
         << "are not positive-definite!" <<std::endl;
        std::cerr << "Try again." << std::endl;
      }
    }else if(reply == 'R'){

      std::cout << "Enter x, y near star to remove [" << x << ","
           << y << "]: " << std::flush;
      std::cin >> x >> y;
      Ultracam::Target s;
      if(field[nccd].del_obj(x,y,s)){
        cpgsci(Subs::RED);
        pgline(s);
        cpgsci(Subs::WHITE);
      }
    }
      }
    }

    // Dump out result

    field.wasc(fname);
  }

  catch(const Ultracam::Input_Error& err){
    std::cerr << "Ultracam::Input_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Ultracam::Ultracam_Error& err){
    std::cerr << "Ultracam::Ultracam_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const Subs::Subs_Error& err){
    std::cerr << "Subs::Subs_Error exception:" << std::endl;
    std::cerr << err << std::endl;
  }
  catch(const std::string& err){
    std::cerr << err << std::endl;
  }
}






