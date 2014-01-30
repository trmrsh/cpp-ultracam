#include <cmath>
#include "trm/target.h"
#include "trm/subs.h"
#include "trm/constants.h"
#include "cpgplot.h"

/** The profile is normalised by the number of counts. For computing it
 * one needs the height. This function computes this.
 */
float Ultracam::Target::height() const {
  return get_counts()*(get_beta()-1)*sqrt(det())/Constants::PI;
}

/** This routine modifies the axx, axy, ayy coefficients to simulate the effect
 * of blurring by a particular seeing value.
 * \param seeing the FWHM of the seeing.
 */
void Ultracam::Target::blurr(float seeing) {

  // Compute eigen-values
  float lambda1, lambda2, fac1, fac2;
  fac1    = (get_axx()+get_ayy())/2.;
  fac2    = sqrt(Subs::sqr(get_axx()-get_ayy())+4.*Subs::sqr(get_axy()))/2.;
  lambda1 = fac1-fac2;
  lambda2 = fac1+fac2;

  // Now the (normliased) eigen-vectors
  float x1, y1, x2, y2;
  if(lambda1 == lambda2){

    x1 = 1.;
    y1 = 0.;
    x2 = 0.;
    y2 = 1.;

  }else{

    if(lambda1 != get_axx() || get_axy() != 0.){
      x1    = get_axy();
      y1    = lambda1-get_axx();
    }else{
      x1    = lambda1-get_ayy();
      y1    = get_axy();
    }
    float norm = sqrt(x1*x1+y1*y1);
    x1 /= norm;
    y1 /= norm;

    if(lambda2 != get_axx() || get_axy() != 0.){
      x2    = get_axy();
      y2    = lambda2-get_axx();
    }else{
      x2    = lambda2-get_ayy();
      y2    = get_axy();
    }
    norm = sqrt(x2*x2+y2*y2);
    x2 /= norm;
    y2 /= norm;
  }

  // Now modify the eigen-values essentially by adding the seeing in quadrature
  // while accounting for the scale factor as a result of the Moffat profile
  float scale = seeing*seeing/(pow(2.,1./get_beta())-1.);
  lambda1 = 1./(1./lambda1 + scale);
  lambda2 = 1./(1./lambda2 + scale);

  // Now re-compute the a coefficients
  axx = lambda1*x1*x1 + lambda2*x2*x2;
  axy = lambda1*x1*y1 + lambda2*x2*y2;
  ayy = lambda1*y1*y1 + lambda2*y2*y2;

}


/** Constructs a Target of full generality.
 * \param xc X ordinate of centre of source
 * \param yc Y ordinate of centre of source
 * \param counts height of source
 * \param axx \f$a_{xx}\f$ coefficient.
 * \param axy \f$a_{xy}\f$ coefficient.
 * \param ayy \f$a_{yy}\f$ coefficient.
 * \param beta \f$\beta\f$ coefficient.
 * \exception The \c a coefficients must be positive-definite and the Moffat \f$\beta\f$ exponent
 * must be larger than 1. If not an
 * Ultracam::Ultracam_Error will be thrown.
 */
Ultracam::Target::Target(float xc, float yc, float counts,
           float axx, float axy,
           float ayy, double beta) :
  xc(xc), yc(yc), counts(counts), axx(axx), axy(axy), ayy(ayy), mbeta(beta) {

  if(bad_targ(axx,axy,ayy) || mbeta <= 1.)
    throw
      Ultracam::Ultracam_Error("Invalid width parameters in Ultracam::Target(float, float, float, float, float, float, double)");
}

/**
 * The \c a coefficients must describe a profile that tends to zero at large
 * distances from the centre. They therefore must form a positive definite matrix.
 * bad_targ checks that this is the case and returns \c true if the coefficients
 * fail the test.
 * \param axx the XX coefficient
 * \param axy the XY coefficient
 * \param ayy the YY coefficient
 * \return \c true if the coefficients are \b not positive definite
 * \relates Ultracam::Target
 */
bool Ultracam::bad_targ(float axx, float axy, float ayy){
  return (axx + ayy <= 0. || axx*ayy-axy*axy <= 0.);
}

/** Set the XX shape coefficient. The coefficients are checked for
 * being positive definte first, and an exception is thrown if they are
 * not. The Target is left in its old state.
 * \param axx the XX coefficient
 * \exception Ultracam::Ultracam_Error if coefficients are not valid.
 */
void Ultracam::Target::set_axx(float axx){
  if(bad_targ(axx,axy,ayy))
    throw Ultracam::Ultracam_Error("Invalid xx width in Ultracam::Target::set_axx(float)");
  this->axx = axx;
}

/** Set the XY shape coefficient. The coefficients are checked for
 * being positive definite first, and an exception is thrown if they are
 * not. The Target is left in its old state.
 * \param axy the XY coefficient
 * \exception Ultracam::Ultracam_Error if coefficients are not valid.
 */
void Ultracam::Target::set_axy(float axy){
  if(bad_targ(axx,axy,ayy))
    throw Ultracam::Ultracam_Error("Invalid xy width in Ultracam::Target::set_axy(float)");
  this->axy = axy;
}

/** Set the YY shape coefficient. The coefficients are checked for
 * being positive definite first, and an exception is thrown if they are
 * not. The Target is left in its old state.
 * \param ayy the YY coefficient
 * \exception Ultracam::Ultracam_Error if coefficients are not valid.
 */
void Ultracam::Target::set_ayy(float ayy){
  if(bad_targ(axx,axy,ayy))
    throw Ultracam::Ultracam_Error("Invalid yy width in Ultracam::Target::set_ayy(float)");
  this->ayy = ayy;
}

/** Set the Moffat \f$\beta\f$ exponent. It must be greater than 1 to ensure
 * convergence of flux.
 * \param beta
 * \exception Ultracam::Ultracam_Error if beta <= 1
 */
void Ultracam::Target::set_beta(double beta){
  if(beta <= 1.)
    throw Ultracam::Ultracam_Error("Invalid Moffat beta exponent in Ultracam::Target::set_beta(double)");
  mbeta = beta;
}

/** Set all of the shape coefficients. The coefficients are checked for
 * being positive definite first, and an exception is thrown if they are
 * not. The Target is left in its old state.
 * \param axx the XX coefficient
 * \param axy the XY coefficient
 * \param ayy the YY coefficient
 * \exception Ultracam::Ultracam_Error if coefficients are not valid.
 */
void Ultracam::Target::set(float axx, float axy, float ayy){
  if(bad_targ(axx,axy,ayy))
    throw Ultracam::Ultracam_Error("Invalid widths in Ultracam::Target::set(float,float,float)");
  this->axx = axx;
  this->axy = axy;
  this->ayy = ayy;
}

/** Returns a number that monotonically increase with the distance of the
 * the Ultracam::Target from the coordinates supplied. Needed for saying which Ultracam::Target
 * is closest to a point.
 * \param x the X ordinate of the point of interest
 * \param y the Y ordinate of the point of interest
 */
float Ultracam::Target::how_far(float x, float y) const {
  using Subs::sqr;
  return sqr(x-xc) + sqr(y-yc);
}

/** Answers the question of whether a point is "close enough" to count as having
 * been picked. Prevents random cursor clicks from deleting whatever object is nearest.
 * \param x the X ordinate of the point of interest
 * \param y the Y ordinate of the point of interest
 */
bool Ultracam::Target::near_enough(float x, float y) const {
  return (height() == 0. && fabs(x-xc) < 10. && fabs(y-yc) < 10.) || height(x-xc,y-yc) > 1.e-3*height();
}

/** This function computes the distance one must go in X and Y to guarantee that
 * the height of the profile is at least as small as level. This basically defines a box
 * around the Target outside of which one can ignore it.
 * \param level the relative height we are aiming for. Must be > 0 and < 1
 * \param dx how far one must go in X (returned)7
 * \param dy how far one must go in Y (returned)
 */
void Ultracam::Target::dist(float level, float& dx, float& dy) const {
  if(level <= 0.f || level >= height())
    throw Ultracam::Ultracam_Error("void Ultracam::Target::dist(float, float&, float&): level out of range");
  using Subs::sqr;
  float delta = 1./pow(double(level/height()),1./get_beta())-1.;
  dx = sqrt(delta/(get_axx()-sqr(get_axy())/get_ayy()));
  dy = sqrt(delta/(get_ayy()-sqr(get_axy())/get_axx()));
}

/**
 * This represents a Ultracam::Target as an ellipse of size adjusted to match a certain
 * level in the profile of the Ultracam::Target
 * \param target the Ultracam::Target to plot
 * \param level the level to define the size of the ellipse plotted
 * \relates Ultracam::Target
 */
void Ultracam::pgline(const Ultracam::Target& target, float level) {
  using Subs::sqr;
  if(level > 0. && target.height() > level){
    float efac = sqrt(1./pow(double(level/target.height()), 1./target.get_beta())-1.);
    float lambda1, lambda2, fac1, fac2, x1, y1, x2, y2, scale;
    fac1    = (target.get_axx()+target.get_ayy())/2.;
    fac2    = sqrt(sqr(target.get_axx()-target.get_ayy())+4.*sqr(target.get_axy()))/2.;
    lambda1 = fac1-fac2;
    lambda2 = fac1+fac2;

    if(lambda1 == lambda2){
      x1 = efac/sqrt(lambda1);
      y1 = 0.;
      x2 = 0.;
      y2 = efac/sqrt(lambda2);
    }else{

      if(lambda1 != target.get_axx() || target.get_axy() != 0.){
    x1    = target.get_axy();
    y1    = lambda1-target.get_axx();
      }else{
    x1    = lambda1-target.get_ayy();
    y1    = target.get_axy();
      }
      scale = efac/sqrt(lambda1*(sqr(x1)+sqr(y1)));
      x1   *= scale;
      y1   *= scale;

      if(lambda2 != target.get_axx() || target.get_axy() != 0.){
    x2    = target.get_axy();
    y2    = lambda2-target.get_axx();
      }else{
    x2    = lambda2-target.get_ayy();
    y2    = target.get_axy();
      }
      scale = efac/sqrt(lambda2*(sqr(x2)+sqr(y2)));
      x2   *= scale;
      y2   *= scale;
    }

    float xp = target.get_xc() + x1;
    float yp = target.get_yc() + y1;

    cpgmove(xp,yp);
    const int NPLOT = 200;
    float theta, c, s;
    for(int i=0; i<NPLOT; i++){
      theta = Constants::TWOPI*float(i+1)/NPLOT;
      c = cos(theta);
      s = sin(theta);
      xp = target.get_xc() + x1*c + x2*s;
      yp = target.get_yc() + y1*c + y2*s;
      cpgdraw(xp,yp);
    }
  }
}

/** Labels a Target on a plot. This places a label to the lower-left of
 * a Ultracam::Target.
 * \param target the Ultracam::Target to plot
 * \param label the label to attach
 * \param level the level to define the size of the ellipse plotted
 * \relates Ultracam::Target
 */
void Ultracam::pgptxt(const Ultracam::Target& target, const std::string& label, float level) {

  if(level < 0. || level > target.height())
    level = 0.01*target.height();

  float dx, dy;
  target.dist(level, dx, dy);
  cpgptxt(target.get_xc()-dx,target.get_yc()-dy,0.,1.1,label.c_str());
}

// For consistency with the CCD class.

/**
 * Ultracam::Target objects are bundled up in to a field of objects by the
 * CCD<Obj> class. This class requires that all supported objects
 * have a function which determines whether any two "clash". This is
 * that function for Ultracam::Target. In fact, it is a dummy function since no
 * restrictions are placed upon Ultracam::Target objects, so you should never be
 * using this outside of CCD<Obj>
 * \param target1 first Target
 * \param target2 second Target
 * \return \c true if they clash (they never do).
 */
bool Ultracam::clash(const Ultracam::Target& target1, const Ultracam::Target& target2){
  return false;
}

// ASCII output
std::ostream& Ultracam::operator<<(std::ostream& s, const Ultracam::Target& target){
  s << "x,y = " << target.get_xc() << ", " << target.get_yc()
    << "; counts = " << target.get_counts()
    << "; xx,xy,yy = " << target.get_axx() << ", " << target.get_axy() << ", " << target.get_ayy()
    << "; beta = " << target.get_beta();
  return s;
}

// ASCII input
std::istream& Ultracam::operator>>(std::istream& s, Ultracam::Target& target){
  char ch;

  float  xc, yc;
  float  counts;
  float  axx, axy, ayy;
  double mbeta;

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> xc) || !s.get(ch) || !(s >> yc))
    throw Ultracam::Ultracam_Error("Invalid input into Ultracam::Target::operator>> (1)");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> counts))
    throw Ultracam::Ultracam_Error("Invalid input into Ultracam::operator>> (2)");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> axx) || !s.get(ch) ||
     !(s >> axy) || !s.get(ch) ||!(s >> ayy))
    throw Ultracam::Ultracam_Error("Invalid input into Ultracam::operator>> (3)");

  if(bad_targ(axx, axy, ayy))
    throw Ultracam::Ultracam_Error("Invalid Ultracam::Target in std::istream& "
         "operator>>(std::istream& s, Ultracam::Target& window): not positive definite");

  while(s.get(ch) && ch != '=');
  if(!s || !(s >> mbeta))
    throw Ultracam::Ultracam_Error("Invalid input into Ultracam::operator>> (4)");

  if(mbeta <= 1.)
    throw Ultracam::Ultracam_Error("Invalid Ultracam::Target in std::istream& "
         "operator>>(std::istream& s, Ultracam::Target& window): beta <= 1.");

  target.xc     = xc;
  target.yc     = yc;
  target.counts = counts;
  target.axx    = axx;
  target.axy    = axy;
  target.ayy    = ayy;
  target.mbeta  = mbeta;

  return s;
}





