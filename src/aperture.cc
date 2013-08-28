// The code starts here

#include <iostream>
#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/aperture.h"
#include "trm/ultracam.h"

int Ultracam::Aperture::Ref_col = 3;
int Ultracam::Aperture::Invalid_col = 2;

using Ultracam::Ultracam_Error;

/** Standard constructor on an Ultracam::Aperture. This constructs evrything except the
 * mask of stars to be eliminated from the sky annulus.
 * \param xr    reference X value for the aperture
 * \param yr    reference Y value for the aperture
 * \param xoff  offset in X of aperture from X reference
 * \param yoff  offset in Y of aperture from Y reference
 * \param rstar radius of star aperture, unbinned pixels
 * \param rsky1 radius of inner sky aperture
 * \param rsky2 radius of outer sky aperture
 * \param ref   true/false according to whether this is an aperture for referencing positions. That is when 
 * apertures are positioned first with a global shift from a few special apertures, is this aperture one of
 * those?
 */
Ultracam::Aperture::Aperture(double xr, double yr, double xoff, 
		   double yoff, float rstar, 
		   float rsky1, float rsky2,
		   bool ref) : 
  x_r(xr), y_r(yr), x_off(xoff), y_off(yoff), r_star(rstar),
  r_sky1(rsky1), r_sky2(rsky2), ref_star(ref), ap_ok(true), mask_(), extra_() {
 
  if(bad_aper(rstar,rsky1,rsky2))
    throw 
      Ultracam_Error("Invalid aperture radii in "
			       "Ultracam::Aperture::Aperture(double,"
			       " double, double,"
			       " float, float, float)");
}

/** Sets the radius of the star aperture
 * \param rstar radius of star aperture, unbinned pixels
 */
void Ultracam::Aperture::set_rstar(float rstar){
  if(bad_aper(rstar,r_sky1,r_sky2))
    throw Ultracam_Error("Invalid star radius in void Ultracam::Aperture::set_rstar(float)");
  r_star = rstar;
}

/** Sets the radius of the inner sky aperture
 * \param rsky1 radius of the inner sky aperture, unbinned pixels
 */
void Ultracam::Aperture::set_rsky1(float rsky1){
  if(bad_aper(r_star,rsky1,r_sky2))
    throw Ultracam_Error("Invalid inner sky radius in void Ultracam::Aperture::set_rsky1(float)");
  r_sky1 = rsky1;
}

/** Sets the radius of the outer sky aperture
 * \param rsky2 radius of the outer sky aperture, unbinned pixels
 */
void Ultracam::Aperture::set_rsky2(float rsky2){
  if(bad_aper(r_star,r_sky1,rsky2))
    throw Ultracam_Error("Invalid outer sky radius in void Ultracam::Aperture::set_rsky2(float)");
  r_sky2 = rsky2;
}

/** Sets all the radii of an aperture at once
 * \param rstar radius of star aperture, unbinned pixels
 * \param rsky1 inner radius of sky annulus, unbinned pixels
 * \param rsky2 outer radius of sky annulus, unbinned pixels
 */
void Ultracam::Aperture::set_radii(float rstar, float rsky1, float rsky2) {
  if(bad_aper(rstar,rsky1,rsky2))
    throw Ultracam_Error("Invalid outer sky radius in void Ultracam::Aperture::set_radii(float, float, float)");
  r_star = rstar;
  r_sky1 = rsky1;
  r_sky2 = rsky2;
}

/** Set all position and size parameters of an aperture
 * \param xr    reference X position, unbinned pixels
 * \param yr    reference Y position, unbinned pixels
 * \param xoff  X offset
 * \param yoff  Y offset
 * \param rstar target aperture radius
 * \param rstar target aperture radius
 * \param rsky1 inner radius of sky annulus
 * \param rsky2 outer radius of sky annulus
 */
void Ultracam::Aperture::set(double xr, double yr, double xoff, 
		   double yoff, float rstar, float rsky1, 
		   float rsky2){
    if(bad_aper(rstar,rsky1,rsky2))
      throw Ultracam_Error("Invalid aperture radii in "
		   "void Ultracam::Aperture::set(double, double, double,"
		   " double, float, float, float)");
    x_r = xr;
    y_r = yr;
    x_off = xoff;
    y_off = yoff;
    r_star = r_star;
    r_sky1 = r_sky1;
    r_sky2 = r_sky2;
}

/** how_far returns a number that can be used to rank the distances
 * of windows from a point. Points inside the star aperture
 * are strongly favoured but otherwise it measures how close one is
 * to the edge of star circle.
 * \param x X coordinate of point to test
 * \param y Y coordinate of point to test
 * \return number to determine the relative closeness of the point to the aperture.
 */
float Ultracam::Aperture::how_far(float x, 
				    float y) const {
  using Subs::sqr;
  float d = sqrt(sqr(x-xpos()) + sqr(y-ypos()));
  if(d < r_star){
    return (d-10000.);
  }else{
    return (d-r_star);
  }
}

/** near_enough returns true/false according to whether a point is 'near enough'
 * The concept of 'near enough' is defined for apertures by this routine.
 * \param x X coordinate of point to test
 * \param y Y coordinate of point to test
 */
bool Ultracam::Aperture::near_enough(float x, float y) const {
  using Subs::sqr;
  const float BORDER = 3.;
  float d = sqrt(sqr(x-xpos()) + sqr(y-ypos()));
  return (d < 1.2*r_sky2 + BORDER);
}

/** Deletes mask i. If somehow you know which mask you want to delete,
 * this routine then does the deletion. Look at setaper for an example
 * of a program to determine which to delete
 * \param i the index of the mask 0 to nmask-1
 */
void Ultracam::Aperture::del_mask(int i){
  if(i < 0 || i >= int(mask_.size()))
    throw Ultracam_Error("void Ultracam::Aperture::del_mask(int): mask index is out of range");
  mask_.erase(mask_.begin()+i);
}

/** Deletes extra star aperture i. If somehow you know which mask you want to delete,
 * this routine then does the deletion. Look at setaper for an example
 * of a program to determine which to delete
 * \param i the index of the mask 0 to nmask-1
 */
void Ultracam::Aperture::del_extra(int i){
  if(i < 0 || i >= int(extra_.size()))
    throw Ultracam_Error("void Ultracam::Aperture::del_extra(int): extra aperture index is out of range");
  extra_.erase(extra_.begin()+i);
}


/** This function checks a potential set of aperture radii to see if they
 * are OK.
 * \param rstar star aperture radius
 * \param rsky1 inner sky annulus radius
 * \param rsky2 outer sky annulus radius
 * \relates Ultracam::Aperture
 */
bool Ultracam::bad_aper(float rstar, float rsky1, 
	      float rsky2){
  return (rstar <= 0. || rsky1 <= 0. || rsky1 >= rsky2);
}

/** This function draws circles representing an Ultracam::Aperture of the right
 * size and position, with colours to represent various states.
 * \param aperture the aperture to draw
 * \relates Ultracam::Aperture
 */
void Ultracam::pgline(const Ultracam::Aperture& aperture) {
  cpgsfs(2);
  float x = aperture.xpos(), y = aperture.ypos();

  int ci;
  cpgqci(&ci);
  if(aperture.ref()) cpgsci(Ultracam::Aperture::Ref_col);
  if(!aperture.valid()) cpgsci(Ultracam::Aperture::Invalid_col);
  cpgcirc(x,y,aperture.rstar());
  cpgcirc(x,y,aperture.rsky1());
  cpgcirc(x,y,aperture.rsky2());
  if(aperture.linked())
    cpgarro(aperture.xpos(),aperture.ypos(),aperture.xref(),aperture.yref());
  cpgsls(2);
  cpgsci(6);
  for(int i=0; i<aperture.nmask(); i++){
    cpgmove(x,y);
    cpgdraw(x+aperture.mask(i).x,y+aperture.mask(i).y);
    cpgcirc(x+aperture.mask(i).x,y+aperture.mask(i).y,aperture.mask(i).z);
  }
  cpgsls(1);
  cpgsci(7);
  for(int i=0; i<aperture.nextra(); i++){
    cpgmove(x,y);
    cpgdraw(x+aperture.extra(i).x,y+aperture.extra(i).y);
    cpgcirc(x+aperture.extra(i).x,y+aperture.extra(i).y,aperture.rstar());
  }
  cpgsci(ci);

}

/**
 * This function labels an Ultracam::Aperture.
 * \param aperture the aperture to label.
 * \param lab the label to use. Follows PGPLOT codes.
 * \relates Ultracam::Aperture
 */
void Ultracam::pgptxt(const Ultracam::Aperture& aperture, const std::string& lab){

  float x1, x2, y1, y2;
  cpgqwin(&x1,&x2,&y1,&y2);
  float xr = (x2-x1)/20., yr = (y2-y1)/20.;
  x1 -= xr;
  x2 += xr; 
  y1 -= yr;
  y2 += yr;

  float x = aperture.xpos()-aperture.rsky2();
  float y = aperture.ypos()-aperture.rsky2();

  int ci;
  cpgqci(&ci);
  if(aperture.ref()) cpgsci(Ultracam::Aperture::Ref_col);

  if(x > std::min(x1,x2) && x < std::max(x1,x2) &&
     y > std::min(y1,y2) && y < std::max(y1,y2))
    cpgptxt(x,y,0.,1.2,lab.c_str());

  if(aperture.ref()){
    x = aperture.xpos()+aperture.rsky2();
    if(x > std::min(x1,x2) && x < std::max(x1,x2) &&
       y > std::min(y1,y2) && y < std::max(y1,y2))
      cpgptxt(x,y,0.,1.2,"*");
  }
  if(aperture.ref()) cpgsci(ci);
}

// 1-line ASCII output/input
std::ostream& Ultracam::operator<<(std::ostream& s, const Ultracam::Aperture& obj){
  s << "x,y = " << obj.xref() << ", " << obj.yref()
    << "; x_off,y_off = " << obj.xoff() << ", " << obj.yoff()
    << "; rstar,rsky1,rsky2 = " << obj.rstar() << ", " 
    << obj.rsky1() << ", " << obj.rsky2() << "; ref = " 
    << obj.ref() << "; state = " << obj.ap_ok;

  s << "; masked =";
  for(int i=0; i<obj.nmask(); i++)
    s << " " << obj.mask(i);
  
  s << "; extra =";
  for(int i=0; i<obj.nextra(); i++)
    s << " " << obj.extra(i);

  return s;
}

std::istream& Ultracam::operator>>(std::istream& s, Ultracam::Aperture& obj){
  char ch;
  double xr, yr, xoff, yoff;
  float rstar, rsky1, rsky2;
  bool refstar, apok;
  std::vector<Ultracam::sky_mask> mask;
  std::vector<Ultracam::extra_star> extra;
  
  // x,y
  while(s.get(ch) && ch != '=');
  if(!s || !(s >> xr) || !s.get(ch) || !(s >> yr)) 
    throw Ultracam_Error("Invalid input into Ultracam::Aperture::operator>> (1)");
  
  // xoff,yoff
  while(s.get(ch) && ch != '=');
  if(!s || !(s >> xoff) || !s.get(ch) || !(s >> yoff)) 
    throw 
      Ultracam_Error("Invalid input into Ultracam::Aperture::operator>> (2)");
  
  // aperture radii
  while(s.get(ch) && ch != '=');
  if(!s || !(s >> rstar) || !s.get(ch) || !(s >> rsky1) 
     || !s.get(ch) || !(s >> rsky2)) 
    throw 
      Ultracam_Error("Invalid input into Ultracam::Aperture::operator>> (3)");

  if(bad_aper(rstar,rsky1,rsky2))
    throw 
      Ultracam_Error("Invalid aperture radii in "
			       "istream& operator>>(std::istream&, Ultracam::Aperture&)");

  // reference or not
  while(s.get(ch) && ch != '=');
  if(!s || !(s >> refstar))
    throw 
      Ultracam_Error("Invalid input into Ultracam::Aperture::operator>> (4)");

  // aperture ok or not
  while(s.get(ch) && ch != '=');
  if(!s || !(s >> apok))
    throw 
      Ultracam_Error("Invalid input into Ultracam::Aperture::operator>> (5)");

  // offset masks
  while(s.get(ch) && ch != '=');
  Ultracam::sky_mask tmask;
  while(s.get(ch) && ch != ';' && ch != '\n'){
    s >> tmask;
    if(!s)  throw Ultracam_Error("Invalid input into Ultracam::Aperture::operator>> (6)");
    mask.push_back(tmask);
  }

  // extra stars
  while(s.get(ch) && ch != '=');
  Ultracam::extra_star textr;
  while(s.get(ch) && ch != '\n'){
    s >> textr;
    if(!s)  throw Ultracam_Error("Invalid input into Ultracam::Aperture::operator>> (7)");
    extra.push_back(textr);
  }

  obj.x_r      = xr;
  obj.y_r      = yr;
  obj.x_off    = xoff;
  obj.y_off    = yoff;
  obj.r_star   = rstar;
  obj.r_sky1   = rsky1;
  obj.r_sky2   = rsky2;
  obj.ref_star = refstar;
  obj.ap_ok    = apok;
  obj.mask_    = mask;
  obj.extra_   = extra;

  return s;
}

/** This function checks that two Ultracam::Apertures do not clash in any way, as
 * one must know when adding Ultracam::Apertures to a group. It is in fact a dummy
 * function because no restrictions are placed upon overlap of apertures.
 * Nevertheless, its is required for compatibility with other objects when
 * grouped under the CCD class.
 * \param obj1 first aperture
 * \param obj2 second aperture
 */
bool Ultracam::clash(const Ultracam::Aperture& obj1, const Ultracam::Aperture& obj2){
  return false;
}























