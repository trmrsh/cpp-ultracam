#include "cpgplot.h"
#include "trm/ultracam.h"

void Ultracam::def_col(bool reverse){
    if(reverse){
    cpgscr(0,1,1,1);
    cpgscr(1,0,0,0);
    cpgscr(2,0.4,0,0);
    cpgscr(3,0,0.4,0);
    cpgscr(4,0,0,0.4);
    }else{
    cpgscr(0,0,0,0);
    cpgscr(1,1,1,1);
    cpgscr(2,1,0,0);
    cpgscr(3,0,1,0);
    cpgscr(4,0,1,0);
    }
}

