/****************************************************************************
 * splatpainter.h
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

// Avoid double definitions
#ifndef INCL_SPLATPAINTER
#define INCL_SPLATPAINTER

/* Splats are sampled from a kernel function; the total area integral of
 * that function should be 1.0.  When there are too few samples, however,
 * the sum does not approximate the integral very well.  This constant
 * is the number of pixels in a splat at which we will renormalize the
 * integral.  It costs some time and space, so we don't always do it.
 */
#define SPLAT_RENORM_CUTOFF 25

class SplatPainter {
 public:
  SplatPainter(StarSplatter* owner_in);
  virtual ~SplatPainter();
  virtual const char* typeName() const;
  virtual void paint( const StarSplatter::Splat* splat,
		      const double sep_fac,
		      gColor* tmp_image, 
		      double& krnl_integral, int& pixels_touched );
  virtual StarSplatter::SplatType getSplatType() const;
 protected:
  StarSplatter* owner;
  void small_splat( const StarSplatter::Splat* splat,
		    const double splat_limit,
		    const double sep_fac,
		    const double energy_scale,
		    gColor scaled_clr,
		    const int imin, const int imax,
		    const int jmin, const int jmax,
		    gColor* tmp_image,
		    double& krnl_integral, int& pixels_touched ) const;
};

#endif // INCL_SPLATPAINTER
