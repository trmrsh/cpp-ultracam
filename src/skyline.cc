#include <cmath>
#include "trm/subs.h"
#include "trm/constants.h"
#include "trm/skyline.h"

/** Returns a number that monotonically increase with the distance of the
 * the Ultracam::Skyline from the coordinates supplied. Needed for saying which Ultracam::Skyline
 * is closest to a point.
 * \param x the X ordinate of the point of interest
 * \param y the Y ordinate of the point of interest
 */
float Ultracam::Skyline::how_far(float x, float y) const {
  return Subs::abs(x-position.get_value(y));
}

// For consistency with the CCD class.

/**
 * Ultracam::Skyline objects are bundled up in to a field of objects by the
 * CCD<Obj> class. This class requires that all supported objects
 * have a function which determines whether any two "clash". This is
 * that function for Ultracam::Skyline. In fact, it is a dummy function since no
 * restrictions are placed upon Ultracam::Skyline objects, so you should never be
 * using this outside of CCD<Obj>
 * \param skyline1 first Skyline
 * \param skyline2 second Skyline
 * \return \c true if they clash (they never do).
 */
bool Ultracam::clash(const Ultracam::Skyline& skyline1, const Ultracam::Skyline& skyline2){
  return false;
}

// ASCII output
std::ostream& Ultracam::operator<<(std::ostream& s, const Ultracam::Skyline& obj){
  s << "Position  = " << obj.position  << " \n"
    << "FWHM      = " << obj.fwhm      << " \n"
    << "Strength  = " << obj.strength  << " \n";
  return s;
}

// ASCII input
std::istream& Ultracam::operator>>(std::istream& s, Ultracam::Skyline& obj){
  char ch;

  while(s.get(ch) && ch != '=');
  s >> obj.position;
  while(s.get(ch) && ch != '=');
  s >> obj.fwhm;
  while(s.get(ch) && ch != '=');
  s >> obj.strength;

  return s;

}

double Ultracam::Skyline::get_position(double y) const {
  return position.get_value(y);
}

double Ultracam::Skyline::get_fwhm(double y) const {
  return fwhm.get_value(y);
}

double Ultracam::Skyline::get_strength() const {
  return strength;
}

void Ultracam::Skyline::set_position(const Subs::Poly& position){
  this->position = position;
}

void Ultracam::Skyline::set_fwhm(const Subs::Poly& fwhm){
  this->fwhm = fwhm;
}

void Ultracam::Skyline::set_strength(double strength){
  this->strength = strength;
}

