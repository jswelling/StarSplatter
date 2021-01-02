/****************************************************************************
 * splatpainter.cc
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
#include "splatpainter.h"

static char rcsid[] = "$Id: splatpainter.cc,v 1.2 2008-07-25 21:08:23 welling Exp $";

/* Notes-
 */

SplatPainter::SplatPainter(StarSplatter* owner_in)
{
  owner= owner_in;
}

SplatPainter::~SplatPainter()
{
  // Nothing to clean up;
}

const char* SplatPainter::typeName() const
{
  return "Base class";
}

StarSplatter::SplatType SplatPainter::getSplatType() const
{
  fprintf(stderr,
	  "Internal error: Base SplatPainter::getSplatType() called!\n");
  exit(-1);
  return (StarSplatter::SplatType)0; // to satisfy compiler
}

void SplatPainter::paint( const StarSplatter::Splat* splat,
			  const double sep_fac,
			  gColor* tmp_image,
			  double& krnl_integral, int& pixels_touched )
{
  fprintf(stderr,"Internal error: Base SplatPainter::paint() called!\n");
  exit(-1);
}

void SplatPainter::small_splat( const StarSplatter::Splat* splat,
				const double splat_limit,
				const double sep_fac,
				const double energy_scale,
				gColor scaled_clr,
				const int imin, const int imax,
				const int jmin, const int jmax,
				gColor* tmp_image, 
				double& krnl_integral, 
				int& pixels_touched ) const
{
  gColor* pixrunner;
  gColor tmp_clr;
  int xsize= owner->image_xsize();
  int ysize= owner->image_ysize();
  int debug_flag= owner->debug();

  if (splat_limit>0.5) {
    // fprintf(stderr,"Strange splat; %d %d to %d %d ",imin,jmin,imax,jmax);
    if (imin==imax) {
      if (jmin==jmax) {
        // degenerate splat, happens to fall on 1 pixel
        if ((imin>=0)&&(imin<xsize)&&(jmin>=0)&&(jmin<ysize)) {
          tmp_clr= scaled_clr;
          tmp_clr.mult_noclamp(energy_scale);
          tmp_clr.clamp_alpha();
          pixrunner= tmp_image + (jmin*xsize) + imin;
          tmp_clr.add_under( *pixrunner );
          *pixrunner= tmp_clr;
          if (debug_flag) {
            krnl_integral += energy_scale;
            pixels_touched++;
          }
        }
      }
      else {
        // 1 pixel in x direction, 2 in y
        if ((imin>=0)&&(imin<xsize)) {
          double y_offset= splat->loc.y()-(double)jmin;
          pixrunner= tmp_image + (jmin*xsize) + imin;
          if ((jmin>=0)&&(jmin<ysize)) {
            tmp_clr= scaled_clr;
            tmp_clr.mult_noclamp( energy_scale*(1.0-y_offset) );
            tmp_clr.clamp_alpha();
            tmp_clr.add_under( *pixrunner );
            *pixrunner= tmp_clr;
            if (debug_flag) {
              krnl_integral += energy_scale*(1.0-y_offset);
              pixels_touched++;
            }
          }
          if ((jmax>=0)&&(jmax<ysize)) {
            pixrunner += xsize;
            tmp_clr= scaled_clr;
            tmp_clr.mult_noclamp( energy_scale*y_offset );
            tmp_clr.clamp_alpha();
            tmp_clr.add_under( *pixrunner );
            *pixrunner= tmp_clr;
            if (debug_flag) {
              krnl_integral += energy_scale*y_offset;
              pixels_touched++;
            }
          }
        }
      }
    }
    else {
      double x_offset= splat->loc.x()-(double)imin;
      if (jmin==jmax) {
        // 2 pixels in x direction, 1 in y
        if ((jmin>=0)&&(jmin<ysize)) {
          pixrunner= tmp_image + (jmin*xsize) + imin;
          if ((imin>=0)&&(imin<xsize)) {
            tmp_clr= scaled_clr;
            tmp_clr.mult_noclamp( energy_scale*(1.0-x_offset) );
            tmp_clr.clamp_alpha();
            tmp_clr.add_under( *pixrunner );
            *pixrunner= tmp_clr;
            if (debug_flag) {
              krnl_integral += energy_scale*(1.0-x_offset);
              pixels_touched++;
            }
          }
          if ((imax>=0)&&(imax<xsize)) {
            pixrunner++;
            tmp_clr= scaled_clr;
            tmp_clr.mult_noclamp( energy_scale*x_offset );
            tmp_clr.clamp_alpha();
            tmp_clr.add_under( *pixrunner );
            *pixrunner= tmp_clr;
            if (debug_flag) {
              krnl_integral += energy_scale*x_offset;
              pixels_touched++;
            }
          }
        }
      }
      else {
        // Hit all 4 pixels
        double y_offset= splat->loc.y()-(double)jmin;
        pixrunner= tmp_image + (jmin*xsize) + imin;
        if ((jmin>=0)&&(jmin<ysize)) {
          if ((imin>=0)&&(imin<xsize)) {
            tmp_clr= scaled_clr;
            tmp_clr.mult_noclamp( energy_scale
				  *(1.0-x_offset)*(1.0-y_offset) );
            tmp_clr.clamp_alpha();
            tmp_clr.add_under( *pixrunner );
            *pixrunner= tmp_clr;
            if (debug_flag) {
              krnl_integral += energy_scale*(1.0-x_offset)*(1.0-y_offset);
              pixels_touched++;
            }
          }
          if ((imax>=0)&&(imax<xsize)) {
            pixrunner++;
            tmp_clr= scaled_clr;
            tmp_clr.mult_noclamp( energy_scale
				  *x_offset*(1.0-y_offset) );
            tmp_clr.clamp_alpha();
            tmp_clr.add_under( *pixrunner );
            *pixrunner= tmp_clr;
            if (debug_flag) {
              krnl_integral += energy_scale*x_offset*(1.0-y_offset);
              pixels_touched++;
            }
          }
        }
        if ((jmax>=0)&&(jmax<ysize)) {
          pixrunner = tmp_image + (jmax*xsize) +imin;
          if ((imin>=0)&&(imin<xsize)) {
            tmp_clr= scaled_clr;
            tmp_clr.mult_noclamp( energy_scale
				  *(1.0-x_offset)*y_offset );
            tmp_clr.clamp_alpha();
            tmp_clr.add_under( *pixrunner );
            *pixrunner= tmp_clr;
            if (debug_flag) {
              krnl_integral += energy_scale*(1.0-x_offset)*y_offset;
              pixels_touched++;
            }
          }
          if ((imax>=0)&&(imax<xsize)) {
            pixrunner++;
            tmp_clr= scaled_clr;
            tmp_clr.mult_noclamp( energy_scale
				  *x_offset*y_offset );
            tmp_clr.clamp_alpha();
            tmp_clr.add_under( *pixrunner );
            *pixrunner= tmp_clr;
            if (debug_flag) {
              krnl_integral += energy_scale*x_offset*y_offset;
              pixels_touched++;
            }
          }
        }
      }
    }
  }
  else {
    // degenerate splat, fits in one pixel
    // fprintf(stderr,"Degenerate splat: ");
    tmp_clr= scaled_clr;
    tmp_clr.mult_noclamp(energy_scale);
    tmp_clr.clamp_alpha();
    pixrunner= tmp_image + (jmin*xsize) + imin;
    tmp_clr.add_under( *pixrunner );
    *pixrunner= tmp_clr;
    if (debug_flag) {
      krnl_integral += energy_scale;
      pixels_touched++;
    }
  }
  
}
