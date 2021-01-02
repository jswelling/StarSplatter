/****************************************************************************
 * gaussiansplatpainter.cc
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
#include "gaussiansplatpainter.h"

static char rcsid[] = "$Id: gaussiansplatpainter.cc,v 1.2 2008-07-25 21:08:23 welling Exp $";

/* Notes-
 */

#define InvPi (1.0/M_PI)
#define InvPiThreeHalves (1.0/(sqrt(M_PI)*sqrt(M_PI)*sqrt(M_PI)))

GaussianSplatPainter::GaussianSplatPainter(StarSplatter* owner_in)
  : SplatPainter(owner_in)
{
  // Nothing to initialize
}

GaussianSplatPainter::~GaussianSplatPainter()
{
  // Nothing to clean up;
}

const char* GaussianSplatPainter::typeName() const
{
  return "Gaussian";
}

StarSplatter::SplatType GaussianSplatPainter::getSplatType() const
{
  return StarSplatter::SPLAT_GAUSSIAN;
}

static inline double kernelGaussian(double hInv, double x, double y) 
{
  return hInv*hInv*InvPi*exp(-hInv*hInv*(x*x+y*y));
}

static inline double cutoffGaussian(double cutoffScale, 
				    double hInv, double sep_fac)
{
  return sqrt( -log(cutoffScale) )/ (hInv*sep_fac);
}

void GaussianSplatPainter::paint( const StarSplatter::Splat* splat,
				  const double sep_fac,
				  gColor* tmp_image, 
				  double& krnl_integral, int& pixels_touched )
{
  int xsize= owner->image_xsize();
  int ysize= owner->image_ysize();
  int debug_flag= owner->debug();
  double splat_cutoff= owner->splat_cutoff_frac();
  gColor scaled_clr= splat->clr;
  scaled_clr.mult_noclamp( splat->density );

  gColor tmp_clr;

  double splat_limit= cutoffGaussian(splat_cutoff,
				     splat->sqrt_exp_constant,
				     sep_fac);

  // fprintf(stderr,"gaussian splat: splat_limit= %f\n",splat_limit);

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
	double kval= kernelGaussian(splat->sqrt_exp_constant,x,y);
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
	  double kval= kernelGaussian(splat->sqrt_exp_constant,x,y);
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

