/****************************************************************************
 * ssplat_usr_modify.cc
 * Author Joel Welling
 * Copyright 1997, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

#include "starsplatter.h"

static char rcsid[] = "$Id: ssplat_usr_modify.cc,v 1.18 2009-06-09 23:46:51 welling Exp $";

double StarSplatter::default_log_rescale_min= 0.001;
double StarSplatter::default_log_rescale_max= 1.0;

int StarSplatter::convert_image_linear(rgbImage* image, 
				       const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;
  // Linear exposure calculation
  for (int jloop=0; jloop<ysize; jloop++) {
    image->setpix( 0, ysize-(jloop+1), (*pixrunner++)*exp_scale );
    for ( ; pixrunner < raw_image + (xsize*(jloop+1)); pixrunner++) {
      image->setnextpix( (*pixrunner)*exp_scale );
    }
  }
  return 1;
}

int StarSplatter::convert_image_log(rgbImage* image, const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;

  fprintf(stdout,"Bounds are min %g, max %g\n",log_rescale_min,log_rescale_max);
  double inv_range=1.0/(log(log_rescale_max) - log(log_rescale_min));
  double log_minmass= log(log_rescale_min);
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      gColor tmp_clr= gColor(pixrunner->r()*exp_scale,
			     pixrunner->g()*exp_scale,
			     pixrunner->b()*exp_scale,
			     pixrunner->a()*exp_scale);
      pixrunner++;
      if (tmp_clr.a() != 0.0) {
#ifdef never
	double alpha= tmp_clr.a();
	tmp_clr *= (1.0/alpha);
	double scale_r= (tmp_clr.r()>0.0) ?
	  inv_range*(log(tmp_clr.r())-log_minmass) : 0.0;
	double scale_g= (tmp_clr.g()>0.0) ?
	  inv_range*(log(tmp_clr.g())-log_minmass) : 0.0;
	double scale_b= (tmp_clr.b()>0.0) ?
	  inv_range*(log(tmp_clr.b())-log_minmass) : 0.0;
	double scale_a= 
	  inv_range*(log(tmp_clr.a())-log_minmass);
#endif
	double scale_r= (tmp_clr.r()>0.0) ?
	  inv_range*(log(tmp_clr.r())-log_minmass) : 0.0;
	double scale_g= (tmp_clr.g()>0.0) ?
	  inv_range*(log(tmp_clr.g())-log_minmass) : 0.0;
	double scale_b= (tmp_clr.b()>0.0) ?
	  inv_range*(log(tmp_clr.b())-log_minmass) : 0.0;
	double scale_a= 
	  inv_range*(log(tmp_clr.a())-log_minmass);
	gColor oclr= gColor(scale_r,scale_g,scale_b,scale_a).clamp();
	image->setpix(iloop, ysize-(jloop+1), oclr);
      }
      else
	image->setpix(iloop, ysize-(jloop+1),
		      gColor(0.0,0.0,0.0,0.0));
    }
  return 1;
}

int StarSplatter::convert_image_log_auto(rgbImage* image, 
					 const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;
  double minmass = 0.0;
  double maxmass = 0.0;
  int foundSome= 0;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      gColor tmp_clr= gColor(pixrunner->r()*exp_scale,
			     pixrunner->g()*exp_scale,
			     pixrunner->b()*exp_scale,
			     pixrunner->a()*exp_scale);
      pixrunner++;
      if (tmp_clr.a() != 0.0) {
	if (!foundSome) {
	  minmass= maxmass= tmp_clr.a();
	  foundSome= 1;
	}
	if (tmp_clr.r()>0.0 && tmp_clr.r()<minmass) minmass= tmp_clr.r();
	if (tmp_clr.r()>maxmass) maxmass= tmp_clr.r();
	if (tmp_clr.g()>0.0 && tmp_clr.g()<minmass) minmass= tmp_clr.g();
	if (tmp_clr.g()>maxmass) maxmass= tmp_clr.g();
	if (tmp_clr.b()>0.0 && tmp_clr.b()<minmass) minmass= tmp_clr.b();
	if (tmp_clr.b()>maxmass) maxmass= tmp_clr.b();
	if (tmp_clr.a()<minmass) minmass= tmp_clr.a();
	if (tmp_clr.a()>maxmass) maxmass= tmp_clr.a();
      }
    }
  }
  if (!foundSome) {
    fprintf(stderr,"splat_all_stars: can't rescale black image!\n");
    return 0;
  }
  
  fprintf(stdout,"log autoscale bounds %g, %g\n", minmass, maxmass);
  
  double log_maxmass= log(maxmass);
  double log_minmass= log(minmass);
  double inv_range=1.0/(log_maxmass-log_minmass);
  pixrunner= raw_image;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      gColor tmp_clr= gColor(pixrunner->r()*exp_scale,
			     pixrunner->g()*exp_scale,
			     pixrunner->b()*exp_scale,
			     pixrunner->a()*exp_scale);
      pixrunner++;
      if (tmp_clr.a() != 0.0) {
	double scale_r= (tmp_clr.r()>0.0) ?
	  inv_range*(log(tmp_clr.r())-log_minmass) : 0.0;
	double scale_g= (tmp_clr.g()>0.0) ?
	  inv_range*(log(tmp_clr.g())-log_minmass) : 0.0;
	double scale_b= (tmp_clr.b()>0.0) ?
	  inv_range*(log(tmp_clr.b())-log_minmass) : 0.0;
	double scale_a= 
	  inv_range*(log(tmp_clr.a())-log_minmass);
	gColor oclr= gColor(scale_r,scale_g,scale_b,scale_a).clamp();
	image->setpix(iloop, ysize-(jloop+1), oclr);
	
      }
      else 
	image->setpix(iloop, ysize-(jloop+1),
		      gColor(0.0,0.0,0.0,0.0));
    }
  }
  return 1;
}

int StarSplatter::convert_image_noopac_linear(rgbImage* image, 
					      const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;
  float max= 0.0;
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      if (pixrunner->r()>max) max= pixrunner->r();
      if (pixrunner->g()>max) max= pixrunner->g();
      if (pixrunner->b()>max) max= pixrunner->b();
      pixrunner++;
    }
  if (max==0.0) {
    for (int jloop=0; jloop<ysize; jloop++) 
      for (int iloop=0; iloop<xsize; iloop++) 
	image->setpix(iloop,ysize-(jloop+1), gColor(0.0,0.0,0.0));
  }
  else {
    float rescale= exp_scale/max;
    pixrunner= raw_image;
    for (int jloop=0; jloop<ysize; jloop++) 
      for (int iloop=0; iloop<xsize; iloop++) {
	float r= pixrunner->r()*rescale;
	float g= pixrunner->g()*rescale;
	float b= pixrunner->b()*rescale;
	float lclMax= fmax(r,fmax(g,b));
	gColor oclr= gColor(r,g,b,lclMax).clamp();
	image->setpix(iloop, ysize-(jloop+1), oclr);
	pixrunner++;
      }
  }
  return 1;
}

int StarSplatter::convert_image_noopac_log(rgbImage* image, 
					   const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;

  fprintf(stdout,"Bounds are min %g, max %g\n",log_rescale_min,log_rescale_max);
  double inv_range=1.0/(log(log_rescale_max) - log(log_rescale_min));
  double log_minmass= log(log_rescale_min);
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      float r= pixrunner->r()*exp_scale;
      float g= pixrunner->g()*exp_scale;
      float b= pixrunner->b()*exp_scale;
      float scale_r= (r>0.0) ? inv_range*(log(r)-log_minmass) : 0.0;
      float scale_g= (g>0.0) ? inv_range*(log(g)-log_minmass) : 0.0;
      float scale_b= (b>0.0) ? inv_range*(log(b)-log_minmass) : 0.0;
      float lclMax= fmax(r,fmax(g,b));
      gColor oclr= gColor(scale_r,scale_g,scale_b,lclMax).clamp();
      image->setpix(iloop, ysize-(jloop+1), oclr);
      pixrunner++;
    }
  return 1;
}

int StarSplatter::convert_image_noopac_log_auto(rgbImage* image, 
						const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;
  float minmass = 0.0;
  float maxmass = 0.0;
  int foundSome= 0;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      float r= pixrunner->r()*exp_scale;
      float g= pixrunner->g()*exp_scale;
      float b= pixrunner->b()*exp_scale;
      float low= fmin(r,fmin(g,b));
      float high= fmax(r,fmax(g,b));
      if (high>0.0) {
	if (!foundSome) {
	  minmass= maxmass= high;
	  foundSome= 1;
	}
      }
      if (maxmass<high) maxmass= high;
      if (low>0.0 && minmass>low) minmass= low;
      pixrunner++;
    }
  }
  if (!foundSome) {
    fprintf(stderr,"splat_all_stars: can't rescale black image!\n");
    return 0;
  }
  
  fprintf(stdout,"log autoscale bounds %g, %g\n", minmass, maxmass);
  
  double inv_range=1.0/(log(maxmass) - log(minmass));
  double log_minmass= log(minmass);
  pixrunner= raw_image;
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      float r= pixrunner->r()*exp_scale;
      float g= pixrunner->g()*exp_scale;
      float b= pixrunner->b()*exp_scale;
      float scale_r= (r>0.0) ? inv_range*(log(r)-log_minmass) : 0.0;
      float scale_g= (g>0.0) ? inv_range*(log(g)-log_minmass) : 0.0;
      float scale_b= (b>0.0) ? inv_range*(log(b)-log_minmass) : 0.0;
      float lclMax= fmax(r,fmax(g,b));
      gColor oclr= gColor(scale_r,scale_g,scale_b,lclMax).clamp();
      image->setpix(iloop, ysize-(jloop+1), oclr);
      pixrunner++;
    }

  return 1;
}

int StarSplatter::convert_image_noopac_log_hsv(rgbImage* image, 
					       const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;

  fprintf(stdout,"Bounds are min %g, max %g\n",log_rescale_min,log_rescale_max);
  double inv_range=1.0/(log(log_rescale_max) - log(log_rescale_min));
  double log_minmass= log(log_rescale_min);
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      float r= pixrunner->r()*exp_scale;
      float g= pixrunner->g()*exp_scale;
      float b= pixrunner->b()*exp_scale;
      float lclMax= fmax(r,fmax(g,b));
      if (lclMax>0.0) {
	gColor tmp_clr= gColor(r,g,b,lclMax);
	gHSVColor tmp_hsv= tmp_clr; // conversion happens in copy
	double scale_v= (tmp_hsv.v()>0.0) ?
	  inv_range*(log(tmp_hsv.v())-log_minmass) : 0.0;
	double scale_a= 
	  inv_range*(log(tmp_clr.a())-log_minmass);
	gHSVColor ohsv= gHSVColor(tmp_hsv.h(),tmp_hsv.s(),scale_v,scale_a);
	gColor oclr= gColor(ohsv).clamp();
	image->setpix(iloop, ysize-(jloop+1), oclr);	
      }
      else {
	image->setpix(iloop, ysize-(jloop+1),
		      gColor(0.0,0.0,0.0,0.0));
      }
      pixrunner++;
    }
  return 1;
}

int StarSplatter::convert_image_noopac_log_hsv_auto(rgbImage* image, 
						    const gColor* raw_image)
{
  
  const gColor* pixrunner= raw_image;
  gHSVColor* hsvImage= new gHSVColor[xsize*ysize];
  gHSVColor* hsvrunner= hsvImage;
  for (pixrunner= raw_image; pixrunner<raw_image+(xsize*ysize);
       pixrunner++) {
    float r= pixrunner->r();
    float g= pixrunner->g();
    float b= pixrunner->b();
    float lclMax= fmax(r,fmax(g,b));
    *hsvrunner= gColor(r,g,b,lclMax); // conversion happens during assignment
    hsvrunner->scale_value(exp_scale);
    hsvrunner++;
  }
  
  double minmass = 0.0;
  double maxmass = 0.0;
  int foundSome= 0;
  hsvrunner= hsvImage;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      gHSVColor tmp_hsv= *hsvrunner++;
      if (tmp_hsv.a() != 0.0) {
	if (!foundSome) {
	  minmass= maxmass= tmp_hsv.a(); // set from lclMax and exp_scale above
	  foundSome= 1;
	}
	if (tmp_hsv.v()>0.0 && tmp_hsv.v()<minmass) minmass= tmp_hsv.v();
	if (tmp_hsv.v()>maxmass) maxmass= tmp_hsv.v();
      }
    }
  }
  if (!foundSome) {
    fprintf(stderr,"splat_all_stars: can't rescale black image!\n");
    return 0;
  }
  
  fprintf(stdout,"hsv log autoscale bounds %g, %g\n", minmass, maxmass);
  
  double log_maxmass= log(maxmass);
  double log_minmass= log(minmass);
  double inv_range=1.0/(log_maxmass-log_minmass);
  hsvrunner= hsvImage;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      gHSVColor tmp_hsv= *hsvrunner++;
      if (tmp_hsv.a() != 0.0) {
	double scale_v= (tmp_hsv.v()>0.0) ?
	  inv_range*(log(tmp_hsv.v())-log_minmass) : 0.0;
	double scale_a= 
	  inv_range*(log(tmp_hsv.a())-log_minmass);
	gHSVColor ohsv= gHSVColor(tmp_hsv.h(),tmp_hsv.s(),
				  scale_v,scale_a);
	gColor oclr= gColor(ohsv);
	image->setpix(iloop, ysize-(jloop+1), oclr);
      }
      else 
	image->setpix(iloop, ysize-(jloop+1),
		      gColor(0.0,0.0,0.0,0.0));
    }
  }

  delete [] hsvImage;

  return 1;
}

int StarSplatter::convert_image_lupton(rgbImage* image, 
				       const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;
  pixrunner= raw_image;
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      float r= pixrunner->r()*exp_scale;
      float g= pixrunner->g()*exp_scale;
      float b= pixrunner->b()*exp_scale;
      float intens= r+g+b;
      float scale= asinh(intens/log_rescale_min)/intens;
      r *= scale;
      g *= scale;
      b *= scale;
      gColor oclr= gColor(r,g,b,scale).clamp();
      image->setpix(iloop, ysize-(jloop+1), oclr);
      pixrunner++;
    }
  return 1;
}

int StarSplatter::convert_image_log_hsv(rgbImage* image, 
					const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;
  fprintf(stdout,"Bounds are min %g, max %g\n",log_rescale_min,log_rescale_max);
  double inv_range=1.0/(log(log_rescale_max) - log(log_rescale_min));
  double log_minmass= log(log_rescale_min);
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      gColor tmp_clr= gColor(pixrunner->r()*exp_scale,
			     pixrunner->g()*exp_scale,
			     pixrunner->b()*exp_scale,
			     pixrunner->a()*exp_scale);
      pixrunner++;
      if (tmp_clr.a() != 0.0) {
	gHSVColor tmp_hsv= tmp_clr; // conversion happens in copy
	double scale_v= (tmp_hsv.v()>0.0) ?
	  inv_range*(log(tmp_hsv.v())-log_minmass) : 0.0;
	double scale_a= 
	  inv_range*(log(tmp_clr.a())-log_minmass);
	gHSVColor ohsv= gHSVColor(tmp_hsv.h(),tmp_hsv.s(),scale_v,scale_a);
	gColor oclr= gColor(ohsv).clamp();
	image->setpix(iloop, ysize-(jloop+1), oclr);
      }
      else
	image->setpix(iloop, ysize-(jloop+1),
		      gColor(0.0,0.0,0.0,0.0));
    }
  return 1;
}

int StarSplatter::convert_image_log_hsv_auto(rgbImage* image, 
					     const gColor* raw_image)
{
  const gColor* pixrunner= raw_image;
  gHSVColor* hsvImage= new gHSVColor[xsize*ysize];
  gHSVColor* hsvrunner= hsvImage;
  for (pixrunner= raw_image; pixrunner<raw_image+(xsize*ysize);
       pixrunner++) {
    *hsvrunner= *pixrunner; // conversion happens during assignment
    hsvrunner->scale_value(exp_scale);
    hsvrunner++;
  }
  
  double minmass = 0.0;
  double maxmass = 0.0;
  int foundSome= 0;
  hsvrunner= hsvImage;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      gHSVColor tmp_hsv= *hsvrunner++;
      if (tmp_hsv.a() != 0.0) {
	if (!foundSome) {
	  minmass= maxmass= tmp_hsv.a();
	  foundSome= 1;
	}
	if (tmp_hsv.v()>0.0 && tmp_hsv.v()<minmass) minmass= tmp_hsv.v();
	if (tmp_hsv.v()>maxmass) maxmass= tmp_hsv.v();
      }
    }
  }
  if (!foundSome) {
    fprintf(stderr,"splat_all_stars: can't rescale black image!\n");
    return 0;
  }
  
  fprintf(stdout,"hsv log autoscale bounds %g, %g\n", minmass, maxmass);
  
  double log_maxmass= log(maxmass);
  double log_minmass= log(minmass);
  double inv_range=1.0/(log_maxmass-log_minmass);
  hsvrunner= hsvImage;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      gHSVColor tmp_hsv= *hsvrunner++;
      if (tmp_hsv.a() != 0.0) {
	double scale_v= (tmp_hsv.v()>0.0) ?
	  inv_range*(log(tmp_hsv.v())-log_minmass) : 0.0;
	double scale_a= 
	  inv_range*(log(tmp_hsv.a())-log_minmass);
	gHSVColor ohsv= gHSVColor(tmp_hsv.h(),tmp_hsv.s(),
				  scale_v,scale_a);
	gColor oclr= gColor(ohsv);
	image->setpix(iloop, ysize-(jloop+1), oclr);
      }
      else 
	image->setpix(iloop, ysize-(jloop+1),
		      gColor(0.0,0.0,0.0,0.0));
    }
  }
  delete [] hsvImage;
  return 1;
}

int StarSplatter::convert_image_late_cmap_r(rgbImage* image, 
					    const gColor* raw_image)
{
  if (!late_cmap) {
    fprintf(stderr,"late_cmap has not been set!\n");
    return 0;
  }
  const gColor* pixrunner= raw_image;
  for (int jloop=0; jloop<ysize; jloop++) {
    double val= pixrunner->r()*exp_scale;
    image->setpix( 0, ysize-(jloop+1), late_cmap->map(val) );
    for ( ; pixrunner < raw_image + (xsize*(jloop+1)); pixrunner++) {
      val= (*pixrunner++).r()*exp_scale;
      image->setnextpix( late_cmap->map(val) );
    }
  }
  return 1;
}

int StarSplatter::convert_image_late_cmap_a(rgbImage* image, 
					    const gColor* raw_image)
{
  if (!late_cmap) {
    fprintf(stderr,"late_cmap has not been set!\n");
    return 0;
  }
  const gColor* pixrunner= raw_image;
  for (int jloop=0; jloop<ysize; jloop++) {
    double val= pixrunner->a()*exp_scale;
    image->setpix( 0, ysize-(jloop+1), late_cmap->map(val) );
    for ( ; pixrunner < raw_image + (xsize*(jloop+1)); pixrunner++) {
      val= (*pixrunner++).a()*exp_scale;
      image->setnextpix( late_cmap->map(val) );
    }
  }
  return 1;
}

int StarSplatter::convert_image_late_cmap_log_r(rgbImage* image, 
						const gColor* raw_image)
{
  if (!late_cmap) {
    fprintf(stderr,"late_cmap has not been set!\n");
    return 0;
  }
  const gColor* pixrunner= raw_image;
  fprintf(stdout,"Bounds are min %g, max %g\n",
	  log_rescale_min,log_rescale_max);
  double inv_range=1.0/(log(log_rescale_max) - log(log_rescale_min));
  double log_minmass= log(log_rescale_min);
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      double val= (*pixrunner++).r()*exp_scale;
      val= (val>0.0) ? inv_range*(log(val)-log_minmass) : 0.0;
      gColor oclr= late_cmap->map(val);
      image->setpix(iloop, ysize-(jloop+1), oclr);
    }
  return 1;
}

int StarSplatter::convert_image_late_cmap_log_a(rgbImage* image, 
						const gColor* raw_image)
{
  if (!late_cmap) {
    fprintf(stderr,"late_cmap has not been set!\n");
    return 0;
  }
  const gColor* pixrunner= raw_image;
  fprintf(stdout,"Bounds are min %g, max %g\n",
	  log_rescale_min,log_rescale_max);
  double inv_range=1.0/(log(log_rescale_max) - log(log_rescale_min));
  double log_minmass= log(log_rescale_min);
  for (int jloop=0; jloop<ysize; jloop++) 
    for (int iloop=0; iloop<xsize; iloop++) {
      double val= (*pixrunner++).a()*exp_scale;
      val= (val>0.0)?inv_range*(log(val)-log_minmass) : 0.0;
      gColor oclr= late_cmap->map(val);
      image->setpix(iloop, ysize-(jloop+1), oclr);
    }
  return 1;
}

int StarSplatter::convert_image_late_cmap_log_r_auto(rgbImage* image, 
						     const gColor* raw_image)
{
  if (!late_cmap) {
    fprintf(stderr,"late_cmap has not been set!\n");
    return 0;
  }
  const gColor* pixrunner= raw_image;
  double minmass = 0.0;
  double maxmass = 0.0;
  int foundSome= 0;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      double val= (*pixrunner++).r()*exp_scale;
      if (val != 0.0) {
	if (!foundSome) {
	  minmass= maxmass= val;
	  foundSome= 1;
	}
	if (val>0.0 && val<minmass) minmass= val;
	if (val>maxmass) maxmass= val;
      }
    }
  }
  if (!foundSome) {
    fprintf(stderr,"splat_all_stars: can't rescale black image!\n");
    return 0;
  }
  
  fprintf(stdout,"log autoscale bounds %g, %g\n", minmass, maxmass);
  
  double log_maxmass= log(maxmass);
  double log_minmass= log(minmass);
  double inv_range=1.0/(log_maxmass-log_minmass);
  pixrunner= raw_image;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      double val= (*pixrunner++).r()*exp_scale;
      val= (val>0.0) ? inv_range*(log(val)-log_minmass) : 0.0;
      gColor oclr= late_cmap->map(val);
      image->setpix(iloop, ysize-(jloop+1), oclr);
    }
  }
  return 1;
}

int StarSplatter::convert_image_late_cmap_log_a_auto(rgbImage* image, 
						     const gColor* raw_image)
{
  if (!late_cmap) {
    fprintf(stderr,"late_cmap has not been set!\n");
    return 0;
  }
  const gColor* pixrunner= raw_image;
  double minmass = 0.0;
  double maxmass = 0.0;
  int foundSome= 0;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      double val= (*pixrunner++).a()*exp_scale;
      if (val != 0.0) {
	if (!foundSome) {
	  minmass= maxmass= val;
	  foundSome= 1;
	}
	if (val>0.0 && val<minmass) minmass= val;
	if (val>maxmass) maxmass= val;
      }
    }
  }
  if (!foundSome) {
    fprintf(stderr,"splat_all_stars: can't rescale black image!\n");
    return 0;
  }
  
  fprintf(stdout,"log autoscale bounds %g, %g\n", minmass, maxmass);
  
  double log_maxmass= log(maxmass);
  double log_minmass= log(minmass);
  double inv_range=1.0/(log_maxmass-log_minmass);
  pixrunner= raw_image;
  for (int jloop=0; jloop<ysize; jloop++) {
    for (int iloop=0; iloop<xsize; iloop++) {
      double val= (*pixrunner++).a()*exp_scale;
      val= (val>0.0) ? inv_range*(log(val)-log_minmass) : 0.0;
      gColor oclr= late_cmap->map(val);
      image->setpix(iloop, ysize-(jloop+1), oclr);
    }
  }
  return 1;
}

int StarSplatter::convert_image( rgbImage* image, const gColor* raw_image )
{
  // Copy the floating point image into the result image
  // Image gets flipped vertically in the process
  switch (current_exposure_type) {
  case ET_LINEAR: 
    if (!convert_image_linear(image,raw_image)) return 0;
    break;

  case ET_LOG:
    if (!convert_image_log(image,raw_image)) return 0;
  break;

  case ET_LOG_AUTO:
    if (!convert_image_log_auto(image,raw_image)) return 0;
    break;

  case ET_NOOPAC_LINEAR:
    if (!convert_image_noopac_linear(image,raw_image)) return 0;
    break;
		      
  case ET_NOOPAC_LOG:
    if (!convert_image_noopac_log(image,raw_image)) return 0;
    break;

  case ET_NOOPAC_LOG_AUTO:
    if (!convert_image_noopac_log_auto(image,raw_image)) return 0;
    break;

  case ET_NOOPAC_LOG_HSV:
    if (!convert_image_noopac_log_hsv(image,raw_image)) return 0;
    break;

  case ET_NOOPAC_LOG_HSV_AUTO:
    if (!convert_image_noopac_log_hsv_auto(image,raw_image)) return 0;
    break;

  case ET_LUPTON:
    if (!convert_image_lupton(image,raw_image)) return 0;
    break;

  case ET_LOG_HSV: 
    if (!convert_image_log_hsv(image,raw_image)) return 0;
  break;

  case ET_LOG_HSV_AUTO: 
    if (!convert_image_log_hsv(image,raw_image)) return 0;
    break;

  case ET_LATE_CMAP_R: 
    if (!convert_image_late_cmap_r(image,raw_image)) return 0;
    break;

  case ET_LATE_CMAP_A: 
    if (!convert_image_late_cmap_a(image,raw_image)) return 0;
    break;

  case ET_LATE_CMAP_LOG_R: 
    if (!convert_image_late_cmap_log_r(image,raw_image)) return 0;
    break;

  case ET_LATE_CMAP_LOG_A: 
    if (!convert_image_late_cmap_log_a(image,raw_image)) return 0;
    break;

  case ET_LATE_CMAP_LOG_R_AUTO: 
    {
    if (!convert_image_late_cmap_log_r_auto(image,raw_image)) return 0;
    }
    break;

  case ET_LATE_CMAP_LOG_A_AUTO: 
    {
    if (!convert_image_late_cmap_log_a_auto(image,raw_image)) return 0;
    }
    break;
  }

  if (debug_flag) {
    // Produce histogram of pixel colors
    int histo[256][4];
    for (int i=0; i<256; i++) 
      histo[i][0]= histo[i][1]= histo[i][2]= histo[i][3]= 0;
    for (int jloop=0; jloop<ysize; jloop++) {
      for (int iloop= 0; iloop<xsize; iloop++) {
	gBColor clr= image->pix(iloop,jloop);
	histo[clr.ir()][0]++;
	histo[clr.ig()][1]++;
	histo[clr.ib()][2]++;
	histo[clr.ia()][3]++;
      }
    }
    fprintf(stderr,"Binned counts for scaled pixel values:\n");
    fprintf(stderr," val     R       G         B         A\n");
    fprintf(stderr," ---   -----   -----     -----     -----\n");
    for (int iloop=0; iloop<256; iloop++) {
      fprintf(stderr,"%3d   %6d   %6d   %6d   %6d\n",
	      iloop,histo[iloop][0],histo[iloop][1],
	      histo[iloop][2],histo[iloop][3]);
    }

  }
  
  return 1;
}
