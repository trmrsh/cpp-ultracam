#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/ultracam.h"
#include "trm/window.h"

const int Ultracam::Window::MAX_NXTOT;
const int Ultracam::Window::MAX_NYTOT;
const int Ultracam::Window::MAX_XBIN;
const int Ultracam::Window::MAX_YBIN;

// Check window parameters for validity (i.e. the class invariant)
// the -1 for llx and lly are kludges for a problem 
// discovered on 05/01/2005

bool Ultracam::bad_window(int llx, int lly, int nx, 
			  int ny, int xbin, 
			  int ybin, int nxtot, 
			  int nytot, int MAX_NXTOT, 
			  int MAX_NYTOT, int MAX_XBIN, 
			  int MAX_YBIN){
  
  return (nx < 0 || ny < 0 || 
	  llx < -1  || llx+xbin*nx-1 > nxtot || 
	  lly < -1  || lly+ybin*ny-1 > nytot ||
	  xbin < 1 || xbin > MAX_XBIN || nxtot > MAX_NXTOT || 
	  ybin < 1 || ybin > MAX_YBIN || nytot > MAX_NYTOT);
}


// General constructor. Throws a string if inputs are invalid.

Ultracam::Window::Window(int llx, int lly, int nx, int ny, 
			 int xbin, int ybin, int nxtot, 
			 int nytot) : ll_x(llx), ll_y(lly), n_x(nx), n_y(ny), 
				      x_bin(xbin), y_bin(ybin), nx_tot(nxtot), ny_tot(nytot) {

  if(bad_window(ll_x, ll_y, n_x, n_y, x_bin, y_bin, nx_tot, ny_tot, MAX_NXTOT, MAX_NYTOT,
		MAX_XBIN, MAX_YBIN))
    throw Ultracam_Error("Invalid window in Ultracam::Window::Window(int, "
			 "int, int, int, "
			 "int, int,"
			 " int, int):\n"
			 
			 + Subs::str(ll_x)   + ", "
			 + Subs::str(ll_y)   + ", "
			 + Subs::str(n_x)    + ", "
			 + Subs::str(n_y)    + ", "
			 + Subs::str(x_bin)  + ", "
			 + Subs::str(y_bin)  + ", "
			 + Subs::str(nx_tot) + ", "
			 + Subs::str(ny_tot) );
}

// Mutators -- they check for invalid states and throw string
// exceptions leaving object untouched.

void Ultracam::Window::set_llx(int llx) {
  if(bad_window(llx, ll_y, n_x, n_y, x_bin, y_bin, nx_tot, ny_tot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw Ultracam_Error(
			 "Invalid window in void Ultracam::Window::set_llx(int llx):\n"
			 + Subs::str(ll_x)   + ", "
			 + Subs::str(ll_y)   + ", "
			 + Subs::str(n_x)    + ", "
			 + Subs::str(n_y)    + ", "
			 + Subs::str(x_bin)  + ", "
			 + Subs::str(y_bin)  + ", "
			 + Subs::str(nx_tot) + ", "
			 + Subs::str(ny_tot) );
  ll_x = llx;
}

void Ultracam::Window::set_lly(int lly) {
  if(bad_window(ll_x, lly, n_x, n_y, x_bin, y_bin, nx_tot, ny_tot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw Ultracam_Error(
			 "Invalid window in void Ultracam::Window::set_llx(int llx):\n"
			 + Subs::str(ll_x)   + ", "
			 + Subs::str(ll_y)   + ", "
			 + Subs::str(n_x)    + ", "
			 + Subs::str(n_y)    + ", "
			 + Subs::str(x_bin)  + ", "
			 + Subs::str(y_bin)  + ", "
			 + Subs::str(nx_tot) + ", "
			 + Subs::str(ny_tot) );
  ll_y = lly;
}

void Ultracam::Window::set_nx(int nx) {
  if(bad_window(ll_x, ll_y, nx, n_y, x_bin, y_bin, nx_tot, ny_tot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw Ultracam_Error("Invalid window in void Ultracam::Window::set_nx(int)");
  n_x = nx;
}

void Ultracam::Window::set_ny(int ny) {
  if(bad_window(ll_x, ll_y, n_x, ny, x_bin, y_bin, nx_tot, ny_tot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw Ultracam_Error("Invalid window in void Ultracam::Window::set_ny(int)");
  n_y = ny;
}

void Ultracam::Window::set_xbin(int xbin) {
  if(bad_window(ll_x, ll_y, n_x, n_y, xbin, y_bin, nx_tot, ny_tot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw Ultracam_Error("Invalid window in void Ultracam::Window::set_xbin(int)");
  x_bin = xbin;
}

void Ultracam::Window::set_ybin(int ybin) {
  if(bad_window(ll_x, ll_y, n_x, n_y, x_bin, ybin, nx_tot, ny_tot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw Ultracam::Ultracam_Error("Invalid window in void Ultracam::Window::set_ybin(int)");
  y_bin = ybin;
}

void Ultracam::Window::set_nxtot(int nxtot) {
  if(bad_window(ll_x, ll_y, n_x, n_y, x_bin, y_bin, nxtot, ny_tot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw Ultracam::Ultracam_Error("Invalid window in void Ultracam::Window::set_nxtot(int)");
  nx_tot = nxtot;
}

void Ultracam::Window::set_nytot(int nytot) {
  if(bad_window(ll_x, ll_y, n_x, n_y, x_bin, y_bin, nx_tot, nytot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw Ultracam::Ultracam_Error("Invalid window in void Ultracam::Window::set_nytot(int)");
  ny_tot = nytot;
}

void Ultracam::Window::set(int llx, int lly, int nx, 
		 int ny, int xbin, int ybin,
		 int nxtot, int nytot){

  if(bad_window(llx, lly, nx, ny, xbin, ybin, nxtot, nytot, 
		MAX_NXTOT, MAX_NYTOT, MAX_XBIN, MAX_YBIN))
    throw 
      Ultracam::Ultracam_Error("Invalid window in void"
			       " Ultracam::Window::set(int, int"
			       ", int, int, int, int,"
			       " int, int)");

  ll_x   = llx;
  ll_y   = lly;
  n_x    = nx;
  n_y    = ny;
  x_bin  = xbin;
  y_bin  = ybin;
  nx_tot = nxtot;
  ny_tot = nytot;
}

// This returns a number that can be used to rank the distances
// of windows from a point

float Ultracam::Window::how_far(float x, float y) const {
  using Subs::sqr;
  if(x < ll_x-0.5 && y < ll_y-0.5){
    return sqr(x-ll_x+0.5) + sqr(y-ll_y+0.5);
  }else if(x < ll_x-0.5 && y > ll_y+n_y*y_bin-0.5){
    return sqr(x-ll_x+0.5) + sqr(y-ll_y-n_y*y_bin+0.5);
  }else if(x < ll_x-0.5){
    return sqr(x-ll_x+0.5);
  }else if(x > ll_x+n_x*x_bin-0.5 && y < ll_y-0.5){
    return sqr(x-ll_x-n_x*x_bin+0.5) + sqr(y-ll_y+0.5);
  }else if(x > ll_x+n_x*x_bin-0.5 && y > ll_y+n_y*y_bin-0.5){
    return sqr(x-ll_x-n_x*x_bin+0.5) + sqr(y-ll_y-n_y*y_bin+0.5);
  }else if(x > ll_x+n_x*x_bin-0.5){
    return sqr(x-ll_x-n_x*x_bin+0.5);
  }else if(y < ll_y-0.5){
    return sqr(y-ll_y+0.5);
  }else if(y > ll_y+n_y*y_bin-0.5){
    return sqr(y-ll_y-n_y*y_bin+0.5);
  }else{
    return 0.;
  }
}

bool Ultracam::Window::near_enough(float x, float y) const {
  const float BORDER = 10.;
  return 
    x > ll_x-0.5-BORDER && y > ll_y-0.5-BORDER &&
    x < ll_x+n_x*x_bin-0.5+BORDER && y < ll_y+n_y*y_bin-0.5+BORDER;
}

// Binary output

void Ultracam::Window::write(std::ofstream& fout) const {
  fout.write((char*)&ll_x,  sizeof(Subs::INT4));
  fout.write((char*)&ll_y,  sizeof(Subs::INT4));
  fout.write((char*)&n_x,   sizeof(Subs::INT4));
  fout.write((char*)&n_y,   sizeof(Subs::INT4));
  fout.write((char*)&x_bin, sizeof(Subs::INT4));
  fout.write((char*)&y_bin, sizeof(Subs::INT4));
  fout.write((char*)&nx_tot,sizeof(Subs::INT4));
  fout.write((char*)&ny_tot,sizeof(Subs::INT4));
}

void Ultracam::Window::read(std::ifstream& fin, bool swap_bytes){
  Subs::INT4 llx, lly, nx, ny, nxtot, nytot;
  Subs::INT4 xbin, ybin;
  fin.read((char*)&llx,  sizeof(Subs::INT4));
  fin.read((char*)&lly,  sizeof(Subs::INT4));
  fin.read((char*)&nx,   sizeof(Subs::INT4));
  fin.read((char*)&ny,   sizeof(Subs::INT4));
  fin.read((char*)&xbin, sizeof(Subs::INT4));
  fin.read((char*)&ybin, sizeof(Subs::INT4));
  fin.read((char*)&nxtot,sizeof(Subs::INT4));
  fin.read((char*)&nytot,sizeof(Subs::INT4));

  if(swap_bytes){
    llx   = Subs::byte_swap(llx);
    lly   = Subs::byte_swap(lly);
    nx    = Subs::byte_swap(nx);
    ny    = Subs::byte_swap(ny);
    xbin  = Subs::byte_swap(xbin);
    ybin  = Subs::byte_swap(ybin);
    nxtot = Subs::byte_swap(nxtot);
    nytot = Subs::byte_swap(nytot);
  }

  try{
    set(llx,lly,nx,ny,xbin,ybin,nxtot,nytot);
  }
  catch(...){
    throw Ultracam::Ultracam_Error("Invalid window in Ultracam::Window::read(std::ifstream&): " +
				   Subs::str(llx) + ", " + Subs::str(lly) + ", " + Subs::str(nx) + 
				   ", " + Subs::str(ny) + ", " + Subs::str(xbin) + ", " +
				   Subs::str(ybin) + ", " + Subs::str(nxtot) + ", " + Subs::str(nytot) );
  }
}

void Ultracam::Window::read_old(std::ifstream& fin, bool swap_bytes){
  Subs::INT4 llx, lly, nx, ny, nxtot, nytot, xbin, ybin;
  fin.read((char*)&llx,  sizeof(Subs::INT4));
  fin.read((char*)&lly,  sizeof(Subs::INT4));
  fin.read((char*)&nx,   sizeof(Subs::INT4));
  fin.read((char*)&ny,   sizeof(Subs::INT4));
  fin.read((char*)&xbin, sizeof(Subs::INT4));
  fin.read((char*)&ybin, sizeof(Subs::INT4));
  fin.read((char*)&nxtot,sizeof(Subs::INT4));
  fin.read((char*)&nytot,sizeof(Subs::INT4));

  if(swap_bytes){
    llx   = Subs::byte_swap(llx);
    lly   = Subs::byte_swap(llx);
    nx    = Subs::byte_swap(nx);
    ny    = Subs::byte_swap(ny);
    xbin  = Subs::byte_swap(xbin);
    ybin  = Subs::byte_swap(ybin);
    nxtot = Subs::byte_swap(nxtot);
    nytot = Subs::byte_swap(nytot);
  }

  try{
    set(llx,lly,nx,ny,xbin,ybin,nxtot,nytot);
  }
  catch(...){
    throw Ultracam::Ultracam_Error("Invalid window in"" Ultracam::Window::read_old(std::ifstream&): " +
				   Subs::str(llx) + ", " + Subs::str(lly) + ", " + Subs::str(nx) + ", " + 
				   Subs::str(ny) + ", " + Subs::str(xbin) + ", " + Subs::str(ybin) + ", " + 
				   Subs::str(nxtot) + ", " + Subs::str(nytot) );
  }
}

bool Ultracam::Window::enclose(float x, float y) const {
  return (x > left() && x < right() && y > bottom() && y < top());
}

bool Ultracam::Window::is_not_null() const {
    return (this->nx() > 0 && this->ny() > 0);
}

bool Ultracam::Window::is_oned() const {
    return (
	(this->nx() == 1 && this->ny() > 0) ||
	(this->nx() > 0 && this->ny() == 1));
}

void Ultracam::pgline(const Ultracam::Window& window) {
  cpgmove(window.left(),window.bottom());
  cpgdraw(window.left(),window.top());
  cpgdraw(window.right(),window.top());
  cpgdraw(window.right(),window.bottom());
  cpgdraw(window.left(),window.bottom());
}
 
void Ultracam::pgptxt(const Ultracam::Window& window, const std::string& label){
  float x, y, x1, x2, y1, y2, xr;
  cpgqwin(&x1,&x2,&y1,&y2);
  xr  = (x2-x1)/20.;
  x = window.llx()-fabs(xr)/1.5;
  y = window.lly();
  if(x > std::min(x1,x2) && x < std::max(x1,x2) &&
     y > std::min(y1,y2) && y < std::max(y1,y2))
    cpgptxt(x,y,0.,0.,label.c_str());
}

// 1-line ASCII output/input

std::ostream& Ultracam::operator<<(std::ostream& s, const Ultracam::Window& window){
  s << "llx,lly = " << window.llx() << ", " << window.lly()
    << "; nx,ny = " << window.nx()  << ", " << window.ny()
    << "; xbin,ybin = " << window.xbin() << ", " << window.ybin() 
    << "; nxtot,nytot = " << window.nxtot() << ", " << window.nytot();
  return s;
}

// ASCII input

std::istream& Ultracam::operator>>(std::istream& s, Ultracam::Window& window){
  char ch;
  int llx, lly, nx, ny, nxtot, nytot;
  int xbin, ybin;

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> llx) || !s.get(ch) || !(s >> lly)) 
    throw 
      Ultracam::Ultracam_Error("Invalid input into Ultracam::Window::operator>> (1)");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> nx) || !s.get(ch) || !(s >> ny)) 
    throw 
      Ultracam::Ultracam_Error("Invalid input into Ultracam::Window::operator>> (2)");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> xbin) || !s.get(ch) || !(s >> ybin)) 
    throw 
      Ultracam::Ultracam_Error("Invalid input into Window::operator>> (3)");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> nxtot) || !s.get(ch) || !(s >> nytot)) 
    throw 
      Ultracam::Ultracam_Error("Invalid input into Window::operator>> (4)");

  try{
    window.set(llx,lly,nx,ny,xbin,ybin,nxtot,nytot);
  }
  catch(...){
    throw 
      Ultracam::Ultracam_Error("Invalid Ultracam::Window in std::istream& "
			       "operator>>(std::istream& s, Ultracam::Window& window)");
  }
  return s;
}

// Test two windows for clashes. This is designed for use
// by CCD when multiple windows are stored. In this case this
// means guarding against overlap and different binning and 
// CCD dimensions

bool Ultracam::clash(const Ultracam::Window& win1, const Ultracam::Window& win2){
  return 
    (win1.xbin() != win2.xbin() || win1.ybin() != win2.ybin() ||
     win1.nxtot() != win2.nxtot() || win1.nytot() != win2.nytot() ||
     (win1.llx() < win2.llx() && 
      win1.llx()+win1.xbin()*win1.nx() > win2.llx()) ||
     (win1.llx() >= win2.llx() && 
      win1.llx() < win2.llx()+win2.xbin()*win2.nx())) &&
    ((win1.lly() < win2.lly() && win1.lly()+win1.ybin()*win1.ny() > 
      win2.lly()) || (win1.lly() >= win2.lly() && 
		      win1.lly() < win2.lly()+win2.ybin()*win2.ny()));
}

bool Ultracam::operator==(const Ultracam::Window& obj1, const Ultracam::Window& obj2){
  return (obj1.xbin()  == obj2.xbin()  && 
	  obj1.ybin()  == obj2.ybin()  && 
	  obj1.nxtot() == obj2.nxtot() && 
	  obj1.nytot() == obj2.nytot() && 
	  obj1.llx()   == obj2.llx()   && 
	  obj1.lly()   == obj2.lly()   && 
	  obj1.nx()    == obj2.nx()    && 
	  obj1.ny()    == obj2.ny());
}

bool Ultracam::operator!=(const Ultracam::Window& obj1, const Ultracam::Window& obj2){
  return (obj1.xbin()  != obj2.xbin()  || 
	  obj1.ybin()  != obj2.ybin()  || 
	  obj1.nxtot() != obj2.nxtot() || 
	  obj1.nytot() != obj2.nytot() || 
	  obj1.llx()   != obj2.llx()   || 
	  obj1.lly()   != obj2.lly()   || 
	  obj1.nx()    != obj2.nx()    || 
	  obj1.ny()    != obj2.ny());
}

/** \relates Ultracam::Window 
 * This function tests two windows to see if they have any pixels in common at all.
 * It is a helper function for when one is re-formatting one Ultracam::Window to its common overlap
 * with another.
 */
bool Ultracam::overlap(const Ultracam::Window& obj1, const Ultracam::Window& obj2){
  return 
    obj1.llx() < obj2.llx()+obj2.xbin()*obj2.nx() && 
    obj2.llx() < obj1.llx()+obj1.xbin()*obj1.nx() && 
    obj1.lly() < obj2.lly()+obj2.ybin()*obj2.ny() && 
    obj2.lly() < obj1.lly()+obj1.ybin()*obj1.ny();
}





