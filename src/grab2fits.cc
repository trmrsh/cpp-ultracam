/*

!!begin
!!title    grab2fits
!!author   T.R. Marsh
!!created  06 May 2002
!!revised  25 Jan 2008
!!root     grab2fits
!!index    grab2fits
!!descr    grabs server files, spits out FITS
!!class    Programs
!!class    Observing
!!class    IO
!!class    FITS
!!css      style.css
!!head1    grabs files from server and turns them into FITS

!!emph{grab2fits} grabs an ultracam run from the server or local disk and splits it up
into FITS files. The files will be dumped in whichever disk this is run. One server file
can generate many FITS files. These will have names consisting of the
server file name + "_001" etc. In addition, an extra "_1", "_2" etc can be added
if the files are split by CCD number as well.  The server must be running for this program
to work of course, or you can access the local disk directly instead. You may experience
problems defining the directory path. It is helpful to look at the messages the server
produces to see where it is trying to find the files in this case.  Note that !!emph{grab2fits} will skip
over junk data in the case of drift mode, so your first file might be number
5 say even though you asked for number 1.

The FITS files produced by this routine can be displayed with 'ds9' and the command-line option
'-mosaicimage wcs' allows all windows to be displayed at once.

!!head2 Invocation

grab2fits [source] (url)/(file) ndigit first last trim [(ncol nrow) twait tmax] bias (biasframe flat (flatframe) threshold
(photon) naccum (split) overwrite

!!head2 Arguments

!!table

!!arg{source}{Data source, either 'l' for local or 's' for server. 'Local' means the usual .xml and .dat
files accessed directly. Do not add either .xml or .dat to the file name; these are assumed.}

!!arg{url/file}{If source='S', this should be the complete URL of the file, e.g. 'http://127.0.0.1:8007/run00000012',
or just the file part in which case the program will try to find a default part to
add from the environment variable ULTRACAM_DEFAULT_URL. Failing this it will add
http://127.0.0.1:8007/, i.e. the local host. If source='L', this should just be a plain file name.}

!!arg{ndigit}{The minimum number of digits to use on output to tack on the frame number. If ndigit=0, then
just enough will be used for each frame, however specifying a number > 0 has the advantage that it is
then much easier to get a listing of the files in temporal order since this is also alphabetical.
e.g. you will end up with files of the form run0012_001, run0012_002, etc if you set ndigit=3.}

!!arg{first}{The first file, starting from 1.}

!!arg{last}{The last file. last=0 will just try to grab as many as possible, otherwise it
should be >= first}

!!arg{trim}{Set trim=true to enable trimming of potential junk rows and columns of each window}

!!arg{ncol}{If trim, then this specifies the number of columns nearest the readouts of each window to be snipped
off as these can be corrupted.}

!!arg{nrow}{If trim, then this specifies the number of rows to snip off the bottom of each window as these
can be corrupted.}

!!arg{twait}{Time to wait between attempts to find a new exposure (seconds).}

!!arg{tmax}{Maximum time to wait before giving up (seconds). Set = 0 to quit as soon as a frame is
not found.}

!!arg{bias}{true/false according to whether you want to subtract a bias frame. You can specify a full-frame
bias because it will be cropped to match whatever your format is. This is useful for ultracam because of
the different bias levels of the 6 readouts.}

!!arg{biasframe}{If bias, then you need to specify the name of the bias frame.}

!!arg{flat}{true/false according to whether you want to apply a flat field. You can specify a full-frame
flat because it will be cropped to match whatever your format is..}

!!arg{flatframe}{If flat, then you need to specify the name of the flatfield frame.}

!!arg{threshold}{If you are applying a bias to ULTRASPEC L3CCD data, you have the option of
converting to photon counts (0 or 1 whether above or below a certain threshold).}

!!arg{photon}{The threshold level if threshold = true}

!!arg{naccum}{Accumulate data into the sum of naccum frames before writing to disk. If some are still
accumulating at the end, they will be written out even if they have not reach 'naccum'.}

!!arg{split}{true/false according to whether you want to split up the resulting files into one per CCD. Otherwise
all CCDs and windows will be dumped to a single FITS file per exposure}

!!arg{overwrite}{true/false according to whether you want to allow existing files to be overwritten}

!!table

Related routines: !!ref{ucm2fits.html}{ucm2fits}, !!ref{fits2ucm.html}{fits2ucm}

!!end

*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <cfloat>
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <string>
#include <fstream>
#include "trm/subs.h"
#include "trm/time.h"
#ifdef HAVE_CFITSIO_FITSIO_H
# include "cfitsio/fitsio.h"
#else
# include "fitsio.h"
#endif
#include "trm/input.h"
#include "trm/header.h"
#include "trm/frame.h"
#include "trm/mccd.h"
#include "trm/window.h"
#include "trm/ultracam.h"

// Main program

int main(int argc, char* argv[]){

    using Ultracam::File_Open_Error;
    using Ultracam::Ultracam_Error;
    using Ultracam::Input_Error;

    try{

        // Construct Input object
        Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

        // Sign-in input variables
        input.sign_in("source",    Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("url",       Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("file",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("ndigit",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("first",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("last",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("trim",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("ncol",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("nrow",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("twait",     Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("tmax",      Subs::Input::GLOBAL, Subs::Input::NOPROMPT);
        input.sign_in("split",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("overwrite", Subs::Input::LOCAL,  Subs::Input::PROMPT);
        input.sign_in("bias",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("biasframe", Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("flat",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("flatframe", Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("threshold", Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("photon",    Subs::Input::GLOBAL, Subs::Input::PROMPT);
        input.sign_in("naccum",    Subs::Input::GLOBAL, Subs::Input::PROMPT);

        // Get inputs

        // Get inputs
        char source;
        input.get_value("source", source, 'S', "sSlL", "data source: L(ocal) or S(erver)?");
        source = toupper(source);
        std::string url;
        if(source == 'S'){
            input.get_value("url", url, "url", "url of file");
        }else{
            input.get_value("file", url, "file", "name of local file");
        }
        int ndigit;
        input.get_value("ndigit", ndigit, 0, 0, 20, "number of digits in file numbers");
        size_t first;
        input.get_value("first", first, size_t(1), size_t(1), size_t(INT_MAX), "first file to access (starting from 1)");
        size_t last;
        input.get_value("last", last, size_t(0), size_t(0), size_t(INT_MAX), "last file to access (0 for all)");
        if(last != 0 && last < first)
            throw Ultracam_Error("Last file must either be 0 or >= first");
        bool trim;
        input.get_value("trim", trim, true, "trim junk lower rows from windows?");
        int ncol, nrow;
        if(trim){
            input.get_value("ncol", ncol, 0, 0, 100, "number of columns to trim from each window");
            input.get_value("nrow", nrow, 0, 0, 100, "number of rows to trim from each window");
        }
        double twait;
        input.get_value("twait", twait, 1., 0., 1000., "time to wait between attempts to find a frame (seconds)");
        double tmax;
        input.get_value("tmax", tmax, 2., 0., 100000., "maximum time to wait before giving up trying to find a frame (seconds)");

        std::cout << "Attempting to access " << url << "\n" << std::endl;

        // Add extra stuff to URL if need be.
        if(url.find("http://") == std::string::npos && source == 'S'){
            char *DEFAULT_URL = getenv(Ultracam::ULTRACAM_DEFAULT_URL);
            if(DEFAULT_URL != NULL){
                url = DEFAULT_URL + url;
            }else{
                url = Ultracam::ULTRACAM_LOCAL_URL + url;
            }
        }else if(url.find("http://") == 0 && source == 'L'){
            throw Ultracam::Input_Error("Should not specify the local file as a URL");
        }

        // Parse the XML file
        Ultracam::Mwindow mwindow;
        Subs::Header header;
        Ultracam::ServerData serverdata;
        parseXML(source, url, mwindow, header, serverdata, trim, ncol, nrow, twait, tmax);

        Ultracam::Frame data(mwindow, header);

        Subs::Header::Hnode* hnode = data.find("Instrument.instrument");
        bool ultraspec = (hnode->has_data() && hnode->value->get_string() == "ULTRASPEC");

        bool bias, thresh = false, flat = false;
        float photon;
        input.get_value("bias", bias, true, "do you want to subtract a bias frame?");
        Ultracam::Frame bias_frame, flat_frame;
        if(bias){
            std::string sbias;
            input.get_value("biasframe", sbias, "bias", "name of bias frame");
            bias_frame.read(sbias);
            bias_frame.crop(mwindow);

            // We need to record this in the frame for potential dark subtraction
            float bias_expose = bias_frame["Exposure"]->get_float();
            data.set("Bias_exposure", new Subs::Hfloat(bias_expose, "Exposure time of bias subtracted from this frame"));


            input.get_value("flat", flat, true, "do you want to apply a flat field?");
            if(flat){
                std::string sflat;
                input.get_value("flatframe", sflat, "flat", "name of flat frame");
                flat_frame.read(sflat);
                flat_frame.crop(mwindow);
            }

            if(ultraspec){
                input.get_value("threshold", thresh, true, "do you want to threshold to get 0 or 1 photons/pix?");
                if(thresh)
                    input.get_value("photon", photon, 50.f, FLT_MIN, FLT_MAX, "threshold level to count as 1 photon");
            }
        }

        int naccum = 1;
        if(ultraspec)
            input.get_value("naccum", naccum, 1, 1, 10000, "number of frames to accumulate before writing");

        bool split = false;
        if(data.size() > 1)
            input.get_value("split", split, false, "split the files to give one FITS file per CCD?");
        bool overwrite;
        input.get_value("overwrite", overwrite, false, "overwrite pre-existing files?");
        input.save();

        std::string::size_type n = url.find_last_of('/');
        std::string server_file, out_file;
        if(n != std::string::npos){
            server_file = url.substr(n+1);
        }else{
            server_file = url;
        }
        size_t nfile = first;

        // Data buffer if naccum > 1
        Ultracam::Frame dbuffer;
        int nstack = 0;
        double ttime = 0.;

        // fits buffers
        char errmsg[FLEN_ERRMSG];

        // stuff to do with FITS tables
        char* ttype[] = {"Name", "Value", "Comment"};
        char* tform[3];
        // Declare space for the format strings ... should be more than enough
        for(int i=0; i<3; i++) tform[i] = new char[10];

        const char* DVAL = "Directory marker";
        const char* CNAM = "CCD number";
        const char* CCOM = "The CCD number of this frame";
        char* SCALE = "LINEAR";
        char* UNITS = "pixels";

        for(;;){

            // Carry on reading until data are OK
            bool get_ok;
            for(;;){
                if(!(get_ok = Ultracam::get_server_frame(source, url, data, serverdata, nfile, twait, tmax))) break;
                if(serverdata.is_junk(nfile)){
                    std::cerr << "Skipping file " << nfile << " which has junk data" << std::endl;
                    nfile++;
                }else{
                    break;
                }
            }
            if(!get_ok) break;

            // Subtract a bias frame
            if(bias) data -= bias_frame;

            // Apply the flat field
            if(flat) data /= flat_frame;

            // Apply threshold
            if(thresh) data.step(photon);

            nstack++;
            if(nstack < naccum){
                if(nstack == 1){
                    dbuffer = data;
                    ttime   = 0.;
                    std::cout << std::endl;
                }else{
                    dbuffer += data;
                }
                ttime   += data["UT_date"]->get_double();
                std::cout << " Frame " << nstack << " of " << naccum << ", time = " << data["UT_date"]->get_time()
                          << " added into data buffer." << std::endl;

            }else{

                // Retrieve from the data buffer if necessary
                if(naccum > 1) {
                    ttime  += data["UT_date"]->get_double();
                    data   += dbuffer;
                    std::cout << " Frame " << nstack << " of " << naccum << ", time = " << data["UT_date"]->get_time()
                              << " added into data buffer." << std::endl;
                    ttime  /= nstack;
                    data.set("UT_date", new Subs::Htime(Subs::Time(ttime), "mean UT date and time at the centre of accumulated exposure"));
                    nstack  = 0;
                    std::cout << std::endl;
                }

                // Extract some header info
                Subs::Time ut_date = data["UT_date"]->get_time();
                int year  = ut_date.year();
                int month = ut_date.month();
                int day   = ut_date.day();
                char dateobs[11];
                sprintf(dateobs,"%4d-%02d-%02d", year, month, day);
                Subs::Time::HMS hms = ut_date.hms();
                char ut[14];
                sprintf(ut, "%02d:%02d:%02d.%04d", hms.hour, hms.min, hms.sec, int(10000.*hms.fsec+0.5));

                // Compute maximum limits of header values. An extra row
                // is added when splitting the frames.
                long int nrow;
                std::string::size_type name_max, value_max, comment_max;
                if(split){
                    nrow         = 1;
                    name_max     = strlen(CNAM);
                    value_max    = Subs::str(data.size()).length();
                    comment_max  = strlen(CCOM);
                }else{
                    nrow         = 0;
                    name_max     = 0;
                    value_max    = 0;
                    comment_max  = 0;
                }

                for(Subs::Header::const_iterator cit=data.begin(); cit != data.end(); cit++){

                    nrow++;
                    name_max = std::max(name_max, cit->fullname().length());

                    if(cit->value->is_a_dir())
                        value_max = std::max(value_max, strlen(DVAL));
                    else
                        value_max = std::max(value_max, cit->value->get_string().length());

                    comment_max = std::max(comment_max, cit->value->get_comment().length());
                }

                // Define formats of each table column
                strcpy(tform[0],(Subs::str(name_max) + "A").c_str());
                strcpy(tform[1],(Subs::str(value_max) + "A").c_str());
                strcpy(tform[2],(Subs::str(comment_max) + "A").c_str());

                const string::size_type MAXCHAR = std::max(std::max(name_max, value_max), comment_max);
                char* parr[1];
                char* &pptr = parr[0];
                pptr = new char[MAXCHAR+1];

                // Write it out
                std::string fits, fname;
                fname = server_file + "_" + Subs::str(int(nfile), ndigit);
                int inumber;
                float fnumber;

                if(split){

                    for(size_t nccd=0; nccd<data.size(); nccd++){
                        fits = fname + "_" + Subs::str(nccd+1) + ".fits";
                        if(overwrite) fits = std::string("!") + fits;

                        // Create a fits file
                        fitsfile *fptr;
                        int status = 0;
                        fits_create_file(&fptr, fits.c_str(), &status);

                        long int dims[2]={0,0}, fpixel[2] = {1,1};

                        // make first HDU a dummy
                        if(bias || naccum > 1)
                            fits_create_img(fptr, FLOAT_IMG, 0, dims, &status);
                        else
                            fits_create_img(fptr, USHORT_IMG, 0, dims, &status);

                        // Create and write an HDU for each window
                        for(size_t nwin=0; nwin<data[nccd].size(); nwin++){
                            Ultracam::Windata &win = data[nccd][nwin];
                            dims[0] = win.nx();
                            dims[1] = win.ny();
                            if(bias || naccum > 1)
                                fits_create_img(fptr, FLOAT_IMG, 2, dims, &status);
                            else
                                fits_create_img(fptr, USHORT_IMG, 2, dims, &status);

                            if(status){
                                fits_get_errstatus(status, errmsg);
                                fits_close_file(fptr, &status);
                                throw Ultracam::Ultracam_Error(std::string("A1: ") + fits + std::string(": ") + std::string(errmsg));
                            }

                            Ultracam::internal_data *array = win.buffer();
                            fits_write_pix(fptr, TFLOAT, fpixel, win.ntot(), array, &status);
                            delete[] array;

                            inumber = nwin + 1;
                            fits_write_key(fptr, TINT, "NWIN", &inumber, "Window number", &status);
                            fits_write_key(fptr, TSTRING, "CTYPE1", SCALE, "Transformation of X scale", &status);
                            fits_write_key(fptr, TSTRING, "CTYPE2", SCALE, "Transformation of Y scale", &status);
                            fits_write_key(fptr, TSTRING, "CUNIT1", UNITS, "Units of transformed X scale", &status);
                            fits_write_key(fptr, TSTRING, "CUNIT2", UNITS, "Units of transformed Y scale", &status);

                            fnumber = 1. - float(data[nccd][nwin].llx() - 1)/data[nccd][nwin].xbin();
                            fits_write_key(fptr, TFLOAT, "CRPIX1", &fnumber, "Pixel equivalent in X of reference point", &status);
                            fnumber = 1. - float(data[nccd][nwin].lly() - 1)/data[nccd][nwin].ybin();
                            fits_write_key(fptr, TFLOAT, "CRPIX2", &fnumber, "Pixel equivalent in Y of reference point", &status);

                            fnumber = 1.;
                            fits_write_key(fptr, TFLOAT, "CRVAL1", &fnumber, "X value of reference point", &status);
                            fits_write_key(fptr, TFLOAT, "CRVAL2", &fnumber, "Y value of reference point", &status);

                            fnumber = data[nccd][nwin].xbin();
                            fits_write_key(fptr, TFLOAT, "CD1_1", &fnumber, "Binning factor in X", &status);

                            // No diagonal terms
                            fnumber = 0.0;
                            fits_write_key(fptr, TFLOAT, "CD1_2", &fnumber, NULL, &status);
                            fits_write_key(fptr, TFLOAT, "CD2_1", &fnumber, NULL, &status);

                            fnumber = float(data[nccd][nwin].ybin());
                            fits_write_key(fptr, TFLOAT, "CD2_2", &fnumber, "Binning factor in Y", &status);
                        }

                        // Add headers as a table
                        fits_create_tbl(fptr, BINARY_TBL, nrow, 3, ttype, tform, NULL, "ULTRACAM Headers", &status);

                        // write name of header item
                        strcpy(pptr, CNAM);
                        fits_write_col(fptr, TSTRING, 1, 1, 1, 1, parr, &status);

                        // write name of header value
                        strcpy(pptr, Subs::str(nccd+1).c_str());
                        fits_write_col(fptr, TSTRING, 2, 1, 1, 1, parr, &status);

                        // write comment
                        strcpy(pptr, CCOM);
                        fits_write_col(fptr, TSTRING, 3, 1, 1, 1, parr, &status);

                        // Write entries to table one by one
                        long int firstrow = 1;

                        for(Subs::Header::const_iterator cit=data.begin(); cit != data.end(); cit++){
                            firstrow++;

                            // write name of header item
                            strcpy(pptr,cit->fullname().c_str());
                            fits_write_col(fptr, TSTRING, 1, firstrow, 1, 1, parr, &status);

                            // write name of header value
                            if(cit->value->is_a_dir()){
                                strcpy(pptr, DVAL);
                                fits_write_col(fptr, TSTRING, 2, firstrow, 1, 1, parr, &status);
                            }else{
                                strcpy(pptr,cit->value->get_string().c_str());
                                fits_write_col(fptr, TSTRING, 2, firstrow, 1, 1, parr, &status);
                            }

                            // write comment
                            strcpy(pptr,cit->value->get_comment().c_str());
                            fits_write_col(fptr, TSTRING, 3, firstrow, 1, 1, parr, &status);
                        }

                        fits_close_file(fptr, &status);
                        fits_report_error(stderr, status);

                        if(status == 0){
                            if(overwrite)
                                std::cout << "Written " << fits.substr(1) << " to disk." << std::endl;
                            else
                                std::cout << "Written " << fits << " to disk." << std::endl;
                        }
                    }

                }else{

                    fits = fname + std::string(".fits");
                    if(overwrite)
                        fits = std::string("!") + fits;

                    // Create a fits file
                    fitsfile *fptr;
                    int status = 0;
                    fits_create_file(&fptr, fits.c_str(), &status);

                    long int dims[2]={0,0}, fpixel[2] = {1,1};

                    // make first HDU a dummy
                    if(bias || naccum > 1)
                        fits_create_img(fptr, FLOAT_IMG, 0, dims, &status);
                    else
                        fits_create_img(fptr, USHORT_IMG, 0, dims, &status);

                    // Create and write an HDU for each window of each CCD
                    float xoff = 0.;
                    for(size_t nccd=0; nccd<data.size(); nccd++){
                        for(size_t nwin=0; nwin<data[nccd].size(); nwin++){

                            Ultracam::Windata &win = data[nccd][nwin];
                            dims[0] = win.nx();
                            dims[1] = win.ny();
                            if(bias || naccum > 1)
                                fits_create_img(fptr, FLOAT_IMG, 2, dims, &status);
                            else
                                fits_create_img(fptr, USHORT_IMG, 2, dims, &status);

                            if(status){
                                fits_get_errstatus(status, errmsg);
                                fits_close_file(fptr, &status);
                                throw Ultracam::Ultracam_Error(std::string("A1: ") + fits + std::string(": ") + std::string(errmsg));
                            }

                            // Write out data
                            Ultracam::internal_data *array = win.buffer();
                            fits_write_pix(fptr, TFLOAT, fpixel, win.ntot(), array, &status);
                            delete[] array;

                            inumber = nccd + 1;
                            fits_write_key(fptr, TINT, "NCCD", &inumber, "CCD number", &status);
                            inumber = nwin + 1;
                            fits_write_key(fptr, TINT,    "NWIN", &inumber, "Window number", &status);
                            fits_write_key(fptr, TSTRING, "CTYPE1", SCALE, "Transformation of X scale", &status);
                            fits_write_key(fptr, TSTRING, "CTYPE2", SCALE, "Transformation of Y scale", &status);
                            fits_write_key(fptr, TSTRING, "CUNIT1", UNITS, "Units of transformed X scale", &status);
                            fits_write_key(fptr, TSTRING, "CUNIT2", UNITS, "Units of transformed Y scale", &status);

                            fnumber = 1. - float(xoff + data[nccd][nwin].llx() - 1)/data[nccd][nwin].xbin();
                            fits_write_key(fptr, TFLOAT, "CRPIX1", &fnumber, "Pixel equivalent in X of reference point", &status);
                            fnumber = 1. - float(data[nccd][nwin].lly() - 1)/data[nccd][nwin].ybin();
                            fits_write_key(fptr, TFLOAT, "CRPIX2", &fnumber, "Pixel equivalent in Y of reference point", &status);

                            fnumber = 1.;
                            fits_write_key(fptr, TFLOAT, "CRVAL1", &fnumber, "X value of reference point", &status);
                            fnumber = 1.;
                            fits_write_key(fptr, TFLOAT, "CRVAL2", &fnumber, "Y value of reference point", &status);

                            fnumber = data[nccd][nwin].xbin();
                            fits_write_key(fptr, TFLOAT, "CD1_1", &fnumber, "Binning factor in X", &status);

                            // No diagonal terms
                            fnumber = 0.0;
                            fits_write_key(fptr, TFLOAT, "CD1_2", &fnumber, NULL, &status);
                            fits_write_key(fptr, TFLOAT, "CD2_1", &fnumber, NULL, &status);

                            fnumber = float(data[nccd][nwin].ybin());
                            fits_write_key(fptr, TFLOAT, "CD2_2", &fnumber, "Binning factor in Y", &status);
                        }
                        xoff += data[nccd][0].nxtot();
                    }

                    // Add headers as a table
                    fits_create_tbl(fptr, BINARY_TBL, nrow, 3, ttype, tform, NULL, "ULTRACAM Headers", &status);

                    // Write entries to table one by one
                    long int firstrow = 0;

                    for(Subs::Header::const_iterator cit=data.begin(); cit != data.end(); cit++){
                        firstrow++;

                        // write name of header item
                        strcpy(pptr,cit->fullname().c_str());
                        fits_write_col(fptr, TSTRING, 1, firstrow, 1, 1, parr, &status);

                        // write name of header value
                        if(cit->value->is_a_dir()){
                            strcpy(pptr, DVAL);
                            fits_write_col(fptr, TSTRING, 2, firstrow, 1, 1, parr, &status);
                        }else{
                            strcpy(pptr,cit->value->get_string().c_str());
                            fits_write_col(fptr, TSTRING, 2, firstrow, 1, 1, parr, &status);
                        }

                        // write comment
                        strcpy(pptr,cit->value->get_comment().c_str());
                        fits_write_col(fptr, TSTRING, 3, firstrow, 1, 1, parr, &status);
                    }

                    fits_close_file(fptr, &status);
                    fits_report_error(stderr, status);
                    if(status == 0){
                        if(overwrite)
                            std::cout << "Written " << fits.substr(1) << " to disk." << std::endl;
                        else
                            std::cout << "Written " << fits << " to disk." << std::endl;
                    }
                }
                delete[] pptr;
            }

            if(last > 0 && size_t(nfile) >= last) break;
            nfile++;
        }

        for(int i=0; i<3; i++) delete[] tform[i];

    }

    // Handle errors
    catch(const Input_Error& err){
        std::cerr << "\nUltracam::Input_Error:" << std::endl;
        std::cerr << err << std::endl;
    }
    catch(const File_Open_Error& err){
        std::cerr << "\nUltracam::File_Open_error:" << std::endl;
        std::cerr << err << std::endl;
    }
    catch(const Ultracam_Error& err){
        std::cerr << "\nUltracam::Ultracam_Error:" << std::endl;
        std::cerr << err << std::endl;
    }
    catch(const Subs::Subs_Error& err){
        std::cerr << "\nSubs::Subs_Error:" << std::endl;
        std::cerr << err << std::endl;
    }
    catch(const std::string& err){
        std::cerr << "\n" << err << std::endl;
    }
}



