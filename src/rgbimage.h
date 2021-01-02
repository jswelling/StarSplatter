/****************************************************************************
 * rgbimage.h
 * Authors Joel Welling, Rob Earhart
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

extern "C" {
#include <im.h>
}
// Clean up an increadible piece of carelessness on the part of im.h
#ifdef VOID 
#undef VOID
#endif

#include "geometry.h"

class netRgbImage;

class rgbImage {
    friend class netRgbImage;
public:
  rgbImage(int xin, int yin, ImVfb *initial_buf= NULL );
  rgbImage( FILE *fp, char *filename );
  // The following constructors make a compressed image.  The image
  // uses the passed in bufs, and eventually deletes them.
  rgbImage(const int xin, const int yin, 
	   const int n_comp_pixels_in, gBColor* comp_pixels_in,
	   const int n_comp_runs_in, int* comp_runs_in, 
	   const int owns_compbuf_in= 1 );
  rgbImage(const int xin, const int yin,
	   const int n_comp_pixels_in, unsigned char* comp_pixels_in,
	   const int n_comp_runs_in, int* comp_runs_in,
	   const int owns_compbuf_in= 1);
  virtual ~rgbImage();
  void clear( gBColor pix );
  void clear() { clear( gBColor() ); } // clears with black
  void add_under( rgbImage *other ); // result is composite of this over other
  void add_over( rgbImage *other ); // result is composite of this under other
  void rescale_by_alpha(); // takes out premult. by alpha; alpha becomes 255
  int pix_r(const int i, const int j ) const
    {return( ImVfbQRed( buf, ImVfbQPtr( buf, i, j )));};
  int pix_g(const int i, const int j ) const
    {return( ImVfbQGreen( buf, ImVfbQPtr( buf, i, j )));};
  int pix_b(const int i, const int j ) const
    {return( ImVfbQBlue( buf, ImVfbQPtr( buf, i, j )));};
  int pix_a(const int i, const int j ) const
    {return( ImVfbQAlpha( buf, ImVfbQPtr( buf, i, j )));};
  gBColor pix( const int i, const int j )
  {
    current_pix= ImVfbQPtr( buf, i, j );
    return( gBColor( ImVfbQRed( buf, current_pix ),
		     ImVfbQGreen( buf, current_pix ),
		     ImVfbQBlue( buf, current_pix ),
		     ImVfbQAlpha( buf, current_pix ) ) );
  }
  gBColor nextpix()
  {
    ImVfbSInc( buf, current_pix );
    return( gBColor( ImVfbQRed( buf, current_pix ),
		     ImVfbQGreen( buf, current_pix ),
		     ImVfbQBlue( buf, current_pix ),
		     ImVfbQAlpha( buf, current_pix ) ) );
  }
  gBColor prevpix()
  {
    ImVfbSDec( buf, current_pix );
    return( gBColor( ImVfbQRed( buf, current_pix ),
		     ImVfbQGreen( buf, current_pix ),
		     ImVfbQBlue( buf, current_pix ),
		     ImVfbQAlpha( buf, current_pix ) ) );
  }
  void setpix( const int i, const int j, const gBColor& pix )
  {
    current_pix= ImVfbQPtr( buf, i, j );
    ImVfbSRed( buf, current_pix, pix.ir() );
    ImVfbSGreen( buf, current_pix, pix.ig() );
    ImVfbSBlue( buf, current_pix, pix.ib() );
    ImVfbSAlpha( buf, current_pix, pix.ia() );
  }
  void setnextpix( const gBColor& pix )
  {
    ImVfbSInc( buf, current_pix );
    ImVfbSRed( buf, current_pix, pix.ir() );
    ImVfbSGreen( buf, current_pix, pix.ig() );
    ImVfbSBlue( buf, current_pix, pix.ib() );
    ImVfbSAlpha( buf, current_pix, pix.ia() );
  }
  int xsize() const { return xdim; }
  int ysize() const { return ydim; }
  int valid() const { return image_valid; };
  int save( char *fname, char *format ); // returns non-zero on success
  int compressed() const { return compressed_flag; }
  void uncompress();
protected:
  int bounds_check( int val ) const
  { return( val < 0 ? 0 : ( val > 255 ? 255 : val ) ); }
  int xdim;
  int ydim;
  char image_valid;
  char owns_buffer;
  char owns_compbuf;
  ImVfb *buf;
  ImVfbPtr current_pix;
  int compressed_flag;
  int n_comp_pixels;
  gBColor* comp_pixels;
  unsigned char* byte_comp_pixels;
  int n_comp_runs;
  int* comp_runs;
  inline float alpha_dif( int i ) { return(1.0 - (1.0/255)*i); }
#ifdef CRAY_T3D
  static unsigned char* mem_block_hook;
  static int mem_block_size; // size of one of the two allocated images
  static unsigned char* get_matched_memory(const int xdim, const int ydim);
#endif
};
