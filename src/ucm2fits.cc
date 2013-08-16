/*

!!begin
!!title  Reads an Ultracam frame and writes out in FITS format
!!author T.R. Marsh
!!created  04 Feb 2002
!!revised  13 Sep 2002
!!root   ucm2fits
!!index  ucm2fits
!!descr  converts an ULTRACAM ucm file to FITS
!!css   style.css
!!class  Programs
!!class  IO
!!class  FITS
!!head1  ucm2fits - converts an Ultracam frame to FITS format

!!emph{ucm2fits} reads an Ultracam ".ucm" file or files and writes out an equivalent
FITS format file or files. Optionally it can either send all CCDs into a single
FITS file or it can split them up into one file per CCD. The headers are written as 
a binary table in the FITS file. 

The FITS files consist of a dummy HDU first followed by one HDU per window, starting from the lower-left
window, then the lower-right, then the next pair etc. If all CCDs are in the file, then the windows are first from CCD1, 
then CCD2, etc. The files can be displayed using 'ds9', and all windows can be displayed using the 'mosaicimage' option which
can be invoked from the command line with
<pre>
ds9 -mosaicimage wcs file.fits
</pre>
GAIA can do something similar. The final HDU is a FITS binary table with all the ULTRACAM headers.

!!head2 Invocation

ucm2fits data split overwrite

!!head2 Command line arguments

!!table
!!arg{data}{The file name or list of file names}
!!arg{split}{true/false according to whether you want to create a FITS file for
each CCD or not. If true, the files will have _1, _2, _3 added to them to indicate
the CCD.}
!!arg{overwrite}{true/false according to whether you want to overwrite any pre-existing files or not.}
!!table

Related routines: !!ref{grab2fits.html}{grab2fits}, !!ref{fits2ucm.html}{fits2ucm}

!!end

*/

#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include "cfitsio/fitsio.h"
#include "trm_subs.h"
#include "trm_input.h"
#include "trm_frame.h"
#include "trm_ultracam.h"

int main(int argc, char* argv[]){

    try{

	// Construct Input object
	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// Define inputs

	input.sign_in("data",      Subs::Input::GLOBAL, Subs::Input::PROMPT);
	input.sign_in("split",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("overwrite", Subs::Input::LOCAL,  Subs::Input::PROMPT);

	std::string fname;
	input.get_value("data", fname, "run001", "data file");
	bool split;
	input.get_value("split", split, false, "split the files to give one FITS file per CCD?");
	bool overwrite;
	input.get_value("overwrite", overwrite, false, "overwrite pre-existing files?");

	// Read file or list
	std::vector<std::string> flist;
	if(Ultracam::Frame::is_ultracam(fname)){
	    flist.push_back(fname);
	}else{      
	    std::ifstream istr(fname.c_str());
	    while(istr >> fname){
		flist.push_back(fname);
	    }
	    istr.close();
	    if(flist.size() == 0) throw Ultracam::Input_Error("No file names loaded");
	}

	std::string fits;

	// stuff to do with FITS tables
	char* ttype[] = {"Name", "Value", "Comment"};
	char* tform[3];
	// Declare space for the format strings ... should be more than enough
	for(int i=0; i<3; i++) tform[i] = new char[10];

	char* parr[1];
	char* &p = parr[0];
	const char* DVAL = "Directory marker";
	const char* CNAM = "CCD number";
	const char* CCOM = "The CCD number of this frame";
	char* SCALE = "LINEAR";
	char* UNITS = "pixels";

	int inumber;
	float fnumber;

	for(size_t nfile=0; nfile<flist.size(); nfile++){

	    fname = flist[nfile];

	    // Read file
	    Ultracam::Frame data(fname);

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
	    p = new char[MAXCHAR+1];

	    if(split){
	
		for(size_t nccd=0; nccd<data.size(); nccd++){
		    fits = fname.substr(0,fname.find(".ucm")) + "_" + Subs::str(nccd+1) + ".fits";
		    if(overwrite) 
			fits = std::string("!") + fits;

		    // Create a fits file
		    fitsfile *fptr;
		    int status = 0;
		    fits_create_file(&fptr, fits.c_str(), &status);
	  
		    long int dims[2]={0,0}, fpixel[2] = {1,1};
		    fits_create_img(fptr, FLOAT_IMG, 0, dims, &status);
		    inumber = nccd + 1;
		    fits_write_key(fptr, TINT, "NCCD", &inumber, "CCD number", &status);
	  
		    // Create and write an HDU for each window
		    for(size_t nwin=0; nwin<data[nccd].size(); nwin++){
			Ultracam::Windata &win = data[nccd][nwin];
			dims[0] = win.nx();
			dims[1] = win.ny();
			fits_create_img(fptr, FLOAT_IMG, 2, dims, &status);

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
		    strcpy(p, CNAM);
		    fits_write_col(fptr, TSTRING, 1, 1, 1, 1, parr, &status);
			
		    // write name of header value
		    strcpy(p, Subs::str(nccd+1).c_str());
		    fits_write_col(fptr, TSTRING, 2, 1, 1, 1, parr, &status);

		    // write comment
		    strcpy(p, CCOM);
		    fits_write_col(fptr, TSTRING, 3, 1, 1, 1, parr, &status);
		    
		    // Write entries to table one by one
		    long int firstrow = 1;

		    for(Subs::Header::const_iterator cit=data.begin(); cit != data.end(); cit++){
			firstrow++;
			
			// write name of header item
			strcpy(p,cit->fullname().c_str());
			fits_write_col(fptr, TSTRING, 1, firstrow, 1, 1, parr, &status);
			
			// write name of header value
			if(cit->value->is_a_dir()){
			    strcpy(p, DVAL);
			    fits_write_col(fptr, TSTRING, 2, firstrow, 1, 1, parr, &status);
			}else{
			    strcpy(p,cit->value->get_string().c_str());
			    fits_write_col(fptr, TSTRING, 2, firstrow, 1, 1, parr, &status);
			}

			// write comment
			strcpy(p,cit->value->get_comment().c_str());
			fits_write_col(fptr, TSTRING, 3, firstrow, 1, 1, parr, &status);
		    }


		    fits_close_file(fptr, &status);
		    fits_report_error(stderr, status);
		}

	    }else{
	
		fits = fname.substr(0,fname.find(".ucm")) + std::string(".fits");
		if(overwrite) 
		    fits = std::string("!") + fits;

		// Create a fits file
		fitsfile *fptr;
		int status = 0;
		fits_create_file(&fptr, fits.c_str(), &status);
	
		long int dims[2]={0,0}, fpixel[2] = {1,1};
		fits_create_img(fptr, FLOAT_IMG, 0, dims, &status);
	
		// Create and write an HDU for each window of each CCD
		float xoff = 0.;
		for(size_t nccd=0; nccd<data.size(); nccd++){
		    for(size_t nwin=0; nwin<data[nccd].size(); nwin++){
			Ultracam::Windata &win = data[nccd][nwin];
			dims[0] = win.nx();
			dims[1] = win.ny();
			fits_create_img(fptr, FLOAT_IMG, 2, dims, &status);
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
		    strcpy(p,cit->fullname().c_str());
		    fits_write_col(fptr, TSTRING, 1, firstrow, 1, 1, parr, &status);
		    
		    // write name of header value
		    if(cit->value->is_a_dir()){
			strcpy(p, DVAL);
			fits_write_col(fptr, TSTRING, 2, firstrow, 1, 1, parr, &status);
		    }else{
			strcpy(p,cit->value->get_string().c_str());
			fits_write_col(fptr, TSTRING, 2, firstrow, 1, 1, parr, &status);
		    }
		    
		    // write comment
		    strcpy(p,cit->value->get_comment().c_str());
		    fits_write_col(fptr, TSTRING, 3, firstrow, 1, 1, parr, &status);
		}

		fits_close_file(fptr, &status);
		fits_report_error(stderr, status);
	    }
	    delete[] p;
	}

	for(int i=0; i<3; i++) delete[] tform[i];

    }
    catch(const std::string& err){
	std::cerr << err << std::endl;
    }
}



