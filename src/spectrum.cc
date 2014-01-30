#include <cmath>
#include "trm/subs.h"
#include "trm/constants.h"
#include "cpgplot.h"
#include "trm/spectrum.h"

/** Returns a number that monotonically increase with the distance of the
 * the Ultracam::Spectrum from the coordinates supplied. Needed for saying which Ultracam::Spectrum
 * is closest to a point.
 * \param x the X ordinate of the point of interest
 * \param y the Y ordinate of the point of interest
 */
float Ultracam::Spectrum::how_far(float x, float y) const {
  return Subs::abs(y-position.get_value(x));
}

// For consistency with the CCD class.

/**
 * Ultracam::Spectrum objects are bundled up in to a field of objects by the
 * CCD<Obj> class. This class requires that all supported objects
 * have a function which determines whether any two "clash". This is
 * that function for Ultracam::Spectrum. In fact, it is a dummy function since no
 * restrictions are placed upon Ultracam::Spectrum objects, so you should never be
 * using this outside of CCD<Obj>
 * \param spectrum1 first Spectrum
 * \param spectrum2 second Spectrum
 * \return \c true if they clash (they never do).
 */
bool Ultracam::clash(const Ultracam::Spectrum& spectrum1, const Ultracam::Spectrum& spectrum2){
  return false;
}

// ASCII output
std::ostream& Ultracam::operator<<(std::ostream& s, const Ultracam::Spectrum& obj){
  s << "Position  = " << obj.position  << " \n"
    << "FWHM      = " << obj.fwhm      << " \n"
    << "Continuum = " << obj.continuum << " \n"
    << "Lines     = " << obj.lines     << " \n";
  return s;
}

// ASCII input
std::istream& Ultracam::operator>>(std::istream& s, Ultracam::Spectrum& obj){
  char ch;

  while(s.get(ch) && ch != '=');
  s >> obj.position;
  if(!obj.position.size()){
    s.setstate(std::ios_base::failbit);
    return s;
  }
  while(s.get(ch) && ch != '=');
  s >> obj.fwhm;
  if(!obj.fwhm.size()){
    s.setstate(std::ios_base::failbit);
    return s;
  }
  while(s.get(ch) && ch != '=');
  s >> obj.continuum;
  if(!obj.continuum.size()){
    s.setstate(std::ios_base::failbit);
    return s;
  }
  while(s.get(ch) && ch != '=');
  s >> obj.lines;

  return s;

}

std::ostream& Ultracam::operator<<(std::ostream& s, const Spectrum::Line& line) {
  s << line.centre << " " << line.height << " " << line.fwhm << " "
    << line.T0     << " " << line.period << " " << line.semiamp;
  return s;
}

std::istream& Ultracam::operator>>(std::istream& s, Spectrum::Line& line) {
  s >> line.centre >> line.height >> line.fwhm >> line.T0 >> line.period >> line.semiamp;
  return s;
}

double Ultracam::Spectrum::get_position(double x) const {
  return position.get_value(x);
}

double Ultracam::Spectrum::get_fwhm(double x) const {
  return fwhm.get_value(x);
}

double Ultracam::Spectrum::get_continuum(double x) const {
  return continuum.get_value(x);
}

double Ultracam::Spectrum::get_line(double x, double time) {

  if(compute_positions || time != told){
    compute_positions = false;
    told  = time;
    xcen.resize(lines.size());
    for(int i=0; i<lines.size(); i++){
      double phase  = (time - lines[i].T0)/lines[i].period;
      xcen[i]       = lines[i].centre + lines[i].semiamp*sin(Constants::TWOPI*phase);
    }
  }

  // Now computations that happen every time
  double sum = 0.;
  for(int i=0; i<lines.size(); i++){
    double diff = Subs::sqr((x-xcen[i])/(lines[i].fwhm/Constants::EFAC))/2.;
    if(diff < 80.)
      sum += lines[i].height*exp(-diff);
  }
  return sum;
}

double Ultracam::Spectrum::get_line(double x) const {

  // Now computations that happen every time
  double sum = 0.;
  for(int i=0; i<lines.size(); i++){
    double diff = Subs::sqr((x-lines[i].centre)/(lines[i].fwhm/Constants::EFAC))/2.;
    if(diff < 80.)
      sum += lines[i].height*exp(-diff);
  }
  return sum;
}

void Ultracam::Spectrum::add_line(const Line& line) {
  lines.push_back(line);
  compute_positions = true;
}

void Ultracam::Spectrum::set_position(const Subs::Poly& position){
  this->position = position;
}

void Ultracam::Spectrum::set_fwhm(const Subs::Poly& fwhm){
  this->fwhm = fwhm;
}

void Ultracam::Spectrum::set_continuum(const Subs::Poly& continuum){
  this->continuum = continuum;
}

