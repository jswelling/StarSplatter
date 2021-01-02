/****************************************************************************
 * circlesplatpainter.cc
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
#include "circlesplatpainter.h"

static char rcsid[] = "$Id: circlesplatpainter.cc,v 1.2 2008-07-25 21:08:22 welling Exp $";

/* Notes-
 */

CircleSplatPainter::CircleSplatPainter(StarSplatter* owner_in)
  : SplatPainter(owner_in)
{
  // Nothing to initialize
}

CircleSplatPainter::~CircleSplatPainter()
{
  // Nothing to clean up;
}

const char* CircleSplatPainter::typeName() const
{
  return "CircleGlyph";
}

StarSplatter::SplatType CircleSplatPainter::getSplatType() const
{
  return StarSplatter::SPLAT_GLYPH_CIRCLE;
}

void CircleSplatPainter::circlept(double xx,double yy,
				  const double sep_fac, 
				  const StarSplatter::Splat* s,
				  gColor* tmp_image,
				  double& krnl_integral,
				  int& pixels_touched) const {
  int xsize= owner->image_xsize();
  int ysize= owner->image_ysize();
  double xadj= s->loc.x()+(xx);
  double yadj= s->loc.y()+(yy);
  double csplat_limit= 0.5; // half-width of the point
  StarSplatter::Splat tmpS= *s;
  tmpS.loc= gPoint(xadj,yadj,s->loc.z(),s->loc.w());
  int cimin= (int)(xadj-csplat_limit+1.0);
  int cimax= (int)(xadj+csplat_limit);
  int cjmin= (int)(yadj-csplat_limit+1.0);
  int cjmax= (int)(yadj+csplat_limit);
  // Use small_splat to draw an antialiased point
  if (cimin>=0 && cimax<xsize && cjmin>=0 && cjmax<ysize)
    small_splat( &tmpS, 1.0, sep_fac, 1.0,
		 s->clr,cimin,cimax,cjmin,cjmax,tmp_image,
		 krnl_integral, pixels_touched );
}

void CircleSplatPainter::paint( const StarSplatter::Splat* splat,
				  const double sep_fac,
				  gColor* tmp_image, 
				  double& krnl_integral, int& pixels_touched )
{
  double lthick=4.5;
  double radius= 1.0/(sep_fac*splat->sqrt_exp_constant);

  double crad_min= radius-0.5*lthick;
  if (crad_min<0.0) crad_min= 0.0;
  for (double crad_shifted= crad_min;
       crad_shifted<=radius+0.5*lthick; crad_shifted += 0.5) {
    double x= 0.0;
    double y= crad_shifted;
    circlept(x,y,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
    circlept(x,-y,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
    circlept(y,x,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
    circlept(-y,x,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
    x += 1.0;
    y= sqrt(crad_shifted*crad_shifted-x*x);
    while (x<=y+0.5) {
      circlept(x,y,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
      circlept(x,-y,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
      circlept(y,x,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
      circlept(-y,x,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
      circlept(y,-x,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
      circlept(-x,y,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
      circlept(-x,-y,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
      circlept(-y,-x,sep_fac,splat,tmp_image,krnl_integral,pixels_touched);
      x += 1.0;
      y= sqrt(crad_shifted*crad_shifted-x*x);
    }
  }
}

