#include "trm/ultracam.h"

int Ultracam::Ppars::npar() const {
  if(symm){
    if(ptype == GAUSSIAN){
      return 5;
    }else if(ptype == MOFFAT){
      return 6;
    }else{
      throw Ultracam_Error("Ultracam::Ppars::npar: unrecognised ptype");
    }
  }else{
    if(ptype == GAUSSIAN){
      return 7;
    }else if(ptype == MOFFAT){
      return 8;
    }else{
      throw Ultracam_Error("Ultracam::Ppars::npar: unrecognised ptype");
    }
  }
}

int Ultracam::Ppars::nmax() const {
  if(ptype == GAUSSIAN){
    return 7;
  }else if(ptype == MOFFAT){
    return 8;
  }else{
    throw Ultracam_Error("Ultracam::Ppars::nmax: unrecognised ptype");
  }
}

double Ultracam::Ppars::get_param(int i) const {
  if(symm){
    switch(i){
    case 0:
      return sky;
    case 1:
      return x;
    case 2:
      return y;
    case 3:
      return height;
    case 4:
      return a;
    case 5:
      return beta;
    default:
      throw Ultracam::Ultracam_Error("double Ultracam::Ppars::get_param(int) const: index out of range (1)");
    }
  }else{
    switch(i){
    case 0:
      return sky;
    case 1:
      return x;
    case 2:
      return y;
    case 3:
      return height;
    case 4:
      return a;
    case 5:
      return b;
    case 6:
      return c;
    case 7:
      return beta;
    default:
      throw Ultracam::Ultracam_Error("double Ultracam::Ppars::get_param(int) const: index out of range (2)");
    }
  }
}

void Ultracam::Ppars::set_param(int i, double val) {

  if(symm){
    switch(i){
    case 0:
      sky = val;
      return;
    case 1:
      x = val;
      return;
    case 2:
      y = val;
      return;
    case 3:
      height = val;
      return;
    case 4:
      a = val;
      return;
    case 5:
      beta = val;
      return;
    default:
      throw Ultracam::Ultracam_Error("double Ultracam::Ppars::set_param(int, double) const: index out of range (1)");
    }
  }else{
    switch(i){
    case 0:
      sky = val;
      return;
    case 1:
      x = val;
      return;
    case 2:
      y = val;
      return;
    case 3:
      height = val;
      return;
    case 4:
      a = val;
      return;
    case 5:
      b = val;
      return;
    case 6:
      c = val;
      return;
    case 7:
      beta = val;
      return;
    default:
      throw Ultracam::Ultracam_Error("double Ultracam::Ppars::set_param(int, double) const: index out of range (2)");
    }
  }

}


