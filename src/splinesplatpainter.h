/****************************************************************************
 * splinesplatpainter.h
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

#include "splatpainter.h"

class SplineSplatPainter : public SplatPainter {
 public:
  SplineSplatPainter(StarSplatter* owner_in);
  virtual ~SplineSplatPainter();
  virtual const char* typeName() const;
  virtual StarSplatter::SplatType getSplatType() const;
  virtual void paint( const StarSplatter::Splat* splat,
		      const double sep_fac,
		      gColor* tmp_image,
		      double& krnl_integral, int& pixels_touched );
};
