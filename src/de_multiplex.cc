//! \file

#include "trm_subs.h"
#include "trm_frame.h"
#include "trm_ultracam.h"

// See later for the ULTRASPEC version.

/**

This function is completely ULTRACAM specific. It de-multiplexes the data stored in the buffer 
sent back by the fileserver. This comes back in the order:

\verbatim
CCD=0, window=0, iy=0, ix=0 
CCD=0, window=1, iy=0, ix=nx-1 
CCD=1, window=0, iy=0, ix=0 
CCD=1, window=1, iy=0, ix=nx-1 
CCD=2, window=0, iy=0, ix=0 
CCD=2, window=1, iy=0, ix=nx-1 

CCD=0, window=0, iy=0, ix=1 
CCD=0, window=1, iy=0, ix=nx-2 
CCD=1, window=0, iy=0, ix=1
CCD=1, window=1, iy=0, ix=nx-2 
CCD=2, window=0, iy=0, ix=1
CCD=2, window=1, iy=0, ix=nx-2 
.
.
.

CCD=0, window=0, iy=1, ix=0 
CCD=0, window=0, iy=1, ix=nx-1 
CCD=1, window=0, iy=1, ix=0
CCD=1, window=0, iy=1, ix=nx-1 
CCD=2, window=0, iy=1, ix=0
CCD=2, window=0, iy=1, ix=nx-1 
\endverbatim

etc until all iy are done, and then onto the next window pair until the end. 
This routine puts all of this into the standard arrays inside the Frame passed to it.
\param buffer a buffer of data returned by the server, without a header
\param data   a data frame to store it into. Needs its format to have been defined
by running parseXML

The routine tries to work out what sort of machine we are one and will swap bytes
if it is thought to be big-endian (as opposed to intel / linux little endian)

*/

void Ultracam::de_multiplex_ultracam(char *buffer, Frame& data){

    // Initialise. 
    // PIX_SHIFT accounts for a problem that was present until May 2007 the cure for which 
    // is to remove the outermost pixel of all windows.
    const int  PIX_SHIFT = data["Instrument.version"]->get_int() < 0 ? 1 : 0;
    const bool TRIM      = data["Trimming.applied"]->get_bool();
    const int  NCOL      = TRIM ? data["Trimming.ncols"]->get_int() + PIX_SHIFT: PIX_SHIFT;
    const int  NROW      = TRIM ? data["Trimming.nrows"]->get_int() : 0;
    const bool LITTLE    = Subs::is_little_endian();
    const bool STRIP     = NCOL > 0 || NROW > 0;

    size_t nwin1=0, nwin2=1;

    // Variables accessed most often are declared 'register' in the hope of speeding things
    register int nccd;
    register const int NCCD = data.size();
    register size_t ip = 0;
    register int iy;
    register Subs::UCHAR cbuff[2];

    // Overscan mode is a special case. Separate it because of rarity and difficulty
    bool normal = (data["Instrument.Readout_Mode_Flag"]->get_int() != ServerData::FULLFRAME_OVERSCAN);

    if(normal){

	register int NX = data[0][nwin1].nx();
	register int ix1=0, ix2 = NX-1;
	iy = 0;

	// Advance buffer pointer if trimming enabled. The factor 4 comes from 2 bytes for
	// each pixel and 2 windows. Should not have been done in overscan mode
	if(STRIP){
	    ip += 4*NCCD*(NX+NCOL)*NROW;
	    ip += 4*NCCD*NCOL;
	}

	for(;;){

	    // Add in pixels from the left of the left window and from the right of the right window for each window
	    // pair and CCD in turn. The pointer to char is cast to a pointer to an unsigned 2-byte integer and then the value 
	    // of the thing pointed at is converted to the internal data type of ULTRACAM, and the pointer is advanced 
	    // appropriately. This loop is the critical one for speed. 

	    for(nccd=0; nccd<NCCD; nccd++){	
		if(LITTLE){
		    data[nccd][nwin1][iy][ix1] = internal_data(*(Subs::UINT2*)(buffer+ip));
		    ip += 2;
		    data[nccd][nwin2][iy][ix2] = internal_data(*(Subs::UINT2*)(buffer+ip));
		    ip += 2;
		}else{
		    cbuff[1] = buffer[ip++];
		    cbuff[0] = buffer[ip++];
		    data[nccd][nwin1][iy][ix1] = internal_data(*(Subs::UINT2*)cbuff);
		    cbuff[1] = buffer[ip++];
		    cbuff[0] = buffer[ip++];
		    data[nccd][nwin2][iy][ix2] = internal_data(*(Subs::UINT2*)cbuff);
		}
	    }
      
	    // Update pointers for next round. 
	    ix1++;
	    ix2--;
	    if(ix1 == NX){
		ix1 = 0;
		ix2 = NX-1;
		iy++;
		if(iy == data[0][nwin1].ny()){
		    nwin2 += 2;
	  
		    // Finished
		    if(nwin2 > data[0].size()) break;
	  
		    nwin1 += 2;
		    iy     = 0;
		    NX     = data[0][nwin1].nx();
		    ix2    = NX-1;
	  
		    // skip lower rows 
		    if(STRIP) ip += 4*NCCD*(NX+NCOL)*NROW;
	  
		}
	
		// skip columns on left of left window, right of right window
		if(STRIP) ip += 4*NCCD*NCOL;
	    }
	}

    }else{

	// Overscan mode is a bit of a bugger. 24 columns on left of left window and right
	// of right window, plus 4 on right of left window and left of right window
	// plus another 8 rows at the top. Very specific implementation here to split
	// between 6 windows with the two parts of the overscan combined into single strips
	// which appear on the right of the main windows and an extra part at the top. This
	// way the mapping of real pixels to image pixel is preserved so object positions stay
	// the same.
    
	register int ix;
	const int  XBIN      = data[0][0].xbin();
	const int  YBIN      = data[0][0].ybin();

	for(iy=0; iy<1032/YBIN; iy++){
	    for(ix=0; ix<540/XBIN; ix++){
		for(nccd=0; nccd<NCCD; nccd++){
		    if(ix < 24/XBIN){
	    
			// left and right overscan windows
			if(LITTLE){
			    data[nccd][2][iy][ix]   = internal_data(*(Subs::UINT2*)(buffer+ip));	 
			    ip += 2;
			    data[nccd][3][iy][28/XBIN-1-ix] = internal_data(*(Subs::UINT2*)(buffer+ip));	  
			    ip += 2;
			}else{
			    cbuff[1] = buffer[ip++];
			    cbuff[0] = buffer[ip++];
			    data[nccd][2][iy][ix]   = internal_data(*(Subs::UINT2*)cbuff);
			    cbuff[1] = buffer[ip++];
			    cbuff[0] = buffer[ip++];
			    data[nccd][3][iy][28/XBIN-1-ix] = internal_data(*(Subs::UINT2*)cbuff);
			}

		    }else if(ix < 536/XBIN){
	    
			if(iy < 1024/YBIN){
			    // left and right data windows
			    if(LITTLE){
				data[nccd][0][iy][ix-24/XBIN]  = internal_data(*(Subs::UINT2*)(buffer+ip));	  
				ip += 2;
				data[nccd][1][iy][536/XBIN-1-ix] = internal_data(*(Subs::UINT2*)(buffer+ip));	  
				ip += 2;
			    }else{
				cbuff[1] = buffer[ip++];
				cbuff[0] = buffer[ip++];
				data[nccd][0][iy][ix-24/XBIN]  = internal_data(*(Subs::UINT2*)cbuff);
				cbuff[1] = buffer[ip++];
				cbuff[0] = buffer[ip++];
				data[nccd][1][iy][536/XBIN-1-ix] = internal_data(*(Subs::UINT2*)cbuff); 
			    }

			}else{

			    // top left and right overscan windows
			    if(LITTLE){
				data[nccd][4][iy-1024/YBIN][ix-24/XBIN]  = internal_data(*(Subs::UINT2*)(buffer+ip));	  
				ip += 2;
				data[nccd][5][iy-1024/YBIN][536/XBIN-1-ix] = internal_data(*(Subs::UINT2*)(buffer+ip));	  
				ip += 2;
			    }else{
				cbuff[1] = buffer[ip++];
				cbuff[0] = buffer[ip++];
				data[nccd][4][iy-1024/YBIN][ix-24/XBIN]  = internal_data(*(Subs::UINT2*)cbuff);
				cbuff[1] = buffer[ip++];
				cbuff[0] = buffer[ip++];
				data[nccd][5][iy-1024/YBIN][536/XBIN-1-ix] = internal_data(*(Subs::UINT2*)cbuff);  
			    }
			}
	    
		    }else{

			// left and right overscan windows again
			if(LITTLE){
			    data[nccd][2][iy][ix-512/XBIN] = internal_data(*(Subs::UINT2*)(buffer+ip));	  
			    ip += 2;
			    data[nccd][3][iy][540/XBIN-1-ix] = internal_data(*(Subs::UINT2*)(buffer+ip));	  
			    ip += 2;
			}else{
			    cbuff[1] = buffer[ip++];
			    cbuff[0] = buffer[ip++];
			    data[nccd][2][iy][ix-512/XBIN] = internal_data(*(Subs::UINT2*)cbuff);
			    cbuff[1] = buffer[ip++];
			    cbuff[0] = buffer[ip++];
			    data[nccd][3][iy][540/XBIN-1-ix] = internal_data(*(Subs::UINT2*)cbuff);  
			}
		    }
		}	
	    }
	}
    }
}


/**

This function is ULTRASPEC specific. It de-multiplexes the data stored in the buffer 
sent back by the fileserver. It is designed for windows strung out in the Y direction
with no overlap. This is the standard ULTRASPEC mode. See later for a drift mode version.

In the standard mode the pixels come back in the order:

\verbatim
window=0, iy=0, ix=0 
window=0, iy=0, ix=1 
window=0, iy=0, ix=2
.
.
.
window=0, iy=1, ix=0
.
.
.
window=1, iy=1, ix=0
.
.
.
\endverbatim

There are two possible read out modes however, and in one of them the read out order is reversed in the X direction, so
that it starts with nx-1. This function swaps this case so that the images will appear the same on the screen. It also has
to make sure that a given format is compatible whether normal or avalanche. It turns out that the only way to ensure this
is to remove any pixels if they are amongst the first 16 that can be read out by either port (overscan pixels). These are 
thus ignored and never appear at any stage.

*/

void Ultracam::de_multiplex_ultraspec(char *buffer, Frame& data, const std::vector<int>& nchop){

    // Initialise
    const bool TRIM   = data["Trimming.applied"]->get_bool();
    const int  NCOL   = TRIM ? data["Trimming.ncols"]->get_int() : 0;
    const int  NROW   = TRIM ? data["Trimming.nrows"]->get_int() : 0;
    const bool LITTLE = Subs::is_little_endian();

    // Variables accessed most often are declared 'register' in the hope of speeding things
    register size_t ip = 0;
    register int iy;
    register Subs::UCHAR cbuff[2];

    // Flag the output being used. This is what indicates reversal or not.
    bool normal = (data["Instrument.Output"]->get_int() == 0);

    if(normal){

	register size_t nwin = 0;
	register int NX = data[0][0].nx();
	register int ix=0;
	iy = 0;

	// Advance buffer pointer if trimming enabled. The factor 2 comes from 2 bytes for each pixel.
	if(TRIM){
	    ip += 2*(NX+nchop[nwin]+NCOL)*NROW;
	    ip += 2*NCOL;
	}
	
	// Skip overscan pixels
	ip += 2*nchop[nwin];

	for(;;){

	    // 'normal' mode we assume that the first pixel read out is the left-most 
	    if(LITTLE){
		data[0][nwin][iy][ix] = internal_data(*(Subs::UINT2*)(buffer+ip));
		ip += 2;
	    }else{
		cbuff[1] = buffer[ip++];
		cbuff[0] = buffer[ip++];
		data[0][nwin][iy][ix] = internal_data(*(Subs::UINT2*)cbuff);
	    }

	    // Update pointers for next round. 
	    ix++;
	    if(ix == NX){
		ix = 0;
		iy++;
		if(iy == data[0][nwin].ny()){

		    nwin++;

		    // Finished
		    if(nwin >= data[0].size()) break;
	  	  
		    iy     = 0;
		    NX     = data[0][nwin].nx();
	  
		    // skip lower rows 
		    if(TRIM) ip += 2*(NX+nchop[nwin]+NCOL)*NROW;
	  
		}
	
		// skip columns on left of window
		if(TRIM) ip += 2*NCOL;

		// Skip overscan pixels
		ip += 2*nchop[nwin];
	    }
	}
    
    }else{
      
	register size_t nwin = 0;
	register int NX = data[0][nwin].nx();
	register int ix = NX-1;
	iy = 0;

	// Advance buffer pointer if trimming enabled. The factor 2 comes from 2 bytes for each pixel.
	if(TRIM){
	    ip += 2*(NX+nchop[nwin]+NCOL)*NROW;
	    ip += 2*NCOL;
	}
      
	// Skip overscan pixels
	ip += 2*nchop[nwin];

	for(;;){
	  
	    // 'abnormal' mode we assume that the first pixel read out is the right-most 
	    if(LITTLE){
		data[0][nwin][iy][ix] = internal_data(*(Subs::UINT2*)(buffer+ip));
		ip += 2;
	    }else{
		cbuff[1] = buffer[ip++];
		cbuff[0] = buffer[ip++];
		data[0][nwin][iy][ix] = internal_data(*(Subs::UINT2*)cbuff);
	    }

	    // Update pointers for next round. 
	    ix--;
	    if(ix < 0){
		iy++;
		if(iy == data[0][nwin].ny()){

		    nwin++;

		    // Finished
		    if(nwin >= data[0].size()) break;
	  	  
		    iy     = 0;
		    NX     = data[0][nwin].nx();
	  
		    // skip lower rows 
		    if(TRIM) ip += 2*(NX+nchop[nwin]+NCOL)*NROW;
	  
		}

		ix = NX-1;
	
		// skip columns on right window
		if(TRIM) ip += 2*NCOL;

		// Skip overscan pixels
		ip += 2*nchop[nwin];
	    }
	}
    }
}

/**

This function is ULTRASPEC specific. It de-multiplexes the data stored in the buffer 
sent back by the fileserver. It is designed for window pairs in the x direction as in the
ULTRASPEC drift mode.

In the drift mode the pixels come back in the order:

\verbatim
window=0, iy=0, ix=0 
window=0, iy=0, ix=1 
window=0, iy=0, ix=2
.
.
.
window=1, iy=0, ix=0
.
.
.
window=0, iy=1, ix=0
.
.
.
\endverbatim

There are two possible read out modes however, and in one of them the read out order is reversed in the X direction, so
that it starts with nx-1. This function swaps this case so that the images will appear the same on the screen. It also has
to make sure that a given format is compatible whether normal or avalanche. It turns out that the only way to ensure this
is to remove any pixels if they are amongst the first 16 that can be read out by either port (overscan pixels). These are 
thus ignored and never appear at any stage.

*/

void Ultracam::de_multiplex_ultraspec_drift(char *buffer, Frame& data, const std::vector<int>& nchop){

    // Initialise
    const bool TRIM   = data["Trimming.applied"]->get_bool();
    const int  NCOL   = TRIM ? data["Trimming.ncols"]->get_int() : 0;
    const int  NROW   = TRIM ? data["Trimming.nrows"]->get_int() : 0;
    const bool LITTLE = Subs::is_little_endian();

    // Variables accessed most often are declared 'register' in the hope of speeding things
    register size_t ip = 0;
    register Subs::UCHAR cbuff[2];

    // Flag the output being used. This is what indicates reversal or not.
    bool normal = (data["Instrument.Output"]->get_int() == 0);

    if(normal){

        // Must deal with window pairs now.
	size_t nwin1 = 0, nwin2 = 1;
	const int NX1 = data[0][nwin1].nx();
	const int NX2 = data[0][nwin2].nx();
	register int iy  = 0;
        register int ix = 0;

        // At the start we are dealing with the first window
        register int NX = NX1;
        size_t nwin     = nwin1;
        bool   first    = true;

	// Advance buffer pointer if trimming enabled. The outer factors of 2 come from 2 bytes for each pixel.
 	if(TRIM){
	    ip += 2*(NX1+NX2+nchop[nwin]+2*NCOL)*NROW;
	    ip += 2*NCOL;
	}
	
	// Skip overscan pixels
	ip += 2*nchop[nwin];

        // pixel input loop
	for(;;){

	    // 'normal' mode we assume that the first pixel read out is the left-most 
            // here we read and squirrel away a pixel's-worth of data:
	    if(LITTLE){
		data[0][nwin][iy][ix] = internal_data(*(Subs::UINT2*)(buffer+ip));
		ip += 2;
	    }else{
		cbuff[1] = buffer[ip++];
		cbuff[0] = buffer[ip++];
		data[0][nwin][iy][ix] = internal_data(*(Subs::UINT2*)cbuff);
	    }

	    // Move to next x
	    ix++;
	    if(ix == NX){
		ix = 0;
                if(first){
                    // switch to second window
                    first = false;
                    nwin  = nwin2;
                    NX    = NX2;
                }else{
                    // switch back to first window, move the y value up one
                    first = true;
                    nwin  = nwin1;
                    iy++;
                    if(iy == data[0][nwin].ny()) break
                    NX    = NX1;
                }
	
		// skip columns on left of window
		if(TRIM) ip += 2*NCOL;

		// Skip overscan pixels
		ip += 2*nchop[nwin];
	    }
	}
    
    }else{

      	size_t nwin1 = 0, nwin2 = 1;
	const int NX1 = data[0][nwin1].nx();
	const int NX2 = data[0][nwin2].nx();
	register int iy  = 0;

        // At the start we are dealing with the first window
        register int NX     = NX1;
        size_t nwin   = nwin1;
        bool   first  = true;

	register int ix = NX-1;

	// Advance buffer pointer if trimming enabled. The factor 2 comes from 2 bytes for each pixel.
	if(TRIM){
	    ip += 2*(NX1+NX2+nchop[nwin]+2*NCOL)*NROW;
	    ip += 2*NCOL;
	}
      
	// Skip overscan pixels
	ip += 2*nchop[nwin];

        // pixel input loop
	for(;;){
	  
	    // 'abnormal' mode we assume that the first pixel read out is the right-most 
	    if(LITTLE){
		data[0][nwin][iy][ix] = internal_data(*(Subs::UINT2*)(buffer+ip));
		ip += 2;
	    }else{
		cbuff[1] = buffer[ip++];
		cbuff[0] = buffer[ip++];
		data[0][nwin][iy][ix] = internal_data(*(Subs::UINT2*)cbuff);
	    }

	    // Move to next x
	    ix--;
	    if(ix < 0){
                if(first){
                    // switch to second window
                    first = false;
                    nwin  = nwin2;
                    NX    = NX2;
                }else{
                    // switch back to first window, move the y value up one
                    first = true;
                    nwin  = nwin1;
                    iy++;
                    if(iy == data[0][nwin].ny()) break;
                    NX    = NX1;
		}

		ix = NX-1;
	
		// skip columns on right window
		if(TRIM) ip += 2*NCOL;

		// Skip overscan pixels
		ip += 2*nchop[nwin];
	    }
	}
    }
}

