#include <vector>
#include "trm/frame.h"
#include "trm/ultracam.h"

/**
 * shift_and_add is a program to shift a frame in x and y, multiply it by a constant and then add it on to another frame. This is
 * faster than a shift routine, followed by a multiply routine, followed by an add routine as it avoids temporaries.
 * \param sum          the frame to add to
 * \param extra        the frame to be shifted and added
 * \param shift        the vector of x &y shifts and whether a CCDshould be addedin at all.
 * \param multiplier   the constant to multiply the frame by before adding it to the sum.
 * \param shift_method the shifting interpolation method.
 */

void Ultracam::shift_and_add(Frame& sum, const Frame& extra, const std::vector<Shift_info>& shift,
                 internal_data multiplier, SHIFT_METHOD shift_method){

  if(sum != extra)
    throw Ultracam_Error("void Ultracam::shift_and_add(Frame&, const Frame&, float, float, Ultracam::internal_data, Ultracam::Shift_method):"
             " two input frames do not have matching formats");

  if(shift.size() != sum.size())
    throw Ultracam_Error("void Ultracam::shift_and_add(Frame&, const Frame&, const std::vector<Shift_info>&, internal_data, Shift_method):"
             " shift std::vector does not match the number of CCDs");

  int ix, iy;
  if(shift_method == Ultracam::NEAREST_PIXEL){

    for(size_t nccd=0; nccd<sum.size(); nccd++){
      if(shift[nccd].ok){
    for(size_t nwin=0; nwin<sum[nccd].size(); nwin++){

      // Time savers
      int nx = sum[nccd][nwin].nx(), ny = sum[nccd][nwin].ny();
      Windata&       wsum   = sum[nccd][nwin];
      const Windata& wextra = extra[nccd][nwin];

      // Shift to the nearest binned pixel
      int dxi = int( floor( shift[nccd].dx / wsum.xbin() + 0.5) );
      int dyi = int( floor( shift[nccd].dy / wsum.ybin() + 0.5) );

      if(dxi == 0 && dyi == 0){

        // Short cut
        for(iy=0; iy<ny; iy++){
          for(ix=0; ix<nx; ix++){
        wsum[iy][ix] += multiplier*wextra[iy][ix];
          }
        }

      }else{

        // A little bit of work. Compute pixel coordinates in 'extra' equivalent to
        // those in 'sum'. If they are off the edge truncate at a valid value (which has
        // the effect of extending the edge value). Finally add in to sum.
        int newiy, newix;
        for(iy=0; iy<ny; iy++){
          newiy = (iy > ny - 1 + dyi) ? (ny - 1) : (iy > dyi ? iy - dyi : 0);
          for(ix=0; ix<nx; ix++){
        newix = (ix > nx - 1 + dxi) ? (nx - 1) : (ix > dxi ? ix - dxi : 0);
        wsum[iy][ix] += multiplier*wextra[newiy][newix];
          }
        }
      }
    }
      }
    }

  }else if(shift_method == Ultracam::LINEAR_INTERPOLATION){

    for(size_t nccd=0; nccd<sum.size(); nccd++){
      if(shift[nccd].ok){
    for(size_t nwin=0; nwin<sum[nccd].size(); nwin++){

      // Time savers
      int nx = sum[nccd][nwin].nx(), ny = sum[nccd][nwin].ny();
      Windata&       wsum   = sum[nccd][nwin];
      const Windata& wextra = extra[nccd][nwin];

      // Compute integer part of shift
      int dxi = int( floor( shift[nccd].dx / wsum.xbin()) );
      int dyi = int( floor( shift[nccd].dy / wsum.ybin()) );

      // Remaining part of shift, numbers between 0 and 1
      float dxr = shift[nccd].dx/wsum.xbin() - float(dxi);
      float dyr = shift[nccd].dy/wsum.ybin() - float(dyi);

      int newiy, newix;
      for(iy=0; iy<ny; iy++){
        newiy = (iy > ny + dyi - 1) ? (ny - 1) : (iy > dyi ? iy - dyi : 0);
        for(ix=0; ix<nx; ix++){
          newix = (ix > nx + dxi - 1) ? (nx - 1) : (ix > dxi ? ix - dxi : 0);

          // newiy, newix refers to pixel located at top-right corner of the 4 pixels surrounding
          // the point we want to interpolate to which is located (dxr,dyr) left/down from this pixel.

          // Linearly interpolate, taking care if we are near bottom or left-hand edges or corner
          // The first case should be the norm
          if(newix > 0 && newiy > 0){

        // Away from any edge
        wsum[iy][ix] += multiplier*(dxr*dyr*wextra[newiy-1][newix-1] + (1-dxr)*dyr*wextra[newiy-1][newix] +
                        dxr*(1-dyr)*wextra[newiy][newix-1] +  (1-dxr)*(1-dyr)*wextra[newiy][newix]);
          }else if(newix > 0){

        // On bottom edge
        wsum[iy][ix] += multiplier*(dxr*wextra[newiy][newix-1] + (1-dxr)*wextra[newiy][newix]);

          }else if(newiy > 0){

        // On left edge
        wsum[iy][ix] += multiplier*(dyr*wextra[newiy-1][newix] + (1-dyr)*wextra[newiy][newix]);

          }else{

        // In bottom-left corner
        wsum[iy][ix] += multiplier*wextra[newiy][newix];
          }
        }
      }
    }
      }
    }
  }else{
    throw Ultracam_Error("void Ultracam::shift_and_add(Frame&, const Frame&, float, float, Ultracam::internal_data, Ultracam::Shift_method):"
             " shift method not recognised");
  }
}


