#include <iostream>
#include "cpgplot.h"
#include "trm_subs.h"
#include "trm_defect.h"
#include "trm_ultracam.h"

/** Constructs a pixel defect
 * \param x        X position of defect
 * \param y        Y position of defect
 * \param severity How bad the defect is
 */
Ultracam::Defect::Defect(float x, float y, how_bad severity) : 
    x1_(x), y1_(y), x2_(x), y2_(y), severity_(severity), cps_(-1) {} 

/** Constructs a hot pixel defect
 * \param x        X position of defect
 * \param y        Y position of defect
 * \param cps      counts per second
 */
Ultracam::Defect::Defect(float x, float y, int cps) : 
    x1_(x), y1_(y), x2_(x), y2_(y), severity_(Ultracam::Defect::MODERATE), cps_(cps) {} 

/** Constructs a line defect
 * \param x1   X position of one end of line defect
 * \param y1   Y position of one end of line defect
 * \param x2   X position of other end of line defect
 * \param y2   Y position of other end of line defect
 * \param severity How bad the defect is
 */
Ultracam::Defect::Defect(float x1, float y1, float x2, float y2, how_bad severity)
    : x1_(x1), y1_(y1), x2_(x2), y2_(y2), severity_(severity), cps_(-1) {} 
  
/** This function returns a number that increases with the distance from
 * the coordinates entered as its arguments. This can be used to work out
 * which of several Ultracam::Defects is closest to a particular position and is needed
 * by the CCD class.
 * \param x X position of point of interest
 * \param y Y position of point of interest
 */
float Ultracam::Defect::how_far(float x, float y) const {
  if((x == x1_ && y == y1_) || (x == x2_ && y == y2_)){
    return 0.;
  }else if(x1_ == x2_ && y1_ == y2_){
    return sqrt(Subs::sqr(x-x1_) + Subs::sqr(y-y1_));
  }else{
    // lambda is a parameter which says where on the line defect is closest to the x,y.
    // It is 0 at x1_,y1_ and 1 at the other end. 
    float lambda = ((x-x1_)*(x2_-x1_)+(y-y1_)*(y2_-y1_))/(Subs::sqr(x2_-x1_)+Subs::sqr(y2_-y1_));
    if(lambda <= 0.){
      return sqrt(Subs::sqr(x-x1_) + Subs::sqr(y-y1_));
    }else if(lambda >= 1.){
      return sqrt(Subs::sqr(x-x2_) + Subs::sqr(y-y2_));
    }else{
      return sqrt(Subs::sqr(x-x1_) + Subs::sqr(y-y1_) - Subs::sqr(lambda)*(Subs::sqr(x2_-x1_)+Subs::sqr(y2_-y1_)));
    }
  }
}

/**
 * When picking a Ultracam::Defect by position, it is useful to know when a position is near enough
 * to consider that the Ultracam::Defect has indeed been selected. This function accomplishes this,
 * returning "true" if the condition is met. It is needed by the CCD class.
 * \param x X position of point of interest
 * \param y Y position of point of interest
 */
bool Ultracam::Defect::near_enough(float x, float y) const {
  return (how_far(x,y) < 10.);
}

/** This routine works out whether a defect may affect a pixel and
 * returns a value suited to setting a bad pixel file. The defect must
 * pass within 1/sqrt(2) of a pixel for anything to happen
 * \param ix  the X index of the pixel, starting at 1 on left of CCD
 * \param iy  the Y index of the pixel, starting at 1 at bottom of CCD
 * \param low the value to return if a weak defect
 * \param high the value to return if a strong defect
 */
float Ultracam::Defect::bad_value(int ix, int iy, float low, float high) const {
  float dmin;
  if(is_a_pixel()){
    dmin = Subs::sqr(ix-x1_) + Subs::sqr(iy-y1_);
  }else{
    double ex  = x2_ - x1_;
    double ey  = y2_ - y1_;
    double pax = ix  - x1_;
    double pay = iy -  y1_;
    double lambda = (ex*pax+ey*pay)/(Subs::sqr(ex)+Subs::sqr(ey));
    lambda = std::min(1., std::max(0., lambda));
    dmin = Subs::sqr(pax - lambda*ex) + Subs::sqr(pay - lambda*ey);
  }
  if(dmin <= 0.5f)
    return severity_ == MODERATE ? low : high;
  else
    return 0.f;
}

/** This function draws defects as either points or lines, with the worst ones
 * 1.5 times as large as the moderate ones when pixels, and twice as thick with
 * solid lines when line defects.
 * \param defect the defect to plot
 * \relates Ultracam::Defect
 */
void Ultracam::pgline(const Ultracam::Defect& defect) {
  cpgsave();
  int ptype = 17;
      
  if(defect.effect() == Ultracam::Defect::MODERATE){
      cpgslw(2);
      cpgsls(2);
      cpgsch(1);
      ptype = 17;
  }else if(defect.effect() == Ultracam::Defect::DISASTER){
      cpgslw(4);
      cpgsls(1);
      cpgsch(1.5);
      ptype = 18;
  }
  
  if(defect.is_a_pixel()){
      cpgpt1(defect.x1(),defect.y1(),ptype);
  }else if(defect.is_a_hot_pixel()){
      cpgpt1(defect.x1(),defect.y1(),1);
      cpgptxt(defect.x1(),defect.y1(),0,0,Subs::str(defect.how_hot()).c_str());
  }else{
      cpgmove(defect.x1(),defect.y1());
      cpgdraw(defect.x2(),defect.y2());
  }
  cpgunsa();
}

/** Dummy routine needed by CCD class
 */
void Ultracam::pgptxt(const Ultracam::Defect& defect, const std::string& lab){}

// 1-line ASCII output/input

std::ostream& Ultracam::operator<<(std::ostream& s, const Ultracam::Defect& obj){
    s << "defect type = ";
    if(obj.is_a_pixel()){
	s << "pixel located at x,y = " << obj.x1() << ", " << obj.y1();
    }else if(obj.is_a_hot_pixel()){
	s << "hot pixel located at x,y = " << obj.x1() << ", " << obj.y1();
    }else{
	s << "line extending from x,y = " << obj.x1() << ", " << obj.y1() 
	  << " to x,y = " << obj.x2() << ", " << obj.y2();
    }
    if(obj.is_a_hot_pixel()){
	s << ", counts/sec = " << obj.how_hot();
    }else if(obj.effect() == Ultracam::Defect::MODERATE){
	s << ", severity = moderate";
    }else if(obj.effect() == Ultracam::Defect::DISASTER){
	s << ", severity = disaster";
    }
    return s;
}

std::istream& Ultracam::operator>>(std::istream& s, Ultracam::Defect& obj){
  char ch;
  std::string dtype, stype;
  float x1, x2, y1, y2;
  int cps = -1;

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> dtype))
    throw Ultracam::Ultracam_Error("Invalid input into Ultracam::Defect::operator>> (1)");
  
  while(s.get(ch) && ch != '=');
  if(!s || !(s >> x1) || !s.get(ch) || !(s >> y1)) 
    throw Ultracam::Ultracam_Error("Invalid input into Ultracam::Defect::operator>> (2)");
  
  if(dtype == "line"){
      while(s.get(ch) && ch != '=');
      if(!s || !(s >> x2) || !s.get(ch) || !(s >> y2)) 
	  throw Ultracam::Ultracam_Error("Invalid input into Ultracam::Defect::operator>> (3)");
  }else if(dtype == "hot"){
      while(s.get(ch) && ch != '=');
      if(!s || !(s >> cps)) 
	  throw Ultracam::Ultracam_Error("Invalid input into Ultracam::Defect::operator>> (3.5)");
  }else if(dtype != "pixel"){
      throw Ultracam::Ultracam_Error("Invalid input into Aperture::operator>> (4)");
  }

  if(dtype == "hot"){
      obj.severity_ = Ultracam::Defect::MODERATE;
      obj.cps_      = cps;
  }else{
      while(s.get(ch) && ch != '=');

      if(!s || !(s >> stype))
	  throw Ultracam::Ultracam_Error("Invalid input into Ultracam::Defect::operator>> (5)");
      if(stype == "moderate"){
	  obj.severity_ = Ultracam::Defect::MODERATE;
      }else if(stype == "disaster"){
	  obj.severity_ = Ultracam::Defect::DISASTER;
      }else{
	  throw Ultracam::Ultracam_Error("Invalid input into Aperture::operator>> (6)");
      }
      obj.cps_ = -1;
  }
  
  obj.x1_ = x1;
  obj.y1_ = y1;
  if(dtype == "line"){
    obj.x2_ = x2;
    obj.y2_ = y2;
  }else{
    obj.x2_ = x1;
    obj.y2_ = y1;
  }

  return s;
}

/**
 * This function checks that two Ultracam::Defects do not clash in any way, as
 * one must know when adding Ultracam::Defects to a group. It is in fact a dummy
 * function because no restrictions are placed upon overlap of Ultracam::Defects.
 * Nevertheless, it is required for compatibility with other objects when
 * grouped under the CCD class.
 * \param obj1 first defect
 * \param obj2 second defect
 */
bool Ultracam::clash(const Ultracam::Defect& obj1, const Ultracam::Defect& obj2){
  return false;
}

/** Changes the coordinates of a defect to reflect new CCD position. This
 * is needed to allow defects of different CCDs to be plotted over each other.
 * See rtplot for an example of this.
 * \param trans   transformation structure
 * \param forward apply the transformation in the forward direction (else reverse).
 */
void Ultracam::Defect::transform(const Ultracam::Transform& trans, bool forward){
  double rangle = Constants::TWOPI*trans.angle/360.;
  double cosine = cos(rangle);
  double sine   = sin(rangle);
  float xnew, ynew;

  if(forward){
    xnew = trans.scale*(cosine*x1_ -   sine*y1_) + trans.xshift;
    ynew = trans.scale*(  sine*x1_ + cosine*y1_) + trans.yshift;
    x1_ = xnew;
    y1_ = ynew;

    xnew = trans.scale*(cosine*x2_ -   sine*y2_) + trans.xshift;
    ynew = trans.scale*(  sine*x2_ + cosine*y2_) + trans.yshift;
    x2_ = xnew;
    y2_ = ynew;

  }else{

    x1_ -= trans.xshift;
    y1_ -= trans.yshift;
    x1_ /= trans.scale;
    y1_ /= trans.scale;
    xnew = cosine*x1_ +   sine*y1_;
    ynew = - sine*x1_ + cosine*y1_;
    x1_ = xnew;
    y1_ = ynew;

    x2_ -= trans.xshift;
    y2_ -= trans.yshift;
    x2_ /= trans.scale;
    y2_ /= trans.scale;
    xnew = cosine*x2_ +   sine*y2_;
    ynew = - sine*x2_ + cosine*y2_;
    x2_ = xnew;
    y2_ = ynew;

  }
}




















