/*

!!begin

!!title   combines a set of frames
!!author  T.R. Marsh
!!created 15 Feb 2002
!!revised 26 May 2010
!!descr   combines a set of frames using median or clipped mean
!!css   style.css
!!root    combine
!!index   combine
!!class   Programs
!!class   Arithematic
!!head1   combine - combines a set of frames using median or clipped mean.

!!emph{combine} coadds a set of frames by taking the median or the clipped mean
at each pixel. Optionally the frames can be divided through by their mean values before
they are combined or have a bias added to keep their means constant. Use this routine 
for getting mean bias and flat field frames.

<strong>NB</strong> Be warned that a common problem to encounter is to have used !!ref{grab.html}{grab} to get the
frames in the first place, but then to ctrl-C it and end up with the final file corrupted.
You need to take care not to include it in the file list you supply to !!emph{combine}.

There is a limit to the number of frames that this routine can handle set by the compiler because all files
have to be opened simultaneously. This is of order 1000 to 4000 files depending on the system. The program will
collapse if you exceed this limit.

!!head2 Invocation

combine list method (sigma careful) adjust output

!!head2 Arguments

!!table
!!arg{list}{List of file names}
!!arg{method}{'m' = median, 'c' = clipped mean. The clipped mean rejects one pixel at a time
and then recomputes the mean and rms again before having another go.}
!!arg{sigma}{If method = 'c', this is the number of sigmas for rejection}
!!arg{careful}{If method = 'c', this controls the mehod of rejection. true means pixels are rejected
one at a time, worst first. false means pixels are rejected in groups which can potentially lead to
the odd false rejection.}
!!arg{adjust}{Whether to adjust the frames before combining. 'i' =ignore, 'n' = normalise
the mean level to match the first frame, 'b' = add a bias to give the same mean as the first frame.
The adjustment is done on an individual CCD basis. 'b' is useful for combining biases where there
may be an overall drift from frame to frame. 'n' is for combining (bias subtracted) sky flats
for example where the level varies substantially but the shape is fixed.}
!!arg{output}{Output frame}
!!table

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

int main(int argc, char* argv[]){

    using Ultracam::Ultracam_Error;
    using Ultracam::internal_data;
    using Ultracam::raw_data;

    try{

	// Construct Input object

	Subs::Input input(argc, argv, Ultracam::ULTRACAM_ENV, Ultracam::ULTRACAM_DIR);

	// sign-in input variables

	input.sign_in("list",      Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("method",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("sigma",     Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("careful",   Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("adjust",    Subs::Input::LOCAL,  Subs::Input::PROMPT);
	input.sign_in("output",    Subs::Input::LOCAL,  Subs::Input::PROMPT);

	// Get inputs

	std::string stlist;
	input.get_value("list", stlist, "list", "list of frames to combine");
	char method;
	input.get_value("method", method, 'c', "cCmM", "what combination method?");
	method = toupper(method);
	float sigma;
	bool careful;
	if(method == 'C'){
	    input.get_value("sigma",   sigma, 3.f, 1.f, FLT_MAX, "threshold multiple of RMS to reject");
	    input.get_value("careful", careful, true, "reject pixels one at a time?");
	}
	char adjust;
	input.get_value("adjust", adjust, 'i', "iInNbB", 
			"i(gnore), n(ormalise), b(ias)");
	adjust = toupper(adjust);
	std::string output;
	input.get_value("output", output, "output", "output file");

	// Read file list

	std::vector<std::string> flist;
	std::string name;

	std::ifstream istr(stlist.c_str());
	while(istr >> name){
	    flist.push_back(name);
	}
	istr.close();

	size_t nfile = flist.size();
	if(nfile == 0) 
	    throw Ultracam::Input_Error("No file names loaded");

	// Read first frame in straight off to provide format
	// information and to set up the final output frame.
 
	Ultracam::Frame out(flist[0]);

	Subs::Header::Hnode *hnode;

	// Ensure that the frame read has a good blue image

	size_t nread = 1;
	bool blue_is_bad;
	hnode = out.find("Frame.bad_blue");
	blue_is_bad = hnode->has_data() ? hnode->value->get_bool() : false;
	while(blue_is_bad && nread < nfile){
	    out.read(flist[nread]);
	    nread++;
	    hnode = out.find("Frame.bad_blue");
	    blue_is_bad = hnode->has_data() ? hnode->value->get_bool() : false;
	}
	if(blue_is_bad)
	    throw Ultracam_Error("Failed to find a frame with a valid blue image");
	nread--;

	if(nfile > 1){

	    // compute and store means of each CCD if needed.
	    // requires reading every frame in. The means are
	    // normalised to the values of the first frame.

	    float mean[nfile][out.size()];
	    bool blue_bad[nfile];
	    Ultracam::Frame temp;

	    if(adjust != 'I'){

		std::cout << "Computing means of each CCD." << std::endl;

		// Mean of first OK frame
		double sum;
		size_t norm;
		for(size_t nc=0; nc<out.size(); nc++){
		    sum  = 0.;
		    norm = 0;
		    for(size_t nw=0; nw<out[nc].size(); nw++){
			sum  += out[nc][nw].sum();
			norm += out[nc][nw].ntot();
		    }
		    if(norm){
			mean[nread][nc] = sum/norm;
		    }else if(adjust == 'N'){
			mean[nread][nc] = 1;
		    }else if(adjust == 'B'){
			mean[nread][nc] = 0;
		    }
		}
	  
		for(size_t nf=0; nf<nfile; nf++){
		    temp.read(flist[nf]);
		    if(temp != out) 
			throw Ultracam_Error(flist[nf] + std::string(" incompatible with ") + flist[0]);

		    hnode = temp.find("Frame.bad_blue");
		    blue_bad[nf] = hnode->has_data() ? hnode->value->get_bool() : false;

		    if(nf != nread){
			
			for(size_t nc=0; nc<out.size(); nc++){
		  
			    sum  = 0.;
			    norm = 0;
			    
			    for(size_t nw=0; nw<out[nc].size(); nw++){
				sum  += temp[nc][nw].sum();
				norm += temp[nc][nw].ntot();
			    }
			    
			    if(norm){ 
				if(adjust == 'N'){
				    mean[nf][nc] = sum/norm/mean[nread][nc];
				}else if(adjust == 'B'){
				    mean[nf][nc] = sum/norm - mean[nread][nc];
				}
			    }else if(adjust == 'N'){
				mean[nf][nc] = 1;
			    }else if(adjust == 'B'){
				mean[nf][nc] = 0;
			    }
			}
		    }
		}
		    
		for(size_t nc=0; nc<out.size(); nc++)
		    mean[nread][nc] = 1.;
	  
		std::cout << "Now combining frames." << std::endl;
	  
	    }else{

		// Need to read the bad_blue status in this case.
		for(size_t nf=0; nf<nfile; nf++){
		    temp.read(flist[nf]);
		    if(temp != out) 
			throw Ultracam_Error(flist[nf] + std::string(" incompatible with ") + flist[0]);

		    hnode = temp.find("Frame.bad_blue");
		    blue_bad[nf] = hnode->has_data() ? hnode->value->get_bool() : false;
		}
	    }
      
	    // progress indicator
	    size_t nptot = 0, ndtot = 0;
	    const size_t MXDOT = 20;
	    size_t ndot = 0, ndadd;
	    for(size_t nc=0; nc<out.size(); nc++)
		for(size_t nw=0; nw<out[nc].size(); nw++)
		    nptot += out[nc][nw].ntot();
      
	    for(size_t ia=0; ia<MXDOT; ia++) std::cout << ".";
	    std::cout << std::endl;

	    // Fdisk
	    Ultracam::Fdisk* file[nfile];

	    const size_t MXBUFF = 8000000; // total buffer size.
	    const size_t NBUFF  = MXBUFF/nfile; // individual buffer size.

	    // Create the Fdisks
	    for(size_t nf=0; nf<nfile; nf++)
		file[nf] = new Ultracam::Fdisk(flist[nf],NBUFF);

	    // Buffer for combining data
	    Ultracam::internal_data comb=0, cdat[nfile];

	    size_t nrejtot = 0, ntot=0;
	    // Now wind through CCDs and windows
	    for(size_t nc=0; nc<out.size(); nc++){
		for(size_t nw=0; nw<out[nc].size(); nw++){
		    ntot += out[nc][nw].ntot();
		    for(int ny=0; ny<out[nc][nw].ny(); ny++){
			for(int nx=0; nx<out[nc][nw].nx(); nx++){
	    
			    // Extract data from the files. Note we have to extract bad blue
			    // data even though it is not used
			    size_t nok=0;
			    if(adjust == 'N'){
				for(size_t nf=0; nf<nfile; nf++)
				    if(nc != 2 || !blue_bad[nf])
					cdat[nok++] = file[nf]->get_next()/mean[nf][nc];
				    else
					file[nf]->get_next();
			    }else if(adjust == 'B'){
				for(size_t nf=0; nf<nfile; nf++)
				    if(nc != 2 || !blue_bad[nf])
					cdat[nok++] = file[nf]->get_next() - mean[nf][nc];
				    else
					file[nf]->get_next();
			    }else{
				for(size_t nf=0; nf<nfile; nf++)
				    if(nc != 2 || !blue_bad[nf])
					cdat[nok++] = file[nf]->get_next();
				    else
					file[nf]->get_next();
			    }

			    // Process the data
			    if(method == 'M'){
		
				if(nok % 2 == 0){
				    Ultracam::internal_data m1 = Subs::select(cdat,nok,nok/2-1);
				    Ultracam::internal_data m2 = Subs::select(cdat,nok,nok/2);
				    comb = (m1+m2)/2.;
				}else{
				    comb = Subs::select(cdat,nok,nok/2);
				}

			    }else if(method == 'C'){
		
				double rawmean, rawrms, mean, rms;
				int nrej;
				Subs::sigma_reject(cdat,nok,sigma,careful,rawmean,rawrms,mean,rms,nrej);
				nrejtot += nrej;
				comb = mean;
			    }
	      
			    // Store
			    out[nc][nw][ny][nx] = comb;
	  
			    // Progress indicator 
			    ndtot++;
			    if((ndadd = ((MXDOT*ndtot)/nptot - ndot))){
				for(size_t ia=0; ia<ndadd; ia++) std::cout << "." << std::flush;
				ndot += ndadd;
			    }
			}
		    }
		}
	    }
      
	    // clear up
	    for(size_t i=0; i<nfile; i++) delete file[i];
      
	    float percent = 100.*nrejtot/float(ntot)/nfile;
	    percent = int(100.*percent+0.5)/100.;
	    if(method == 'C')
		std::cout << "\n" << nrejtot << " pixels rejected = "  << percent << "% of the total. " << std::endl;
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


