/****************************************************************************
 * starsplatter.cc
 * Author Joel Welling
 * Copyright 1996, 2008, Pittsburgh Supercomputing Center, 
 *                       Carnegie Mellon University
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
#include "gaussiansplatpainter.h"
#include "splinesplatpainter.h"
#include "circlesplatpainter.h"

/* Notes-
 */

const char* ssplat_home_page_string="http://www.psc.edu/Packages/StarSplatter_Home";
const char* ssplat_version_string="StarSplatter version 2.1";
const char* ssplat_copyright_string=
"StarSplatter copyright 1996, 1997, 2008 Pittsburgh Supercomputing Center, \n\
Carnegie Mellon University.  Author Joel Welling.\n\
\n\
Permission to use, copy, and modify this software and its documentation \n\
without fee for personal use or use within your organization is hereby \n\
granted, provided that the above copyright notice is preserved in all \n\
copies and that that copyright and this permission notice appear in \n\
supporting documentation.  Permission to redistribute this software to \n\
other organizations or individuals is not granted;  that must be \n\
negotiated with the PSC.  Neither the PSC nor Carnegie Mellon \n\
University make any representations about the suitability of this \n\
software for any purpose.  It is provided \"as is\" without express or \n\
implied warranty.\n";

short StarSplatter::screen_minz= -32768;
short StarSplatter::screen_maxz= 32767;

int StarSplatter::initial_sbunch_table_size= 10;
double StarSplatter::default_gaussian_splat_cutoff= 0.01;

StarSplatter::StarSplatter()
  : cam( gPoint(0.0,0.0,1.0), gPoint(0.0,0.0,0.0), gVector(0.0,1.0,0.0),
	 45.0, -0.5, -1.5 )
{
  xsize= ysize= 0;
  cam_set_flag= 0;
  debug_flag= 0;
  exp_scale= 1.0;
  current_exposure_type= ET_LINEAR;
  log_rescale_min= default_log_rescale_min;
  log_rescale_max= default_log_rescale_max;

  sbunch_table= new StarBunch*[initial_sbunch_table_size];
  sbunch_table_size= initial_sbunch_table_size;
  n_sbunches= 0;
  total_stars= 0;
  total_stars_after_clipping= 0;
  splatbuf= NULL;
  splatbuf_size= 0;
  splat_cutoff= default_gaussian_splat_cutoff;
  late_cmap= NULL;
  current_splat_painter= new GaussianSplatPainter(this);
}

StarSplatter::~StarSplatter()
{
  delete [] sbunch_table;
}

StarSplatter::SplatType StarSplatter::splat_type() const
{ 
  return current_splat_painter->getSplatType(); 
}

void StarSplatter::set_splat_type( SplatType t )
{
  delete current_splat_painter;
  switch (t) {
  case SPLAT_GAUSSIAN:
    current_splat_painter= new GaussianSplatPainter(this);
    break;
  case SPLAT_SPLINE:
    current_splat_painter= new SplineSplatPainter(this);
    break;
  case SPLAT_GLYPH_CIRCLE:
    current_splat_painter= new CircleSplatPainter(this);
    break;
  }
}

void StarSplatter::dump( FILE* ofile )
{
  fprintf(ofile,"StarSplatter renderer: image xdim %d, ydim %d\n",xsize,ysize);
  fprintf(ofile,"     %d particle sets registered; %d particles total\n",
	  n_sbunches,total_stars);
  if (cam_set_flag)
    fprintf(ofile,"     camera is set\n");
  else fprintf(ofile,"     camera is not set\n");
  const char* exp_type_string= "*unknown*";
  switch (current_exposure_type) {
  case ET_LINEAR: exp_type_string= "linear";
    break;
  case ET_LOG: exp_type_string= "log";
    break;
  case ET_LOG_AUTO: exp_type_string= "log_auto";
    break;
  case ET_NOOPAC_LINEAR: exp_type_string= "noopac_linear";
    break;
  case ET_NOOPAC_LOG: exp_type_string= "noopac_log";
    break;
  case ET_NOOPAC_LOG_AUTO: exp_type_string= "noopac_log_auto";
    break;
  case ET_NOOPAC_LOG_HSV: exp_type_string= "noopac_log_hsv";
    break;
  case ET_NOOPAC_LOG_HSV_AUTO: exp_type_string= "noopac_log_hsv_auto";
    break;
  case ET_LUPTON: exp_type_string= "noopac_log_auto";
    break;
  case ET_LOG_HSV: exp_type_string= "log_hsv";
    break;
  case ET_LOG_HSV_AUTO: exp_type_string= "log_hsv_auto";
    break;
  case ET_LATE_CMAP_R: exp_type_string= "late_cmap_r";
    break;
  case ET_LATE_CMAP_A: exp_type_string= "late_cmap_a";
    break;
  case ET_LATE_CMAP_LOG_R: exp_type_string= "late_cmap_log_r";
    break;
  case ET_LATE_CMAP_LOG_A: exp_type_string= "late_cmap_log_a";
    break;
  case ET_LATE_CMAP_LOG_R_AUTO: exp_type_string= "late_cmap_log_r_auto";
    break;
  case ET_LATE_CMAP_LOG_A_AUTO: exp_type_string= "late_cmap_log_a_auto";
    break;
  }
  fprintf(ofile,"     exposure type is %s\n", exp_type_string);
  fprintf(ofile,"     splat type is %s\n",current_splat_painter->typeName());
  fprintf(ofile,"     log exposure bounds %g, %g\n",
	  log_rescale_min, log_rescale_max);
  fprintf(ofile,"     debug %s, splat_cutoff %f, exposure scale %f\n", 
	  (debug()) ? "on" : "off", splat_cutoff, exp_scale);
  fprintf(ofile,"     world transformation follows:\n");
  const float* data= world_trans.floatrep();
  for (int i=0; i<16; i+=4)
    fprintf(ofile,"      ( %f %f %f %f )\n",
            data[i], data[i+1], data[i+2], data[i+3]);
}

void StarSplatter::clear_stars()
{
  n_sbunches= 0;
  total_stars= 0;
  total_stars_after_clipping= 0;
}

void StarSplatter::add_stars( StarBunch* sbunch_in )
{
  if (n_sbunches >= (sbunch_table_size-1)) {
    // Grow the table
    int new_size= 2*sbunch_table_size;
    StarBunch** new_table= new StarBunch*[new_size];
    for (int i=0; i<sbunch_table_size; i++) new_table[i]= sbunch_table[i];
    delete [] sbunch_table;
    sbunch_table= new_table;
    sbunch_table_size= new_size;
  }

  sbunch_table[n_sbunches++]= sbunch_in;
  total_stars += sbunch_in->nstars();
}

void StarSplatter::transform_and_merge()
{
  // Check splatbuf size
  if (splatbuf_size < total_stars) {
    delete [] splatbuf;
    splatbuf_size= total_stars;
    splatbuf= new Splat[splatbuf_size];
    if (!splatbuf) {
      fprintf(stderr,
	  "StarSplatter: Out of memory allocating splat buffer (%d bytes)!\n",
	      splatbuf_size);
      exit(-1);
    }
  }

  // Get the camera transformation
  gTransfm* cam_trans= cam.screen_projection_matrix( xsize, ysize,
						     screen_minz, 
						     screen_maxz );
  Splat* srunner= splatbuf;
  double clip_xsize= xsize-1;
  double clip_ysize= ysize-1;
  for (int i=0; i<n_sbunches; i++) {
    for (int particle_index=0; particle_index<sbunch_table[i]->nstars();
	 particle_index++) {
      if (!sbunch_table[i]->valid(particle_index)) continue;
      gPoint orientpt= world_trans*(sbunch_table[i]->coords(particle_index));
      gPoint projpt= *cam_trans*orientpt;
      if (((projpt.w()>0.0)
	   &&((projpt.x()>0.0) && (projpt.x()<clip_xsize*projpt.w())
	      &&(projpt.y()>0.0) && (projpt.y()<clip_ysize*projpt.w())
	      &&(projpt.z()>screen_minz*projpt.w()) 
	      &&(projpt.z()<screen_maxz*projpt.w())))
	  ||((projpt.w()<0.0)
	     &&(((projpt.x()<0.0) && (projpt.x()>clip_xsize*projpt.w())
		 &&(projpt.y()<0.0) && (projpt.y()>clip_ysize*projpt.w())
		 &&(projpt.z()<screen_minz*projpt.w()) 
		 &&(projpt.z()>screen_maxz*projpt.w()))))) {
	projpt.homogenize();
	srunner->loc= projpt;
	srunner->range= (orientpt - cam.frompt()).length();
	srunner->bunch_index= i;
	srunner->density= sbunch_table[i]->density( particle_index );
	srunner->sqrt_exp_constant= 
	  sbunch_table[i]->sqrt_exp_constant(particle_index);
	srunner->clr= sbunch_table[i]->clr(particle_index);
	srunner++;
      }
    }
  }
  total_stars_after_clipping= srunner - splatbuf;
  if (debug()) fprintf(stderr,
		       "%d of %d stars remain after clipping\n",
		       total_stars_after_clipping, total_stars);

  // Clean up
  delete cam_trans;
}

int StarSplatter::splat_depth_compare( const void* s1, const void* s2 )
{
  double z1= ((Splat*)s1)->loc.z();
  double z2= ((Splat*)s2)->loc.z();
  if (z1<z2) return -1;
  else if (z1>z2) return 1;
  else return 0;
}

void StarSplatter::sort()
{
  // Sort the splat buffer
  qsort(splatbuf, total_stars_after_clipping, sizeof(Splat),
	splat_depth_compare);
  if (debug()) fprintf(stderr,"Sort complete\n");
}

void StarSplatter::point_splat_all_stars( rgbImage* image )
{
  for (Splat* srunner= splatbuf;
       srunner< splatbuf + total_stars_after_clipping;
       srunner++) {
    image->setpix( (int)(srunner->loc.x()), 
		   ysize-((int)(srunner->loc.y())+1),
		   srunner->clr );
  }
}

int StarSplatter::splat_all_stars( rgbImage* image )
{
  // Note that this routine assumes square pixels

  gColor* pixrunner;

  // Create and clear the temporary image 
  // (default constructor is transparent black)
  gColor* tmp_image= new gColor[ xsize*ysize ]; // stored as floats
  if (!tmp_image) {
    fprintf(stderr,
	"splat_all_stars: Unable to allocate temporary image (%ld bytes)!\n",
	    xsize*ysize*sizeof(gColor));
    return 0;
  }

  // divergence of rays through adjacent pixels
  double pix_div= 
    (xsize >= ysize) ? 
    (DegtoRad*cam.fov())/ysize : (DegtoRad*cam.fov())/xsize; // small angle

  double energy_measure_min= 0.0;
  double energy_measure_max= 0.0;
  double energy_measure_ave= 0.0;
  int pix_hit_min= 0;
  int pix_hit_max= 0;
  int pix_hit_sum= 0;

  for (Splat* srunner= splatbuf; 
       srunner< splatbuf + total_stars_after_clipping; 
       srunner++) {
      
    double energy_measure;
    int pixels_touched= 0;
    double r= (cam.parallel_proj()) ? 
      ((cam.atpt() - cam.frompt()).length()) : srunner->range;

    double sep_fac= r*pix_div;
    
    double krnl_integral= 0.0; // different scaling from table case

    current_splat_painter->paint( srunner, sep_fac, tmp_image, 
				  krnl_integral, pixels_touched );

    if (debug_flag) {
      // update energy statistics
      energy_measure= krnl_integral*sep_fac*sep_fac;

      if (srunner==splatbuf) {
	energy_measure_min= energy_measure;
	energy_measure_max= energy_measure;
	pix_hit_min= pixels_touched;
	pix_hit_max= pixels_touched;
      }
      else {
	if (energy_measure<energy_measure_min) 
	  energy_measure_min= energy_measure;
	if (energy_measure>energy_measure_max) 
	  energy_measure_max= energy_measure;
	if (pixels_touched<pix_hit_min) pix_hit_min= pixels_touched;
	if (pixels_touched>pix_hit_max) pix_hit_max= pixels_touched;
      }
      energy_measure_ave += energy_measure;
      pix_hit_sum += pixels_touched;
      if (!(((int)(srunner-splatbuf)+1)%10000)) 
	fprintf(stderr,"%d splats done; this splat %d pixels\n",
		(int)(srunner-splatbuf)+1, pixels_touched);
    }
  }

  if (debug_flag) {
    energy_measure_ave /= total_stars_after_clipping;
    fprintf(stderr,"splatted %d particles\n",total_stars_after_clipping);
    if (!total_stars_after_clipping) 
      total_stars_after_clipping= 1; /* avoid a divide-by-zero */
    fprintf(stderr,"%d to %d pixels per particle (average %f)\n",
	    pix_hit_min,pix_hit_max,
    	    (((double)pix_hit_sum)/((double)total_stars_after_clipping)));
    fprintf(stderr,"energy measure %f to %f, average %f, vs. 1.0 ideal\n",
	    energy_measure_min, energy_measure_max, energy_measure_ave);

    // Generate exposure histogram info
    double rmin, rmax, rave;
    double gmin, gmax, gave;
    double bmin, bmax, bave;
    double amin, amax, aave;

    rmin= rmax= rave= tmp_image->r();
    gmin= gmax= gave= tmp_image->g();
    bmin= bmax= bave= tmp_image->b();
    amin= amax= aave= tmp_image->a();
    for (pixrunner= tmp_image+1; pixrunner < tmp_image+xsize*ysize;
	 pixrunner++) {
      if (pixrunner->r()<rmin) rmin= pixrunner->r();
      if (pixrunner->r()>rmax) rmax= pixrunner->r();
      rave += pixrunner->r();
      if (pixrunner->g()<gmin) gmin= pixrunner->g();
      if (pixrunner->g()>gmax) gmax= pixrunner->g();
      gave += pixrunner->g();
      if (pixrunner->b()<bmin) bmin= pixrunner->b();
      if (pixrunner->b()>bmax) bmax= pixrunner->b();
      bave += pixrunner->b();
      if (pixrunner->a()<amin) amin= pixrunner->a();
      if (pixrunner->a()>amax) amax= pixrunner->a();
      aave += pixrunner->a();
    }
    rave /= (double)(xsize*ysize);
    gave /= (double)(xsize*ysize);
    bave /= (double)(xsize*ysize);
    aave /= (double)(xsize*ysize);
    fprintf(stderr,"Pixel statistics before image rescaling:\n");
    fprintf(stderr,"%f < r < %f; average %f\n",rmin,rmax,rave);
    fprintf(stderr,"%f < g < %f; average %f\n",gmin,gmax,gave);
    fprintf(stderr,"%f < b < %f; average %f\n",bmin,bmax,bave);
    fprintf(stderr,"%f < a < %f; average %f\n",amin,amax,aave);

    int histo[100][4];
    for (int i=0; i<100; i++) 
      histo[i][0]= histo[i][1]= histo[i][2]= histo[i][3]= 0;

    pixrunner= tmp_image;
    for (pixrunner= tmp_image; pixrunner-tmp_image < xsize*ysize; 
	 pixrunner++) {
      if (rmax>rmin)
	histo[(int)(99*((pixrunner->r()-rmin)/(rmax-rmin))+0.5)][0]++;
      if (gmax>gmin)
	histo[(int)(99*((pixrunner->g()-gmin)/(gmax-gmin))+0.5)][1]++;
      if (bmax>bmin)
	histo[(int)(99*((pixrunner->b()-bmin)/(bmax-bmin))+0.5)][2]++;
      if (amax>amin)
	histo[(int)(99*((pixrunner->a()-amin)/(amax-amin))+0.5)][3]++;
    }

    fprintf(stderr,"Binned counts within these ranges:\n");
    fprintf(stderr," bin     R       G         B         A\n");
    fprintf(stderr," ---   -----   -----     -----     -----\n");
    for (int i=0; i<100; i++) {
      fprintf(stderr,"%3d   %6d   %6d   %6d   %6d\n",
	      i,histo[i][0],histo[i][1],histo[i][2],histo[i][3]);
    }
  }

  // Copy the double precision image into the result image
  // Image gets flipped vertically in the process
  if (!convert_image(image, tmp_image)) {
    delete [] tmp_image;
    return 0;
  }

  // Clean up
  delete [] tmp_image;

  return 1;
}

rgbImage* StarSplatter::render()
{
  if (!xsize || !ysize) {
    fprintf(stderr,"StarSplatter::render: image dims not set!\n");
    return NULL;
  }
  if (!cam_set_flag) {
    fprintf(stderr,"StarSplatter::render: camera not set!\n");
    return NULL;
  }

  rgbImage* result= new rgbImage( xsize, ysize );
  result->clear();

  // Transform particles, and merge into splatbuf
  transform_and_merge();

  // Depth sort the splatbuf
  sort();

  // Splat the splatbuf
  if (!splat_all_stars( result )) {
    // splatting failed for some reason
    delete result;
    return NULL;
  }

  return result;
}

rgbImage* StarSplatter::render_points()
{
  if (!xsize || !ysize) {
    fprintf(stderr,"StarSplatter::render: image dims not set!\n");
    return NULL;
  }
  if (!cam_set_flag) {
    fprintf(stderr,"StarSplatter::render: camera not set!\n");
    return NULL;
  }

  rgbImage* result= new rgbImage( xsize, ysize );
  result->clear();

  // Transform particles, and merge into splatbuf
  transform_and_merge();

  // Depth sort the splatbuf
  sort();

  // Splat the splatbuf
  point_splat_all_stars( result );

  return result;
}

void StarSplatter::set_late_colormap(const gColor* colors, const int xdim,
				     const double min, const double max)
{
  delete late_cmap;
  late_cmap= new StarBunchCMap(colors, xdim, 1, min, max, 0.0, 0.0);
}

