/*

!!begin

!!title   makeflat to make a flat field
!!author  T.R. Marsh
!!created 27 May 2003
!!revised 09 Jan 2006
!!descr   makes a series of frames into a flat field.
!!css     style.css
!!root    makeflat
!!index   makeflat
!!class   Programs 
!!class   Observing
!!class   Arithematic
!!head1   makeflat - a series of frames into a flat field.

!!emph{makeflat} coadds a set of frames by taking the median or the clipped
mean at each pixel. It is designed to take the hassle out of producing sky
flats for ULTRACAM data. The user can specify thresholds to determine which
frames to use for the flats because typically evening flats start saturated
and only become unsaturated after a while. "Saturated" for ULTRACAM really
means no up/down "pepper" pattern which typically appears above about ~28,000
in the g and u CCDs and ~50,000 in the red CCD.  The program first determines
the mean levels of all input frames (which must be bias subtracted beforehand)
and then averages them in groups of similar mean level, each of size
!!emph{ngroup} (see below). This averaging is done via a clipped mean or
median. The averages are then co-added. This procedure correctly weights low
and high signal flats avoiding the problem in simultaneous combination of all
frames (with !!ref{combine.html}{combine} for example) that one give ratty
data and good data equal weight.

Consider taking 70 frames and setting ngroup=7. The frames will then be
grouped by mean level into 10 groups. Each of these will be combined with
normalisation. The frames must be normalised to make median/clipped mean
combining possible under circumstances when the mean level is always
changing. The 10 resulting frames should be free of stars, assuming that the
telescope was moved as the flat was being taken because the median will kick
them out. The 10 frames are then scaled back to the mean level of their input
groups and then added together. This is the step that allocates the correct
weight to both high and low count frames.

The final step is to normalise each CCD by its mean. You can separately
normalise over different regions later by hand if you prefer. Frames are also
selected with a lower threshold on the basis that if the level is low,
uncertainty in the bias level might cause errors in the final flat. Of course,
setting this level may depend on just what is available. The program tells you
how many valid flats there were at the end to help you decide.

<p>
The 'sigma clipping' method used is not without pitfalls: consider what happens
in a case of pure gaussian noise. Applying clipping at 1.96 sigma should lead
to rejection of 5% of the points. In practice however it causes more like 12% rejection.
This is because as points are rejected, the estimated RMS becomes too small. Even worse
at 1.6449 sigma one expects 10% rejection whereas 45% are rejected, while, much better,
at 2.5758 sigma one expects 1% rejection cf 1% observed. 2.5 sigma is thus a sensible minimum 
to adopt.

<p>
<strong>NB</strong> If you ctrl-C'ed !!ref{grab.html}{grab} while getting the files for !!emph{makeflat}
you may corrupt the last file. You can tell this by seeing if there is a file in addition to whatever
!!ref{grab.html}{grab} says it has written to disk. You should delete such files because they will prevent
!!emph{makeflat} from working.

<p>
<strong><font color="red">NB The flats must be bias subtracted prior to
running this routine. If there are no valid frames for any CCD, the corresponding CCD
will be set = 1 on output and you will need to splice it in from a different flat 
using !!ref{uset.html}{uset}.
</font></strong>

!!head2 Invocation

makeflat list method (sigma careful) ngroup region low high output

!!head2 Arguments

!!table
!!arg{list}{List of file names.}
!!arg{method}{'m' = median, 'c' = clipped mean. The clipped mean rejects one pixel at a time
and then recomputes the mean and rms again before having another go.}
!!arg{sigma}{If method = 'c', this is the number of sigmas for rejection. See above for some information
on choosing this.}
!!arg{careful}{If method = 'c', this controls the mehod of rejection. true means pixels are rejected
one at a time, worst first. false means pixels are rejected in groups which can potentially lead to
the odd false rejection.}
!!arg{ngroup}{The number of frames/group to be combined prior to summing. This is used as a minimum number if it
does not divide the number of available frames equally. A typical number is 7.}
!!arg{region}{The region over which to measure the average, specified as a window file (e.g. see
!!ref{setwin.html}{setwin}).  Set equal to "FULLFRAME" to get the entire frame.}
!!arg{low}{Lowest mean level to include. You will be prompted for a value for <strong>each</strong> CCD}
!!arg{high}{Highest mean level to include. You will need to specity a value for <strong>each</strong> CCD}
!!arg{satval}{It is possible for saturated frames to have overall the correct mean, thus an extra safeguard is to 
eliminate saturated frames. First you need to define a level to count as saturated. You may need to account for
bias subtraction}
!!arg{maxsat}{This specifies the maximum percentage of pixels that can be saturated before a frame is kicked out}
!!arg{output}{Output frame}
!!table

!!head2 Guidance notes

Assuming you have observed the twilight sky with ULTRACAM you will typically have a large
file which may start with some or all of the CCDs saturated, and then later one by one, the
CCDs will come off saturation. Even after this point, the green and blue typically show extensive
"peppering" above count levels of about 30,000/pixel. Late in the run, the count levels may be low
and you may not want these. The first thing to do is to use !!ref{grab.html}{grab}
to grab the files into a series of ucm files.You should next subtract the bias frame from each of these.
Next work out the upper and lower levels for each CCD of flats that you think will be OK. You are now
ready to use !!emph{makeflat}. The output from !!emph{makeflat} should be immediately useable by
!!ref{reduce.html}{reduce}. However sometimes you may have take the flats from different runs for different
filters, in which case !!ref{uset.html}{uset} is useful to splice different frames together. You should note 
the number of frames that !!emph{makeflat} says it has found for each CCD in deciding upon this.

!!end

*/

#include <cstdlib>
#include <cfloat>
#include <string>
#include <map>
#include "trm/subs.h"
#include "trm/input.h"
#include "trm/frame.h"
#include "trm/fdisk.h"
#include "trm/ultracam.h"

// Basic structure for each CCD keyed by mean value inside 'map' containers
struct Info{
    std::string file;
    size_t npix;
    Info(const std::string& fname, const size_t& np) : file(fname), npix(np) {}
};

int main(int argc, char* argv[]){

    using Ultracam::Ultracam_Error;
    using Ultracam::internal_data;
    using Ultracam::raw_data;

    try{

	// Construct Input object

	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// Sign-in input variables
	input.sign_in("list",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("method",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("sigma",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("careful",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("npgroup",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("region",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("low",       Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("high",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("satval",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("maxsat",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("output",    Subs::Input::LOCAL,  Subs::Input::PROMPT);

	// Get inputs
	std::string stlist;
	input.get_value("list", stlist, "list", "list of frames to combine");

	// Read file list
	std::vector<std::string> flist;
	std::string name;
	std::ifstream istr(stlist.c_str());
	while(istr >> name){
	    flist.push_back(name);
	}
	istr.close();
	size_t nfile = flist.size();
	if(nfile == 0) throw Ultracam::Input_Error("No file names loaded");

	char method;
	input.get_value("method", method, 'c', "cCmM", "what combination method?");
	method = toupper(method);
	float sigma;
	bool careful;
	if(method == 'C'){
	    input.get_value("sigma",   sigma, 3.f, 1.f, FLT_MAX, "threshold multiple of RMS to reject");
	    input.get_value("careful", careful, true, "reject pixels one at a time?");
	}
	int npgroup;
	input.get_value("npgroup", npgroup, 1, 1, 1000, "number of frames per sub-group");

	// Read first frame to define number of CCDs
	Ultracam::Frame out(flist[0]);

	std::string sregion;
	input.get_value("region", sregion, "FULLFRAME", "region over which to determine the mean");
	Ultracam::Mwindow region;
	if(sregion != "FULLFRAME"){

	    region.rasc(sregion);

	}else{

	    // Build up a full frame window from the first file
	    // Based upon the fact that 'Windata's are also 'Window's
	    region.resize(out.size());
	    for(size_t nccd=0; nccd<out.size(); nccd++)
		for(size_t nwin=0; nwin<out[nccd].size(); nwin++)
		    region[nccd].push_back(out[nccd][nwin]); 

	}

	if(sregion != "FULLFRAME" && out.size() != region.size())
	    throw Ultracam::Input_Error("First data frame and region files have differing numbers of CCDs");

	std::vector<double> low;
	input.get_value("low",  low,  7000.,  1., DBL_MAX, out.size(), "lowest mean level to consider");
	std::vector<double> high;
	input.get_value("high", high, 30000., 1., DBL_MAX, out.size(), "highest mean level to consider");
	float satval;
	input.get_value("satval", satval, 61000.f, -FLT_MAX, FLT_MAX, "saturation value");
	float maxsat;
	input.get_value("maxsat", maxsat, 0.1f, 0.f, 100.f, "maximum percentage saturated pixels");
	std::string output;
	input.get_value("output", output, "output", "output file");

	Ultracam::Image::Stats stats;

	if(nfile > 1){

	    out = 0;

	    // Compute and store means of each CCD.
	    // The means are used as the keys of one multimap/ccd
	    // which leads back to the appropriate file name
	    std::vector< std::multimap<double,Info> > mean(out.size());

	    std::cout << "Computing means of each CCD." << std::endl;

	    Ultracam::Frame temp;

	    int nok[out.size()];
	    for(size_t nc=0; nc<out.size(); nc++)
		nok[nc] = 0;

	    Subs::Array1D<internal_data> buff;

	    for(size_t nf=0; nf<nfile; nf++){

		// load in frame
		temp.read(flist[nf]);
		if(temp != out) 
		    throw Ultracam_Error(flist[nf] + std::string(" is incompatible with ") + flist[0]);

		// compute and store mean as a key leading to information on the file
		// number of saturated pixels is tested at this point.
		for(size_t nc=0; nc<out.size(); nc++){
	    
		    temp[nc].buffer(region[nc], buff);
	    
		    if(buff.size() == 0)
			throw Ultracam_Error("No overlap of normalisation region and data for file = " + flist[nf]);
	    
		    // Count number of saturated pixels
		    int nsat = 0;
		    for(int n=0; n<buff.size(); n++)
			if(buff[n] > satval) nsat++;
	      
		    float buff_mean = buff.mean();
		    if(nsat < int(buff.size()*maxsat/100.)){
			mean[nc].insert(std::make_pair(buff_mean, Info(flist[nf], buff.size())));
			if(buff_mean > low[nc] && buff_mean < high[nc]) nok[nc]++;
		    }else{
			mean[nc].insert(std::make_pair(0., Info(flist[nf], buff.size())));
		    }
		}
	    } 

	    const size_t MXBUFF = 8000000; // total buffer size.
	    size_t nbuff; // individual buffer size.
	    int nvalid, ngroup, ncount, nstart, nend, ncomb;
	    size_t nrejtot = 0, ntot=0;

	    // Iterator over means
	    std::multimap<double,Info>::const_iterator mmit;

	    // Compute total number of pixels for the progress indicator
	    // slightly rubbish bit of code here to avoid limitations of 32-bit
	    // integers. 
	    double nptot = 0., ndtot = 0.;
	    const unsigned long int MXDOT = 20;
	    unsigned int ndot = 0, ndadd;
	    for(size_t nc=0; nc<out.size(); nc++){

		if(nok[nc] == 0){
		    std::cout << "There are no valid frames for CCD " << nc + 1 << std::endl;
		}else if(nok[nc] == 1){
		    std::cout << "There is 1 valid frame for CCD " << nc + 1 << std::endl;
		}else{
		    std::cout << "There are " << nok[nc] << " valid frames for CCD " << nc + 1 << std::endl;
		}

		for(mmit = mean[nc].begin(); mmit != mean[nc].end(); mmit++){
		    if(mmit->first > low[nc] && mmit->first < high[nc]){
			for(size_t nw=0; nw<out[nc].size(); nw++)
			    nptot += out[nc][nw].ntot();
		    }
		}
	    }

	    if(nptot < 5000.){
		std::cout << "Now combining " << int(nptot) << " pixels of data" << std::endl;
	    }else if(nptot < 500000.){
		std::cout << "Now combining " << int(nptot/100.+0.5)/10. << " thousand pixels of data" << std::endl;
	    }else{
		std::cout << "Now combining " << int(nptot/100000.+0.5)/10. << " million pixels of data" << std::endl;
	    }

	    for(size_t ia=0; ia<MXDOT; ia++) std::cout << ".";
	    std::cout << std::endl;
      
	    // Loop over each CCD separately
	    for(size_t nc=0; nc<out.size(); nc++){

		// Evaluate total number of valid frames.
		nvalid = 0;	
		for(mmit = mean[nc].begin(); mmit != mean[nc].end(); mmit++)
		    if(mmit->first > low[nc] && mmit->first < high[nc]) nvalid++;

		if(nvalid > 0){

		    // Compute number of groups
		    ngroup = std::max(1, nvalid / npgroup);
	  
		    // Now start the averaging in groups loop
		    for(int ng=0; ng<ngroup; ng++){
			nstart = npgroup*ng;
			nend   = nstart + npgroup;
			if(ng == ngroup - 1) nend = nvalid;
	    
			// Fdisk pointers
			std::vector<Ultracam::Fdisk*> fptr(nend-nstart);
			std::vector<double> aver(nend-nstart);
	    
			// compute buffer size
			nbuff = MXBUFF / (nend-nstart); 
			double norm = 0.;
	    
			// Create the Fdisks. Note that the normalisation adopted here ensures correct
			// weighting according to the number and level of frames in each group
			for(mmit = mean[nc].begin(), ncomb=0, ncount=0; mmit != mean[nc].end(); mmit++){
			    if(mmit->first > low[nc] && mmit->first < high[nc]){
				if(ncount >= nstart && ncount < nend){
				    aver[ncomb]   = mmit->first;
				    fptr[ncomb++] = new Ultracam::Fdisk(mmit->second.file, nbuff, nc+1);
				    norm         += mmit->first;
				}
				ncount++;
			    }
			}
	    
			// Buffer for combining data
			Ultracam::internal_data comb=0, cdat[ncomb];
	    
			// Initialise temporary file to zero.
			temp[nc] = 0.;
	    
			// Now wind through the windows
			for(size_t nw=0; nw<out[nc].size(); nw++){
			    ntot += temp[nc][nw].ntot();
			    for(int ny=0; ny<temp[nc][nw].ny(); ny++){
				for(int nx=0; nx<temp[nc][nw].nx(); nx++){
		  
				    // Extract and normalise data from the files.
				    for(int nf=0; nf<ncomb; nf++)
					cdat[nf] = fptr[nf]->get_next()/aver[nf]*norm;
		  
				    // Process the data
				    if(method == 'M'){
					if(nfile % 2 == 0){
					    comb = (Subs::select(cdat,ncomb,ncomb/2-1) + Subs::select(cdat,ncomb,ncomb/2) ) / 2.;
					}else{
					    comb = Subs::select(cdat,ncomb,ncomb/2);
					}
				    }else if(method == 'C'){     	  
					double rawmean, rawrms, mean, rms;
					int nrej;
					Subs::sigma_reject(cdat,ncomb,sigma,careful,rawmean,rawrms,mean,rms,nrej);
					nrejtot += nrej;
					comb = mean;
				    }
		  
				    // Store
				    temp[nc][nw][ny][nx] = comb;
		  
				    // Progress indicator 
				    ndtot += ncomb;
				    if((ndadd = (int(MXDOT*ndtot/nptot) - ndot))){
					for(size_t ia=0; ia<ndadd; ia++) std::cout << "." << std::flush;
					ndot += ndadd;
				    }
				}
			    }
			}
	  

			// Add the processed frame in.
			out[nc] += temp[nc];

			// delete Fdisk pointers
			for(int nf=0; nf<ncomb; nf++)
			    delete fptr[nf];

		    }

		}else{
		    out[nc] = 1;
		}
	    }
           
	    if(method == 'C'){
		float percent = 100.*nrejtot/nptot;
		percent = int(100.*percent+0.5)/100.;
		std::cout << "\n" << nrejtot << " pixels rejected = "  << percent << "% of the total. " << std::endl;
	    }
	}

	for(size_t nc=0; nc<out.size(); nc++){
	    // Normalise the frame
	    stats = out[nc].statistics(region[nc],100.,false,false);
	    out[nc] /= stats.raw_mean;
	}

	// Output data
	out.write(output);
      
	std::cout << "\nFinished.\n" << std::endl;

    }

    catch(const Ultracam_Error& err){
	std::cerr << "\nUltracam::Ultracam_Error exception:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const Subs::Subs_Error& err){
	std::cerr << "\nSubs::Subs_Error exception:" << std::endl;
	std::cerr << err << std::endl;
    }
    catch(const std::string& err){
	std::cerr << "\n" << err << std::endl;
    }
}


