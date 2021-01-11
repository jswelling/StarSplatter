%define DOCSTRING
"This module provides tools for rendering particles, in particular the
results of SPH numerical simulations.  The StarBunch class embodies a 
collection of particles; the StarSplatter class provides mechanisms to 
render sets of these collections.  Some other classes are provided to 
manipulate rendering geometry and rendered images."
%enddef
%module (docstring=DOCSTRING) starsplatter
%{
#include <assert.h>
#include <iostream>
#include <map>
#include <utility>
#include "starsplatter.h"
%}

%include "typemaps.i"

// Notes-
//


%feature("autodoc","1");

%typemap(in) FILE* {
  int fd = PyObject_AsFileDescriptor($input);
  $1 = fdopen(fd, "r");
}
  
class gPoint {
public:
  gPoint( float xin, float yin, float zin );
  gPoint( float xin, float yin, float zin, float win );
  gPoint();
  float x();
  float y();
  float z();
  float w();
  void homogenize();
};

%extend gPoint {
char *__str__() {
  static char tmp[256];
  sprintf(tmp,"gPoint(%g,%g,%g,%g)",self->x(),self->y(),self->z(),self->w());
  return tmp;
 }
}

%typemap(in) gPoint (double temp[4]) {   // temp[4] becomes a local variable
  if (PyTuple_Check($input)) {
    temp[3]= 1.0;
    if (PyArg_ParseTuple($input,"ddd|d",temp,temp+1,temp+2,temp+3)) {
      $1 = gPoint(temp[0],temp[1],temp[2],temp[3]);
    }
    else {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements");
      return NULL;
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple.");
    return NULL;
  }
}

%typemap(out) gPoint {
  $result= PyTuple_New(4);
  PyTuple_SetItem($result, 0, PyFloat_FromDouble($1.x()));
  PyTuple_SetItem($result, 1, PyFloat_FromDouble($1.y()));
  PyTuple_SetItem($result, 2, PyFloat_FromDouble($1.z()));
  PyTuple_SetItem($result, 3, PyFloat_FromDouble($1.w()));
}

%typecheck(SWIG_TYPECHECK_POINTER) gPoint {
  $1 = PyTuple_Check($input) ? 1 : 0;
}

class gVector {
public:
  gVector( float xin, float yin, float zin );
  gVector( float xin, float yin, float zin, float win );
  gVector();
  float x();
  float y();
  float z();
  float w();
  void homogenize();
  float lengthsqr();
  float length();
  void normalize();
};

%extend gVector {
char *__str__() {
  static char tmp[256];
  sprintf(tmp,"gVector(%g,%g,%g,%g)",self->x(),self->y(),self->z(),self->w());
  return tmp;
 }
}

%typemap(in) gVector (double temp[4]) {   // temp[4] becomes a local variable
  if (PyTuple_Check($input)) {
    temp[3]= 1.0;
    if (PyArg_ParseTuple($input,"ddd|d",temp,temp+1,temp+2,temp+3)) {
      $1 = gVector(temp[0],temp[1],temp[2],temp[3]);
    }
    else {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements");
      return NULL;
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple.");
    return NULL;
  }
}

%typemap(out) gVector {
  $result= PyTuple_New(4);
  PyTuple_SetItem($result, 0, PyFloat_FromDouble($1.x()));
  PyTuple_SetItem($result, 1, PyFloat_FromDouble($1.y()));
  PyTuple_SetItem($result, 2, PyFloat_FromDouble($1.z()));
  PyTuple_SetItem($result, 3, PyFloat_FromDouble($1.w()));
}

%typecheck(SWIG_TYPECHECK_POINTER) gVector {
  $1 = PyTuple_Check($input) ? 1 : 0;
}

%typemap(in) gVector* (double temp[4], gVector t) {   // temp[4], t becomes local variables
  if (PyTuple_Check($input)) {
    temp[3]= 1.0;
    if (PyArg_ParseTuple($input,"ddd|d",temp,temp+1,temp+2,temp+3)) {
      t= gVector(temp[0],temp[1],temp[2],temp[3]);
      $1 = &t;
    }
    else {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements");
      return NULL;
    }
  }
  else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple.");
    return NULL;
  }
}

class gColor { // Floating point rep of a color
public:
  gColor( float rin, float gin, float bin, float ain= 1.0 ) ;
  gColor();
  gColor( const gColor& other );
  float r();
  float g();
  float b();
  float a();
  int ir();
  int ig();
  int ib();
  int ia();
  gColor clamp();
  gColor clamp_alpha();
    // Note that color values are assumed to be pre-multiplied by opacity!
  void add_under( const gColor & other);
  void add_noclamp( const gColor& other );
  void subtract_noclamp( const gColor& other );
  void mult_noclamp( const float fac );
  void scale_by_alpha();
};

%extend gColor {
char *__str__() {
  static char tmp[256];
  sprintf(tmp,"gColor(%g,%g,%g,%g)",self->r(),self->g(),self->b(),self->a());
  return tmp;
 }
}

%typemap(in) gColor (double temp[4]) {   // temp[4] becomes a local variable
  if (PyTuple_Check($input)) {
    if (!PyArg_ParseTuple($input,"dddd",temp,temp+1,temp+2,temp+3)) {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements");
      return NULL;
    }
    $1 = gColor(temp[0],temp[1],temp[2],temp[3]);
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple.");
    return NULL;
  }
}

%typemap(in) gColor& (double temp[4],gColor tmpClr) {
  if (PyTuple_Check($input)) {
    if (!PyArg_ParseTuple($input,"dddd",temp,temp+1,temp+2,temp+3)) {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements");
      return NULL;
    }
    tmpClr= gColor(temp[0],temp[1],temp[2],temp[3]);
    $1 = &tmpClr;
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple.");
    return NULL;
  }
}

%typemap(out) gColor {
  $result= PyTuple_New(4);
  PyTuple_SetItem($result, 0, PyFloat_FromDouble($1.r()));
  PyTuple_SetItem($result, 1, PyFloat_FromDouble($1.g()));
  PyTuple_SetItem($result, 2, PyFloat_FromDouble($1.b()));
  PyTuple_SetItem($result, 3, PyFloat_FromDouble($1.a()));
};


%typecheck(SWIG_TYPECHECK_POINTER) gColor {
  $1 = PyTuple_Check($input) ? 1 : 0;
};

%typemap(in) (const gColor* colors, const int xdim) {
  int i;
  if (!PyList_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expecting a list");
    return NULL;
  }
  $2 = PyList_Size($input);
  $1 = (gColor*) malloc($2*sizeof(gColor));
  for (i = 0; i < $2; i++) {
    double temp[4];
    PyObject *obj = PyList_GetItem($input,i);
    if (PyTuple_Check(obj)) {
      if (!PyArg_ParseTuple(obj,"dddd",temp,temp+1,temp+2,temp+3)) {
	PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements here");
	return NULL;
      }
      $1[i] = gColor(temp[0],temp[1],temp[2],temp[3]);
    } else {
      PyErr_SetString(PyExc_TypeError,"expected a tuple.");
      return NULL;
    }
  }
}
%typemap(freearg) (const gColor* colors, const int xdim) {
  if ($1) free($1);
}
%typemap(in) (const gColor* colors, const int xdim, const int ydim) {
  int iRow;
  int iCol;
  if (!PyList_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expecting a list");
    return NULL;
  }
  else {
    PyObject *row= PyList_GetItem($input,0);
    if (!PyList_Check(row)) {
      PyErr_SetString(PyExc_ValueError, "Expecting a list of lists");
      return NULL;
    }
    $2 = PyList_Size(row);
    $3 = PyList_Size($input);
    $1 = (gColor*) malloc($2*$3*sizeof(gColor));
    for (iRow = 0; iRow < $3; iRow++) {
      PyObject *row= PyList_GetItem($input,iRow);
      if (!PyList_Check(row)) {
	PyErr_SetString(PyExc_ValueError, "Expecting a list of lists");
	return NULL;
      }
      if (PyList_Size(row) != $2) {
	PyErr_SetString(PyExc_ValueError, "Color array is not regular");
	return NULL;
      }
      for (iCol= 0; iCol<$2; iCol++) {
	double temp[4];
	PyObject *obj = PyList_GetItem(row,iCol);
	if (PyTuple_Check(obj)) {
	  if (!PyArg_ParseTuple(obj,"dddd",temp,temp+1,temp+2,temp+3)) {
	    PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements");
	    return NULL;
	  }
	  $1[iCol+($2*iRow)] = gColor(temp[0],temp[1],temp[2],temp[3]);
	} else {
	  PyErr_SetString(PyExc_TypeError,"expected a tuple.");
	  return NULL;
	}
      }
    }
  }
}
%typemap(freearg) (const gColor* colors, const int xdim, const int ydim) {
  if ($1) free($1);
}

class gBColor { // A more compact, byte rep of a color
public:
  gBColor( float rin, float gin, float bin, float ain= 1.0 );
  gBColor( int rin, int gin, int bin, int ain= 255 );
  gBColor();
  gBColor( const gColor& other ); // defined after gColor
  void clear();
  float r() const;
  float g() const;
  float b() const;
  float a() const;
  int ir() const;
  int ig() const;
  int ib() const;
  int ia() const;
  int operator==( const gBColor& other ) const;
  int operator!=( const gBColor& other ) const;
  gBColor operator*( const float fac );
  gBColor operator+( const gBColor& other );
  gBColor operator+=( const gBColor& other );
  gBColor operator*=( const float factor );
  gBColor alpha_weighted() const;
  unsigned char* bytes() const;
  gBColor scale_alpha( const float factor );
};

%extend gBColor {
char *__str__() {
  static char tmp[256];
  sprintf(tmp,"gBColor(%d,%d,%d,%d)",
	  self->ir(),self->ig(),self->ig(),self->ia());
  return tmp;
 }
}

%typemap(in) gBColor (double temp[4]) {   // temp[4] becomes a local variable
  if (PyTuple_Check($input)) {
    if (!PyArg_ParseTuple($input,"dddd",temp,temp+1,temp+2,temp+3)) {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements");
      return NULL;
    }
    $1 = gBColor((float)temp[0],(float)temp[1],(float)temp[2],(float)temp[3]);
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple.");
    return NULL;
  }
}

%typemap(in) gBColor& (double temp[4],gBColor tmpClr) {
  if (PyTuple_Check($input)) {
    if (!PyArg_ParseTuple($input,"dddd",temp,temp+1,temp+2,temp+3)) {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 elements");
      return NULL;
    }
    tmpClr= gBColor(temp[0],temp[1],temp[2],temp[3]);
    $1 = &tmpClr;
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple.");
    return NULL;
  }
}

%typemap(out) gBColor {
  $result= PyTuple_New(4);
  PyTuple_SetItem($result, 0, PyFloat_FromDouble($1.r()));
  PyTuple_SetItem($result, 1, PyFloat_FromDouble($1.g()));
  PyTuple_SetItem($result, 2, PyFloat_FromDouble($1.b()));
  PyTuple_SetItem($result, 3, PyFloat_FromDouble($1.a()));
};

class gTransfm {
public:
  gTransfm();
  gTransfm( const float data_in[16] );
  gTransfm( const gTransfm& a );
  static gTransfm *scale( float val );
  static gTransfm *scale( float xval, float yval, float zval );
  static gTransfm *rotation( gVector *axis, float angle );
  static gTransfm *translation( float x, float y, float z );
  static gTransfm identity;
  void transpose_self();
  gTransfm& operator*( const gTransfm& a );
  gVector operator*( const gVector& vec ) const;
  gPoint operator*( const gPoint& pt ) const;
  void dump() const; // writes to stderr
  int operator==( const gTransfm& other ) const;
  int operator!=( const gTransfm& other ) const;
  /************
   * These methods look like they would be problematic to interface
   ***********/
  // gTransfm& operator=( const gTransfm& a ); 
  // char *tostring() const; // allocates memory;  caller must free
  // static gTransfm fromstring( char **buf );
  // const float *floatrep() const;
};

class gBoundBox {
public:
    gBoundBox( float xin_llf, float yin_llf, float zin_llf,
	       float xin_trb, float yin_trb, float zin_trb );
    gBoundBox();
    ~gBoundBox();
    gBoundBox( const gBoundBox& a );
    int inside( const gPoint& pt );
    int intersect( const gVector& dir, const gPoint& origin, 
		   const float mindist, float *maxdist );
    int operator==( const gBoundBox& other );
    int operator!=( const gBoundBox& other );
    void union_with( const gBoundBox& a );
    float xmin();
    float xmax();
    float ymin();
    float ymax();
    float zmin();
    float zmax();
    gPoint center();
  %feature("docstring",
"Both points are assumed to lie within the boundbox.  The box is treated
as a region with periodic boundary conditions, and the returned point is
the reflection of the first point which lies closest to the fixed 
point.") wrap_together;
    gPoint wrap_together( gPoint pt, gPoint fixedPt );
  %feature("docstring",
"The boundbox is treated as the boundary of a region with periodic
boundary conditions.  The returned point is the reflection of the given point
which lies inside the boundary.") wrap;
    gPoint wrap( gPoint pt );
};

%extend gBoundBox {
char *__str__() {
  static char tmp[1024];
  sprintf(tmp,"gBoundBox((%g,%g,%g),(%g,%g,%g))",
	  self->xmin(), self->ymin(), self->zmin(),
	  self->xmax(), self->ymax(), self->zmax());
  return tmp;
 }
}

class Camera {
public:
  Camera( const gPoint lookfm_in, const gPoint lookat_in, const gVector up_in, 
	  const double fovea_in, const double hither_in, const double yon_in,
	  const int parallel_flag_in=0 );
  /*  Camera( const Camera& other );*/
  virtual ~Camera();
  gPoint frompt();
  gPoint atpt();
  gVector updir();
  gVector pointing_dir();
  double fov();
  double hither_dist();
  double yon_dist();
  double view_dist();
  int parallel_proj();
  void set_parallel_proj();
  void set_perspective_proj();
};

%extend Camera {
char *__str__() {
  static char tmp[1024];
  gPoint atpt= self->atpt();
  gPoint frompt= self->frompt();
  gVector up= self->updir();
  sprintf(tmp,"Camera((%g,%g,%g,%g),(%g,%g,%g,%g),(%g,%g,%g,%g),%g,%g,%g,%d)",
	  frompt.x(),frompt.y(),frompt.z(),frompt.w(),
	  atpt.x(),atpt.y(),atpt.z(),atpt.w(),
	  up.x(),up.y(),up.z(),up.w(),
	  self->fov(),self->hither_dist(),self->yon_dist(),
	  self->parallel_proj());
  return tmp;
 }
}

// We have all the parts necessary to parse the cball interface now.
%include "cball.i"

class rgbImage {
public:
  rgbImage( int xin, int yin );
  rgbImage( FILE *fp, char *filename );
  virtual ~rgbImage();
  void clear( gBColor pix );
  void add_under( rgbImage *other );
  void add_over( rgbImage *other );
  void rescale_by_alpha();
  gBColor pix( const int i, const int j );
  gBColor nextpix();
  gBColor prevpix();
  int xsize();
  int ysize();
  int valid();
  int save( char *fname, char *format ); // returns non-zero on success
  int compressed();
  void uncompress();
};

%contract StarBunch::set_attr( const int whichAttr, const int val ) {
 require: 
  whichAttr < StarBunch::ATTRIBUTE_LAST;
}

%contract StarBunch::attr( const int whichAttr ) {
 require: 
  whichAttr < StarBunch::ATTRIBUTE_LAST;
}

// The following contracts require a bit of a hack- use of (arg1) to
// get at the class instance.
%contract StarBunch::set_coords( const int i, const gPoint pt ) {
 require:
  i>=0 && i < (arg1)->nstars();
}
%contract StarBunch::coords( const int i ) {
 require:
  i>=0 && i < (arg1)->nstars();
}
%contract StarBunch::set_prop( const int iStar, const int iProp, const double value ) {
 require:
  iStar>=0 && iStar < (arg1)->nstars();
  iProp>=0 && iProp < (arg1)->nprops();
}
%contract StarBunch::prop( const int iStar, const int iProp ) {
 require:
  iStar>=0 && iStar < (arg1)->nstars();
  iProp>=0 && iProp < (arg1)->nprops();
}
%contract StarBunch::set_propName( const int iProp, const char* name ) {
 require:
  iProp>=0 && iProp < (arg1)->nprops();
}
%contract StarBunch::propName( const int iProp ) {
 require:
  iProp>=0 && iProp < (arg1)->nprops();
}
%contract StarBunch::clr( const int i ) {
 require:
  i>=0 && i < (arg1)->nstars();
}
%contract StarBunch::id( const int i ) {
 require:
  i>=0 && i<(arg1)->nstars();
  (arg1)->has_ids();
}
%contract StarBunch::set_valid( const int i, const int validFlag ) {
 require:
  i>=0 && i<(arg1)->nstars();  
}
%contract StarBunch::valid( const int i ) {
 require:
  i>=0 && i<(arg1)->nstars();
}
%contract StarBunch::set_id( const int i, const unsigned int val ) {
 require:
  i>=0 && i<(arg1)->nstars();  
}
%contract StarBunch::density( const int i ) {
 require:
  i>=0 && i < (arg1)->nstars();
}
%contract StarBunch::set_density( const int i, const double val ) {
 require:
  (i>=0 && i < (arg1)->nstars());
}
%contract StarBunch::exp_constant( const int i ) {
 require:
  i>=0 && i < (arg1)->nstars();
}
%contract StarBunch::set_exp_constant( const int i, const double val ) {
 require:
  i>=0 && i < (arg1)->nstars();
}
%contract StarBunch::scale_length(const int i ) {
 require:
  i>=0 && i < (arg1)->nstars();
}
%contract StarBunch::set_scale_length( const int i, const double val ) {
 require:
  i>=0 && i < (arg1)->nstars();
}
%contract StarBunch::sort_ascending_by_prop( const int iProp ) {
 require:
  (iProp>=0 && iProp < (arg1)->nprops());
}
%contract StarBunch::sort_ascending_by_id() {
 require:
  (arg1)->has_ids();
}
%contract StarBunch::deallocate_prop_index( const int iProp ) {
 require:
  (iProp>=0 && iProp<(arg1)->nprops());
}

class StarBunch {
public:
  enum AttributeID { COLOR_ALG=0, COLOR_PROP1, COLOR_PROP2, 
	COLOR_PROP1_USE_LOG, COLOR_PROP2_USE_LOG, DEBUG_LEVEL,
	ATTRIBUTE_LAST };
  enum ColorAlgType { CM_CONSTANT=0, CM_COLORMAP_1D, CM_COLORMAP_2D };
  static const char* PER_PARTICLE_DENSITIES_PROP_NAME const;
  static const char* PER_PARTICLE_SQRT_EXP_CONSTANTS_PROP_NAME const;
  static const char* ID_PROP_NAME const;
  static const char* VALID_PROP_NAME const;
  static const char* VEL_X_NAME const;
  static const char* VEL_Y_NAME const;
  static const char* VEL_Z_NAME const;
  StarBunch( const int nstars_in=0 );
  ~StarBunch();
  void set_attr( const int whichAttr, const int val );
  int attr( const int whichAttr );
  %exception load_ascii_xyzxyz {
    $action
      if (!result) {
	PyErr_SetString(PyExc_IOError,"load_ascii_xyzxyz failed");
	return NULL;
      }
  }
  int load_ascii_xyzxyz( FILE* infile ); // returns non-zero on success
  %exception load_raw_xyzxyz {
    $action
      if (!result) {
	PyErr_SetString(PyExc_IOError,"load_raw_xyzxyz failed");
	return NULL;
      }
  }
  int load_raw_xyzxyz( FILE* infile ); // returns non-zero on success
  %exception load_ascii_xyzdkxyzdk {
    $action
      if (!result) {
	PyErr_SetString(PyExc_IOError,"load_ascii_xyzdkxyzdk failed");
	return NULL;
      }
  }
  int load_ascii_xyzdkxyzdk( FILE* infile ); // returns non-zero on success
  %exception load_raw_xyzdkxyzdk {
    $action
      if (!result) {
	PyErr_SetString(PyExc_IOError,"load_raw_xyzdkxyzdk failed");
	return NULL;
      }
  }
  int load_raw_xyzdkxyzdk( FILE* infile ); // returns non-zero on success
  void set_nstars( const int nstars_in );
  void set_nprops( const int nprops_in );
  int nprops();
  void set_bunch_color( const gColor& clr_in );
  void set_time( const double time_in );
%feature("docstring","This is the cosmological redshift z, or zero for non-cosmological simulations") set_z;
  void set_z( const double z_in );
%feature("docstring","This is the cosmological scale factor a, or zero for non-cosmological simulations") set_a;
  void set_a( const double a_in );
  void set_density( const double dens_in );
  void set_exp_constant( const double exp_const_in );
  void set_scale_length( const double scale_length_in );
  int nstars();
  gBoundBox boundBox();
  gColor bunch_color();
  double time();
%feature("docstring","This is the cosmological redshift z, or zero for non-cosmological simulations") z;
  double z();
%feature("docstring","This is the cosmological scale factor a, or zero for non-cosmological simulations") a;
  double a();
  double density();
  double exp_constant();
  double scale_length();
/*   void dump( PyObject* ofile, const int dump_coords= 0 );  // FILE* is a problem; replace with %extend */
  int has_ids();
  int has_valids();
  int has_per_part_densities();
  int has_per_part_exp_constants();
  int ninvalid();
  void set_coords( const int i, const gPoint pt );
  gPoint coords( const int i);
  void set_prop( const int iStar, const int iProp, const double value );
  double prop( const int iStar, const int iProp );
  void set_propName( const int iProp, const char* name );
  const char* propName( const int iProp );
  gColor clr( const int i );
  long id( const int i );
  void set_valid( const int i, const int validFlag );
  unsigned int valid( const int i );
  void set_id( const int i, const long val );
  double density( const int i );
  void set_density( const int i, const double val );
  double exp_constant( const int i );
  void set_exp_constant( const int i, const double val );
  double scale_length(const int i );
  void set_scale_length( const int i, const double val );
  void set_colormap1D(const gColor* colors, const int xdim,
		      const double min, const double max);
  // x dimension varies fastest!
  void set_colormap2D(const gColor* colors, const int xdim, const int ydim,
		      const double minX, const double maxX,
		      const double minY, const double maxY);
  void crop( const gPoint pt, const gVector dir );
  int  sort_ascending_by_prop( const int iProp );
  int  sort_ascending_by_id();
  int allocate_next_free_prop_index(const char* name);
  void deallocate_prop_index(const int iProp);
  // returns non-zero on success- turn it into an exception
%exception copy_stars {
  $action
  if (!result) {
    PyErr_SetString(PyExc_RuntimeError,"copy_stars failed");
    return NULL;
  }
}
  int copy_stars( const StarBunch* src );
  // returns non-zero on success- turn it into an exception
%exception fill_invalid_from {
  $action
  if (!result) {
    PyErr_SetString(PyExc_RuntimeError,"fill_invalid_from failed");
    return NULL;
  }
}
  int fill_invalid_from( StarBunch* src, int sorted=0, int src_sorted=0 );
  int get_prop_index_by_name(const char* name); // returns -1 on failure
  void wrap_periodic( const gBoundBox& newWorldBBox );
};

%extend StarBunch {
  void dump(PyObject* fp, const int dump_coords=0) {
    FILE* f = fdopen(PyObject_AsFileDescriptor(fp), "w");
    self->dump(f, dump_coords);
    fflush(f);
 }
}



// This routine returns non-zero on success- turn it into an exception
%rename(load_tipsy_box_ascii) ssplat_load_tipsy_box_ascii; // it will be in module namespace
%exception ssplat_identify_unshared_ids {
  $action
  if (!result) {
    PyErr_SetString(PyExc_IOError,"load_tipsy_box_ascii failed");
    return NULL;
  }
}

extern int ssplat_load_tipsy_box_ascii( FILE* infile, StarBunch* gas,
					StarBunch* stars, StarBunch* dark );

%typemap(in) (StarBunch** sbunch_tbl, const int tbl_size) {
  int i;
  if (!PyList_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expecting a list");
    return NULL;
  }
  $2 = PyList_Size($input);
  $1 = (StarBunch **) malloc($2*sizeof(StarBunch*));
  for (i = 0; i < $2; i++) {
    StarBunch* temp;
    PyObject *obj = PyList_GetItem($input,i);
    if ((SWIG_ConvertPtr(obj, (void**)&temp, $descriptor(StarBunch*),0))==-1) {
      PyErr_SetString(PyExc_ValueError, 
		      "A list element was not a StarBunch pointer!");
      return NULL;
    }
    $1[i] = temp;
  }
}
%typemap(freearg) (StarBunch** sbunch_tbl, const int tbl_size) {
  if ($1) free($1);
}

// These routines return a tuple (non-zero on success, num bunches read)
// -turn failures into exceptions
%rename(load_dubinski) ssplat_load_dubinski; // it will be in module namespace
%exception ssplat_load_dubinski {
  $action
  if (!result) {
    PyErr_SetString(PyExc_IOError,"load_dubinski failed");
    return NULL;
  }
}
extern int ssplat_load_dubinski( FILE* infile, StarBunch** sbunch_tbl,
				 const int tbl_size, int* OUTPUT );
%rename(load_dubinski_raw) ssplat_load_dubinski_raw; // it will be in module namespace
%exception ssplat_load_dubinski_raw {
  $action
  if (!result) {
    PyErr_SetString(PyExc_IOError,"load_dubinski_raw failed");
    return NULL;
  }
}
extern int ssplat_load_dubinski_raw( FILE* infile, StarBunch** sbunch_tbl,
				     const int tbl_size, int* OUTPUT );
%rename(load_gadget) ssplat_load_gadget; // it will be in module namespace
%exception ssplat_load_gadget {
  $action
  if (!result) {
    PyErr_SetString(PyExc_IOError,"load_gadget failed");
    return NULL;
  }
}
%feature("autodoc","load_gadget(infile, [gasBunch,haloBunch,diskBunch,bulgeBunch,bndryBunch]) -> ( int, int )") ssplat_load_gadget;
%feature("docstring",
"Returned tuple is ( success, nBunchesRead ), where success is nonzero 
when the file was loaded successfully.  If the file is not loaded 
successfully, RuntimeError is raised.") ssplat_load_gadget;
extern int ssplat_load_gadget ( FILE* infile, StarBunch** sbunch_tbl,
				const int tbl_size, int* OUTPUT );
	

// This routine returns 1 on success- turn it into an exception
%rename(identify_unshared_ids) ssplat_identify_unshared_ids;
%exception ssplat_identify_unshared_ids {
  $action
  if (!result) {
    PyErr_SetString(PyExc_RuntimeError,"identify_unshared_ids failed");
    return NULL;
  }
}
int ssplat_identify_unshared_ids(StarBunch* sb1, StarBunch* sb2);

// These routine produces a StarBunch on success, NULL on failure- turn
// it into an exception
%feature("docstring",
"""
vel_scale is a factor to adjust units between positions and velocities.
For example, if positions are in kiloparsecs, velocities are in
kilometers per second and times are in seconds, vel_scale would be
1.0/(30.86e15).
""") ssplat_starbunch_interpolate;
%rename(starbunch_interpolate) ssplat_starbunch_interpolate;
%newobject ssplat_starbunch_interpolate;
%exception ssplat_starbunch_interpolate {
  $action
  if (!result) {
    PyErr_SetString(PyExc_RuntimeError,"starbunch_interpolate failed");
    return NULL;
  }
}
StarBunch* ssplat_starbunch_interpolate(StarBunch* sb1, StarBunch* sb2,
					const double time, 
					const double vel_scale,
					const int sb1_sorted= 0,
					const int sb2_sorted= 0);
%feature("docstring",
"""
vel_scale is a factor to adjust units between positions and velocities.
For example, if positions are in kiloparsecs, velocities are in
kilometers per second and times are in seconds, vel_scale would be
1.0/(30.86e15).
""") ssplat_starbunch_interpolate_periodic_bc;
%rename(starbunch_interpolate_periodic_bc) 
  ssplat_starbunch_interpolate_periodic_bc;
%newobject ssplat_starbunch_interpolate_periodic_bc;
%exception ssplat_starbunch_interpolate_periodic_bc {
  $action
  if (!result) {
    PyErr_SetString(PyExc_RuntimeError,
		    "starbunch_interpolate_periodic_bc failed");
    return NULL;
  }
}
StarBunch* ssplat_starbunch_interpolate_periodic_bc(StarBunch* sb1, 
						    StarBunch* sb2,
						    const double time,
						    const double vel_scale,
						    const gBoundBox& worldBB,
						    const int sb1_sorted= 0,
						    const int sb2_sorted= 0);


// %typemap(in) StarBunch* add_stars_sbunch_in (int res, void* argp) {
//   argp= 0;
//   res= 0;
//   assert($argnum==2); // Intended to catch the owning object in argnum 1
//   res = SWIG_ConvertPtr($input, &argp, $1_descriptor, 0 |  0 );
//   if (!SWIG_IsOK(res)) {
//     SWIG_exception_fail(SWIG_ArgError(res), 
// 			"in method 'StarSplatter_add_stars', argument"
// 			"$argnum" " of type 'StarBunch*'");
//   }
//   Py_INCREF($input);

// //  _StarSplatter_StarBunch_mmap.insert(std::pair<StarSplatter*,PyObject*>(arg1,
// //									 $input));
//   $1= reinterpret_cast< $type >(argp);
// }

class StarSplatter {
public:
  StarSplatter();
  ~StarSplatter();
  enum ExposureType { ET_LINEAR, ET_LOG, ET_LOG_AUTO,
		      ET_LOG_HSV, ET_LOG_HSV_AUTO,
		      ET_NOOPAC_LINEAR,
		      ET_NOOPAC_LOG, ET_NOOPAC_LOG_AUTO,
		      ET_NOOPAC_LOG_HSV, ET_NOOPAC_LOG_HSV_AUTO,
		      ET_LUPTON,
		      ET_LATE_CMAP_R, ET_LATE_CMAP_A,
		      ET_LATE_CMAP_LOG_R, ET_LATE_CMAP_LOG_A,
		      ET_LATE_CMAP_LOG_R_AUTO, ET_LATE_CMAP_LOG_A_AUTO };
  enum SplatType { SPLAT_GAUSSIAN, SPLAT_SPLINE, SPLAT_GLYPH_CIRCLE };
  void set_image_dims( const int xsize_in, const int ysize_in );
  void set_camera( const Camera& cam_in );
  void set_transform( const gTransfm& trans_in );
%pythonprepend clear_stars() %{
        ren= args[0]
        ren.liveBunches= []
%}
  void clear_stars();
%pythonappend add_stars(StarBunch*) %{
        if not hasattr(self, "liveBunches"): self.liveBunches= []
        self.liveBunches.append(sbunch_in)
%}
  void add_stars( StarBunch* sbunch_in ); // Bunch is not copied!
  %exception render {
    $action
    if (!result) {
      PyErr_SetString(PyExc_RuntimeError,"render failed");
      return NULL;
    }
  }
  rgbImage* render(); // returns null on failure
  %exception render_points {
    $action
    if (!result) {
      PyErr_SetString(PyExc_RuntimeError,"render failed");
      return NULL;
    }
  }
  rgbImage* render_points(); // returns null on failure
  int image_xsize();
  int image_ysize();
  double splat_cutoff_frac();
  void set_splat_cutoff_frac( const double val_in );
  const gTransfm& transform();
  const int camera_set();
  const Camera& camera();
  void dump( FILE* ofile );
/*   void dump( PyObject* ofile );  // FILE* is a problem; replace with %extend */
  void set_debug( const int flag );
  int debug();
  void set_exposure_type( const StarSplatter::ExposureType type );
  StarSplatter::ExposureType get_exposure_type();
  void set_log_rescale_bounds( const double min_in, const double max_in );
  const void get_log_rescale_bounds( double* min_out, double* max_out );
  void set_late_colormap(const gColor* colors, const int xdim,
			 const double min, const double max);
  double exposure_scale();
  void set_exposure_scale( const double scale_in );
  SplatType splat_type();
  void set_splat_type( SplatType splatType );
};

%extend StarSplatter {
  void dump(PyObject* fp) {
    FILE* f = fdopen(PyObject_AsFileDescriptor(fp), "w");
    self->dump(f);
    fflush(f);
 }
}
