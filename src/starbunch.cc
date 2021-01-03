/****************************************************************************
 * starbunch.cc
 * Author Joel Welling
 * Copyright 1996, 2008, Pittsburgh Supercomputing Center,
 *                       Carnegie Mellon University
 *
 * Permission to use, copy, and modify this software and its documentation
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
#include <string.h>

#include "geometry.h"
#include "starbunch.h"

//////////////////////////////////////////////////////////
// Notes-
// -I could make a copy constructor (python clone function) out of
//  clone_empty() and add_stars().  Do I want to do that?
// -Add some assert()'s to the .h file where we index into prop table,
//  to make sure prop and star indices are valid?
// -ints are being used for rec number in propTable; we may need
//  something larger.
// -We need the Python equivalent of the Tcl documentation.
// -All of the scripts should produce 'pretty' pictures.
// -is USE_LOG implemented for color map properties?
//////////////////////////////////////////////////////////

/* Must match StarBunch::AttributeID */
static const char* attributeIDNames[]= { 
  "ColorAlg", 
  "ColorProperty1",
  "ColorProperty2",
  "ColorProperty1UseLog",
  "ColorProperty2UseLog",
  "DebugLevel",
  (const char*)NULL
};

/* Must match StarBunch::ColorAlgType */
static const char* colorAlgTypeNames[]= {
  "ColorConstant",
  "Colormap1D",
  "Colormap2D",
  (const char*)NULL
};

/* Space in the property table will be allocated in units of these-
 * must hold a double.
 */
#define PROPSIZE sizeof(double)

StarBunchCMap::StarBunchCMap( const gColor* data_in, 
		    const int xdim_in, const int ydim_in,
		    const double minX_in, const double maxX_in, 
		    const double minY_in, const double maxY_in)
{
  xdim= xdim_in;
  ydim= ydim_in;
  minX= minX_in;
  minY= minY_in;
  maxX= maxX_in;
  maxY= maxY_in;
  data= new gColor[xdim*ydim];
  if (!data) {
    fprintf(stderr,"Unable to allocate space for %ld colors!\n",
	    (long)xdim*(long)ydim);
    exit(-1);
  }
  for (int i=0; i<xdim*ydim; i++) data[i]= data_in[i];
}

StarBunchCMap::StarBunchCMap( const StarBunchCMap& other )
{
  xdim= other.xdim;
  ydim= other.ydim;
  minX= other.minX;
  minY= other.minY;
  maxX= other.maxX;
  maxY= other.maxY;
  data= new gColor[xdim*ydim];
  if (!data) {
    fprintf(stderr,"Unable to allocate space for %ld colors!\n",
	    (long)xdim*(long)ydim);
    exit(-1);
  }
  for (int i=0; i<xdim*ydim; i++) data[i]= other.data[i];
}

StarBunchCMap::~StarBunchCMap()
{
  delete [] data;
}

gColor StarBunchCMap::lookup( double xScaled, double yScaled )
{
  if (xScaled<0.0) xScaled= 0.0;
  if (xScaled>xdim-1) xScaled= xdim-1;
  if (yScaled<0.0) yScaled= 0.0;
  if (yScaled>ydim-1) yScaled= ydim-1;

  double xt= trunc(xScaled);
  int xLow= (int)xt;
  double xShift= xScaled-xt;

  if (yScaled==0.0) {
    gColor low= data[xLow];
    if (xLow==xdim-1) return low;
    else {
      gColor hi= data[xLow+1];
      return ((low*(1.0-xShift)) + (hi*xShift));
    }
  }
  else {
    double yt= trunc(yScaled);
    int yLow= (int)yt;
    double yShift= yScaled-yt;
    int lowlowLoc= yLow*xdim + xLow;
    int hilowLoc= lowlowLoc;
    if (xLow!=xdim-1) hilowLoc++;
    int lowhiLoc= lowlowLoc;
    if (yLow!=ydim-1) lowhiLoc += xdim;
    int hihiLoc= lowhiLoc;
    if (lowlowLoc != hilowLoc) hihiLoc++;
    gColor lowlow= data[lowlowLoc];
    gColor hilow= data[hilowLoc];
    gColor lowhi= data[lowhiLoc];
    gColor hihi= data[hihiLoc];
    return ( (((lowlow*(1.0-xShift))+hilow*xShift)*(1.0-yShift))
	     + (((lowhi*(1.0-xShift))+hihi*xShift)*yShift) );
  }
}

gColor StarBunchCMap::map( double x )
{
  double xScaled= ((x-minX)*(xdim-1))/(maxX-minX);
  return lookup(xScaled,0.0);
}

gColor StarBunchCMap::map( double x, double y )
{
  double xScaled= ((x-minX)*(xdim-1))/(maxX-minX);
  double yScaled= ((x-minY)*(ydim-1))/(maxY-minY);
  return lookup(xScaled,yScaled);
}

/* I am going to hell for this non-reentrant sort implementation */
StarBunch* StarBunch::bunchBeingSorted= NULL;
int StarBunch::sortingPropIndex= 0;

const char* StarBunch::PER_PARTICLE_DENSITIES_PROP_NAME= 
  "unscaledPerParticleDensities";
const char* StarBunch::PER_PARTICLE_SQRT_EXP_CONSTANTS_PROP_NAME=
  "unscaledPerParticleSqrtExpConstants";
const char* StarBunch::ID_PROP_NAME= "particleID";
const char* StarBunch::VALID_PROP_NAME= "particleRecValid";
const char* StarBunch::VEL_X_NAME= "Velocity_x";
const char* StarBunch::VEL_Y_NAME= "Velocity_y";
const char* StarBunch::VEL_Z_NAME= "Velocity_z";

StarBunch::StarBunch( const int nstars_in )
{
  // We're storing longs and doubles in the same field size, PROPSIZE.
  assert(sizeof(double)<=PROPSIZE);
  assert(sizeof(long)<=PROPSIZE);

  // clear attributes first since debugging level is an attribute
  // and we don't want debugging during the constructor.
  for (int i=0; i<ATTRIBUTE_LAST; i++) bunch_attributes[i]= 0;

  num_stars= num_props= num_proptable_recs= 0;
  propTable= NULL;
  propNameTable= NULL;
  resize_property_table(nstars_in,0);

  id_index= -1;                     // invalid index means it doesn't exist
  valid_index= -1;                  // invalid index means it doesn't exist
  per_part_densities_index= -1;     // invalid index means it doesn't exist
  per_part_sqrt_exp_constants_index= -1; // invalid means it doesn't exist
  invalid_count= 0;
  densityval= 1.0;
  time_val= 0.0;
  z_val= 0.0;
  a_val= 0.0;
  sqrt_exponent_constant= 1.0;
  cmap1D= NULL;
  cmap2D= NULL;
  bbox= NULL;
}

StarBunch::~StarBunch()
{
  delete [] propTable;
  if (propNameTable) {
    for (int i=0; i<num_props; i++) delete propNameTable[i];
    delete [] propNameTable;
  }
  delete bbox;
}

StarBunch* StarBunch::clone_empty() const
{
  StarBunch* sb= new StarBunch(0);
  
  for (int i=0; i<ATTRIBUTE_LAST; i++) 
    sb->set_attr(i, attr(i));

  sb->set_time( time() );
  sb->set_density( density() );
  sb->set_exp_constant( exp_constant() );
  sb->set_bunch_color( bunch_color() );

  sb->set_nprops(nprops());
  for (int i=0; i<nprops(); i++) {
    sb->set_propName( i, propName(i) );
  }

  sb->per_part_densities_index= per_part_densities_index;
  sb->per_part_sqrt_exp_constants_index= per_part_sqrt_exp_constants_index;
  sb->id_index= id_index;
  sb->valid_index= valid_index;
  sb->invalid_count= invalid_count;

  if (cmap1D) {
    sb->cmap1D= new StarBunchCMap( *cmap1D );
  }
  if (cmap2D) {
    sb->cmap2D= new StarBunchCMap( *cmap2D );
  }

  return sb;
}

void StarBunch::resize_property_table(const int newNStars, const int newNProps)
{
  /////////////////
  // Policies:
  // -if nprops has grown, mark new props empty and initialize to zero
  // -if nprops has shrunk, clear the property name list.  This is
  //  to minimize ambiguity; remember that the caller may be saving
  //  property indices.
  // -if nprops is unchanged, don't change property name list
  // -table parts beyond current num stars are initialized to zero
  // -if the new size holds fewer stars than the old, the star set is
  //  just truncated.
  /////////////////

  if (debugLevel())
    fprintf(stderr,
	    "Resizing property table; recs=%d of %d -> %d, nprops %d -> %d\n",
	    num_stars,num_proptable_recs,newNStars,num_props,newNProps);

  int newRecSize= sizeof(gPoint)+newNProps*PROPSIZE;
  if (newNProps<num_props) {
    // Shrink the property table and clear the property name list
    // We are reallocating the table anyway, so we might as well
    // set the number of records to match the new requested nstars
    char* newPropTable= new char[newNStars*newRecSize];
    if (!newPropTable) {
      fprintf(stderr,"Unable to allocate %d bytes!\n",newNStars*newRecSize);
      exit(-1);
    }

    int limStars= (newNStars<num_stars ? newNStars:num_stars);
    char* here= newPropTable;
    for (int i=0; i<limStars; i++) {
      *(gPoint*)here= coords(i);
      bzero(here+sizeof(gPoint),newNProps*PROPSIZE);
      here += newRecSize;
    }
    if (limStars<newNStars) bzero(here,(newNStars-limStars)*newRecSize);

    delete [] propTable;
    propTable= newPropTable;
    num_stars= num_proptable_recs= newNStars;

    char** newNameTable= new char*[newNProps];
    if (!newNameTable) {
      fprintf(stderr,"Unable to allocate %d char*'s!\n",
	      newNProps);
      exit(-1);
    }
    for (int i=0; i<newNProps; i++) newNameTable[i]= NULL;
    for (int i=0; i<num_props; i++) delete [] propNameTable[i];
    delete [] propNameTable;
    propNameTable= newNameTable;
    num_props= newNProps;

  }
  else if (newNProps==num_props) {
    // Leave number of properties unchanged
    assert(newRecSize=propRecSize());
    if (newNStars<=num_proptable_recs) {
      // Just reset num_stars to match newNStars; leave rest of recs idle
      if (newNStars>num_stars) {
	// Newly active recs have to be zeroed out.
	bzero(propTable+num_stars*newRecSize,
	      (newNStars-num_stars)*newRecSize);
      }
      num_stars= newNStars;
    }
    else {
      // Grow the property table, copying old active recs and filling
      // the rest with zeros.
      char* newPropTable= new char[newNStars*newRecSize];
      if (!newPropTable) {
	fprintf(stderr,"Unable to allocate %d bytes!\n",newNStars*newRecSize);
	exit(-1);
      }
      memcpy(newPropTable,propTable,num_stars*newRecSize);
      bzero(newPropTable+num_stars*newRecSize,
	    (newNStars-num_stars)*newRecSize);
      delete [] propTable;
      propTable= newPropTable;
      num_stars= num_proptable_recs= newNStars;
    }
  }
  else {
    // newNProps > num_props
    // Add new empty properties (initialized to zero) 
    // We are reallocating the table anyway, so we might as well
    // set the number of records to match the new requested nstars
    char* newPropTable= new char[newNStars*newRecSize];
    if (!newPropTable) {
      fprintf(stderr,"Unable to allocate %d bytes!\n",newNStars*newRecSize);
      exit(-1);
    }

    int limStars= (newNStars<num_stars ? newNStars:num_stars);
    char* here= newPropTable;
    for (int i=0; i<limStars; i++) {
      memcpy(here, propRecPtr(i), propRecSize());
      bzero(here+propRecSize(), (newNProps-num_props)*PROPSIZE);
      here += newRecSize;
    }
    if (limStars<newNStars) bzero(here,(newNStars-limStars)*newRecSize);
    delete [] propTable;
    propTable= newPropTable;
    num_stars= num_proptable_recs= newNStars;

    // Recreate the property name list with added blank slots
    char** newNameTable= new char*[newNProps];
    if (!newNameTable) {
      fprintf(stderr,"Unable to allocate %d char*'s!\n",
	      newNProps);
      exit(-1);
    }
    for (int i=0; i<num_props; i++) {
      newNameTable[i]= propNameTable[i];
      propNameTable[i]= NULL;
    }
    for (int i=num_props; i<newNProps; i++) newNameTable[i]= NULL;
    delete [] propNameTable;
    propNameTable= newNameTable;
    num_props= newNProps;
  }
}

void StarBunch::updateBoundBox()
{
  delete bbox;
  double xmin= 0.0;
  double xmax= 0.0;
  double xave= 0.0;
  double ymin= 0.0;
  double ymax= 0.0;
  double yave= 0.0;
  double zmin= 0.0;
  double zmax= 0.0;
  double zave= 0.0;
  if (num_stars) {
    xmin= xmax= xave= coords(0).x();
    ymin= ymax= yave= coords(0).y();
    zmin= zmax= zave= coords(0).z();
    for (int i=1; i<num_stars; i++) {
      if (coords(i).x() < xmin) xmin= coords(i).x();
      if (coords(i).x() > xmax) xmax= coords(i).x();
      if (coords(i).y() < ymin) ymin= coords(i).y();
      if (coords(i).y() > ymax) ymax= coords(i).y();
      if (coords(i).z() < zmin) zmin= coords(i).z();
      if (coords(i).z() > zmax) zmax= coords(i).z();
    }
  }
  bbox= new gBoundBox(xmin,ymin,zmin,xmax,ymax,zmax);
}

void StarBunch::create_id_storage()
{
  if (!has_ids()) 
    id_index= allocate_next_free_prop_index(ID_PROP_NAME);
}

void StarBunch::create_valid_storage()
{
  if (!has_valids()) 
    valid_index= allocate_next_free_prop_index(VALID_PROP_NAME);
  for (int i=0; i<nstars(); i++) *longPropPtr(i,valid_index)= 1;
  invalid_count= 0;
}

void StarBunch::create_per_part_density_storage()
{
  if (!has_per_part_densities()) 
    per_part_densities_index= 
      allocate_next_free_prop_index(PER_PARTICLE_DENSITIES_PROP_NAME);
}

void StarBunch::create_per_part_sqrt_exp_constant_storage()
{
  if (!has_per_part_exp_constants())
    per_part_sqrt_exp_constants_index= 
      allocate_next_free_prop_index(PER_PARTICLE_SQRT_EXP_CONSTANTS_PROP_NAME);
}


void StarBunch::set_attr( const int whichAttr, const int val )
{
  if (debugLevel())
    fprintf(stderr,"set_attr %s <- %d\n",attributeIDNames[whichAttr],val);
  switch (whichAttr) {
  case COLOR_PROP1:
  case COLOR_PROP2:
    bunch_attributes[whichAttr]= val;
    if (nprops()<=val)
      resize_property_table( nstars(), val+1 ); // make sure there is room
    break;
  case COLOR_ALG: // No special treatment needed
  default:
    if (whichAttr<ATTRIBUTE_LAST) bunch_attributes[whichAttr]= val;
  }
}

void StarBunch::set_nprops( const int nprops_in )
{
  resize_property_table(nstars(), nprops_in);
}

void StarBunch::set_nstars( const int nstars_in )
{
  resize_property_table(nstars_in, nprops());
}

void StarBunch::set_propName( const int iProp, const char* name )
{
  if (iProp>=0 && iProp<num_props) { 
    if (iProp==per_part_densities_index) per_part_densities_index= -1;
    if (iProp==per_part_sqrt_exp_constants_index) 
      per_part_sqrt_exp_constants_index= -1;
    if (iProp==id_index) id_index= -1;
    if (iProp==valid_index) valid_index= -1;
    if (propNameTable[iProp])
      delete [] propNameTable[iProp];
    
    if (name) propNameTable[iProp]= strdup(name);
    else propNameTable[iProp]= NULL;
  }
  else {
    fprintf(stderr,
	    "Attempted to set name of property %d, which does not exist!\n",
	    iProp);
  }
}

const char* StarBunch::propName( const int iProp ) const
{
  if (iProp>=0 && iProp<num_props) return propNameTable[iProp];
  else return NULL;
}

void StarBunch::set_colormap1D(const gColor* colors, const int xdim,
		      const double min, const double max)
{
  delete cmap1D;
  cmap1D= new StarBunchCMap(colors, xdim, 1, min, max, 0.0, 0.0);
}

  // x dimension varies fastest!
void StarBunch::set_colormap2D(const gColor* colors, 
			       const int xdim, const int ydim,
			       const double minX, const double maxX,
			       const double minY, const double maxY)
{
  delete cmap2D;
  cmap2D= new StarBunchCMap(colors, xdim, ydim, minX, maxX, minY, maxY);
}



void StarBunch::dump( FILE* ofile, const int dump_coords )
{
  double xave= 0.0;
  double yave= 0.0;
  double zave= 0.0;
  gBoundBox lclBox= boundBox();
  fprintf(ofile,"Star bunch:\n");
  fprintf(ofile,
	  "  %d stars, time= %g, z= %g, a= %g, bunch color (%g %g %g %g)\n",
	  num_stars, time_val, z_val, a_val,
	  bunchClr.r(), bunchClr.g(), bunchClr.b(), bunchClr.a());
  for (int iSet=0; iSet<ATTRIBUTE_LAST; iSet++) {
    if (iSet==COLOR_ALG)
      fprintf(ofile,"  %s: %s\n",attributeIDNames[iSet],
	      colorAlgTypeNames[bunch_attributes[iSet]]);
    else
      fprintf(ofile,"  %s: %d\n",attributeIDNames[iSet],
	      bunch_attributes[iSet]);
  }
  if (num_stars) {
    xave= coords(0).x();
    yave= coords(0).y();
    zave= coords(0).z();
    for (int i=1; i<num_stars; i++) {
      xave += coords(i).x();
      yave += coords(i).y();
      zave += coords(i).z();
    }
    xave /= (double)num_stars;
    yave /= (double)num_stars;
    zave /= (double)num_stars;
    fprintf(ofile,"  %g <= x <= %g; average %g\n", 
	    lclBox.xmin(), lclBox.xmax(), xave);
    fprintf(ofile,"  %g <= y <= %g; average %g\n", 
	    lclBox.ymin(), lclBox.ymax(), yave);
    fprintf(ofile,"  %g <= z <= %g; average %g\n", 
	    lclBox.zmin(), lclBox.zmax(), zave);
  }
  fprintf(ofile,"  density (scale factor if per particle) %g\n", density());
  fprintf(ofile,"  exponent constant (scale factor if per particle) %g\n",
	  exp_constant());

  if (num_stars && num_props) {
    for (int iProp=0; iProp<num_props; iProp++) {
      if (iProp==valid_index) {
	long min= valid(0);
	long max= valid(0);
	for (int i=1; i<nstars(); i++) {
	  if (valid(i)<min) min=valid(i);
	  if (valid(i)>max) max=valid(i);
	}
	fprintf(ofile,"  Property %d: %ld <= %s <= %ld\n",
		iProp,min,
		(propNameTable[iProp]?propNameTable[iProp]:"unnamed"),
		max);
      }
      else if (iProp==id_index) {
	long min= id(0);
	long max= id(0);
	for (int i=1; i<nstars(); i++) {
	  if (id(i)<min) min=id(i);
	  if (id(i)>max) max=id(i);
	}
	fprintf(ofile,"  Property %d: %ld <= %s <= %ld\n",
		iProp,min,
		(propNameTable[iProp]?propNameTable[iProp]:"unnamed"),
		max);
      }
      else {
	double min= prop(0,iProp);
	double max= prop(0,iProp);
	double ave= prop(0,iProp);
	for (int i=1; i<num_stars; i++) {
	  ave += prop(i,iProp);
	  if (prop(i,iProp)<min) min=prop(i,iProp);
	  if (prop(i,iProp)>max) max=prop(i,iProp);
	}
	fprintf(ofile,"  Property %d: %g <= %s <= %g; average %g\n",
		iProp,min,
		(propNameTable[iProp]?propNameTable[iProp]:"unnamed"),
		max,ave/num_stars);
      }
    }
  }

  if (dump_coords && num_stars) {
    fprintf(ofile,"  coords, densities, exp_constants, colors follow:\n");
    for (int i=0; i<num_stars; i++) {
      gPoint loc= coords(i);
      gColor color= clr(i);
      fprintf(ofile,"  %d: (%g %g %g), %g, %g,\n         (%g %g %g %g)\n",
	      i,loc.x(),loc.y(),loc.z(),
	      density(i),exp_constant(i),
	      color.r(),color.g(),color.b(),color.a());
    }
  }
}

int StarBunch::load_ascii_xyzxyz( FILE* infile ) 
{
  double x, y, z;

  for (int i=0; i<num_stars; i++) {
    if (fscanf(infile,"%lg %lg %lg",&x,&y,&z) != 3) {
      fprintf(stderr,
	"StarBunch::load_ascii_xyzxyz: error reading %d'th of %d coord triples\n",
	      i, num_stars);
      return 0;
    }
    set_coords(i,gPoint(x,y,z));
  }

  return 1;
}

int StarBunch::load_raw_xyzxyz( FILE* infile )
{
  const int chunksize= 1024;
  float inbuf[3*chunksize];

  int n_to_go= nstars();
  int n_chunk= (nstars() > chunksize) ? chunksize : nstars();
  int index_base= 0;
  while (n_to_go) {
    if (fread(inbuf, 3*n_chunk*sizeof(float), 1, infile) != 1) {
      fprintf(stderr,"Error reading chunk from raw coord file!\n");
      return 0;
    }
    float* runner= inbuf;
    for (int i=0; i<n_chunk; i++) {
      set_coords(index_base+i, gPoint(*runner,*(runner+1),*(runner+2)));
      runner += 3;
    }
    index_base += n_chunk;
    n_to_go -= n_chunk;
    n_chunk= (n_to_go > chunksize) ? chunksize : n_to_go;
  }
  return 1;
}

int StarBunch::load_ascii_xyzdkxyzdk( FILE* infile ) 
{
  double x, y, z, den, exp_const;

  for (int i=0; i<num_stars; i++) {
    if (fscanf(infile,"%lg %lg %lg %lg %lg",&x,&y,&z,&den,&exp_const) != 5) {
      fprintf(stderr,
	      "StarBunch::load_ascii: error reading %d'th of %d coord-d-k tuples\n",
	      i, num_stars);
      return 0;
    }
    set_coords(i, gPoint(x,y,z));
    set_density(i,den);
    set_exp_constant(i,exp_const);
  }

  return 1;
}

int StarBunch::load_raw_xyzdkxyzdk( FILE* infile )
{
  const int chunksize= 1024;
  float inbuf[5*chunksize];

  int n_to_go= nstars();
  int n_chunk= (nstars() > chunksize) ? chunksize : nstars();
  int index_base= 0;
  while (n_to_go) {
    if (fread(inbuf, 5*n_chunk*sizeof(float), 1, infile) != 1) {
      fprintf(stderr,"Error reading chunk from raw coord file!\n");
      return 0;
    }
    float* runner= inbuf;
    for (int i=0; i<n_chunk; i++) {
      set_coords(index_base+i, gPoint(*runner,*(runner+1),*(runner+2)));
      runner += 3;
      set_density(index_base+i,*runner++);
      set_exp_constant(index_base+i,*runner++);
    }
    index_base += n_chunk;
    n_to_go -= n_chunk;
    n_chunk= (n_to_go > chunksize) ? chunksize : n_to_go;
  }
  return 1;
}

void StarBunch::crop( const gPoint pt, const gVector dir )
{
  int survivorSlot= 0;
  if (debugLevel())
    fprintf(stderr,"Cropping against (%g,%g,%g)(%g,%g,%g)\n",
	    pt.x()/pt.w(),pt.y()/pt.w(),pt.z()/pt.w(),
	    dir.x()/dir.w(),dir.y()/dir.w(),dir.z()/dir.w());
  // Pack survivors to the bottom of the propTable, then truncate
  // the table.
  for (int i=0; i<nstars(); i++) {
    if ((coords(i)-pt)*dir>=0.0)
      memcpy(propRecPtr(survivorSlot++), propRecPtr(i), propRecSize());
  }
  resize_property_table(survivorSlot,nprops());
  if (debugLevel()) {
    fprintf(stderr,"%d of %d survive\n",survivorSlot,nstars());
    fprintf(stderr,"Completed crop\n");
  }
}

int StarBunch_compareProp( const void* p1, const void* p2 )
{
  StarBunch* sb= StarBunch::bunchBeingSorted;
  int iProp= StarBunch::sortingPropIndex;
  int index1= ((char*)p1 - sb->propTable)/sb->propRecSize();
  int index2= ((char*)p2 - sb->propTable)/sb->propRecSize();
  if (iProp==sb->valid_index) {
    if (sb->valid(index1)<sb->valid(index2)) return -1;
    else if (sb->valid(index1)>sb->valid(index2)) return 1;
    else return 0;
  }
  else if (iProp==sb->id_index) {
    if (sb->id(index1)<sb->id(index2)) return -1;
    else if (sb->id(index1)>sb->id(index2)) return 1;
    else return 0;
  }
  else {
    if (sb->prop(index1,iProp)<sb->prop(index2,iProp)) return -1;
    else if (sb->prop(index1,iProp)>sb->prop(index2,iProp)) return 1;
    else return 0;
  }
}

int StarBunch::sort_ascending_by_prop( const int iProp )
{
  if (nstars()<2) return 1;
  if (iProp<0 || iProp>=nprops()) {
    if (debugLevel()) 
      fprintf(stderr,"Cannot sort on prop %d; index out of range\n",iProp);
    return 0;
  }
  assert(StarBunch::bunchBeingSorted==NULL);
  StarBunch::bunchBeingSorted= this;
  StarBunch::sortingPropIndex= iProp;
  qsort(propTable,nstars(),propRecSize(),StarBunch_compareProp);
  StarBunch::bunchBeingSorted= NULL;
  return 1;
}

int StarBunch::sort_ascending_by_id()
{
  if (nstars()<2) return 1;
  if (!has_ids()) {
    if (debugLevel())
      fprintf(stderr,"Cannot sort; bunch has no ids!\n");
    return 0;
  }
  if (has_ids()) return sort_ascending_by_prop(id_index);
  else return 0;
}

int StarBunch::allocate_next_free_prop_index(const char* name)
{
  int retval= -1;

  // If it's already allocated, just use the existing value.
  // It's an error to have two props of the same name, so
  // this is safest.
  int existingID= get_prop_index_by_name(name);
  if (existingID>=0) retval= existingID;
  else {

    // If there is a blank field, use it.
    for (int i=0; i<num_props; i++)
      if (propNameTable[i]==NULL) {
	set_propName(i,name);
	retval= i;
	break;
      }
    
    // Current table is full; we must grow it.
    if (retval<0) { 
      int oldNProps= nprops();
      set_nprops(oldNProps+1);
      set_propName(oldNProps,name);
      retval= oldNProps;
    }
  }
    
  // Certain privileged names get their associated IDs cached
  if (!strcmp(name,ID_PROP_NAME)) id_index= retval;
  else if (!strcmp(name,VALID_PROP_NAME)) valid_index= retval;
  else if (!strcmp(name,PER_PARTICLE_DENSITIES_PROP_NAME))
    per_part_densities_index= retval;
  else if (!strcmp(name,PER_PARTICLE_SQRT_EXP_CONSTANTS_PROP_NAME))
    per_part_sqrt_exp_constants_index= retval;

  return retval;
}

void StarBunch::deallocate_prop_index(const int iProp)
{
  set_propName( iProp, NULL );
}

int StarBunch::copy_stars(const StarBunch* src)
{
  // Bail early if there are no stars to copy
  if (src->nstars()==0) return 1;

  // Check that source has the needed property fields
  int* propMap= new int[nprops()];
  for (int i=0; i<nprops(); i++) {
    const char* nm= propName(i);
    if (nm==NULL) propMap[i]= -1;
    else {
      int found= 0;
      for (int j=0; j<src->nprops(); j++) {
	const char* onm= src->propName(j);
	if (onm && !strcmp(nm,onm)) {
	  propMap[i]= j;
	  found= 1;
	  break;
	}
      }
      if (!found) {
	fprintf(stderr,"add_stars: source lacks property <%s>\n",nm);
	return 0;
      }
    }
  }
  // Copy in the stars 
  int offset= nstars();
  set_nstars(nstars() + src->nstars());
  for (int i=0; i<src->nstars(); i++) {
    set_coords( i+offset, src->coords(i) );
    for (int j=0; j<nprops(); j++) 
      if (propMap[j]!=-1) 
	memcpy( propPtr(i+offset,j), src->propPtr(i,propMap[j]), PROPSIZE );
  }
  return 1;
}

int StarBunch::fill_invalid_from( StarBunch* src, int sorted, int src_sorted )
{
  if (!ninvalid()) return 1; // Nothing to do

  // Check that source has the needed property fields
  int* propMap= new int[nprops()];
  for (int i=0; i<nprops(); i++) {
    const char* nm= propName(i);
    if (nm==NULL) propMap[i]= -1;
    else {
      int found= 0;
      for (int j=0; i<src->nprops(); j++) {
	const char* onm= src->propName(j);
	if (onm && !strcmp(nm,onm)) {
	  propMap[i]= j;
	  found= 1;
	  break;
	}
      }
      if (!found) {
	fprintf(stderr,"fill_invalid_from: source lacks property <%s>\n",nm);
	return 0;
      }
    }
  }

  if (!(sorted || sort_ascending_by_id())) return 0;
  if (!(src_sorted || src->sort_ascending_by_id())) return 0;
  
  int offset1= 0;
  int offset2= 0;
  int changes= 0;
  while (offset1<nstars() && offset2<src->nstars()) {
    unsigned int id1= id(offset1);
    unsigned int id2= src->id(offset2);
    if (id1==id2) {
      if (!valid(offset1) && src->valid(offset2)) {
	set_coords( offset1, src->coords(offset2) );
	for (int j=0; j<nprops(); j++) 
	  if (propMap[j]!=-1)
	    memcpy( propPtr(offset1,j), src->propPtr(offset2,propMap[j]),
		    PROPSIZE );
	changes++;
      }
      offset1++;
      offset2++;
    }
    else if (id1<id2) {
      offset1++;
    }
    else {
      offset2++;
    }
  }

  invalid_count -= changes;
  return 1;
}

int StarBunch::get_prop_index_by_name( const char* name ) const
{
  for (int i=0; i<nprops(); i++)
    if ( propName(i) && !strcmp(propName(i),name) ) return i;
  return -1;
}

void StarBunch::wrap_periodic(const gBoundBox& newWorldBBox)
{
  for (int i=0; i<nstars(); i++)
    set_coords(i, newWorldBBox.wrap(coords(i)));

  // Signal that boundbox is invalid
  delete bbox;
  bbox= NULL;
}
