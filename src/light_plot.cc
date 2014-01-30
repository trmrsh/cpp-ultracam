#include <map>
#include <deque>
#include <time.h>
#include "cpgplot.h"
#include "trm/subs.h"
#include "trm/constants.h"
#include "trm/plot.h"
#include "trm/ultracam.h"
#include "trm/reduce.h"

// Variables that are set by reading from the input file with read_reduce_file.
// Enclosed in a namespace for safety.

namespace Reduce {

    extern float   lightcurve_frac;
    extern X_UNITS lightcurve_xunits;
    extern float   lightcurve_max_xrange;
    extern float   lightcurve_extend_xrange;
    extern bool    lightcurve_linear;
    extern bool    lightcurve_yrange_fixed;
    extern bool    lightcurve_invert;
    extern float   lightcurve_y1;
    extern float   lightcurve_y2;
    extern float   lightcurve_extend_yrange;
    extern std::vector<Laps> lightcurve_targ;

    extern bool position_plot;
    extern float position_frac;
    extern std::vector<Paps> position_targ;
    extern bool  position_x_yrange_fixed;
    extern float position_x_y1;
    extern float position_x_y2;
    extern bool  position_y_yrange_fixed;
    extern float position_y_y1;
    extern float position_y_y2;
    extern float position_extend_yrange;

    extern bool transmission_plot;
    extern float transmission_frac;
    extern float transmission_ymax;
    extern std::vector<Taps> transmission_targ;

    extern bool seeing_plot;
    extern float seeing_frac;
    extern std::vector<Faps> seeing_targ;
    extern float seeing_scale;
    extern float seeing_ymax;
    extern float seeing_extend_yrange;

};

// Useful typedefs
typedef std::vector<Reduce::Laps>::iterator LCIT;
typedef std::vector<Reduce::Paps>::iterator LPIT;
typedef std::vector<Reduce::Taps>::iterator TRIT;
typedef std::vector<Reduce::Faps>::iterator FWIT;

// Short functions defined at the end.
bool code_ok_to_plot(Reduce::ERROR_CODES ecode);
int plot_symb(Reduce::ERROR_CODES ecode);
bool ok_to_plot_lc(LCIT lci, const std::vector<std::vector<Reduce::Point> >& all_ccds);
bool ok_to_plot_pos(LPIT lpi, const std::vector<std::vector<Reduce::Point> >& all_ccds);
bool ok_to_plot_trans(TRIT tri, const std::vector<std::vector<Reduce::Point> >& all_ccds);
bool ok_to_plot_fwhm(FWIT fwi, const std::vector<std::vector<Reduce::Point> >& all_ccds);

/** Program to plot the light curve within reduce
 * \param lcurve_plot the Plot for the light curve
 * \param all_ccds the data for all apertures of all CCDs from the latest frame
 * \param ut_date the time from the latest frame
 * \param makehcopy true to make a hard copy of the current plot
 * \param hcopy name of hard copy device.
 * \param title title string for the light curve plot
 */

void Ultracam::light_plot(const Subs::Plot& lcurve_plot, const std::vector<std::vector<Reduce::Point> >& all_ccds, const Subs::Time& ut_date,
              bool makehcopy, const std::string& hcopy, const std::string& title){

    // Character size multiplier
    const float CH     = 1.;

    // Variables that must be preserved between calls
    static bool first = true;
    static float time_since_start;
    static std::deque<std::pair<float,std::vector<std::vector<Reduce::Point> > > > lc_buffer;
    static std::vector<std::vector<Reduce::Point> > first_point;
    static Subs::Time first_time;
    static float xlcp1=0., xlcp2=0., ylc1=0., ylc2=0., yxp1=0., yxp2=0., yyp1=0., yyp2=0., yfw2 = Reduce::seeing_ymax;
    static Subs::Plot::Panel lc_panel, xp_panel, yp_panel, trans_panel, fwhm_panel;
    static float xvlp1, xvlp2, yvl1, yvl2, yvxp1, yvxp2, yvyp1, yvyp2, yvtr1, yvtr2, yvfw1, yvfw2;
    static float top_edge, bottom_edge;

    float x, xp, yp, trans, seeing;
    bool new_light_axes  = false;
    bool new_xpos_axes   = false;
    bool new_ypos_axes   = false;
    bool new_trans_axes  = false;
    bool new_fwhm_axes   = false;

    Subs::Plot hard;

    if(makehcopy){
    hard.open(hcopy);
    hard.focus();
    new_light_axes = true;
    }else if(lcurve_plot.is_open()){
    // Ensure we have the focus
    lcurve_plot.focus();
    }

//    Ultracam::def_col();

    // Compute time since the start of the run as the X display variable
    bool first_plot;
    if(first){

        // check all_ccds -- if no valid times, bail out.
        bool bail_out = true;
        for(size_t nccd=0; nccd<all_ccds.size(); nccd++){
            for(size_t naper=0; naper<all_ccds[nccd].size(); naper++){
                if(all_ccds[nccd][naper].time_ok){
                    bail_out = false;
                    break;
                }
            }
            if(!bail_out) break;
        }
        if(bail_out) return;




    x = time_since_start = 0.f;
    lc_buffer.push_back(std::make_pair(time_since_start, all_ccds));
    first_point = all_ccds;
    first_time  = ut_date;
    first_plot  = true;
    xvlp1  = 4.*CH/40.;
    xvlp2  = 1.-4.*CH/40.;
    float total = Reduce::lightcurve_frac;
    if(Reduce::position_plot)
        total += Reduce::position_frac;
    if(Reduce::transmission_plot)
        total += Reduce::transmission_frac;
    if(Reduce::seeing_plot)
        total += Reduce::seeing_frac;

    // Define vertical extents of plots, top to bottom
    float vertical_extent = 1-8.*CH/40.;

    // First for the light curves
    top_edge = yvl2 = 1.-4.*CH/40.;
    yvl1 = yvl2 - vertical_extent*Reduce::lightcurve_frac/total;

    bottom_edge = yvl1;

    // Second for the positions
    if(Reduce::position_plot){
        yvxp2  = bottom_edge;
        yvyp1  = yvxp2 - vertical_extent*Reduce::position_frac/total;
        yvxp1  = (yvyp1+yvxp2)/2.;
        yvyp2  = yvxp1;
        bottom_edge = yvyp1;
    }

    // Third for the transmission
    if(Reduce::transmission_plot){
        yvtr2  = bottom_edge;
        yvtr1  = yvtr2 - vertical_extent*Reduce::transmission_frac/total;
        bottom_edge = yvtr1;
    }

    // Fourth for the seeing
    if(Reduce::seeing_plot){
        yvfw2  = bottom_edge;
        yvfw1  = yvfw2 - vertical_extent*Reduce::seeing_frac/total;
        bottom_edge = yvfw1;
    }

    xlcp1 = x;
    if(Reduce::lightcurve_max_xrange > 0.){
        xlcp2 = x + std::max(Reduce::lightcurve_max_xrange, Reduce::lightcurve_extend_xrange);
    }else{
        xlcp2 = x + Reduce::lightcurve_extend_xrange;
    }
    if(Reduce::lightcurve_yrange_fixed){
        ylc1 = Reduce::lightcurve_y1;
        ylc2 = Reduce::lightcurve_y2;
    }
    new_light_axes  = true;
    new_xpos_axes   = true;
    new_ypos_axes   = true;
    new_trans_axes  = true;
    new_fwhm_axes   = true;

    }else{

    switch(Reduce::lightcurve_xunits){
        case Reduce::SECONDS:
        time_since_start = 86400.f*(ut_date.mjd()-first_time.mjd());
        break;
        case Reduce::MINUTES:
        time_since_start = 1440.f*(ut_date.mjd()-first_time.mjd());
        break;
        case Reduce::HOURS:
        time_since_start = 24.f*(ut_date.mjd()-first_time.mjd());
        break;
        case Reduce::DAYS:
        time_since_start = (ut_date.mjd()-first_time.mjd());
        break;
        default:
        throw Ultracam_Error("Incorrect lightcurve_xunits option. Should not be possible");
    }
    lc_buffer.push_back(std::make_pair(time_since_start, all_ccds));
    first_plot  = false;
    }

    x = time_since_start;

    // Adjust light curve X limits
    if(x > xlcp2){
    new_light_axes  = true;
    new_xpos_axes   = true;
    new_ypos_axes   = true;
    new_trans_axes  = true;
    new_fwhm_axes   = true;
    if(Reduce::lightcurve_max_xrange > 0.){
        xlcp1 = x - Reduce::lightcurve_max_xrange;
        xlcp2 = x + Reduce::lightcurve_extend_xrange;
    }else{
        while(x > xlcp2){
        xlcp2 += Reduce::lightcurve_extend_xrange;
        }
    }
    }

    // Adjust light curve Y limits
    float yt, yc, yte, yce, y, ye;
    if(!Reduce::lightcurve_yrange_fixed){
    bool newy  = false;
    float  yl=0., yh=0.;
    if(!first) yl = yh = (ylc1+ylc2)/2.;
    for(LCIT lci=Reduce::lightcurve_targ.begin(); lci != Reduce::lightcurve_targ.end(); lci++){

        if(ok_to_plot_lc(lci, lc_buffer.back().second)){

        yt   = lc_buffer.back().second[lci->nccd][lci->targ].flux;
        yte  = lc_buffer.back().second[lci->nccd][lci->targ].ferr;

        // If we don't want a comparison, use dummy values that allow the remainder of the
        // code to stay the same.
        if(lci->use_comp){
            yc   = lc_buffer.back().second[lci->nccd][lci->comp].flux;
            yce  = lc_buffer.back().second[lci->nccd][lci->comp].ferr;
        }else{
            yc  = 1.;
            yce = 0.;
        }

        y  = yt/yc; // y value of point
        ye = Subs::abs(y)*sqrt(Subs::sqr(yte/yt)+Subs::sqr(yce/yc)); // 1 sigma error

        if(!Reduce::lightcurve_linear){
            if(y > 0.){
            ye =  2.5/log(10.)*ye/y;
            y  = -2.5*log10(y);
            }else{
            ye = -1.; // flag to ignore
            }
        }

        y += lci->offset;

        if(first && lci == Reduce::lightcurve_targ.begin()){
            if(ye > 0.){
            yl    = y-1.1*ye;
            yh    = y+1.1*ye;
            ylc1  = yl;
            ylc2  = yh;
            }else{
            ylc1 = yl = -0.1;
            ylc2 = yh = +0.1;
            }
        }else if(ye > 0.){
            yl = yl > y-1.1*ye ? y-1.1*ye : yl;
            yh = yh < y+1.1*ye ? y+1.1*ye : yh;
        }
        }else if(first && lci == Reduce::lightcurve_targ.begin()){
        ylc1 = yl = -0.1;
        ylc2 = yh = +0.1;
        }
    }

    if(yl < ylc1){
        ylc1 = yl;
        newy = true;
    }
    if(yh > ylc2){
        ylc2 = yh;
        newy = true;
    }
    if(newy){
        float yrange = ylc2-ylc1;
        ylc1 = ylc1 - Reduce::lightcurve_extend_yrange*yrange/2.;
        ylc2 = ylc2 + Reduce::lightcurve_extend_yrange*yrange/2.;
        new_light_axes = true;
    }
    }

    // Position
    if(Reduce::position_plot){
    // Adjust X position Y limits
    float xp;
    if(!Reduce::position_x_yrange_fixed){
        float newy  = false;
        float yl=0., yh=0.;
        if(!first) yl = yh = (yxp1+yxp2)/2.;
        for(LPIT lpi=Reduce::position_targ.begin(); lpi != Reduce::position_targ.end(); lpi++){

        if(ok_to_plot_pos(lpi, lc_buffer.back().second)){

            xp   = lc_buffer.back().second[lpi->nccd][lpi->targ].xpos +
            lpi->off - first_point[lpi->nccd][lpi->targ].xpos;

            if(first && lpi == Reduce::position_targ.begin()){
            yl   = xp - 0.5;
            yh   = xp + 0.5;
            yxp1 = yl;
            yxp2 = yh;
            }else{
            yl = yl > xp-0.5 ? xp-0.5 : yl;
            yh = yh < xp+0.5 ? xp+0.5 : yh;
            }
        }
        }

        if(yl < yxp1){
        yxp1 = yl;
        newy = true;
        }
        if(yh > yxp2){
        yxp2 = yh;
        newy = true;
        }
        if(newy && !first_plot){
        float yrange = yxp2-yxp1;
        yxp1 = yxp1 - Reduce::position_extend_yrange*yrange/2.;
        yxp2 = yxp2 + Reduce::position_extend_yrange*yrange/2.;
        new_xpos_axes = true;
        }
    }

    // Adjust Y position Y limits
    float yp;
    if(!Reduce::position_y_yrange_fixed){
        float newy  = false;
        float yl=0., yh=0.;
        if(!first) yl = yh = (yyp1+yyp2)/2.;
        for(LPIT lpi=Reduce::position_targ.begin(); lpi != Reduce::position_targ.end(); lpi++){

        if(ok_to_plot_pos(lpi, lc_buffer.back().second)){

            yp   = lc_buffer.back().second[lpi->nccd][lpi->targ].ypos +
            lpi->off - first_point[lpi->nccd][lpi->targ].ypos;

            if(first && lpi == Reduce::position_targ.begin()){
            yl    = yp - 0.5;
            yh    = yp + 0.5;
            yyp1  = yl;
            yyp2  = yh;
            }else{
            yl = yl > yp-0.5 ? yp-0.5 : yl;
            yh = yh < yp+0.5 ? yp+0.5 : yh;
            }
        }
        }

        if(yl < yyp1){
        yyp1 = yl;
        newy = true;
        }
        if(yh > yyp2){
        yyp2 = yh;
        newy = true;
        }
        if(newy && !first_plot){
        float yrange = yyp2-yyp1;
        yyp1 = yyp1 - Reduce::position_extend_yrange*yrange/2.;
        yyp2 = yyp2 + Reduce::position_extend_yrange*yrange/2.;
        new_ypos_axes = true;
        }
    }
    }

    // Transmission
    if(Reduce::transmission_plot){

    for(TRIT tri=Reduce::transmission_targ.begin(); tri != Reduce::transmission_targ.end(); tri++){

        if(ok_to_plot_trans(tri, lc_buffer.back().second)){

        if((trans = lc_buffer.back().second[tri->nccd][tri->targ].flux / lc_buffer.back().second[tri->nccd][tri->targ].exposure) > 0.){

            if(first && tri == Reduce::transmission_targ.begin()){
            tri->fmax = trans;
            }else if(trans > Reduce::transmission_ymax*tri->fmax/100.){
            tri->fmax = trans;
            new_trans_axes = true;
            }
        }
        }
    }
    }

    // Seeing
    if(Reduce::seeing_plot){

    for(FWIT fwi=Reduce::seeing_targ.begin(); fwi != Reduce::seeing_targ.end(); fwi++){

        if(ok_to_plot_fwhm(fwi, lc_buffer.back().second)){

        seeing = Reduce::seeing_scale*lc_buffer.back().second[fwi->nccd][0].fwhm;

        if(seeing > yfw2){
            yfw2 *= Reduce::seeing_extend_yrange;
            new_fwhm_axes = true;
        }
        }
    }
    }

    // Trim data from start of buffer
    if(Reduce::lightcurve_max_xrange <= 0.){
    size_t ntrim = 0;
    while(lc_buffer[ntrim].first < xlcp1 && ntrim < lc_buffer.size()){
        ntrim++;
    }
    for(size_t nt=0; nt<ntrim; nt++) lc_buffer.pop_front();
    }

    // Finally we can register the fact that we have started
    first = false;

    // Return if no plot needed
    if(!lcurve_plot.is_open() && !makehcopy) return;

    typedef std::deque<std::pair<float, std::vector<std::vector<Reduce::Point> > > >::iterator LCBIT;

    // Re-do axes and re-plot if any have to be reset
    if(new_light_axes || new_xpos_axes || new_ypos_axes || new_trans_axes || new_fwhm_axes){

    // First the light-curve
    if(Reduce::lightcurve_yrange_fixed || !Reduce::lightcurve_invert){
        lc_panel.set(xlcp1, xlcp2, ylc1, ylc2, false, xvlp1, xvlp2, yvl1, yvl2);
    }else{
        lc_panel.set(xlcp1, xlcp2, ylc2, ylc1, false, xvlp1, xvlp2, yvl1, yvl2);
    }

    lc_panel.focus();
    cpgeras();

    // Now start the plotting
    cpgbbuf();

    // Light curves
    cpgsci(Subs::BLUE);
    if(Reduce::position_plot || Reduce::transmission_plot || Reduce::seeing_plot){
        cpgbox("BCST",0.,0,"BCNST",0.,0);
    }else{
        cpgbox("BCNST",0.,0,"BCNST",0.,0);
    }

    cpgsci(Subs::RED);
    if(Reduce::lightcurve_linear){
        cpglab(" ", "Target / Comparison", title.c_str());
    }else{
        cpglab(" ", "Mag (Targ) - Mag (Comp)", title.c_str());
    }

    for(LCBIT lcbi=lc_buffer.begin(); lcbi != lc_buffer.end(); lcbi++){
        for(LCIT lci=Reduce::lightcurve_targ.begin(); lci != Reduce::lightcurve_targ.end(); lci++){

        if(ok_to_plot_lc(lci, lcbi->second)){

            yt   = lcbi->second[lci->nccd][lci->targ].flux;
            yte  = lcbi->second[lci->nccd][lci->targ].ferr;

            // If we don't want a comparison, use dummy values that allow the remainder of the
            // code to stay the same.
            if(lci->use_comp){
            yc   = lcbi->second[lci->nccd][lci->comp].flux;
            yce  = lcbi->second[lci->nccd][lci->comp].ferr;
            }else{
            yc  = 1.;
            yce = 0.;
            }

            y  = yt/yc;
            ye = Subs::abs(y)*sqrt(Subs::sqr(yte/yt)+Subs::sqr(yce/yc));

            if(!Reduce::lightcurve_linear){
            if(y > 0.){
                ye = 2.5/log(10.)*ye/y;
                y  = -2.5*log10(y);
            }else{
                ye = -1.;
            }
            }

            if(ye > 0.){
            y += lci->offset;
            if(lci->errcol >= 0){
                cpgsci(lci->errcol);
                cpgerr1(2,lcbi->first,y,ye,0.);
                cpgerr1(4,lcbi->first,y,ye,0.);
            }
            if(lci->colour >= 0){
                cpgsci(lci->colour);
                cpgpt1(lcbi->first, y, plot_symb(std::max(lcbi->second[lci->nccd][lci->targ].code, lcbi->second[lci->nccd][lci->comp].code)));
            }
            }
        }
        }
    }

    // Position
    if(Reduce::position_plot){

        // X position data
        xp_panel.set(xlcp1, xlcp2, yxp1, yxp2, false, xvlp1, xvlp2, yvxp1, yvxp2);
        xp_panel.focus();

        cpgsci(Subs::BLUE);
        cpgbox("BCST",0.,0,"BCNST",0.,0);
        cpgsci(Subs::RED);
        cpglab(" ", "X", " ");

        for(LCBIT lcbi=lc_buffer.begin(); lcbi != lc_buffer.end(); lcbi++){
            for(LPIT lpi=Reduce::position_targ.begin(); lpi != Reduce::position_targ.end(); lpi++){

                if(ok_to_plot_pos(lpi, lcbi->second)){

                    xp   = lcbi->second[lpi->nccd][lpi->targ].xpos + lpi->off -
                        first_point[lpi->nccd][lpi->targ].xpos;

                    if(lpi->colour){
                        cpgsci(lpi->colour);
                        cpgpt1(lcbi->first,xp,plot_symb(lcbi->second[lpi->nccd][lpi->targ].code));
                    }
                }
            }
        }

        // Y position data
        yp_panel.set(xlcp1, xlcp2, yyp1, yyp2, false, xvlp1, xvlp2, yvyp1, yvyp2);
        yp_panel.focus();
        cpgsci(Subs::BLUE);
        if(Reduce::transmission_plot || Reduce::seeing_plot){
        cpgbox("BCST",0.,0,"BCNST",0.,0);
        }else{
        cpgbox("BCNST",0.,0,"BCNST",0.,0);
        }
        cpgsci(Subs::RED);
        cpglab(" ", "Y", " ");

        for(LCBIT lcbi=lc_buffer.begin(); lcbi != lc_buffer.end(); lcbi++){
            for(LPIT lpi=Reduce::position_targ.begin(); lpi != Reduce::position_targ.end(); lpi++){

                if(ok_to_plot_pos(lpi, lcbi->second)){
                    yp   = lcbi->second[lpi->nccd][lpi->targ].ypos + lpi->off -
                        first_point[lpi->nccd][lpi->targ].ypos;

                    if(lpi->colour){
                        cpgsci(lpi->colour);
                        cpgpt1(lcbi->first,yp,plot_symb(lcbi->second[lpi->nccd][lpi->targ].code));
                    }
                }
            }
        }
    }

    // Transmission
    if(Reduce::transmission_plot){

        trans_panel.set(xlcp1, xlcp2, 0., Reduce::transmission_ymax, false, xvlp1, xvlp2, yvtr1, yvtr2);
        trans_panel.focus();
        cpgsci(Subs::BLUE);
        if(Reduce::seeing_plot){
        cpgbox("BCST",0.,0,"BCNST",40.,4);
        }else{
        cpgbox("BCNST",0.,0,"BCNST",40.,4);
        }
        cpgsci(Subs::RED);
        cpglab(" ", "% trans", " ");

        for(LCBIT lcbi=lc_buffer.begin(); lcbi != lc_buffer.end(); lcbi++){
        for(TRIT tri=Reduce::transmission_targ.begin(); tri != Reduce::transmission_targ.end(); tri++){

            if(ok_to_plot_trans(tri, lcbi->second)){

            if((trans = lcbi->second[tri->nccd][tri->targ].flux / lcbi->second[tri->nccd][tri->targ].exposure) > 0.){

                trans /=  tri->fmax/100;

                if(tri->colour >= 0){
                cpgsci(tri->colour);
                cpgpt1(lcbi->first, trans, plot_symb(lcbi->second[tri->nccd][tri->targ].code));
                }
            }
            }
        }
        }
    }

    // Seeing
    if(Reduce::seeing_plot){

        fwhm_panel.set(xlcp1, xlcp2, 0., yfw2, false, xvlp1, xvlp2, yvfw1, yvfw2);
        fwhm_panel.focus();

        cpgsci(Subs::BLUE);
        cpgbox("BCNST",0.,0,"BCNST",1.,5);
        cpgsci(Subs::RED);
        cpglab(" ", "FWHM", " ");

        for(LCBIT lcbi=lc_buffer.begin(); lcbi != lc_buffer.end(); lcbi++){
        for(FWIT fwi=Reduce::seeing_targ.begin(); fwi != Reduce::seeing_targ.end(); fwi++){

            if(ok_to_plot_fwhm(fwi, lcbi->second)){

            seeing = Reduce::seeing_scale*lcbi->second[fwi->nccd][0].fwhm;

            if(fwi->colour >= 0){
                cpgsci(fwi->colour);
                cpgpt1(lcbi->first, seeing, plot_symb(lcbi->second[fwi->nccd][fwi->targ].code));
            }
            }
        }
        }
    }

    // Add X axis label
    cpgsvp(xvlp1, xvlp2, bottom_edge, top_edge);
    cpgsci(Subs::RED);
    switch(Reduce::lightcurve_xunits){
        case Reduce::SECONDS:
        cpglab("Time since start (seconds)", " ", " ");
        break;
        case Reduce::MINUTES:
        cpglab("Time since start (minutes)", " ", " ");
        break;
        case Reduce::HOURS:
        cpglab("Time since start (hours)", " ", " ");
        break;
        case Reduce::DAYS:
        cpglab("Time since start (days)", " ", " ");
        break;
    }

    // Display
    cpgebuf();

    }else{

    // No change of axes, plot most recent point only
    lc_panel.focus();
    for(LCIT lci=Reduce::lightcurve_targ.begin(); lci != Reduce::lightcurve_targ.end(); lci++){

        // Check that the CCD number, target and comparison apertures are within range. Since this
        // stage is always reached we do not need to do the same checks when replotting.

        if(lci->nccd >= lc_buffer.back().second.size())
        throw Ultracam_Error("Light curve CCD number out of range = " + Subs::str(lci->nccd+1));
        if(lci->targ >= lc_buffer.back().second[lci->nccd].size())
        throw Ultracam_Error("Target aperture for light curve out of range = " + Subs::str(lci->targ+1));
        if(lci->comp >= lc_buffer.back().second[lci->nccd].size())
        throw Ultracam_Error("Comparison aperture for light curve out of range = " + Subs::str(lci->comp+1));

        if(ok_to_plot_lc(lci, lc_buffer.back().second)){

        yt   = lc_buffer.back().second[lci->nccd][lci->targ].flux;
        yte  = lc_buffer.back().second[lci->nccd][lci->targ].ferr;

        // If we don't want a comparison, use dummy values that allow the remainder of the
        // code to stay the same.
        if(lci->use_comp){
            yc   = lc_buffer.back().second[lci->nccd][lci->comp].flux;
            yce  = lc_buffer.back().second[lci->nccd][lci->comp].ferr;
        }else{
            yc  = 1.;
            yce = 0.;
        }

        y  = yt/yc;
        ye = Subs::abs(y)*sqrt(Subs::sqr(yte/yt)+Subs::sqr(yce/yc));

        if(!Reduce::lightcurve_linear){
            if(y > 0.){
            ye =  2.5/log(10.)*ye/y;
            y  = -2.5*log10(y);
            }else{
            ye = -1.;
            }
        }

        if(ye > 0.){
            y += lci->offset;
            if(lci->errcol >= 0){
            cpgsci(lci->errcol);
            cpgerr1(2,lc_buffer.back().first,y,ye,0.);
            cpgerr1(4,lc_buffer.back().first,y,ye,0.);
            }
            if(lci->colour >= 0){
            cpgsci(lci->colour);
            cpgpt1(lc_buffer.back().first, y, plot_symb(std::max(lc_buffer.back().second[lci->nccd][lci->targ].code, lc_buffer.back().second[lci->nccd][lci->comp].code)));
            }
        }
        }
    }

    // Position
    if(Reduce::position_plot){

        xp_panel.focus();
        for(LPIT lpi=Reduce::position_targ.begin(); lpi != Reduce::position_targ.end(); lpi++){
        if(ok_to_plot_pos(lpi, lc_buffer.back().second)){
            xp   = lc_buffer.back().second[lpi->nccd][lpi->targ].xpos + lpi->off -
            first_point[lpi->nccd][lpi->targ].xpos;
            if(lpi->colour >= 0){
            cpgsci(lpi->colour);
            cpgpt1(lc_buffer.back().first,xp,plot_symb(lc_buffer.back().second[lpi->nccd][lpi->targ].code));
            }
        }
        }

        yp_panel.focus();
        for(LPIT lpi=Reduce::position_targ.begin(); lpi != Reduce::position_targ.end(); lpi++){
        if(ok_to_plot_pos(lpi, lc_buffer.back().second)){
            yp   = lc_buffer.back().second[lpi->nccd][lpi->targ].ypos + lpi->off -
            first_point[lpi->nccd][lpi->targ].ypos;
            if(lpi->colour >= 0){
            cpgsci(lpi->colour);
            cpgpt1(lc_buffer.back().first,yp,plot_symb(lc_buffer.back().second[lpi->nccd][lpi->targ].code));
            }
        }
        }

    }

    // Transmission
    if(Reduce::transmission_plot){

        trans_panel.focus();
        for(TRIT tri=Reduce::transmission_targ.begin(); tri != Reduce::transmission_targ.end(); tri++){

        if(ok_to_plot_trans(tri, lc_buffer.back().second)){

            trans = lc_buffer.back().second[tri->nccd][tri->targ].flux / lc_buffer.back().second[tri->nccd][tri->targ].exposure;

            if(trans > 0.){
            trans /=  tri->fmax/100.;

            if(tri->colour){
                cpgsci(tri->colour);
                cpgpt1(lc_buffer.back().first,trans,plot_symb(lc_buffer.back().second[tri->nccd][tri->targ].code));
            }
            }
        }
        }
    }

    // Seeing
    if(Reduce::seeing_plot){

        fwhm_panel.focus();
        for(FWIT fwi=Reduce::seeing_targ.begin(); fwi != Reduce::seeing_targ.end(); fwi++){

        if(ok_to_plot_fwhm(fwi, lc_buffer.back().second)){

            seeing = Reduce::seeing_scale*lc_buffer.back().second[fwi->nccd][0].fwhm;

            if(fwi->colour >= 0){
            cpgsci(fwi->colour);
            cpgpt1(lc_buffer.back().first,seeing,plot_symb(lc_buffer.back().second[fwi->nccd][fwi->targ].code));
            }
        }
        }
    }
    }
}

//! Determines whether a given error code is ok for plotting
bool code_ok_to_plot(Reduce::ERROR_CODES ecode){
    return ((
         ecode == Reduce::OK ||
         ecode == Reduce::COSMIC_RAY_DETECTED_IN_TARGET_APERTURE ||
         ecode == Reduce::SKY_OVERLAPS_EDGE_OF_WINDOW ||
         ecode == Reduce::SKY_OVERLAPS_AND_COSMIC_RAY_DETECTED ||
         ecode == Reduce::SKY_NEGATIVE ||
         ecode == Reduce::NO_SKY ||
         ecode == Reduce::EXTRA_APERTURES_IGNORED ||
         ecode == Reduce::PEPPERED ||
         ecode == Reduce::SATURATION) &&
        ecode != Reduce::BLUE_IS_JUNK);
}

//! Determines symbol code to plot.
/** Normal points are plotted as dots, peppered are plotted
 * as 6 pointed stars, saturated are plotted as 5 pointed
 * stars; other bad points are plotted as 'x'
 */
int plot_symb(Reduce::ERROR_CODES ecode){
    switch(ecode){
    case Reduce::OK:
        return 1;
    case Reduce::PEPPERED:
        return 3;
    case Reduce::SATURATION:
        return 12;
    default:
        return 5;
    }
}

//! Encapsulates whether a light curve point can be plotted
/** \param lci iterator of vector of light curve plotting vector
 * \param all_ccds structure of extracted flux info for all apertures of all CCDs
 */
bool ok_to_plot_lc(LCIT lci, const std::vector<std::vector<Reduce::Point> >& all_ccds){
    return (lci->nccd < all_ccds.size() && lci->targ < all_ccds[lci->nccd].size() &&
        (!lci->use_comp || lci->use_comp < all_ccds[lci->nccd].size()) &&
        code_ok_to_plot(all_ccds[lci->nccd][lci->targ].code) &&
        (!lci->use_comp || code_ok_to_plot(all_ccds[lci->nccd][lci->comp].code)) &&
        all_ccds[lci->nccd][lci->targ].time_ok);
}

//! Encapsulates whether a position point can be plotted
/** \param lpi iterator of vector of position plotting vector
 * \param all_ccds structure of extracted flux info for all apertures of all CCDs
 */
bool ok_to_plot_pos(LPIT lpi, const std::vector<std::vector<Reduce::Point> >& all_ccds){
    return (lpi->nccd < all_ccds.size() && lpi->targ < all_ccds[lpi->nccd].size() &&
        code_ok_to_plot(all_ccds[lpi->nccd][lpi->targ].code) &&
        all_ccds[lpi->nccd][lpi->targ].time_ok);
}

//! Encapsulates whether a transmission point can be plotted
/** \param tri iterator of vector of transmission plotting vector
 * \param all_ccds structure of extracted flux info for all apertures of all CCDs
 */
bool ok_to_plot_trans(TRIT tri, const std::vector<std::vector<Reduce::Point> >& all_ccds){
    return (tri->nccd < all_ccds.size() && tri->targ < all_ccds[tri->nccd].size() &&
        code_ok_to_plot(all_ccds[tri->nccd][tri->targ].code) &&
        all_ccds[tri->nccd][tri->targ].time_ok);
}

//! Encapsulates whether a seeing point can be plotted
/** \param fwi iterator of vector of transmission plotting vector
 * \param all_ccds structure of extracted flux info for all apertures of all CCDs
 */
bool ok_to_plot_fwhm(FWIT fwi, const std::vector<std::vector<Reduce::Point> >& all_ccds){
    return (fwi->nccd < all_ccds.size() && all_ccds[fwi->nccd].size() > 0 &&
        all_ccds[fwi->nccd][0].fwhm > 0 && code_ok_to_plot(all_ccds[fwi->nccd][fwi->targ].code) &&
        all_ccds[fwi->nccd][fwi->targ].time_ok);
}
