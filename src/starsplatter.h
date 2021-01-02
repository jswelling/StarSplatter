/****************************************************************************
 * starsplatter.h
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

extern char* progname;

#include "geometry.h"
#include "rgbimage.h"
#include "camera.h"
#include "starbunch.h"

struct Tcl_Interp;

extern int SSplat_Init(Tcl_Interp* interp);

extern const char* ssplat_version_string;

extern const char* ssplat_copyright_string;

extern const char* ssplat_home_page_string;

// This routine returns non-zero on success
extern int ssplat_load_tipsy_box_ascii( FILE* infile, StarBunch* gas,
					StarBunch* stars, StarBunch* dark );

// These routines return non-zero on success
// sbunch_tbl must point to tbl_size valid (non-NULL) StarBunch*'s.
extern int ssplat_load_dubinski( FILE* infile, StarBunch** sbunch_tbl,
				 const int tbl_size, int* bunches_read );
extern int ssplat_load_dubinski_raw( FILE* infile, StarBunch** sbunch_tbl,
				     const int tbl_size, int* bunches_read );

// This routine returns non-zero on success
extern int ssplat_load_gadget ( FILE* infile, StarBunch** sbunch_tbl,
				const int tbl_size, int* bunches_read );

class SplatPainter;

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
  struct Splat {
    gPoint loc;
    gColor clr;
    double range;
    double density;
    double sqrt_exp_constant;
    int bunch_index;
    char filler[64-(sizeof(gPoint)+sizeof(gColor)
		    +3*sizeof(double)+sizeof(int))];
  };
  void set_image_dims( const int xsize_in, const int ysize_in )
  { xsize= xsize_in; ysize= ysize_in; }
  void set_camera( const Camera& cam_in )
  { cam= cam_in; cam_set_flag= 1; }
  void set_transform( const gTransfm& trans_in )
  { world_trans= trans_in; }
  void clear_stars();
  void add_stars( StarBunch* sbunch_in ); // Note bunch is not copied!
  rgbImage* render(); // returns null on failure
  rgbImage* render_points();
  int image_xsize() const { return xsize; }
  int image_ysize() const { return ysize; }
  double splat_cutoff_frac() const { return splat_cutoff; }
  void set_splat_cutoff_frac( const double val_in ) { splat_cutoff= val_in; }
  const gTransfm& transform() const { return world_trans; }
  const int camera_set() const { return cam_set_flag; }
  const Camera& camera() const { return cam; }
  void dump( FILE* ofile );
  void set_debug( const int flag ) { debug_flag= flag; }
  int debug() const { return debug_flag; }
  void set_exposure_type( const StarSplatter::ExposureType type )
  { current_exposure_type= type; }
  StarSplatter::ExposureType get_exposure_type() const
  { return current_exposure_type; }
  void set_log_rescale_bounds( const double min_in, const double max_in )
  { 
    log_rescale_min= min_in;
    log_rescale_max= max_in;
  }
  const void get_log_rescale_bounds( double* min_out, double* max_out )
  {
    *min_out= log_rescale_min;
    *max_out= log_rescale_max;
  }
  void set_late_colormap(const gColor* colors, const int xdim,
			 const double min, const double max);
  double exposure_scale() const { return exp_scale; }
  void set_exposure_scale( const double scale_in ) { exp_scale= scale_in; }
  SplatType splat_type() const;
  void set_splat_type(SplatType t);
private:
  int debug_flag;
  ExposureType current_exposure_type;
  double log_rescale_min;
  double log_rescale_max;
  int xsize;
  int ysize;
  double splat_cutoff;
  double exp_scale;
  Camera cam;
  int cam_set_flag;
  gTransfm world_trans;
  StarBunch** sbunch_table;
  int sbunch_table_size;
  int n_sbunches;
  int total_stars;
  int total_stars_after_clipping;
  Splat* splatbuf;
  int splatbuf_size;
  SplatPainter* current_splat_painter;
  StarBunchCMap* late_cmap;
  static short screen_minz;
  static short screen_maxz;
  static int initial_sbunch_table_size;
  static double default_gaussian_splat_cutoff;
  static double default_log_rescale_min;
  static double default_log_rescale_max;
  void transform_and_merge();
  void sort();
  int convert_image( rgbImage* image, const gColor* raw_image );
  int splat_all_stars( rgbImage* image ); // returns 0 on failure
  void point_splat_all_stars( rgbImage* image ); 
  int convert_image_linear(rgbImage* image, const gColor* raw_image);
  int convert_image_log(rgbImage* image, const gColor* raw_image);
  int convert_image_log_auto(rgbImage* image, const gColor* raw_image);
  int convert_image_noopac_linear(rgbImage* image, const gColor* raw_image);
  int convert_image_noopac_log(rgbImage* image, const gColor* raw_image);
  int convert_image_noopac_log_auto(rgbImage* image, const gColor* raw_image);
  int convert_image_noopac_log_hsv(rgbImage* image, const gColor* raw_image);
  int convert_image_noopac_log_hsv_auto(rgbImage* image, 
					const gColor* raw_image);
  int convert_image_lupton(rgbImage* image, const gColor* raw_image);
  int convert_image_log_hsv(rgbImage* image, const gColor* raw_image);
  int convert_image_log_hsv_auto(rgbImage* image, const gColor* raw_image);
  int convert_image_late_cmap_r(rgbImage* image, const gColor* raw_image);
  int convert_image_late_cmap_a(rgbImage* image, const gColor* raw_image);
  int convert_image_late_cmap_log_r(rgbImage* image, const gColor* raw_image);
  int convert_image_late_cmap_log_a(rgbImage* image, const gColor* raw_image);
  int convert_image_late_cmap_log_r_auto(rgbImage* image, 
					 const gColor* raw_image);
  int convert_image_late_cmap_log_a_auto(rgbImage* image, 
					 const gColor* raw_image);
  static int splat_depth_compare( const void* s1, const void* s2 ); //for qsort
};

