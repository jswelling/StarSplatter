/****************************************************************************
 * interpolate.cc
 * Author Joel Welling
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
#include <assert.h>
#include <string>
#include <set>
#include <algorithm>

#include "starsplatter.h"

// to allow for rounding error
#define EPSILON 0.000001

#define IN_SET(set, charptr) (set.find(charptr)!=set.end())
#define LINEAR( v1, v2, a) (a*v2 + (1.0-a)*v1)
#define HERMITE( v1, dv1, v2, dv2, a ) \
  (((a*a*(2.0*a - 3.0) + 1.0)*v1) + (a*(a*(a - 2.0) + 1.0)*dv1)	\
   + ((-2.0*a + 3.0)*a*a*v2) + (a*a*(a - 1.0)*dv2))
#define HERMITEDERIV( v1, dv1, v2, dv2, a ) \
  ((6.0*a*(a - 1.0)*v1) + ((a*(3.0*a - 4.0) + 1.0)*dv1)	\
   + (-6.0*a*(a - 1.0)*v2) + (a*(3.0*a - 2.0)*dv2))
#define XPLUSVT(x,v,t) ( x + (v)*(t) )

typedef enum { INTRP_SKIP, INTRP_ZERO, INTRP_LINEAR } InterpMode;

int ssplat_identify_unshared_ids(StarBunch* sb1, StarBunch* sb2)
{
  if (!sb1->sort_ascending_by_id()) return 0;
  if (!sb2->sort_ascending_by_id()) return 0;

  // Count unique IDs
  int nUniqueStars= 0;
  int offset1= 0;
  int offset2= 0;
  while (offset1<sb1->nstars() && offset2<sb2->nstars()) {
    unsigned int id1= sb1->id(offset1);
    unsigned int id2= sb2->id(offset2);
    if (id1==id2) {
      nUniqueStars++;
      offset1++;
      offset2++;
    }
    else if (id1<id2) {
      nUniqueStars++;
      offset1++;
    }
    else {
      nUniqueStars++;
      offset2++;
    }
  }
  if (offset1<sb1->nstars()) nUniqueStars += (sb1->nstars()-offset1);
  else if (offset2<sb2->nstars()) nUniqueStars += (sb2->nstars()-offset2);
  
  // I can now use sb->set_nstars() to create space for additional stars,
  // sb->allocate_next_free_prop_index() to make space for a 'valid' flag,
  // and a merge sort to find missing IDs.
  int newStart1= sb1->nstars();
  int newStart2= sb2->nstars();
  sb1->set_nstars(nUniqueStars);
  sb2->set_nstars(nUniqueStars);
  for (int i=0; i<newStart1; i++) sb1->set_valid(i,1);
  for (int i=0; i<newStart2; i++) sb2->set_valid(i,1);
  int here1= newStart1;
  int here2= newStart2;
  offset1= 0;
  offset2= 0;
  while (offset1<newStart1 && offset2<newStart2) {
    unsigned int id1= sb1->id(offset1);
    unsigned int id2= sb2->id(offset2);
    if (id1==id2) {
      offset1++;
      offset2++;
    }
    else if (id1<id2) {
      sb2->set_id(here2++,id1);
      offset1++;
    }
    else {
      sb1->set_id(here1++,id2);
      offset2++;
    }
  }
  if (offset1<newStart1) {
    for (; offset1<newStart1; offset1++) 
      sb2->set_id(here2++,sb1->id(offset1));
  }
  else if (offset2<newStart2) {
    for (; offset2<newStart2; offset2++) 
      sb1->set_id(here1++,sb2->id(offset2));
  }
  for (int i=newStart1; i<sb1->nstars(); i++) sb1->set_valid(i,0);
  sb1->invalid_count= sb1->nstars()-newStart1;
  for (int i=newStart2; i<sb2->nstars(); i++) sb2->set_valid(i,0);
  sb2->invalid_count= sb2->nstars()-newStart2;

  return 1;
}

static inline void interpolate_one_star_simple(StarBunch* nb, 
					       const StarBunch* sb1, 
					       const StarBunch* sb2, 
					       int iStar, 
					       InterpMode* interpModeTable,
					       int* sb1_propIndexTable, 
					       int* sb2_propIndexTable,
					       int vxIndex, int vyIndex, 
					       int vzIndex, double vel_scale)
{
  // The parts of interpolation which don't depend on 
  // periodic boundary conditions

  double alpha= (nb->time()-sb1->time())/(sb2->time()-sb1->time());
  
  for (int iProp=0; iProp<nb->nprops(); iProp++) {
    switch (interpModeTable[iProp]) {
    case INTRP_SKIP:
      break;
    case INTRP_ZERO:
      nb->set_prop(iStar, iProp, 0.0);
      break;
    case INTRP_LINEAR:
      nb->set_prop(iStar, iProp,
		   LINEAR(sb1->prop(iStar, sb1_propIndexTable[iProp]),
			  sb2->prop(iStar, sb2_propIndexTable[iProp]),
			  alpha));
      break;
    }
  }
}

static inline void interpolate_one_star(StarBunch* nb, 
					const StarBunch* sb1, 
					const StarBunch* sb2, 
					int iStar, 
					InterpMode* interpModeTable,
					int* sb1_propIndexTable, 
					int* sb2_propIndexTable,
					int vxIndex, int vyIndex, int vzIndex,
					double vel_scale)
{
  double alphaScale= sb2->time()-sb1->time();
  double alpha= (nb->time()-sb1->time())/alphaScale;
  // These velocities are rate of change with respect to alpha,
  // in appropriate units
  double vx1= 
    vel_scale*alphaScale*sb1->prop(iStar,sb1_propIndexTable[vxIndex]);
  double vx2= 
    vel_scale*alphaScale*sb2->prop(iStar,sb2_propIndexTable[vxIndex]);
  double vy1= 
    vel_scale*alphaScale*sb1->prop(iStar,sb1_propIndexTable[vyIndex]);
  double vy2= 
    vel_scale*alphaScale*sb2->prop(iStar,sb2_propIndexTable[vyIndex]);
  double vz1= 
    vel_scale*alphaScale*sb1->prop(iStar,sb1_propIndexTable[vzIndex]);
  double vz2= 
    vel_scale*alphaScale*sb2->prop(iStar,sb2_propIndexTable[vzIndex]);
  
  double newX= HERMITE( sb1->coords(iStar).x(), vx1, 
			sb2->coords(iStar).x(), vx2,
			alpha );
  double newY= HERMITE( sb1->coords(iStar).y(), vy1,
			sb2->coords(iStar).y(), vy2,
			alpha );
  double newZ= HERMITE( sb1->coords(iStar).z(), vz1,
			sb2->coords(iStar).z(), vz2,
			alpha );
  nb->set_coords(iStar,gPoint((float)newX, (float)newY, (float)newZ));

  double unscaleVel= 1.0/(vel_scale*alphaScale);
  double scaledVx= HERMITEDERIV( sb1->coords(iStar).x(), vx1, 
				 sb2->coords(iStar).x(), vx2, alpha );
  nb->set_prop(iStar, vxIndex, unscaleVel*scaledVx);
  double scaledVy= HERMITEDERIV( sb1->coords(iStar).y(), vy1, 
				 sb2->coords(iStar).y(), vy2, alpha );
  nb->set_prop(iStar, vyIndex, unscaleVel*scaledVy);
  double scaledVz= HERMITEDERIV( sb1->coords(iStar).z(), vz1, 
				 sb2->coords(iStar).z(), vz2, alpha );
  nb->set_prop(iStar, vzIndex, unscaleVel*scaledVz);

  interpolate_one_star_simple( nb, sb1, sb2, iStar,
			       interpModeTable, 
			       sb1_propIndexTable, sb2_propIndexTable,
			       vxIndex, vyIndex, vzIndex, vel_scale );
}

static inline void interpolate_one_star_wrap(StarBunch* nb, 
					     const StarBunch* sb1, 
					     const StarBunch* sb2, 
					     int iStar, 
					     InterpMode* interpModeTable,
					     int* sb1_propIndexTable, 
					     int* sb2_propIndexTable,
					     int vxIndex, int vyIndex, 
					     int vzIndex,
					     const gBoundBox& worldBB,
					     double vel_scale)
{
  double alphaScale= sb2->time()-sb1->time();
  double alpha= (nb->time()-sb1->time())/alphaScale;
  // These velocities are rate of change with respect to alpha,
  // in appropriate units
  double vx1= 
    vel_scale*alphaScale*sb1->prop(iStar,sb1_propIndexTable[vxIndex]);
  double vx2= 
    vel_scale*alphaScale*sb2->prop(iStar,sb2_propIndexTable[vxIndex]);
  double vy1= 
    vel_scale*alphaScale*sb1->prop(iStar,sb1_propIndexTable[vyIndex]);
  double vy2= 
    vel_scale*alphaScale*sb2->prop(iStar,sb2_propIndexTable[vyIndex]);
  double vz1= 
    vel_scale*alphaScale*sb1->prop(iStar,sb1_propIndexTable[vzIndex]);
  double vz2= 
    vel_scale*alphaScale*sb2->prop(iStar,sb2_propIndexTable[vzIndex]);
  
  gPoint loc1= worldBB.wrap(sb1->coords(iStar));
  gPoint loc2= worldBB.wrap(sb2->coords(iStar));
  loc1= worldBB.wrap_together(loc1,loc2);
  double newX= HERMITE( loc1.x(), vx1, 
			loc2.x(), vx2,
			alpha );
  double newY= HERMITE( loc1.y(), vy1,
			loc2.y(), vy2,
			alpha );
  double newZ= HERMITE( loc1.z(), vz1,
			loc2.z(), vz2,
			alpha );
  gPoint extrapPt= gPoint((float)newX, (float)newY, (float)newZ);
  nb->set_coords(iStar,worldBB.wrap(extrapPt));

  double unscaleVel= 1.0/(vel_scale*alphaScale);
  double scaledVx= HERMITEDERIV( loc1.x(), vx1, loc2.x(), vx2, alpha );
  nb->set_prop(iStar, vxIndex, unscaleVel*scaledVx);
  double scaledVy= HERMITEDERIV( loc1.y(), vy1, loc2.y(), vy2, alpha );
  nb->set_prop(iStar, vyIndex, unscaleVel*scaledVy);
  double scaledVz= HERMITEDERIV( loc1.z(), vz1, loc2.z(), vz2, alpha );
  nb->set_prop(iStar, vzIndex, unscaleVel*scaledVz);

  interpolate_one_star_simple( nb, sb1, sb2, iStar,
			       interpModeTable, 
			       sb1_propIndexTable, sb2_propIndexTable,
			       vxIndex, vyIndex, vzIndex, vel_scale );
}

static inline void extrapolate_one_star_simple( StarBunch* nb, 
						const StarBunch* sb, 
						const StarBunch* emptySb, 
						int iStar, 
						InterpMode* interpModeTable,
						int* propIndexTable, 
						int vxIndex, int vyIndex, 
						int vzIndex,
						int opticalDensityIndex,
						double vel_scale )
{
  // The parts of extrapolation where periodic BC don't matter
  double alpha= (nb->time()-sb->time())/(emptySb->time()-sb->time());
  double vx= sb->prop(iStar,propIndexTable[vxIndex]);
  double vy= sb->prop(iStar,propIndexTable[vyIndex]);
  double vz= sb->prop(iStar,propIndexTable[vzIndex]);

  nb->set_prop(iStar, vxIndex, vx);
  nb->set_prop(iStar, vyIndex, vy);
  nb->set_prop(iStar, vzIndex, vz);

  for (int iProp=0; iProp<nb->nprops(); iProp++) {
    if (iProp==opticalDensityIndex) {
      // fade this particle
      nb->set_prop(iStar, iProp,
		   LINEAR(sb->prop(iStar,propIndexTable[iProp]),
			  0.0, alpha));
    }
    else {
      switch (interpModeTable[iProp]) {
      case INTRP_SKIP:
	break;
      case INTRP_ZERO:
	nb->set_prop(iStar, iProp, 0.0);
	break;
      case INTRP_LINEAR:
	nb->set_prop(iStar, iProp,
		     sb->prop(iStar, propIndexTable[iProp]));
      break;
      }
    }
  }
}

static inline void extrapolate_one_star(StarBunch* nb, 
					const StarBunch* sb, 
					const StarBunch* emptySb, 
					int iStar, 
					InterpMode* interpModeTable,
					int* propIndexTable, 
					int vxIndex, int vyIndex, int vzIndex,
					int opticalDensityIndex,
					double vel_scale)
{
  double dt= nb->time() - sb->time();
  // These are velocities with respect to time, not alpha, because
  // the XPLUSVT extrapolation mechanism is being used
  double vx= sb->prop(iStar,propIndexTable[vxIndex]);
  double vy= sb->prop(iStar,propIndexTable[vyIndex]);
  double vz= sb->prop(iStar,propIndexTable[vzIndex]);

  double newX= XPLUSVT( sb->coords(iStar).x(), vel_scale*vx, dt );
  double newY= XPLUSVT( sb->coords(iStar).y(), vel_scale*vy, dt );
  double newZ= XPLUSVT( sb->coords(iStar).z(), vel_scale*vz, dt );
  nb->set_coords(iStar,gPoint((float)newX, (float)newY, (float)newZ));

  extrapolate_one_star_simple( nb, sb, emptySb, iStar, 
			       interpModeTable, propIndexTable, 
			       vxIndex, vyIndex, vzIndex,
			       opticalDensityIndex, vel_scale );
}

static inline void extrapolate_one_star_wrap(StarBunch* nb, 
					     const StarBunch* sb, 
					     const StarBunch* emptySb, 
					     int iStar, 
					     InterpMode* interpModeTable,
					     int* propIndexTable, 
					     int vxIndex, int vyIndex, 
					     int vzIndex,
					     int opticalDensityIndex,
					     const gBoundBox& worldBB,
					     double vel_scale)
{
  double dt= nb->time() - sb->time();
  // These are velocities with respect to time, not alpha, because
  // the XPLUSVT extrapolation mechanism is being used
  double vx= sb->prop(iStar,propIndexTable[vxIndex]);
  double vy= sb->prop(iStar,propIndexTable[vyIndex]);
  double vz= sb->prop(iStar,propIndexTable[vzIndex]);

  gPoint wrappedPt= worldBB.wrap(sb->coords(iStar));
  double newX= XPLUSVT( wrappedPt.x(), vel_scale*vx, dt );
  double newY= XPLUSVT( wrappedPt.y(), vel_scale*vy, dt );
  double newZ= XPLUSVT( wrappedPt.z(), vel_scale*vz, dt );
  gPoint extrapPt= gPoint((float)newX, (float)newY, (float)newZ);
  nb->set_coords(iStar,worldBB.wrap(extrapPt));

  extrapolate_one_star_simple( nb, sb, emptySb, iStar, 
			       interpModeTable, propIndexTable, 
			       vxIndex, vyIndex, vzIndex,
			       opticalDensityIndex, vel_scale );
}

static int interp_inputs_valid(const StarBunch* sb1, const StarBunch* sb2, 
			       const double time)
{
  // Require that both sb's have the same number of active stars
  if (sb1->nstars() != sb2->nstars()) {
    if (sb1->debugLevel() || sb2->debugLevel())
      fprintf(stderr,"interpolate: star counts do not match\n");
    return 0;
  }
  
  // If either sb contains invalid stars, they must both contain
  // per particle densities so that the stars can be 'faded out'
  if ((sb1->ninvalid() || sb2->ninvalid())
      && !(sb1->has_per_part_densities() 
	   && sb2->has_per_part_densities())) {
    if (sb1->debugLevel() || sb2->debugLevel())
      fprintf(stderr,
	      "interpolate: input bunches contain invalid stars and do not contain per-particle densities\n");
    return 0;
  }
  
  // Require that the two sb's have different times, and that the
  // interpolation time is somewhere in between.
  if ((sb1->time() == sb2->time())
      || (time - sb1->time())*(sb2->time() - time)<=-EPSILON) {
    fprintf(stderr,"interpolate: bunch times do not bound interp time\n");
    return 0;
  }
  
  // Require that both sb's have IDs
  if (!sb1->has_ids() || !sb2->has_ids()) {
    fprintf(stderr,"interpolate: one or both StarBunch(s) lack id info\n");
    return 0;
  }
  
  return 1;
}

static void get_common_props( std::set<std::string>& commonProps,
			    const StarBunch* sb1, const StarBunch* sb2)
{
  // Find the maximal set of common properties.
  std::set<std::string> leftSet;
  std::set<std::string> rightSet;
  for (int i=0; i<sb1->nprops(); i++)
    if (sb1->propName(i)) leftSet.insert(sb1->propName(i));
  for (int i=0; i<sb2->nprops(); i++)
    if (sb2->propName(i)) rightSet.insert(sb2->propName(i));
  std::set_intersection(leftSet.begin(),leftSet.end(),
			rightSet.begin(),rightSet.end(),
			std::insert_iterator< std::set<std::string> >(commonProps,
								      commonProps.begin()));
}

static StarBunch* build_interpolation_bunch( const StarBunch* sb,
					     std::set<std::string> commonProps,
					     double time )
{
  StarBunch* newBunch= sb->clone_empty();
  newBunch->set_time(time);

  // Zero any prop which is not common
  for (int i=0; i<newBunch->nprops(); i++) {
    const char* name= newBunch->propName(i);
    if (name && !IN_SET(commonProps,name)) newBunch->deallocate_prop_index(i);
  }
  
  // Set stars to nstars
  newBunch->set_nstars(sb->nstars());
  
  return newBunch;
}

static int* build_prop_index_table( const StarBunch* newBunch,
				    const StarBunch* oldBunch,
				    std::set<std::string>& commonProps )
{
  int* propIndexTable= new int[newBunch->nprops()];

  for (int i=0; i<newBunch->nprops(); i++) {
    const char* name= newBunch->propName(i);
    if (name && IN_SET(commonProps, name))
      propIndexTable[i]= oldBunch->get_prop_index_by_name(name);
  }

  return propIndexTable;
}
				    
static InterpMode* build_interp_mode_table( const StarBunch* newBunch,
					    std::set<std::string> commonProps,
					    int vxIndex, int vyIndex,
					    int vzIndex )
{
  InterpMode* interpModeTable= new InterpMode[newBunch->nprops()];

  for (int i=0; i<newBunch->nprops(); i++) {
    const char* name= newBunch->propName(i);
    if (name && IN_SET(commonProps, name))
      interpModeTable[i]= INTRP_LINEAR;
    else 
      interpModeTable[i]= INTRP_ZERO; // these prop columns are deallocated
  }
  interpModeTable[vxIndex]= INTRP_SKIP;
  interpModeTable[vyIndex]= INTRP_SKIP;
  interpModeTable[vzIndex]= INTRP_SKIP;

  assert(newBunch->has_ids());
  interpModeTable[newBunch->get_prop_index_by_name(StarBunch::ID_PROP_NAME)]= 
    INTRP_SKIP;

  if (newBunch->has_valids()) 
    interpModeTable[newBunch->get_prop_index_by_name(StarBunch::VALID_PROP_NAME)]= INTRP_SKIP;

  return interpModeTable;
}
					    

StarBunch* ssplat_starbunch_interpolate(StarBunch* sb1, StarBunch* sb2,
					const double time,
					const double vel_scale,
					const int sb1_sorted,
					const int sb2_sorted)
{
  int i;

  // Short circuit if both bunches contain zero stars
  if (sb1->nstars()==0 && sb2->nstars()==0)
    return sb1->clone_empty();

  if (!interp_inputs_valid(sb1,sb2,time)) return NULL;

  // Find the maximal set of common properties.
  std::set<std::string> commonProps;
  get_common_props(commonProps, sb1, sb2);

  // Require that both sb's have velocity info
  if (!IN_SET(commonProps,StarBunch::VEL_X_NAME)
      || !IN_SET(commonProps,StarBunch::VEL_Y_NAME)
      || !IN_SET(commonProps,StarBunch::VEL_Z_NAME)) {
    fprintf(stderr,"interpolate: an input StarBunch lacks velocity info\n");
    return NULL;
  }
  
  // Sort if necessary
  if (!sb1_sorted) sb1->sort_ascending_by_id();
  if (!sb2_sorted) sb2->sort_ascending_by_id();
  
  // Require that they have stars with matching id's.
  for (i=0; i<sb1->nstars(); i++)
    if (sb1->id(i) != sb2->id(i)) {
      fprintf(stderr,"interpolate: id's do not match at star %d\n",i);
      return NULL;
    }
  
  // Clone sb1, and set its property list appropriately.
  StarBunch* newBunch= build_interpolation_bunch( sb1, commonProps, time );

  // Go down through the stars, setting everything by interpolation.
  // We have to build some index tables first.
  int vxIndex= newBunch->get_prop_index_by_name(StarBunch::VEL_X_NAME);
  int vyIndex= newBunch->get_prop_index_by_name(StarBunch::VEL_Y_NAME);
  int vzIndex= newBunch->get_prop_index_by_name(StarBunch::VEL_Z_NAME);
  assert( vxIndex>=0 && vyIndex>=0 && vzIndex>=0 );
  int opticalDensityIndex=
    newBunch->get_prop_index_by_name(StarBunch::PER_PARTICLE_DENSITIES_PROP_NAME);
  
  int* sb1_propIndexTable= build_prop_index_table(newBunch, sb1, commonProps);
  int* sb2_propIndexTable= build_prop_index_table(newBunch, sb2, commonProps);

  InterpMode* interpModeTable= build_interp_mode_table( newBunch,
							commonProps,
							vxIndex, vyIndex,
							vzIndex );

  // Interpolate data for each star.  The algorithm to use depends
  // on whether one or both end points is valid.
  for (i=0; i<newBunch->nstars(); i++) {
    
    newBunch->set_id(i, sb1->id(i)); 
    if (newBunch->has_valids()) 
      newBunch->set_valid(i,(sb1->valid(i) || sb2->valid(i)));

    if (sb1->valid(i)) {
      if (sb2->valid(i)) {
	interpolate_one_star(newBunch, sb1, sb2, i, 
			     interpModeTable,
			     sb1_propIndexTable, sb2_propIndexTable,
			     vxIndex, vyIndex, vzIndex, vel_scale);
      }
      else {
	// Extrapolate from left
	extrapolate_one_star(newBunch, sb1, sb2, i, 
			     interpModeTable,
			     sb1_propIndexTable, 
			     vxIndex, vyIndex, vzIndex,
			     opticalDensityIndex, vel_scale);
      }
    }
    else {
      if (sb2->valid(i)) {
	// Extrapolate from right
	extrapolate_one_star(newBunch, sb2, sb1, i, 
			     interpModeTable,
			     sb2_propIndexTable, 
			     vxIndex, vyIndex, vzIndex,
			     opticalDensityIndex, vel_scale);
      }
      else {
	// This star is invalid; make it invisible
	assert(opticalDensityIndex>=0);
	newBunch->set_prop(i, opticalDensityIndex, 0.0);
      }
    }
  }
  
  delete [] sb2_propIndexTable;
  delete [] sb1_propIndexTable;
  delete [] interpModeTable;

  
  if (sb1->debugLevel()) {
    fprintf(stderr,"Interpolated bunch follows\n");
    newBunch->dump(stderr,1);
  }
  return newBunch;
}

StarBunch* ssplat_starbunch_interpolate_periodic_bc(StarBunch* sb1, 
						    StarBunch* sb2,
						    const double time,
						    const double vel_scale,
						    const gBoundBox& worldBB,
						    const int sb1_sorted,
						    const int sb2_sorted)
{
  int i;

  // Short circuit if both bunches contain zero stars
  if (sb1->nstars()==0 && sb2->nstars()==0)
    return sb1->clone_empty();

  if (!interp_inputs_valid(sb1,sb2,time)) return NULL;

  // Find the maximal set of common properties.
  std::set<std::string> commonProps;
  get_common_props(commonProps, sb1, sb2);

  // Require that both sb's have velocity info
  if (!IN_SET(commonProps,StarBunch::VEL_X_NAME)
      || !IN_SET(commonProps,StarBunch::VEL_Y_NAME)
      || !IN_SET(commonProps,StarBunch::VEL_Z_NAME)) {
    fprintf(stderr,"interpolate: an input StarBunch lacks velocity info\n");
    return NULL;
  }
  
  // Sort if necessary
  if (!sb1_sorted) sb1->sort_ascending_by_id();
  if (!sb2_sorted) sb2->sort_ascending_by_id();
  
  // Require that they have stars with matching id's.
  for (i=0; i<sb1->nstars(); i++)
    if (sb1->id(i) != sb2->id(i)) {
      fprintf(stderr,"interpolate: id's do not match at star %d\n",i);
      return NULL;
    }
  
  // Clone sb1, and set its property list appropriately.
  StarBunch* newBunch= build_interpolation_bunch( sb1, commonProps, time );

  // Go down through the stars, setting everything by interpolation.
  // We have to build some index tables first.
  int vxIndex= newBunch->get_prop_index_by_name(StarBunch::VEL_X_NAME);
  int vyIndex= newBunch->get_prop_index_by_name(StarBunch::VEL_Y_NAME);
  int vzIndex= newBunch->get_prop_index_by_name(StarBunch::VEL_Z_NAME);
  assert( vxIndex>=0 && vyIndex>=0 && vzIndex>=0 );
  int opticalDensityIndex=
    newBunch->get_prop_index_by_name(StarBunch::PER_PARTICLE_DENSITIES_PROP_NAME);
  
  int* sb1_propIndexTable= build_prop_index_table(newBunch, sb1, commonProps);
  int* sb2_propIndexTable= build_prop_index_table(newBunch, sb2, commonProps);

  InterpMode* interpModeTable= build_interp_mode_table( newBunch,
							commonProps,
							vxIndex, vyIndex,
							vzIndex );

  // Interpolate data for each star.  The algorithm to use depends
  // on whether one or both end points is valid.
  for (i=0; i<newBunch->nstars(); i++) {
    
    newBunch->set_id(i, sb1->id(i)); 
    if (newBunch->has_valids()) 
      newBunch->set_valid(i,(sb1->valid(i) || sb2->valid(i)));

    if (sb1->valid(i)) {
      if (sb2->valid(i)) {
	interpolate_one_star_wrap(newBunch, sb1, sb2, i, 
				  interpModeTable,
				  sb1_propIndexTable, sb2_propIndexTable,
				  vxIndex, vyIndex, vzIndex,
				  worldBB, vel_scale);
      }
      else {
	// Extrapolate from left
	extrapolate_one_star_wrap(newBunch, sb1, sb2, i, 
				  interpModeTable,
				  sb1_propIndexTable, 
				  vxIndex, vyIndex, vzIndex,
				  opticalDensityIndex, worldBB, vel_scale);
      }
    }
    else {
      if (sb2->valid(i)) {
	// Extrapolate from right
	extrapolate_one_star_wrap(newBunch, sb2, sb1, i, 
				  interpModeTable,
				  sb2_propIndexTable, 
				  vxIndex, vyIndex, vzIndex,
				  opticalDensityIndex, worldBB, vel_scale);
      }
      else {
	// This star is invalid; make it invisible
	assert(opticalDensityIndex>=0);
	newBunch->set_prop(i, opticalDensityIndex, 0.0);
      }
    }
  }
  
  delete [] sb2_propIndexTable;
  delete [] sb1_propIndexTable;
  delete [] interpModeTable;

  
  if (sb1->debugLevel()) {
    fprintf(stderr,"Interpolated bunch follows\n");
    newBunch->dump(stderr,1);
  }
  return newBunch;
}

