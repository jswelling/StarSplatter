/****************************************************************************
 * starbunch.h
 * Author Joel Welling
 * Copyright 1996, Pittsburgh Supercomputing Center, Carnegie Mellon University
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

class StarBunch;

class StarBunchCMap {
 public:
  StarBunchCMap( const gColor* data_in, const int xdim_in, const int ydim_in,
	    const double minX_in, const double maxX_in, 
	    const double minY_in, const double maxY_in);
  StarBunchCMap( const StarBunchCMap& other );
  ~StarBunchCMap();
  gColor map( double x );
  gColor map( double x, double y );
 private:
  gColor lookup( double xScaled, double yScaled );
  gColor* data;
  int xdim;
  int ydim;
  double minX;
  double minY;
  double maxX;
  double maxY;
};

// This routine returns non-zero on success
extern int ssplat_identify_unshared_ids(StarBunch* sb1, StarBunch* sb2);

// These routine returns a valid StarBunch on success, or NULL on failure
extern StarBunch* 
ssplat_starbunch_interpolate(StarBunch* sb1, StarBunch* sb2,
			     const double time,
			     const double vel_scale,
			     const int sb1_sorted=0,
			     const int sb2_sorted=0);
extern StarBunch* 
ssplat_starbunch_interpolate_periodic_bc(StarBunch* sb1, 
					 StarBunch* sb2,
					 const double time,
					 const double vel_scale,
					 const gBoundBox& worldBB,
					 const int sb1_sorted=0,
					 const int sb2_sorted=0);
					

class StarBunch {
public:
  friend int ssplat_identify_unshared_ids(StarBunch* sb1, StarBunch* sb2);
  enum AttributeID { COLOR_ALG=0, COLOR_PROP1, COLOR_PROP2, 
		     COLOR_PROP1_USE_LOG, COLOR_PROP2_USE_LOG,
		     DEBUG_LEVEL, ATTRIBUTE_LAST };
  enum ColorAlgType { 
    CM_CONSTANT=0, CM_COLORMAP_1D, CM_COLORMAP_2D, CM_COLORMAP_LAST
  };
  /* These are the names of specific properties used by the algorithm */
  static const char* PER_PARTICLE_DENSITIES_PROP_NAME;
  static const char* PER_PARTICLE_SQRT_EXP_CONSTANTS_PROP_NAME;
  static const char* ID_PROP_NAME;
  static const char* VALID_PROP_NAME;
  static const char* VEL_X_NAME;
  static const char* VEL_Y_NAME;
  static const char* VEL_Z_NAME;
  StarBunch( const int nstars_in=0 );
  ~StarBunch();
  StarBunch* clone_empty() const;
  void set_attr( const int whichAttr, const int val );
  int attr( const int whichAttr ) const {
    if (whichAttr>=0 && whichAttr<ATTRIBUTE_LAST) 
      return bunch_attributes[whichAttr];
    else return 0;
  }
  int debugLevel() const { return bunch_attributes[DEBUG_LEVEL]; }
  int load_ascii_xyzxyz( FILE* infile ); // returns non-zero on success
  int load_raw_xyzxyz( FILE* infile ); // returns non-zero on success
  int load_ascii_xyzdkxyzdk( FILE* infile ); // returns non-zero on success
  int load_raw_xyzdkxyzdk( FILE* infile ); // returns non-zero on success
  void set_nstars( const int nstars_in );
  int nstars() const { return num_stars; }
  void set_nprops( const int nprops_in );
  int nprops() const { return num_props; }
  void set_bunch_color( const gColor& clr_in ) { bunchClr= clr_in; }
  void set_time( const double time_in ) { time_val= time_in; }
  void set_z( const double z_in ) { z_val= z_in; }
  void set_a( const double a_in ) { a_val= a_in; }
  void set_density( const double dens_in ) { densityval= dens_in; }
  void set_exp_constant( const double exp_const_in ) 
  { sqrt_exponent_constant= sqrt(exp_const_in); }
  void set_scale_length( const double scale_length_in )
  { sqrt_exponent_constant= 1.0/scale_length_in; }
  gColor bunch_color() const { return bunchClr; }
  double time() const { return time_val; }
  double z() const { return z_val; }
  double a() const { return a_val; }
  double density() const { return densityval; }
  double sqrt_exp_constant() const
  { return sqrt_exponent_constant; }
  double exp_constant() const 
  { return sqrt_exp_constant()*sqrt_exp_constant(); }
  double scale_length() const { return 1.0/sqrt_exp_constant(); }
  void dump( FILE* ofile, const int dump_coords= 0 );
  int has_ids() const 
  { return (id_index >= 0); }
  int has_valids() const
  { return (valid_index >= 0); }
  int ninvalid() const
  { if (has_valids()) return invalid_count; else return 0; }
  int has_per_part_densities() const 
  { return (per_part_densities_index >= 0); }
  int has_per_part_exp_constants() const 
  { return (per_part_sqrt_exp_constants_index >= 0); }
  void set_coords( const int i, const gPoint pt )
  {
    if (bbox) { 
      delete bbox;
      bbox= NULL;
    }
    *pointPtr(i)= pt;
  }
  gPoint coords( const int i ) const
  {
    return *pointPtr(i);
  }
  int allocate_next_free_prop_index(const char* name);
  void deallocate_prop_index(const int iProp);
  void set_prop( const int iStar, const int iProp, const double value )
  { *doublePropPtr(iStar,iProp)= value; }
  double prop( const int iStar, const int iProp ) const
  { return *doublePropPtr(iStar,iProp); }
  void set_propName( const int iProp, const char* name );
  const char* propName( const int iProp ) const;
  gBoundBox boundBox() 
  {
    if (!bbox) updateBoundBox();
    return *bbox;
  }
  gColor clr( const int i ) const
  {
    switch (bunch_attributes[COLOR_ALG]) {
    case CM_COLORMAP_1D:
      if (cmap1D) {
	if (bunch_attributes[COLOR_PROP1_USE_LOG])
	  return cmap1D->map(log10(prop(i,bunch_attributes[COLOR_PROP1])))
	    *bunch_color();
	else
	  return cmap1D->map(prop(i,bunch_attributes[COLOR_PROP1]))
	    *bunch_color();
      }
      else return bunch_color();
    case CM_COLORMAP_2D:
      if (cmap2D) {
	if (bunch_attributes[COLOR_PROP1_USE_LOG]) {
	  if (bunch_attributes[COLOR_PROP2_USE_LOG]) {
	    return cmap2D->map(log10(prop(i,bunch_attributes[COLOR_PROP1])),
			       log10(prop(i,bunch_attributes[COLOR_PROP2])))
	      *bunch_color();
	  }
	  else {
	    return cmap2D->map(log10(prop(i,bunch_attributes[COLOR_PROP1])),
			       prop(i,bunch_attributes[COLOR_PROP2]))
	      *bunch_color();
	  }
	}
	else {
	  if (bunch_attributes[COLOR_PROP2_USE_LOG]) {
	    return cmap2D->map(prop(i,bunch_attributes[COLOR_PROP1]),
			       log10(prop(i,bunch_attributes[COLOR_PROP2])))
	      *bunch_color();
	  }
	  else {
	    return cmap2D->map(prop(i,bunch_attributes[COLOR_PROP1]),
			       prop(i,bunch_attributes[COLOR_PROP2]))
	      *bunch_color();
	  }
	}
      }
      else return bunch_color();
    default:
      return bunch_color();
    }
  }
  long id( const int i ) const
  { 
    if (has_ids()) return *longPropPtr(i, id_index);
    else return -1;
  }
  void set_id( const int i, const long val )
  {
    if (!has_ids()) create_id_storage();
    *longPropPtr(i, id_index)= val;
  }
  int valid( const int i ) const
  {
    if (has_valids()) return *longPropPtr(i,valid_index);
    else return 1; /* rec is valid unless otherwise specified */
  }
  void set_valid( const int i, const int validFlag )
  {
    if (!has_valids()) create_valid_storage();
    if (validFlag) {
      if (!valid(i)) invalid_count -= 1;
      *longPropPtr(i,valid_index)= 1;
    }
    else {
      if (valid(i)) invalid_count += 1;
      *longPropPtr(i,valid_index)= 0;
    }
  }
  double density( const int i ) const
  { 
    if (has_per_part_densities()) 
      return (density()*prop(i,per_part_densities_index));
    else return density();
  }
  void set_density( const int i, const double val )
  {
    if (!has_per_part_densities()) create_per_part_density_storage();
    set_prop(i, per_part_densities_index, val);
  }
  double exp_constant( const int i ) const
  { return sqrt_exp_constant(i)*sqrt_exp_constant(i); }
  double sqrt_exp_constant(const int i) const
  {
    if (has_per_part_exp_constants()) {
      return sqrt_exp_constant()*prop(i,per_part_sqrt_exp_constants_index);
    }
    else return sqrt_exp_constant();
  }
  void set_exp_constant( const int i, const double val )
  {
    if (!has_per_part_exp_constants()) 
      create_per_part_sqrt_exp_constant_storage();
    set_prop( i, per_part_sqrt_exp_constants_index, sqrt(val) );
  }
  double scale_length(const int i ) const 
  { return 1.0/sqrt_exp_constant(i); }
  void set_scale_length( const int i, const double val )
  {
    if (!has_per_part_exp_constants()) 
      create_per_part_sqrt_exp_constant_storage();
    double tmp_val= 1.0/val; // allow global scale to be effective
    set_prop( i, per_part_sqrt_exp_constants_index, tmp_val );
  }
  void set_colormap1D(const gColor* colors, const int xdim,
		      const double min, const double max);
  
  // x dimension varies fastest!
  void set_colormap2D(const gColor* colors, const int xdim, const int ydim,
		      const double minX, const double maxX,
		      const double minY, const double maxY);

  void crop( const gPoint pt, const gVector dir );
  int sort_ascending_by_prop( const int iProp );
  int sort_ascending_by_id();
  int copy_stars( const StarBunch* src );
  int fill_invalid_from( StarBunch* src, int sorted=0, int src_sorted=0 );
  int get_prop_index_by_name(const char* name) const; // returns -1 on failure
  void wrap_periodic(const gBoundBox& newWorldBBox);
 private:
  static StarBunch* bunchBeingSorted;
  static int sortingPropIndex;
  friend int StarBunch_compareProp( const void* p1, const void* p2 );
  static const int PROPSIZE=sizeof(double);
  int propRecSize() const 
  { return sizeof(gPoint)+nprops()*PROPSIZE; }
  long propOffset( int iStar, int propID ) const 
  { return iStar*propRecSize() + sizeof(gPoint) + propID*PROPSIZE; }
  char* propRecPtr( int iStar ) const
  { return propTable+iStar*propRecSize(); }
  char* propPtr( int iStar, int propID ) const
  { return propTable+propOffset(iStar,propID); }
  long* longPropPtr( int iStar, int propID ) const 
  { return (long*)(propTable+propOffset(iStar,propID)); }
  double* doublePropPtr( int iStar, int propID ) const 
  { return (double*)(propTable+propOffset(iStar,propID)); }
  gPoint* pointPtr( int iStar ) const
  { return (gPoint*)(propTable + iStar*propRecSize()); }
  void resize_property_table(const int newNStars, const int newNProps);
  void create_id_storage();
  void create_valid_storage();
  void create_per_part_density_storage();
  void create_per_part_sqrt_exp_constant_storage();
  void updateBoundBox();
  char* propTable;
  int per_part_densities_index;
  int per_part_sqrt_exp_constants_index;
  int id_index;
  int valid_index;
  int invalid_count;
  char** propNameTable;
  int bunch_attributes[ATTRIBUTE_LAST];
  int num_stars; // number of records with active entries (some may be invalid)
  int num_props; // number of property slots in propTable
  int num_proptable_recs; // number of records in propTable; >= num_stars
  gColor bunchClr;
  double time_val;
  double densityval;
  double sqrt_exponent_constant;
  double z_val;
  double a_val;
  StarBunchCMap* cmap1D;
  StarBunchCMap* cmap2D;
  gBoundBox* bbox;
};
