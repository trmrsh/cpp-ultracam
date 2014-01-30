/*

!!begin

!!title   Collapses Ultracam frames
!!author  T.R. Marsh
!!created 07 November 2006
!!revised 02 December 2006
!!descr   Collapses Ultracam frames
!!css     style.css
!!root    collapse
!!index   collapse
!!class   Programs
!!class   Arithematic
!!class   Spectra
!!class   Manipulation
!!head1   collapse - collapses Ultracam frames

!!emph{collapse} collapses an Ultracam frame in either the X or Y direction
by summing or averaging over a specified range of X or Y. The window structure
is retained, such that a window of nx by ny will become 1 by ny if collapsed in the
X direction, or 0 by ny if collapsed in the X direction over an X range that does not
overlap. This allows later use of the command !!ref{expand.html}{expand}. A simple operation such
as a collapse becomes complex for multiple windows. One may in some cases want to treat each window
separately but in others combine results. !!emph{collapse} has various options to make this
possible. The lower-left corner coordinates of the windows are left untouched by this routine.

!!head2 Invocation

collapse input dirn method bridge (x1 x2)/(y1 y2) medfilt output!!break

!!head2 Arguments

!!table
!!arg{input}{Input frame}
!!arg{dirn}{Direction to collapse in, 'X' or 'Y'. dirn='X' means summing columns for instance.}
!!arg{method}{'S' for sum, 'A' for average.}
!!arg{bridge}{true if the profile is to cross adjacent windows, false if the collapse is only
on a per window basis. The meaning of this is as follows. Say there are two windows of 100x100, one of which has a lower
Y limit of 11 and the other 31, and you collapse in the X direction. If you don't bridge then each window ends up with an
X dimension of 1, and a profile equal to the sum or average of its 2D equivalent. If you do bridge, the profiles will be summed or averaged
in their region of overlap and this part will end up being the same for each window. In this case a window must have all its pixels covered
by the combined profile to end up with a non-zero dimension in the collapse direction.}
!!arg{x1}{If dirn='X', this is the first X value to include. Unbinned pixels. In the case of binning, a pixel must be
wholly included in the range to contribute.}
!!arg{x2}{If dirn='X', this is the last X value to include. Unbinned pixels. In the case of binning, a pixel must be
wholly included in the range to contribute.}
!!arg{y1}{If dirn='Y', this is the first Y value to include. Unbinned pixels. In the case of binning, a pixel must be
wholly included in the range to contribute.}
!!arg{y2}{If dirn='Y', this is the last Y value to include. Unbinned pixels. In the case of binning, a pixel must be
wholly included in the range to contribute.}
!!arg{medfilt}{half-width of median filter, 0 for none at all}
!!arg{output}{Output, a frame in which one of the dimensions of the windows is collapsed to 1.}
!!table

See also !!ref{expand.html}{expand}

!!end

*/

#include <cstdlib>
#include <string>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/array1d.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/ultracam.h"

int main(int argc, char* argv[]){

    using Ultracam::Ultracam_Error;

    try{

    // Construct Input object
    Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

    // Sign-in input variables
    input.sign_in("input",  Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("dirn",   Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("method", Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("bridge", Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("x1",     Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("x2",     Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("y1",     Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("y2",     Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("medfilt",Subs::Input::LOCAL, Subs::Input::PROMPT);
    input.sign_in("output", Subs::Input::LOCAL, Subs::Input::PROMPT);

    // Get inputs
    std::string sinput;
    input.get_value("input", sinput, "input", "file to collapse");
    Ultracam::Frame indata(sinput);

    char dirn;
    input.get_value("dirn", dirn, 'x', "xXyY", "direction to collapse in X or Y");
    dirn = std::toupper(dirn);

    char method;
    input.get_value("method", method, 'a', "aAsS", "method, S(um) or A(verage)");
    method = std::toupper(method);

    bool bridge;
    input.get_value("bridge", bridge, true, "average/sum the profile across windows?");

    int x1, x2, y1, y2;
    if(dirn == 'X'){
        input.get_value("x1", x1, 1, 0, indata[0].nxtot(), "first X value to include in collapse");
        input.get_value("x2", x2, indata[0].nxtot(), x1, indata[0].nxtot(), "last X value to include in collapse");
    }else{
        input.get_value("y1", y1, 1, 0, indata[0].nytot(), "first Y value to include in collapse");
        input.get_value("y2", y2, indata[0].nytot(), y1, indata[0].nytot(), "last Y value to include in collapse");
    }

    int medfilt;
    input.get_value("medfilt", medfilt, 0, 0, 1000, "half width of median filter in binned pixels (0 for no filter)");

    std::string output;
    input.get_value("output", output, "output", "file to dump result to");

    if(bridge){

        // First we need to check that this is even possible, which means that
        // all windows on a given CCD must be in step
        for(size_t nccd=0; nccd<indata.size(); nccd++){
        if(indata[nccd].size()){
            const Ultracam::Windata& refwin = indata[nccd][0];
            for(size_t nwin=1; nwin<indata[nccd].size(); nwin++){
            const Ultracam::Windata& win = indata[nccd][nwin];
            if(dirn == 'X'){
                if(win.ybin() != refwin.ybin() || (win.lly() - refwin.lly()) % win.ybin() != 0)
                throw Ultracam::Ultracam_Error("Windows have different Y binning factors or are not in step in the Y direction");
            }else{
                if(win.xbin() != refwin.xbin() || (win.llx() - refwin.llx()) % win.xbin() != 0)
                throw Ultracam::Ultracam_Error("Windows have different X binning factors or are not in step in the X direction");
            }
            }
        }
        }

        // We are OK; now we will create a single grand profile for each CCD
        for(size_t nccd=0; nccd<indata.size(); nccd++){

        if(indata[nccd].size()){

            // Derive range of grand profile.
            int lwin = 0, uwin = 0, bin = 0;
            for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
            const Ultracam::Windata& win = indata[nccd][nwin];
            if(nwin == 0){
                if(dirn == 'X'){
                lwin = win.lly();
                uwin = win.lly()  + win.ybin()*win.ny();
                bin  = win.ybin();
                }else{
                lwin = win.llx();
                uwin = win.llx()  + win.xbin()*win.nx();
                bin  = win.xbin();
                }
            }else{
                if(dirn == 'X'){
                lwin = win.lly() < lwin ? win.lly() : lwin;
                uwin = win.lly()  + win.ybin()*win.ny() > uwin ? win.lly()  + win.ybin()*win.ny() : uwin;
                }else{
                lwin = win.llx() < lwin ? win.llx() : lwin;
                uwin = win.llx()  + win.xbin()*win.nx() > uwin ? win.llx()  + win.xbin()*win.nx() : uwin;
                }
            }
            }

            // Median filter prior to anything else
            if(medfilt){
            Subs::Array1D<float> buff, filtbuff;

            if(dirn == 'X'){
                // Loop over every y pixel with potential data in it, extracting all valid
                // pixels any row into the buffer 'buff'
                for(int iy=lwin; iy<uwin; iy+=bin){
                for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
                    const Ultracam::Windata& win = indata[nccd][nwin];
                    int ny = (iy - win.lly())/bin;
                    if(ny >= 0 && ny < win.ny()){
                    int nx1 = (x1 - win.llx() + win.xbin() - 1)/win.xbin();
                    nx1 = nx1 > 0 ? nx1 : 0;
                    int nx2 = (x2 - win.llx() - win.xbin() + 1)/win.xbin() + 1;
                    nx2 = nx2 <= win.nx() ? nx2 : win.nx();
                    for(int nx=nx1; nx<nx2; nx++)
                        buff.push_back(win[ny][nx]);
                    }
                }

                // Apply the median filter and transfer back into frame
                if(buff.size()){

                    Subs::medfilt(buff, filtbuff, 2*medfilt+1);

                    int ibp = 0;
                    for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
                    Ultracam::Windata& win = indata[nccd][nwin];
                    int ny = (iy - win.lly())/bin;
                    if(ny >= 0 && ny < win.ny()){
                        int nx1 = (x1 - win.llx() + win.xbin() - 1)/win.xbin();
                        nx1 = nx1 > 0 ? nx1 : 0;
                        int nx2 = (x2 - win.llx() - win.xbin() + 1)/win.xbin() + 1;
                        nx2 = nx2 <= win.nx() ? nx2 : win.nx();
                        for(int nx=nx1; nx<nx2; nx++)
                        win[ny][nx] = filtbuff[ibp++];
                    }
                    }
                    buff.clear();
                }
                }

            }else{

                // Loop over every x pixel with potential data in it, extracting all valid
                // pixels in any column into the buffer 'buff'
                for(int ix=lwin; ix<uwin; ix+=bin){

                for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
                    const Ultracam::Windata& win = indata[nccd][nwin];
                    int nx = (ix - win.llx())/bin;
                    if(nx >= 0 && nx < win.nx()){
                    int ny1 = (y1 - win.lly() + win.ybin() - 1)/win.ybin();
                    ny1 = ny1 > 0 ? ny1 : 0;
                    int ny2 = (y2 - win.lly() - win.ybin() + 1)/win.ybin() + 1;
                    ny2 = ny2 <= win.ny() ? ny2 : win.ny();
                    for(int ny=ny1; ny<ny2; ny++)
                        buff.push_back(win[ny][nx]);
                    }
                }

                // Apply the median filter and transfer back into frame
                if(buff.size()){

                    Subs::medfilt(buff, filtbuff, 2*medfilt+1);

                    int ibp = 0;
                    for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
                    Ultracam::Windata& win = indata[nccd][nwin];
                    int nx = (ix - win.llx())/bin;
                    if(nx >= 0 && nx < win.nx()){
                        int ny1 = (y1 - win.lly() + win.ybin() - 1)/win.ybin();
                        ny1 = ny1 > 0 ? ny1 : 0;
                        int ny2 = (y2 - win.lly() - win.ybin() + 1)/win.ybin() + 1;
                        ny2 = ny2 <= win.ny() ? ny2 : win.ny();
                        for(int ny=ny1; ny<ny2; ny++)
                        win[ny][nx] = filtbuff[ibp++];
                    }
                    }
                    buff.clear();
                }
                }
            }
            }

            // Grab sufficient space
            int npx = (uwin-lwin)/bin;
            Subs::Array1D<int>   npix(npx);
            Subs::Array1D<float> sum(npx);
            npix = 0;
            sum  = 0;

            for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
            const Ultracam::Windata& win = indata[nccd][nwin];
            if(dirn == 'X'){
                int nx1 = (x1 - win.llx() + win.xbin() - 1)/win.xbin();
                nx1 = nx1 > 0 ? nx1 : 0;
                int nx2 = (x2 - win.llx() - win.xbin() + 1)/win.xbin() + 1;
                nx2 = nx2 <= win.nx() ? nx2 : win.nx();

                // Register this window correctly in the grand sum
                // Potential for page faults here
                int off = (win.lly()-lwin)/win.ybin();
                for(int ny=0; ny<win.ny(); ny++){
                for(int nx=nx1; nx<nx2; nx++){
                    npix[ny+off]++;
                    sum[ny+off] += win[ny][nx];
                }
                }

            }else{

                int ny1 = (y1 - win.lly() + win.ybin() - 1)/win.ybin();
                ny1 = ny1 > 0 ? ny1 : 0;
                int ny2 = (y2 - win.lly() - win.ybin() + 1)/win.ybin() + 1;
                ny2 = ny2 <= win.ny() ? ny2 : win.ny();
                int off = (win.llx()-lwin)/win.xbin();
                for(int ny=ny1; ny<ny2; ny++){
                for(int nx=0; nx<win.nx(); nx++){
                    npix[nx+off]++;
                    sum[nx+off] += win[ny][nx];
                }
                }
            }
            }

            // Normalise in the averaging case
            if(method == 'A'){
            for(int i=0; i<npx; i++)
                if(npix[i]) sum[i] /= npix[i];
            }

            // Now we go through window by window to see if they are covered
            for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
            Ultracam::Windata& win = indata[nccd][nwin];
            if(dirn == 'X'){

                int off = (win.lly()-lwin)/win.ybin();
                bool covered = true;
                for(int ny=0; ny<win.ny(); ny++)
                if(!(covered = npix[ny+off] > 0)) break;

                if(covered){
                // Collapse window to width 1 in X and transfer data
                win.resize(win.ny(),1);
                for(int ny=0; ny<win.ny(); ny++)
                    win[ny][0] = sum[ny+off];
                }else{
                // Collapse window to width 0 in X
                win.resize(win.ny(),0);
                }

            }else{

                int off = (win.llx()-lwin)/win.xbin();
                bool covered = true;
                for(int nx=0; nx<win.nx(); nx++)
                if(!(covered = npix[nx+off] > 0)) break;

                if(covered){
                // Collapse window to width 1 in Y and transfer data
                win.resize(1, win.nx());
                for(int nx=0; nx<win.nx(); nx++)
                    win[0][nx] = sum[nx+off];
                }else{
                // Collapse window to width 0 in Y
                win.resize(0, win.nx());
                }
            }
            }
        }
        }

    }else{

        // Collapse each window, one by one.
        Subs::Array1D<float> sum, buff, filtbuff;

        for(size_t nccd=0; nccd<indata.size(); nccd++){
        for(size_t nwin=0; nwin<indata[nccd].size(); nwin++){
            Ultracam::Windata& win = indata[nccd][nwin];
            if(dirn == 'X'){
            sum.resize(win.ny());
            sum  = 0.;
            int nx1 = (x1 - win.llx() + win.xbin() - 1)/win.xbin();
            nx1 = nx1 > 0 ? nx1 : 0;
            int nx2 = (x2 - win.llx() - win.xbin() + 1)/win.xbin() + 1;
            nx2 = nx2 <= win.nx() ? nx2 : win.nx();
            int npix = nx2-nx1;

            if(npix){

                buff.resize(npix);
                for(int ny=0; ny<win.ny(); ny++){

                // Extract row
                for(int nx=nx1; nx<nx2; nx++)
                    buff[nx-nx1] = win[ny][nx];

                // Median filter
                if(medfilt){
                    Subs::medfilt(buff, filtbuff, 2*medfilt+1);
                    buff = filtbuff;
                }

                // Add into sum
                for(int nx=nx1; nx<nx2; nx++)
                    sum[ny] += buff[nx-nx1];
                }

                if(method == 'A'){
                for(int ny=0; ny<win.ny(); ny++)
                    sum[ny] /= npix;
                }

                // Collapse window to width 1 in X and transfer data
                win.resize(win.ny(),1);
                for(int ny=0; ny<win.ny(); ny++)
                win[ny][0] = sum[ny];
            }else{
                // Collapse window to width 0 in X
                win.resize(win.ny(),0);
            }

            }else{

            sum.resize(win.nx());
            sum  = 0.;
            int ny1 = (y1 - win.lly() + win.ybin() - 1)/win.ybin();
            ny1 = ny1 > 0 ? ny1 : 0;
            int ny2 = (y2 - win.lly() - win.ybin() + 1)/win.ybin() + 1;
            ny2 = ny2 <= win.ny() ? ny2 : win.ny();
            int npix = ny2-ny1;
            if(npix){

                buff.resize(npix);
                for(int nx=0; nx<win.nx(); nx++){

                // Extract column
                for(int ny=ny1; ny<ny2; ny++)
                    buff[ny-ny1] = win[ny][nx];

                // Median filter
                if(medfilt){
                    Subs::medfilt(buff, filtbuff, 2*medfilt+1);
                    buff = filtbuff;
                }

                // Add into sum
                for(int ny=ny1; ny<ny2; ny++)
                    sum[nx] += buff[ny-ny1];
                }

                if(method == 'A'){
                for(int nx=0; nx<win.nx(); nx++)
                    sum[nx] /= npix;
                }

                // Collapse window to width 1 in X and transfer data
                win.resize(1, win.nx());
                for(int nx=0; nx<win.nx(); nx++)
                win[0][nx] = sum[nx];
            }else{
                // Collapse window to width 0 in X
                win.resize(0,win.nx());
            }
            }
        }
        }
    }

    // Write out the result
    indata.write(output);


    }

    catch(const Ultracam::Input_Error& err){
    std::cerr << "Ultracam::Input_Error exception:" << std::endl;
    std::cerr << err << std::endl;
    }
    catch(const Ultracam::Ultracam_Error& err){
    std::cerr << "Ultracam::Ultracam_Error exception:" << std::endl;
    std::cerr << err << std::endl;
    }
    catch(const Subs::Subs_Error& err){
    std::cerr << "Subs::Subs_Error exception:" << std::endl;
    std::cerr << err << std::endl;
    }
    catch(const std::string& err){
    std::cerr << err << std::endl;
    }
}



