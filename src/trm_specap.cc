#include "trm_ultracam.h"
#include "trm_ccd.h"
#include "trm_windata.h"
#include "trm_specap.h"

float Ultracam::Specap::how_far(float x, float y) const {
  return fabs(y-get_ypos());
}

bool Ultracam::Specap::near_enough(float x, float y) const {
  return (how_far(x, y) < 5);
}

/** Shifts an extraction region. 
 * \param shift the number of unbinned pixels towards the right to shift by.
 */
void Ultracam::Specap::add_shift(double shift){

  // First move the object
  ylow += shift;
  yhigh   += shift;
  ypos   += shift;

  // Then the sky regions
  for(size_t i=0; i<sky_regions.size(); i++){
    if(!sky_regions[i].fixed){
      sky_regions[i].ylow   += shift;
      sky_regions[i].yhigh  += shift;
    }      
  }

}

/** Works out whether the Specap overlaps a unique window in a given CCD image. 
 * \param winds a CCD's worth of windatas
 * \return the index of the window the Specap overlaps. -1 means it does
 * not overlap anything, windows.size() means it overlaps with more than one
 */
int Ultracam::Specap::unique_window(const CCD<Windata>& wins) const {
 
  int nover = 0, nwin = -1;
  for(size_t i=0; i<wins.size(); i++){
    if(wins[i].left() < xright && wins[i].right() > xleft && wins[i].bottom() < ylow && wins[i].top() > yhigh){
      nover++;
      nwin = i;
    }
  }
  if(nover == 0){
    return -1;
  }else if(nover == 1){
    return nwin;
  }else{
    return wins.size();
  }    
}

void Ultracam::Specap::set_yslow(double yslow) {
  if(yslow > ylow)
    throw Ultracam_Error("void set_yslow(double): yslow = " + Subs::str(yslow) + " is greater than ylow = " + Subs::str(ylow));
  this->yslow = yslow;
}

void Ultracam::Specap::set_ylow(double ylow) {
  if(ylow < yslow)
    throw Ultracam_Error("void set_ylow(double): ylow = " + Subs::str(ylow) + " is less than yslow = " + Subs::str(yslow));
  if(ylow > ypos)
    throw Ultracam_Error("void set_ylow(double): ylow = " + Subs::str(ylow) + " is greater than ypos = " + Subs::str(ypos));
  this->ylow = ylow;
}

void Ultracam::Specap::set_ypos(double ypos) {
  if(ypos < ylow)
    throw Ultracam_Error("void set_ypos(double): ypos = " + Subs::str(ypos) + " is less than ylow = " + Subs::str(ylow));
  if(ypos > yhigh)
    throw Ultracam_Error("void set_ypos(double): ypos = " + Subs::str(ypos) + " is greater than yhigh = " + Subs::str(yhigh));
  this->ypos = ypos;
}

void Ultracam::Specap::set_yhigh(double yhigh) {
  if(yhigh < ypos)
    throw Ultracam_Error("void set_yhigh(double): yhigh = " + Subs::str(yhigh) + " is less than ypos = " + Subs::str(ypos));
  if(yhigh > yshigh)
    throw Ultracam_Error("void set_yhigh(double): yhigh = " + Subs::str(yhigh) + " is greater than yshigh = " + Subs::str(yshigh));
  this->yhigh = yhigh;
}

void Ultracam::Specap::set_yshigh(double yshigh) {
  if(yshigh < yhigh)
    throw Ultracam_Error("void set_yshigh(double): yshigh = " + Subs::str(yshigh) + " is less than yhigh = " + Subs::str(yhigh));
  this->yshigh = yshigh;
}

std::ostream& Ultracam::operator<<(std::ostream& s, const Specap::Skyreg& obj) {
  s << obj.ylow << " " << obj.yhigh << " " << obj.good << " " << obj.fixed;
  return s;
}

std::istream& Ultracam::operator>>(std::istream& s, Specap::Skyreg& obj) {
  s >> obj.ylow >> obj.yhigh >> obj.good >> obj.fixed;
  return s;
}

std::ostream& Ultracam::operator<<(std::ostream& s, const Specap& obj) {
  s << "yslow = " << obj.yslow << ", ylow = " << obj.ylow << ", ypos = " << obj.ypos << ", yhigh = " << obj.yhigh << ", yshigh = " << obj.yshigh 
    << ", xleft = " << obj.xleft << ", xright = " << obj.xright << ", nsky = " << obj.nsky();
  for(int i=0; i<obj.nsky(); i++) s << " " << obj.sky(i);
  return s;
}

std::istream& Ultracam::operator>>(std::istream& s, Specap& obj) {

  char ch;
  int nsky;
  double yslow, ylow, ypos, yhigh, yshigh, xleft, xright;
  Specap::Skyreg skyreg;
  std::vector<Specap::Skyreg> skyregs;
  
  while(s.get(ch) && ch != '=');
  if(!s || !(s >> yslow))
    throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): yslow unreadable");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> ylow))
    throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): ylow unreadable");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> ypos))
    throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): ypos unreadable");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> yhigh))
    throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): yhigh unreadable");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> yshigh))
    throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): yshigh unreadable");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> xleft))
    throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): xleft unreadable");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> xright))
    throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): xright unreadable");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> nsky))
    throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): nsky unreadable");

  for(int i=0; i<nsky; i++){
    if(!s || !(s >> skyreg))
      throw Ultracam_Error("Ultracam::::operator>>(std::istream&, Ultracam::Specap&): sky region number " + Subs::str(i+1) + " unreadable");
    skyregs.push_back(skyreg);
  }

  // Ok all read, let's save it
  obj.yslow   = yslow;
  obj.ylow = ylow;
  obj.ypos   = ypos;
  obj.yhigh   = yhigh;
  obj.yshigh  = yshigh;
  obj.xleft  = xleft;
  obj.xright = xright;
  obj.delete_sky();
  for(int i=0; i<nsky; i++)
    obj.push_back(skyregs[i]);

  return s;

}

/** Checks two regions do not clash which means that their 
 * object regions must not overlap.
 */
bool Ultracam::clash(const Specap& obj1, const Specap& obj2) {
  return (
	  (obj1.ylow >= obj2.ylow && obj1.ylow <= obj2.yhigh) ||
	  (obj1.ylow <  obj2.ylow && obj1.yhigh   >= obj2.ylow)
	  );
}

void Ultracam::Specap::delete_sky(int i){
  if(i< 0 || i >= int(sky_regions.size()))
    throw Ultracam_Error("Ultracam::Specap::delete_sky(int): sky region index = " + Subs::str(i) + " is out of range");
  sky_regions.erase(sky_regions.begin()+i);
}

const Ultracam::Specap::Skyreg& Ultracam::Specap::sky(int i) const {
  if(i< 0 || i >= int(sky_regions.size()))
    throw Ultracam_Error("Ultracam::Specap::sky(int): sky region index = " + Subs::str(i) + " is out of range");
  return sky_regions[i];
}

/** This plots a Specap with green lines for the object region, blue for
 *  the sky regions, red for the anti-sky regions and red dashed for the bad sky
 *  regions which do not move with the object. Dashed green lines are used to
 * mark the target position and the search region.
 * \param specap the spectrum extraction regions to plot. 
 * \param profile true if the plot is a plot of a profile, false if it is an image.
 */
void Ultracam::pgline(const Specap& specap, bool profile) {

  float x1, x2, y1, y2;
  cpgqwin(&x1,&x2,&y1,&y2);

  if(profile){

    // First draw the object
    cpgsci(3);

    // Draw vertical solid lines at edges of extraction region and a horizontal one
    // half way up joining them
    cpgsls(1);
    cpgmove(specap.get_ylow(), y1); 
    cpgdraw(specap.get_ylow(), y2);
    cpgmove(specap.get_yhigh(), y1); 
    cpgdraw(specap.get_yhigh(), y2);
    cpgmove(specap.get_ylow(), (y1+y2)/2.); 
    cpgdraw(specap.get_yhigh(), (y1+y2)/2.);

    // Dashed line at object position and marking search region
    cpgsls(2);
    cpgmove(specap.get_ypos(), y1); 
    cpgdraw(specap.get_ypos(), y2);
    cpgmove(specap.get_yslow(), y1); 
    cpgdraw(specap.get_yslow(), y2);
    cpgmove(specap.get_yshigh(), y1); 
    cpgdraw(specap.get_yshigh(), y2);
    cpgmove(specap.get_yslow(), (1.1*y1+y2)/2.1); 
    cpgdraw(specap.get_yshigh(), (1.1*y1+y2)/2.1);

    // Then the sky regions
    for(int i=0; i<specap.nsky(); i++){
      const Specap::Skyreg& skyreg = specap.sky(i);
      if(skyreg.good){
	cpgsci(5);
	cpgsls(1);
      }else if(skyreg.fixed){
	cpgsci(2);
	cpgsls(1);
      }else{
	cpgsci(2);
	cpgsls(2);
      }
      cpgmove(skyreg.ylow, y1); 
      cpgdraw(skyreg.ylow, y2);
      cpgmove(skyreg.yhigh,   y1); 
      cpgdraw(skyreg.yhigh,   y2);
      cpgmove(skyreg.ylow, (2.*y1+y2)/3.); 
      cpgdraw(skyreg.yhigh,   (2.*y1+y2)/3.);
      cpgmove((skyreg.ylow+skyreg.yhigh)/2., (2.*y1+y2)/3.); 
      cpgdraw(specap.get_ypos(), (y1+y2)/2.); 
    }

  }else{
    throw Ultracam_Error("Ultracam::pgline(const Specap&, bool): profile = false option not supported yet");
  }
}




















