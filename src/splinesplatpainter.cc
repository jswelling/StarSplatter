/****************************************************************************
 * splinesplatpainter.cc
 * Author Joel Welling
 * Copyright 2008, Pittsburgh Supercomputing Center, Carnegie Mellon University
 *
 * Permission use, copy, and modify this software and its documentation
 * without fee for personal use or use within your organization is hereby
 * granted, provided that the above copyright notice is preserved in all
 * copies and that that copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to
 * other organizations or individuals is not granted;  that must be
 * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
 * University make any representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "starsplatter.h"
#include "splinesplatpainter.h"

/* Notes-
 */

////////////////
//
// The following lookup table aproximates the projexted spline SPH
// kernel of rho^2, where rho^2= (x/h)^2 + (y/h)^2.  This is the 
// 'Gadget' code's version of the spline kernel, where the compact 
// support of the kernel extends out to rho=1.
// 
// From test_lut.cc :
//   for fun(v)= projected spline kernel of sqrt(v) 
//   min max defsqr at k= 0.496178, ends 0.908127 0.000220545 
//   for LUT_SIZE 32
//   (4000 sample points)
//   k= -0.156488, ends -1.99586 7.74214e-07 minimizes the ratio
//   of diff^2/funval^2, with the only significant point being the
//   rightmost non-zero point.  The effectiveness of these settings
//   on that ratio is dominated by the right endpoint, with some
//   contrib from k.  A right end value of 0.0 works almost as well.
//
////////////////
#define LUT_SIZE 32
#define LUT_ENTRIES (LUT_SIZE-2)
#define LUT_OFFSET 1
#define K_PROJ_SPLINE 0.496178
static float tbl_proj_spline[32]= {
  0.908127,
  0.749999,
  0.621011,
  0.513289,
  0.422364,
  0.345726,
  0.281486,
  0.228083,
  0.184101,
  0.148131,
  0.118702,
  0.0945984,
  0.0748692,
  0.0587595,
  0.0456573,
  0.0350603,
  0.0265513,
  0.0197807,
  0.0144537,
  0.0103199,
  0.00716569,
  0.00480871,
  0.00309275,
  0.00188416,
  0.00106886,
  0.000549853,
  0.000245384,
  8.74274e-05,
  2.06396e-05,
  1.78129e-06,
  0.0,
  0.0,
};

static inline float table_lookup(const float x_in, 
				 const float xmin, 
				 const float invXMaxMinusMin, 
				 const float k,
				 const float tbl[LUT_SIZE])
{
  float indexMax= (float)(LUT_ENTRIES-1);
  float x= ((float)indexMax)*(x_in-xmin)*invXMaxMinusMin;
  x= (x<0.0)?0.0:((x>indexMax)?indexMax:x);
  float xFloor= floor(x);
  if (xFloor==indexMax) return tbl[LUT_OFFSET+LUT_ENTRIES-1];
  else {
    int iFloor= (int)xFloor + LUT_OFFSET;
    int iCeil= iFloor+1;
    float u= x-xFloor;
    //float k = 0.5; // 'spring constant'
    //////////////////
    // linear interpolation
    // return xDelta*tbl[iCeil]+(1.0-xDelta)*tbl[iFloor];
    //////////////////
    // Catmul-Rom interpolation
    // Extra point exists at each end for padding
    // Order of points is iPrev, iFloor, iCeil, iNext
    int iPrev= iFloor-1;
    int iNext= iCeil+1;
    float A= ((-k*u+2.0*k)*u - k)*u+0.0;
    float B= (((2.0-k)*u+(k-3.0))*u*u)+1.0;
    float C= (((k-2.0)*u+(3.0-2.0*k))*u + k)*u;
    float D= k*(u-1.0)*u*u;
    return (A*tbl[iPrev] + B*tbl[iFloor] + C*tbl[iCeil] + D*tbl[iNext]);
  }
}

static inline double projectedSplineKernel(double hInv, double x, double y)
{
  return (double)hInv*hInv*(8.0/M_PI)*
    table_lookup(hInv*hInv*(x*x+y*y), 0.0, 1.0,
		 K_PROJ_SPLINE, tbl_proj_spline);
}
SplineSplatPainter::SplineSplatPainter(StarSplatter* owner_in)
  : SplatPainter(owner_in)
{
  // Nothing to initialize
}

SplineSplatPainter::~SplineSplatPainter()
{
  // Nothing to clean up;
}

const char* SplineSplatPainter::typeName() const
{
  return "projected spline";
}

StarSplatter::SplatType SplineSplatPainter::getSplatType() const
{
  return StarSplatter::SPLAT_SPLINE;
}

void SplineSplatPainter::paint( const StarSplatter::Splat* splat,
				  const double sep_fac,
				  gColor* tmp_image, 
				  double& krnl_integral, int& pixels_touched )
{
  int xsize= owner->image_xsize();
  int ysize= owner->image_ysize();
  int debug_flag= owner->debug();
  gColor scaled_clr= splat->clr;
  scaled_clr.mult_noclamp( splat->density );

  gColor tmp_clr;

  double splat_limit= 1.0/(splat->sqrt_exp_constant*sep_fac);

  // fprintf(stderr,"spline splat: splat_limit= %f\n",splat_limit);

  int imin= (int)(splat->loc.x()-splat_limit+1.0); // ceil
  int imax= (int)(splat->loc.x()+splat_limit); // floor
  int jmin= (int)(splat->loc.y()-splat_limit+1.0); // ceil
  int jmax= (int)(splat->loc.y()+splat_limit); // floor

  if (imin<0) imin= 0;
  if (imax>=xsize) imax= xsize-1;
  if (jmin<0) jmin= 0;
  if (jmax>=ysize) jmax= ysize-1;    

  int maxNSplats= ((imax-imin)+1)*((jmax-jmin)+1);
  if (maxNSplats > (double)SPLAT_RENORM_CUTOFF) { 
    gColor* pixrunner;
    double y= sep_fac*((double)jmin - splat->loc.y());
    for (int j=jmin; j<=jmax; j++) {
      pixrunner= tmp_image + (j*xsize) + imin;
      double x= sep_fac*((double)imin - splat->loc.x());
      for (int i=imin; i<=imax; i++) {
	// Calc and add color here
	double kval= projectedSplineKernel(splat->sqrt_exp_constant,x,y);
	tmp_clr= scaled_clr;
	tmp_clr.mult_noclamp(kval);
	tmp_clr.clamp_alpha();
	tmp_clr.add_under( *pixrunner );
	*pixrunner++= tmp_clr;
	if (debug_flag) {
	  pixels_touched++;
	  krnl_integral += kval;
	}
	x += sep_fac;
      }
      y += sep_fac;
    }
  }
  else {
    if (splat_limit > 1.0) {
      /* Here we need to renormalize */
      double samples[SPLAT_RENORM_CUTOFF];
      double* here= samples;
      double sum= 0.0;
      double y= sep_fac*((double)jmin - splat->loc.y());
      for (int j=jmin; j<=jmax; j++) {
	double x= sep_fac*((double)imin - splat->loc.x());
	for (int i=imin; i<=imax; i++) {
	  double kval= projectedSplineKernel(splat->sqrt_exp_constant,x,y);
	  assert(here-samples < SPLAT_RENORM_CUTOFF);
	  *here++= kval;
	  sum += kval;
	  x += sep_fac;
	}
	y += sep_fac;
      }
      gColor* pixrunner;
      double invSum= 1.0/(sum*sep_fac*sep_fac); // sum *should* = 1.0/(sep_fac^2)
      //fprintf(stderr,"invSum= %g\n",invSum);
      krnl_integral *= invSum;
      here= samples;
      for (int j=jmin; j<=jmax; j++) {
	pixrunner= tmp_image + (j*xsize) + imin;
	for (int i=imin; i<=imax; i++) {
	  double kval= invSum*(*here++);
	  if (debug_flag) {
	    pixels_touched++;
	    krnl_integral += kval;
	  }
	  tmp_clr= scaled_clr;
	  tmp_clr.mult_noclamp(kval);
	  tmp_clr.clamp_alpha();
	  tmp_clr.add_under( *pixrunner );
	  *pixrunner++= tmp_clr;
	}
      }
    }
    else {
      small_splat( splat, splat_limit, sep_fac, 1.0/(sep_fac*sep_fac),
		   scaled_clr,
		   imin, imax, jmin, jmax, tmp_image,
		   krnl_integral, pixels_touched );
    }
  }

}

