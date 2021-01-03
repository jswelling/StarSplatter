/****************************************************************************
 * camera.cc
 * Author Joel Welling
 * Copyright 1993, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include <string.h>
#include <math.h>
#include "rgbimage.h"
#include "camera.h"

Camera::Camera( const gPoint lookfm_in, const gPoint lookat_in, 
		const gVector up_in, const double fovea_in, 
		const double hither_in, const double yon_in,
		const int parallel_flag_in )
{
  lookat= lookat_in;
  lookfm= lookfm_in;
  up= up_in;
  fovea= fovea_in;
  hither= hither_in;
  yon= yon_in;
  lookdir= lookat - lookfm;
  lookdist= lookdir.length();
  lookdir.normalize();
  parallel_flag= parallel_flag_in;
}

Camera::Camera( const Camera& other )
{
  lookat= other.lookat;
  lookfm= other.lookfm;
  up= other.up;
  lookdir= other.lookdir;
  fovea= other.fovea;
  hither= other.hither;
  yon= other.yon;
  lookdist= other.lookdist;
  background= other.background;
  parallel_flag= other.parallel_flag;
}

Camera::~Camera()
{
  // Nothing to delete
}

Camera& Camera::operator=( const Camera& other )
{
  if (this != &other) {
    lookat= other.lookat;
    lookfm= other.lookfm;
    up= other.up;
    lookdir= other.lookdir;
    fovea= other.fovea;
    hither= other.hither;
    yon= other.yon;
    lookdist= other.lookdist;
    background= other.background;
    parallel_flag= other.parallel_flag;
  }
  return( *this );
}

gTransfm* Camera::screen_projection_matrix( const int screenx, 
					   const int screeny,
					   const int minz, const int maxz )
{
  int screenwidth= (screenx >= screeny) ? screeny : screenx;
  int zrange= maxz - minz;

  // Align the viewing direction with the negative Z axis, with up 
  // aimed in the Y direction and the lookfrom point at the origin.
  // This takes several steps:

  // Translate the lookfrom point to the origin
  gTransfm* align= gTransfm::translation(-lookfm.x(),-lookfm.y(),-lookfm.z());

  // Rotate the lookat point onto the -z axis
  gVector shifted_lookat= lookat - lookfm;
  gVector neg_z_axis( 0.0, 0.0, -1.0 );
  gTransfm* rot= shifted_lookat.aligning_rotation( neg_z_axis );

  // Align the up vector with the y axis 
  gVector normup= up.normal_component_wrt( shifted_lookat );
  gVector rup= *rot * normup;
  gVector y_axis( 0.0, 1.0, 0.0 );
  gTransfm* urot= rup.aligning_rotation( y_axis );

  float data[16];
  for (int i=0; i<16; i++) data[i]= 0.0;
  if (parallel_flag) { 
    // Parallel projection

    double view_width= lookdist * tan( 0.5*DegtoRad*fovea );

    // Map the region bounded by -0.5*view_width < x < 0.5*view_width,
    // -0.5*view_width < y < 0.5*view_width, hither < z < yon 
    // to the ranges 0 < xproj < screenwidth, 0 < yproj < screenwidth,
    // minz < zproj < maxz 
    // Objects far from the lookfm point have z values near minz!
    data[0]= data[5]= 0.5*(double)screenwidth/(double)view_width;
    data[3]= 0.5*screenx;
    data[7]= 0.5*screeny;
    data[10]= ((double)zrange)/(hither-yon);
    data[11]= ((minz*hither)-(maxz*yon))/(hither-yon);
    data[15]= 1.0;
  }
  else { 
    // Perspective projection:
    // Map the region within the frustrum of vision to the ranges
    // 0 < xproj < screenwidth, 0 < yproj < screenwidth, minz < zproj < maxz
    // Objects far from the lookfm point have z values near minz!
    data[0]= data[5]= 0.5*(double)screenwidth / tan( 0.5*DegtoRad*fovea );
    data[2]= -0.5*screenx;
    data[6]= -0.5*screeny;
    data[10]= (((double)zrange)/(1.0 - (hither/yon))) - maxz;
    data[11]= ((-hither)*((double)zrange))/(1.0 - (hither/yon));
    data[14]= -1.0;
  }
  gTransfm* perspec= new gTransfm( data );

  *align= *perspec * *urot * *rot * *align;
  
  delete rot; 
  delete urot;
  delete perspec;

  return align;
}
