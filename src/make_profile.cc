#include "trm/array1d.h"
#include "trm/ultracam.h"
#include "trm/windata.h"

/** This collapses a Windata and a corresponding variance to make a profile in the Y-direction,
 * normalising the results by the number of pixels used, which are also returned. This is used
 * in spectrum reduction. It is possible to apply a median filter to help removal of cosmic rays
 * prior to the collapse.
 * \param data the data
 * \param dvar equivalent variances
 * \param x1 left-hand X-limit
 * \param x2 right-hand X-limit
 * \param y1 lower Y-limit
 * \param y2 upper Y-limit
 * \param hwidth half width of median filter in pixels. Width used will be 2*hwidth+1. hwidth = 0 means no median filter
 * \param prof the returned profile
 * \param pvar the returned variances
 * \param npix the returned numbers of pixels used
 * \return true if at least one pixel was added into the profile
 */
bool Ultracam::make_profile(const Windata& data, const Windata& dvar, float x1, float x2, float y1, float y2,
                int hwidth, Subs::Array1D<float>& prof, Subs::Array1D<float>& pvar, Subs::Array1D<int>& npix){

    if(data != dvar)
    throw Ultracam_Error("make_profile: data and dvar do not match!");

    if(hwidth < 0)
    throw Ultracam_Error("make_profile: hwidth < 0");

    static Subs::Array1D<float> dbuff, vbuff, fdbuff, fvbuff;
    bool ok = false;

    if(data.left() < x2 && data.right() > x1 && data.bottom() < y2 && data.top() > y1){

    prof.resize(data.ny());
    pvar.resize(data.ny());
    npix.resize(data.ny());

    prof = 0.;
    pvar = 0.;
    npix = 0;

    if(hwidth == 0){

        for(int iy=0; iy<data.ny(); iy++){
        double yccd = data.yccd(iy);
        if(yccd >= y1 && yccd <= y2){
            for(int ix=0; ix<data.nx(); ix++){
            double xccd = data.xccd(ix);
            if(xccd >= x1 && xccd <= x2){
                prof[iy] += data[iy][ix];
                pvar[iy] += dvar[iy][ix];
                npix[iy]++;
            }
            }
        }
        }

    }else{

        for(int iy=0; iy<data.ny(); iy++){
        double yccd = data.yccd(iy);
        if(yccd >= y1 && yccd <= y2){

            dbuff.clear();
            vbuff.clear();

            for(int ix=0; ix<data.nx(); ix++){
            double xccd = data.xccd(ix);
            if(xccd >= x1 && xccd <= x2){
                dbuff.push_back(data[iy][ix]);
                vbuff.push_back(dvar[iy][ix]);
            }
            }

            // Median filter
            if(dbuff.size()){

            Subs::medfilt(dbuff, fdbuff, 2*hwidth+1);
            Subs::medfilt(vbuff, fvbuff, 2*hwidth+1);

            for(int ix=0, ip=0; ix<data.nx(); ix++){
                double xccd = data.xccd(ix);
                if(xccd >= x1 && xccd <= x2){
                prof[iy] += fdbuff[ip];
                pvar[iy] += fvbuff[ip];
                npix[iy]++;
                ip++;
                }
            }
            }
        }
        }
    }

    // Normalise
    for(int iy=0; iy<data.ny(); iy++){
        if(npix[iy]){
        ok = true;
        prof[iy] /= npix[iy];
        pvar[iy] /= Subs::sqr(npix[iy]);
        }
    }
    }

    return ok;
}

