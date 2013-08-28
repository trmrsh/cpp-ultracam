/*

!!begin
!!title   Zap cosmic rays
!!author  T.R. Marsh
!!created 07 May 2002
!!descr   Detects and zaps cosmic rays
!!root    zapcosmic
!!index   zapcosmic.cc
!!class   Functions
!!css     style.css
!!head1   zapcosmic -- detects and zaps cosmic rays

!!emph{zapcosmic} is a routine to detect and interpolate cosmic rays.
It also returns a list of their positions. It works by locating maxima within
a search region and seeing if they exceed the average of the neighbours by more than
a preset amount. This is rather crude and should be improved upon some time.

!!head1 Function call

void Ultracam::zapcosmic(internal_data **dat, int nx, int ny, int hwidth_x, int hwidth_x, float xcen, float ycen, 
float thresh_height, float thresh_frac, vector<pair<int,int>>& zapped)

!!head2 Arguments

!!table

!!arg{dat}{2D array such that dat[iy][ix] gives the value of (ix,iy).}
!!arg{nx}{X dimension}
!!arg{ny}{Y dimension}
!!arg{hwidth_x}{Half-width to use in X. Search is over a region
+/- hwidth_x around the pixel nearest to the x,y position.}
!!arg{hwidth_y}{Half-width to use in Y.}
!!arg{xcen}{Central position in X.} 
!!arg{ycen}{Central position in Y.}
!!arg{thresh_height}{Minimum value to bother with}
!!arg{thresh_ratio}{Minimum ratio of pixel to its nearest neighbours to count as a cosmic ray}
!!table

!!end

*/

#include <stdlib.h>
#include <string>
#include <vector>
#include "trm/subs.h"
#include "trm/constants.h"
#include "trm/ultracam.h"

#ifndef DEBUG

void Ultracam::zapcosmic(internal_data **dat, int nx, int ny, 
			 int hwidth_x, int hwidth_y, 
			 float xcen, float ycen,
			 float thresh_height, float thresh_ratio,
			 std::vector<std::pair<int, int> >& zapped){

#else

void Ultracam::zapcosmic(CheckX* dat, int nx, int ny, 
			 int hwidth_x, int hwidth_y, 
			 float xcen, float ycen,
			 float thresh_height, float thresh_ratio,
			 std::vector<std::pair<int, int> >& zapped){

#endif

  using Subs::sqr;
  using Subs::centroid;

  // Check start position

  if(xcen <= -0.5 || xcen >= nx-0.5 || ycen <= -0.5 || ycen >= ny-0.5)
    throw Ultracam_Error("zapcosmic: initial position outside array boundary");

  // Define region to examine

  int xlo_ = int(xcen+0.5) - hwidth_x;
  int xhi_ = int(xcen+0.5) + hwidth_x;
  int ylo_ = int(ycen+0.5) - hwidth_y;
  int yhi_ = int(ycen+0.5) + hwidth_y;

  int xlo = xlo_ < 0  ? 0   : xlo_;
  int xhi = int(xhi_) < nx ? xhi_ : nx-1;
  int ylo = ylo_ < 0  ? 0   : ylo_;
  int yhi = int(yhi_) < ny ? yhi_ : ny-1;

  zapped.clear();

  int nave, nrej = 1;
  float mean, cval, val;

  // repeat process if any are rejected as once one has gone, another nearby may too.

  bool carry_on;
  while(nrej){
    nrej = 0;
    for(int iy=ylo; iy<=yhi; iy++){
      for(int ix=xlo; ix<=xhi; ix++){
	
	cval = dat[iy][ix];
	
	nave = 0;
	mean = 0.f;
	carry_on = true;
	// Look at 8 pixels around the one of interest in array order
	
	// lower-left
	if(carry_on && ix > 0 && iy > 0){
	  if((val = dat[iy-1][ix-1]) > cval){
	    carry_on = false;
	  }else{
	    nave++;
	    mean += val;
	  }
	}
	
	// lower-middle
	if(carry_on && iy > 0){
	  if((val = dat[iy-1][ix]) > cval){
	    carry_on = false;
	  }else{
	    nave++;
	    mean += val;
	  }
	}
	
	// lower-right
	if(carry_on && iy > 0 && ix < nx-1){
	  if((val = dat[iy-1][ix+1]) > cval){
	    carry_on = false;
	  }else{
	    nave++;
	    mean += val;
	  }
	}

	// middle-left
	if(carry_on && ix > 0){
	  if((val = dat[iy][ix-1]) > cval){
	    carry_on = false;
	  }else{
	    nave++;
	    mean += val;
	  }
	}
	
	// middle-right
	if(carry_on && ix < nx-1){
	  if((val = dat[iy][ix+1]) > cval){
	    carry_on = false;
	  }else{
	    nave++;
	    mean += val;
	  }
	}
	
	// upper-left
	if(carry_on && iy < ny-1 && ix > 0){
	  if((val = dat[iy+1][ix-1]) > cval){
	    carry_on = false;
	  }else{
	    nave++;
	    mean += val;
	  }
	}

	// upper-middle
	if(carry_on && iy < ny-1){
	  if((val = dat[iy+1][ix]) > cval){
	    carry_on = false;
	  }else{
	    nave++;
	    mean += val;
	  }
	}

	// upper-right
	if(carry_on && iy < ny-1 && ix < nx-1){
	  if((val = dat[iy+1][ix+1]) > cval){
	    carry_on = false;
	  }else{
	    nave++;
	    mean += val;
	  }
	}
	
	// If we have reached here, we have a maximum. Zap it if it passes the thresholds,
	// store its location, and register a reject.
	
	if(carry_on && nave){
	  mean /= nave;	
	  if(cval > mean + thresh_height && cval > thresh_ratio*mean){
	    dat[iy][ix] = mean;
	    zapped.push_back(std::make_pair(ix, iy));
	    nrej++;
	  }
	}
      }
    }
  }
}
