/****************************************************************************
 * rgbimage.cc
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
#include <stdio.h>
#include <stdlib.h>
#include "rgbimage.h"

#ifdef INCL_PNG
#include <png.h>
#endif

#ifdef CRAY_T3D
extern "C" void PREF(int* ...); // cache prefetch routine
unsigned char* rgbImage::mem_block_hook= NULL;
int rgbImage::mem_block_size= 0;
#endif

static char rcsid[] = "$Id: rgbimage.cc,v 1.13 2008-07-25 21:08:23 welling Exp $";

#ifdef AVOID_IMTOOLS
static ImVfb* ImVfbAlloc_private( const int xdim_in, const int ydim_in )
{
  ImVfb* result= new ImVfb;
  result->vfb_width= xdim_in;
  result->vfb_height= ydim_in;
  result->vfb_fields= IMVFBRGB | IMVFBALPHA;
  result->vfb_nbytes= 4;
  result->vfb_clt= IMCLTNULL;
  result->vfb_roff= 0;
  result->vfb_goff= 1;
  result->vfb_boff= 2;
  result->vfb_aoff= 3;
  result->vfb_i8off= 0;
  result->vfb_wpoff= 0;
  result->vfb_i16off= 0;
  result->vfb_zoff= 0;
  result->vfb_moff= 0;
  result->vfb_fpoff= 0;
  result->vfb_ioff= 0;
#ifdef CRAY_T3D
  // We allocate an image twice as big as was requested, so the second
  // half can be used to play cache management tricks.  4096 bytes
  // is half the T3D cache size.
  int imagesize= result->vfb_nbytes * result->vfb_width * result->vfb_height;
  int imagesize_adj_for_cache= ((imagesize + 8191)/8192) * 8192;
  result->vfb_pfirst= new unsigned char[ 2*imagesize_adj_for_cache + 4096 ];
#else
  result->vfb_pfirst= 
    new unsigned char[result->vfb_nbytes * result->vfb_width 
		     * result->vfb_height];
#endif
  result->vfb_plast= result->vfb_pfirst 
    + result->vfb_nbytes * result->vfb_width * result->vfb_height;
  
  return result;
}

static void ImVfbFree_private( ImVfb* buf )
{
  delete buf->vfb_pfirst;
  delete buf;
}
#endif



rgbImage::rgbImage( int xin, int yin, ImVfb *initial_buf )
{

    xdim= xin;
    ydim= yin;
    if (!initial_buf) {
#ifndef AVOID_IMTOOLS
      buf= ImVfbAlloc( xdim, ydim, IMVFBRGB | IMVFBALPHA);
#else
      buf= ImVfbAlloc_private( xdim, ydim );
#ifdef CRAY_T3D
      mem_block_hook= (unsigned char*)ImVfbQPtr(buf,0,0);
      mem_block_size= (((4 * xin * yin) + 8191)/8192) * 8192;
#ifdef never
char msgbuf[128];
sprintf(msgbuf,"setting block size to %x=%d\n",mem_block_size,mem_block_size);
fprintf(stderr,msgbuf);
#endif
#endif // CRAY_T3D
#endif // AVOID_IMTOOLS
      owns_buffer= 1;
    }
    else {
      buf= initial_buf;
      owns_buffer= 0;
    }
    current_pix= ImVfbQPtr( buf, 0, 0 );
    image_valid= 1;
    compressed_flag= 0;
    n_comp_pixels= n_comp_runs= 0;
    comp_pixels= NULL;
    byte_comp_pixels= NULL;
    comp_runs= NULL;
    owns_compbuf= 0;
}

rgbImage::rgbImage( FILE *fp, char *fname )
{
#ifndef AVOID_IMTOOLS
  owns_buffer= 0;
  // Allocate tag tables and add error stream
  TagTable *dataTable= TagTableAlloc();
  if (!dataTable) {
    image_valid= 0;
    return;
  }
  TagTable *flagsTable= TagTableAlloc();
  if (!dataTable || !flagsTable) {
    image_valid= 0;
    return;
  }
  FILE *tmp= stderr;
  if ( TagTableAppend( flagsTable,
		       TagEntryAlloc("error stream", POINTER, &tmp))
       == -1 ) {
    image_valid= 0;
    return;
  }

  // Get the input file format
  char *type= ImFileQFFormat( fp, fname );
  if (!type) {
    image_valid= 0;
    TagTableFree( dataTable );
    TagTableFree( flagsTable );
    return;
  }

  // Read the file
  if ( ImFileFRead( fp, type, flagsTable, dataTable ) == -1 ) {
    image_valid= 0;
    TagTableFree( dataTable );
    TagTableFree( flagsTable );
    return;
  }

  // Get the image
  TagEntry *imageEntry= TagTableQDirect( dataTable, "image vfb", 0 );
  if (!imageEntry) {
    image_valid= 0;
    TagTableFree( dataTable );
    TagTableFree( flagsTable );
    return;
  }
  if (TagEntryQValue( imageEntry, &buf ) == -1) {
    image_valid= 0;
    TagTableFree( dataTable );
    TagTableFree( flagsTable );
    return;
  }
  owns_buffer= 1;

  // Convert to RGB if necessary
  if ( !(ImVfbQFields(buf)) & IMVFBRGB ) {
    if ( !ImVfbToRgb(buf, buf) ) {
      image_valid= 0;
      TagTableFree( dataTable );
      TagTableFree( flagsTable );
      return;
    }
  }

  xdim= ImVfbQWidth( buf );
  ydim= ImVfbQHeight( buf );
  current_pix= ImVfbQPtr( buf, 0, 0 );

  // Clean up
  TagTableFree( dataTable );
  TagTableFree( flagsTable );

  // Heh. So it\'s valid after all.
  image_valid= 1;
  compressed_flag= 0;
  n_comp_pixels= n_comp_runs= 0;
  comp_pixels= NULL;
  byte_comp_pixels= NULL;
  comp_runs= NULL;
  owns_compbuf= 0;

#else // ifndef AVOID_IMTOOLS

  fprintf(stderr,"rgbImage::rgbImage: linking error;  this is the non-IMTOOLS version of rgbImage;  it can't read the file <%s>!\n",
	  fname);
  exit(-1);

#endif
}

rgbImage::rgbImage( const int xin, const int yin,
		   const int n_comp_pixels_in, gBColor* comp_pixels_in,
		   const int n_comp_runs_in, int* comp_runs_in,
		   const int owns_compbuf_in )
{  
  xdim= xin;
  ydim= yin;
  buf= NULL;
  current_pix= NULL;
  compressed_flag= 1;
  image_valid= 1;
  owns_buffer= 0;
  
  n_comp_pixels= n_comp_pixels_in;
  n_comp_runs= n_comp_runs_in;
  comp_pixels= comp_pixels_in;  // *not* a copy of the buffer!
  byte_comp_pixels= NULL;
  comp_runs= comp_runs_in;      // *not* a copy of the buffer!
  owns_compbuf= owns_compbuf_in;
}

rgbImage::rgbImage( const int xin, const int yin,
		   const int n_comp_pixels_in, unsigned char* comp_pixels_in,
		   const int n_comp_runs_in, int* comp_runs_in,
		   const int owns_compbuf_in )
{
  xdim= xin;
  ydim= yin;
  buf= NULL;
  current_pix= NULL;
  compressed_flag= 1;
  image_valid= 1;
  owns_buffer= 0;
  
  n_comp_pixels= n_comp_pixels_in;
  n_comp_runs= n_comp_runs_in;
  comp_pixels= NULL;
  byte_comp_pixels= comp_pixels_in;  // *not* a copy of the buffer!
  comp_runs= comp_runs_in;      // *not* a copy of the buffer!
  owns_compbuf= owns_compbuf_in;
}

rgbImage::~rgbImage()
{
  if (valid() && owns_buffer) {
#ifndef AVOID_IMTOOLS
    ImVfbFree( buf );
#else
    ImVfbFree_private( buf );
#endif
  }
  if (owns_compbuf) {
    delete [] comp_pixels;
    delete [] byte_comp_pixels;
  }
  delete comp_runs; // always deleted
}

void rgbImage::clear( gBColor pix )
{
  if (compressed()) {
    if (comp_pixels) {
      for (gBColor* thispix= comp_pixels; 
	   thispix < comp_pixels + n_comp_pixels; thispix++) 
	*thispix= pix;
    }
    else {
      for (unsigned char* runner= byte_comp_pixels;
	   runner < byte_comp_pixels + 4*n_comp_pixels; ) {
	*runner++= pix.ir();
	*runner++= pix.ig();
	*runner++= pix.ib();
	*runner++= pix.ia();
      }
    }
  }
  else {
    ImVfbPtr p, p1, p2;
    p1= ImVfbQPtr( buf, 0, 0 );
    p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
    for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
      ImVfbSRed( buf, p, pix.ir() );
      ImVfbSGreen( buf, p, pix.ig() );
      ImVfbSBlue( buf, p, pix.ib() );
      ImVfbSAlpha( buf, p, pix.ia() );
    }
  }
}

void rgbImage::add_over( rgbImage *other )
// Note that this method assumes premultiplication by alpha!
{
  int alpha_byte;
  float alphadif;

  if (compressed()) uncompress();

  if (other->compressed()) {
    register ImVfbPtr p= ImVfbQPtr( buf, 0, 0 );
    gBColor transparent_black; // initializes properly
    register int i;

    if (other->comp_pixels) {
      register gBColor* src_pixel= other->comp_pixels;

      for (int* src_runs= other->comp_runs; 
	   src_runs < other->comp_runs + other->n_comp_runs; 
	   src_runs++) {
	if (*src_runs<0) {
	  p= (ImVfbPtr)((char*)p + -4 * (*src_runs));
	}
	else {
	  for (i=0; i<*src_runs; i++) {
	    if ((src_pixel->ia() == 255) || (ImVfbQAlpha(buf,p) == 0)) { 
	      ImVfbSRed( buf, p, src_pixel->ir() );
	      ImVfbSGreen( buf, p, src_pixel->ig() );
	      ImVfbSBlue( buf, p, src_pixel->ib() );
	      ImVfbSAlpha( buf, p, src_pixel->ia() );
	    }
	    // No black source pixels in compressed images
	    else {
	      alphadif= alpha_dif(src_pixel->ia());
	      ImVfbSRed( buf, p, 
			(int)(src_pixel->ir()+alphadif*ImVfbQRed(buf,p)) );
	      ImVfbSGreen( buf, p, 
			  (int)(src_pixel->ig()+alphadif*ImVfbQGreen(buf,p)) );
	      ImVfbSBlue( buf, p, 
			 (int)(src_pixel->ib()+alphadif*ImVfbQBlue(buf,p)) );
	      ImVfbSAlpha( buf, p, 
			  (int)(src_pixel->ia()+alphadif*ImVfbQAlpha(buf,p)) );
	    }
	    ImVfbSInc( buf, p );
	    src_pixel++;
	  }
	}
      }
    }
    else { // compression by bytes
      register unsigned char* src_byte= other->byte_comp_pixels;
      for (int* src_runs= other->comp_runs; 
	   src_runs < other->comp_runs + other->n_comp_runs; 
	   src_runs++) {
	if (*src_runs<0) {
	  p= (ImVfbPtr)((char*)p + -4 * (*src_runs));
	}
	else {
#ifdef CRAY_T3D
	  // At this point one might think that the right thing to do
	  // would be to prefetch appropriate strips of the image into
	  // the cache, to accelerate compositing.  Timing tests show,
	  // however, that for realistic images this is of negligible
	  // benefit.  The compositing algorithm skips over regions of
	  // complete opacity, which makes prefetching in such regions
	  // a waste of time.  The thing that probably really slows this
	  // step down is the write-through nature of the cache, which
	  // may cause a write to happen each time a pixel is finished.
	  int pix_count= *src_runs;
	  register short pix_tmp_store;
	  unsigned char* pix_tmp= (unsigned char*)&pix_tmp_store;
	  for (i=0; i<pix_count; i++) {
	    alpha_byte= *(src_byte+3);
	    if ((alpha_byte == 255) || (ImVfbQAlpha(buf,p) == 0)) { 
	      *(short*)p= *(short*)src_byte;
	      src_byte += 4;
	    }
	    // Black source pixels may occur on the T3D, because of the
	    // shmem mechanism for passing images.
	    else if (alpha_byte == 0) {
	      // do nothing; pixel is correct
	      src_byte += 4;
	    }
	    else {
	      alphadif= alpha_dif(alpha_byte);
	      pix_tmp[0]= (int)((*src_byte++) + alphadif*ImVfbQRed(buf,p));
	      pix_tmp[1]= (int)((*src_byte++) + alphadif*ImVfbQGreen(buf,p));
	      pix_tmp[2]= (int)((*src_byte++) + alphadif*ImVfbQBlue(buf,p));
	      pix_tmp[3]= (int)((*src_byte++) + alphadif*ImVfbQAlpha(buf,p));
	      *(short*)p= pix_tmp_store;
	    }
	    ImVfbSInc( buf, p );
	  }
#ifdef never
	  // Cray T3D cache is 1024 64-bit words long, arranged as 256 
	  // lines of 4 words each.  The source and target arrays are
	  // aligned to be offset by 1/2 the cache.  We want to strip
	  // mine the merge operation by loading the source and target
	  // arrays in 512 word chunks.  Since each 64 bit word contains
	  // 2 pixels, we want to load 1024-pixel blocks, which are
	  // 128 cache lines long each.  
	  int cache_lines_to_fetch= 128;
	  int pix_count= *src_runs;
	  int pix_this_run= pix_count;
	  while (pix_count>0) {
	    pix_this_run= (pix_count >= 1024) ? 1024 : pix_count;
	    PREF( &cache_lines_to_fetch, (long*)src_byte, (long*)p );
	    
	    for (i=0; i<pix_this_run; i++) {
	      alpha_byte= *(src_byte+3);
	      if ((alpha_byte == 255) || (ImVfbQAlpha(buf,p) == 0)) { 
		ImVfbSRed( buf, p, *src_byte ); src_byte++;
		ImVfbSGreen( buf, p, *src_byte ); src_byte++;
		ImVfbSBlue( buf, p, *src_byte ); src_byte++;
		ImVfbSAlpha( buf, p, *src_byte ); src_byte++;
	      }
	      // Black source pixels may occur on the T3D, because of the
	      // shmem mechanism for passing images.
	      else if (alpha_byte == 0) {
		// do nothing; pixel is correct
		src_byte += 4;
	      }
	      else {
		alphadif= alpha_dif(alpha_byte);
		ImVfbSRed( buf, p, 
			  (int)((*src_byte) + alphadif*ImVfbQRed(buf,p)) );
		src_byte++;
		ImVfbSGreen( buf, p, 
			    (int)((*src_byte) + alphadif*ImVfbQGreen(buf,p)) );
		src_byte++;
		ImVfbSBlue( buf, p, 
			   (int)((*src_byte) + alphadif*ImVfbQBlue(buf,p)) );
		src_byte++;
		ImVfbSAlpha( buf, p, 
			    (int)((*src_byte) + alphadif*ImVfbQAlpha(buf,p)) );
		src_byte++;
	      }
	      ImVfbSInc( buf, p );
	    }
	    pix_count -= pix_this_run;
	  }
#endif
#else
	  for (i=0; i<*src_runs; i++) {
	    alpha_byte= *(src_byte+3);
	    if ((alpha_byte == 255) || (ImVfbQAlpha(buf,p) == 0)) { 
	      ImVfbSRed( buf, p, *src_byte ); src_byte++;
	      ImVfbSGreen( buf, p, *src_byte ); src_byte++;
	      ImVfbSBlue( buf, p, *src_byte ); src_byte++;
	      ImVfbSAlpha( buf, p, *src_byte ); src_byte++;
	    }
	    // No black source pixels in compressed images in normal case
	    else {
	      alphadif= alpha_dif(alpha_byte);
	      ImVfbSRed( buf, p, 
			(int)((*src_byte) + alphadif*ImVfbQRed(buf,p)) );
	      src_byte++;
	      ImVfbSGreen( buf, p, 
			  (int)((*src_byte) + alphadif*ImVfbQGreen(buf,p)) );
	      src_byte++;
	      ImVfbSBlue( buf, p, 
			 (int)((*src_byte) + alphadif*ImVfbQBlue(buf,p)) );
	      src_byte++;
	      ImVfbSAlpha( buf, p, 
			  (int)((*src_byte) + alphadif*ImVfbQAlpha(buf,p)) );
	      src_byte++;
	    }
	    ImVfbSInc( buf, p );
	  }
#endif
	}
      }
    }
  }
  else { // uncompressed source
    register ImVfbPtr p;
    ImVfbPtr p1, p2;
    register ImVfbPtr p_other;
    p1= ImVfbQPtr( buf, 0, 0 );
    p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
    p_other= ImVfbQPtr( other->buf, 0, 0 );
    for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
      if ((ImVfbQAlpha(other->buf,p_other) == 255) 
	  || (ImVfbQAlpha(buf,p) == 0)) {
	ImVfbSRed( buf, p, ImVfbQRed( other->buf, p_other ) );
	ImVfbSGreen( buf, p, ImVfbQGreen( other->buf, p_other ) );
	ImVfbSBlue( buf, p, ImVfbQBlue( other->buf, p_other ) );
	ImVfbSAlpha( buf, p, ImVfbQAlpha( other->buf, p_other ) );
      }
      else if ( ImVfbQAlpha(other->buf,p_other) == 0 ) {
	// Do nothing;  pixel is correct
      }
      else {
	alphadif= alpha_dif(ImVfbQAlpha( other->buf, p ) );
	ImVfbSRed( buf, p, (int)(ImVfbQRed(other->buf,p_other)
				 + alphadif*ImVfbQRed(buf,p)) );
	ImVfbSGreen( buf, p, (int)(ImVfbQGreen(other->buf,p_other)
				   + alphadif*ImVfbQGreen(buf,p)) );
	ImVfbSBlue( buf, p, (int)(ImVfbQBlue(other->buf,p_other)
				  + alphadif*ImVfbQBlue(buf,p)) );
	ImVfbSAlpha( buf, p, (int)(ImVfbQAlpha(other->buf,p_other)
				   + alphadif*ImVfbQAlpha(buf,p)) );
      }
      ImVfbSInc( other->buf, p_other );
    }
  }
}

void rgbImage::add_under( rgbImage *other )
// Note that this method assumes premultiplication by alpha!
{
  float alphadif;

  if (compressed()) uncompress();

  register ImVfbPtr p= ImVfbQPtr( buf, 0, 0 );
  gBColor transparent_black; // initializes properly
  register int i;

  if (other->compressed()) {
    if (other->comp_pixels) {
      register gBColor* src_pixel= other->comp_pixels;
      for (int* src_runs= other->comp_runs; 
	   src_runs < other->comp_runs + other->n_comp_runs; 
	   src_runs++) {
	if (*src_runs<0) {
	  p= (ImVfbPtr)((char*)p + -4 * (*src_runs));
	}
	else {
	  for (i=0; i<*src_runs; i++) {
	    // No black source pixels in compressed images
	    if ( ImVfbQAlpha(buf,p) == 255 ) {
	      // Pixel is correct; do nothing
	    }
	    else if ( ImVfbQAlpha(buf,p) == 0 ) {
	      ImVfbSRed( buf, p, src_pixel->ir() );
	      ImVfbSGreen( buf, p, src_pixel->ig() );
	      ImVfbSBlue( buf, p, src_pixel->ib() );
	      ImVfbSAlpha( buf, p, src_pixel->ia() );
	    }
	    else {
	      alphadif= alpha_dif( ImVfbQAlpha( buf, p ) );
	      ImVfbSRed( buf, p, (int)(ImVfbQRed(buf,p)
				       + alphadif*src_pixel->ir()) );
	      ImVfbSGreen( buf, p, (int)(ImVfbQGreen(buf,p)
					 + alphadif*src_pixel->ig()) );
	      ImVfbSBlue( buf, p, (int)(ImVfbQBlue(buf,p)
					+ alphadif*src_pixel->ib()) );
	      ImVfbSAlpha( buf, p, (int)(ImVfbQAlpha(buf,p)
					 + alphadif*src_pixel->ia()) );
	    }
	    ImVfbSInc( buf, p );
	    src_pixel++;
	  }
	}
      }
    }
    else { // compressed by bytes
      register unsigned char* src_byte= other->byte_comp_pixels;
      for (int* src_runs= other->comp_runs; 
	   src_runs < other->comp_runs + other->n_comp_runs; 
	   src_runs++) {
	if (*src_runs<0) {
	  p= (ImVfbPtr)((char*)p + -4 * (*src_runs));
	}
	else {
#ifdef CRAY_T3D
	  // At this point one might think that the right thing to do
	  // would be to prefetch appropriate strips of the image into
	  // the cache, to accelerate compositing.  Timing tests show,
	  // however, that for realistic images this is of negligible
	  // benefit.  The compositing algorithm skips over regions of
	  // complete opacity, which makes prefetching in such regions
	  // a waste of time.  The thing that probably really slows this
	  // step down is the write-through nature of the cache, which
	  // may cause a write to happen each time a pixel is finished.
	  int pix_count= *src_runs;
	  register short pix_tmp_store;
	  unsigned char* pix_tmp= (unsigned char*)&pix_tmp_store;
	  for (i=0; i<pix_count; i++) {
	    // Black source pixels may occur on the T3D, because of the
	    // shmem mechanism for passing images.
	    if ( (ImVfbQAlpha(buf,p) == 255) || (*(src_byte+3) == 0) ) {
	      // Pixel is correct; do nothing
	      src_byte += 4;
	    }
	    else if ( ImVfbQAlpha(buf,p) == 0 ) {
	      *(short*)p= *(short*)src_byte;
	      src_byte += 4;
	    }
	    else {
	      alphadif= alpha_dif( ImVfbQAlpha( buf, p ) );
	      pix_tmp[0]= (int)(ImVfbQRed(buf,p) + alphadif*(*src_byte++));
	      pix_tmp[1]= (int)(ImVfbQGreen(buf,p) + alphadif*(*src_byte++));
	      pix_tmp[2]= (int)(ImVfbQBlue(buf,p) + alphadif*(*src_byte++));
	      pix_tmp[3]= (int)(ImVfbQAlpha(buf,p) + alphadif*(*src_byte++));
	      *(short*)p= pix_tmp_store;
	    }
	    ImVfbSInc( buf, p );
	  }
#else // ifdef CRAY_T3D
	  for (i=0; i<*src_runs; i++) {
	    // No black source pixels in compressed images in usual case
	    if ( ImVfbQAlpha(buf,p) == 255 ) {
	      // Pixel is correct; do nothing
	      src_byte += 4;
	    }
	    else if ( ImVfbQAlpha(buf,p) == 0 ) {
	      ImVfbSRed( buf, p, *src_byte ); src_byte++;
	      ImVfbSGreen( buf, p, *src_byte ); src_byte++;
	      ImVfbSBlue( buf, p, *src_byte ); src_byte++;
	      ImVfbSAlpha( buf, p, *src_byte ); src_byte++;
	    }
	    else {
	      alphadif= alpha_dif( ImVfbQAlpha( buf, p ) );
	      ImVfbSRed( buf, p, (int)(ImVfbQRed(buf,p)
				       + alphadif*(*src_byte)) );
	      src_byte++;
	      ImVfbSGreen( buf, p, (int)(ImVfbQGreen(buf,p)
					 + alphadif*(*src_byte)) );
	      src_byte++;
	      ImVfbSBlue( buf, p, (int)(ImVfbQBlue(buf,p)
					+ alphadif*(*src_byte)) );
	      src_byte++;
	      ImVfbSAlpha( buf, p, (int)(ImVfbQAlpha(buf,p)
					 + alphadif*(*src_byte)) );
	      src_byte++;
	    }
	    ImVfbSInc( buf, p );
	  }
#endif // CRAY_T3D
	}
      }
    }
  }
  else { // uncompressed source
    register ImVfbPtr p;
    ImVfbPtr p1, p2;
    register ImVfbPtr p_other;
    p1= ImVfbQPtr( buf, 0, 0 );
    p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
    p_other= ImVfbQPtr( other->buf, 0, 0 );
    for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
      if ((ImVfbQAlpha(buf,p) == 255) 
	  || (ImVfbQAlpha(other->buf, p_other) == 0)) {
	// Pixel is correct;  do nothing
      }
      else if ( ImVfbQAlpha(buf,p) == 0 ) {
	ImVfbSRed( buf, p, ImVfbQRed(other->buf, p_other) );
	ImVfbSGreen( buf, p, ImVfbQGreen(other->buf, p_other) );
	ImVfbSBlue( buf, p, ImVfbQBlue(other->buf, p_other) );
	ImVfbSAlpha( buf, p, ImVfbQAlpha(other->buf, p_other) );
      }
      else {
	alphadif= alpha_dif( ImVfbQAlpha( buf, p ) );
	ImVfbSRed( buf, p, (int)(ImVfbQRed(buf,p)
				 + alphadif*ImVfbQRed(other->buf,p_other)));
	ImVfbSGreen( buf, p, (int)(ImVfbQGreen(buf,p)
				 + alphadif*ImVfbQGreen(other->buf,p_other)));
	ImVfbSBlue( buf, p, (int)(ImVfbQBlue(buf,p)
				 + alphadif*ImVfbQBlue(other->buf,p_other)));
	ImVfbSAlpha( buf, p, (int)(ImVfbQAlpha(buf,p)
				 + alphadif*ImVfbQAlpha(other->buf,p_other)));
      }
      ImVfbSInc( other->buf, p_other );
    }
  }
}

void rgbImage::rescale_by_alpha()
// Note that this method assumes premultiplication by alpha!  The purpose
// of this routine is to undo that premultiplication.
{
  float inv_alpha;

  if (compressed()) {
    if (comp_pixels) {
      for (gBColor* thispix= comp_pixels; thispix<comp_pixels+n_comp_pixels;
	   thispix++) {
	if ((thispix->ia() != 255) && (thispix->ia() != 0)) {
	  inv_alpha= 255.0/((float)thispix->ia());
	  *thispix= gBColor( bounds_check((int)(inv_alpha * thispix->ir())),
			    bounds_check((int)(inv_alpha * thispix->ig())),
			    bounds_check((int)(inv_alpha * thispix->ib())),
			    255 );
	}
      }
    }
    else { // compressed by bytes
      register int ialpha;
      register float inv_alpha;
      for (unsigned char* runner= byte_comp_pixels;
	   runner < byte_comp_pixels + 4*n_comp_pixels; ) {
	ialpha= *(runner+3);
	if ((ialpha != 0) && (ialpha != 255)) {
	  inv_alpha= 255.0/(float)ialpha;
	  *runner++= bounds_check((int)(inv_alpha * *runner)); // r
	  *runner++= bounds_check((int)(inv_alpha * *runner)); // g
	  *runner++= bounds_check((int)(inv_alpha * *runner)); // b
	  *runner++= 255; // a
	}
	else {
	  runner += 4;
	}
      }
    }
  }
  else {
    ImVfbPtr p, p1, p2;
    p1= ImVfbQPtr( buf, 0, 0 );
    p2= ImVfbQPtr( buf, xdim-1, ydim-1 );
    for ( p=p1; p<=p2; ImVfbSInc(buf,p) ) {
      if ((ImVfbQAlpha( buf, p ) != 255) && (ImVfbQAlpha( buf, p ) != 0)) {
	inv_alpha= 255.0/((float)ImVfbQAlpha( buf, p ));
	ImVfbSRed( buf, p, bounds_check((int)(inv_alpha*ImVfbQRed(buf,p))) );
	ImVfbSGreen( buf, p, 
		    bounds_check((int)(inv_alpha*ImVfbQGreen(buf,p))) );
	ImVfbSBlue( buf, p, bounds_check((int)(inv_alpha*ImVfbQBlue(buf,p))) );
	ImVfbSAlpha( buf, p, 255 );
      }
    }
  }
}

#ifdef AVOID_IMTOOLS

static int write_int16(FILE *ofile, const unsigned int val)
{
  if (putc(val/256, ofile) == EOF) return 0;
  if (putc(val % 256, ofile) == EOF) return 0;
  return 1;
}

static int write_int32(FILE *ofile, const unsigned int val)
{
  unsigned int tval= val;
  if (putc(tval/(256*256*256), ofile) == EOF) return 0;
  tval= tval % (256*256*256);
  if (putc(val/(256*256), ofile) == EOF) return 0;
  tval= tval % (256*256);
  if (putc(tval/256, ofile) == EOF) return 0;
  if (putc(tval%256, ofile) == EOF) return 0;
  return 1;
}

static int write_rational(FILE *ofile,
			  const unsigned int numerator, 
			  const unsigned int denominator)
{
  if (!write_int32(ofile,numerator)) return 0;
  if (!write_int32(ofile,denominator)) return 0;
  return 1;
}

static int write_field(FILE *ofile, const int tag, const int type, 
		       const unsigned int value)
{
  if (!write_int16(ofile,tag)) return 0;
  if (!write_int16(ofile,type)) return 0;
  if (!write_int32(ofile, 1)) return 0;
  switch (type) {
  case 1: 
    { // byte
      if (putc(value,ofile) == EOF) return 0;
      for (int i=0; i<3; i++) if (putc(0,ofile) == EOF) return 0;
    }
    break;
  case 3: // short
    if (!write_int16(ofile,value)) return 0;
    if (!write_int16(ofile,0)) return 0;
    break;
  case 4: // long
    if (!write_int32(ofile,value)) return 0;
    break;
  default:
    fprintf(stderr,"TIFF writing error: write_field can't write type %d!\n",
	    type);
    return 0;
    break;
  }

  return 1;
}

static int write_field_offset(FILE *ofile, const int tag, const int type,
			     const int count, const int table_offset)
{
  if (!write_int16(ofile,tag)) return 0;
  if (!write_int16(ofile,type)) return 0;
  if (!write_int32(ofile, count)) return 0;
  if (!write_int32(ofile, table_offset)) return 0;
  return 1;
}

static int write_to_tiff(const char *fname, const int xdim, const int ydim,
			 ImVfb* image)
{
  FILE *ofile= fopen(fname,"w");
  if (ofile == NULL) {
    fprintf(stderr,"rgbImage::save: cannot open <%s> for writing!\n",fname);
    return(0);
  }

  int num_ifd_entries= 13;

  int bps_offset=
    2+2+4   // header
    +2      // number of directory entries
    +num_ifd_entries*12  // number of IFD entries * size of an entry
    +4;     // four bytes of zero after last IFD

  int xres_offset= bps_offset + 2*4;  // shift by bits per sample size
  int yres_offset= xres_offset + 2*4; // shift by xres_offset size

  int image_offset= yres_offset + 2*4; // shift by yres_offset size

  // Write the header

  if (!write_int16(ofile,0x4d4d)) return 0;     // byte order MM
  if (!write_int16(ofile,0x2a)) return 0;       // TIFF magic number
  if (!write_int32(ofile,0x8)) return 0;        // Offset of first IFD
  if (!write_int16(ofile,num_ifd_entries)) return 0; // IFD begins; num entries
  
  if (!write_field(ofile,0x100,4,xdim)) return 0; // width in long format
  if (!write_field(ofile,0x101,4,ydim)) return 0; // height in long format

  // bits per sample
  if (!write_field_offset(ofile, 0x102, 3, 4, bps_offset)) return 0;

  if (!write_field(ofile,0x103,3,1)) return 0;  // compression
  if (!write_field(ofile,0x106,3,2)) return 0;  // photometric interpretation

  // strip offsets
  if (!write_field(ofile,0x111,4,image_offset)) return 0; // One strip offset

  if (!write_field(ofile,0x115,3,4)) return 0;  // samples per pixel
  if (!write_field(ofile,0x116,3,ydim)) return 0;  // rows per strip

  // strip byte counts
  int strip_byte_count= 4*xdim*ydim;
  if (!write_field(ofile,0x117,4,strip_byte_count)) return 0; // one byte count

  // X and Y resolution
  if (!write_field_offset(ofile,0x11a,5,1,xres_offset)) return 0;
  if (!write_field_offset(ofile,0x11b,5,1,yres_offset)) return 0;
  if (!write_field(ofile,0x128, 3, 1)) return 0;        // ResolutionUnit
  if (!write_field(ofile,0x152, 3, 1)) return 0;        // ExtraSamples info

  if (!write_int32(ofile,0)) return 0; // close IFD

  // Write the needed array data

  // Bits per sample table
  for (int i=0; i<4; i++) {
    if (!write_int16(ofile,8)) return 0;
  }

  // X and Y resolution, in that order
  if (!write_rational(ofile,1,1)) return 0;
  if (!write_rational(ofile,1,1)) return 0;

  // Write the image
  ImVfbPtr p, p1, p2;
  p1= ImVfbQPtr( image, 0, 0 );
  p2= ImVfbQPtr( image, xdim-1, ydim-1 );
  for ( p=p1; p<=p2; ImVfbSInc(image,p) ) {
    if (putc(ImVfbQRed(image,p),ofile) == EOF) return 0;
    if (putc(ImVfbQGreen(image,p),ofile) == EOF) return 0;
    if (putc(ImVfbQBlue(image,p),ofile) == EOF) return 0;
    if (putc(ImVfbQAlpha(image,p),ofile) == EOF) return 0;
  }

  // Close up
  if (fclose(ofile) == EOF) {
    fprintf(stderr,"rgbImage::save: error closing file <%s>!\n",fname);
    return(0);
  }
    
  return(1);
}

static int write_to_ps( const char *fname, const int xdim, const int ydim,
			ImVfb* image )
{
  FILE *ofile= fopen(fname,"w");
  if (ofile == NULL) {
    fprintf(stderr,"rgbImage::save: cannot open <%s> for writing!\n",fname);
    return(0);
  }

  // Calculate Postscript transformation coefficients to place image
  // in center of a 7.5 inch by 10 inch region on page center.  Postscript
  // uses a coordinate system which is 72 points per inch.
  int xshift, yshift, xscale, yscale;
  float fxshift, fyshift, fxscale, fyscale;
  if ((xdim * 10.0) > (ydim * 7.5)) {
    // Touches X edges
    fxscale= 7.5*72;
    fxshift= 0.5*72;
    fyscale= (fxscale*ydim)/(float)xdim;
    fyshift= 5.5*72 - 0.5*fyscale;
  }
  else {
    // Touches Y edges
    fyscale= 10.0*72;
    fyshift= 0.5*72;
    fxscale= (fyscale*xdim)/(float)ydim;
    fxshift= 4.25*72 - 0.5*fxscale;
  }
  xscale= (int)fxscale;
  yscale= (int)fyscale;
  xshift= (int)fxshift;
  yshift= (int)fyshift;

  static char header_string[]=
    "%%!PS-Adobe-1.0 \n\
%%%%Creator: VFleet \n\
%%%%BoundingBox 0. 0. 432. 576. \n\
%%%%EndComments \n\
save gsave \n\
/str %d string def \n\
%d %d translate \n\
%d %d scale \n\
%d %d 8 [%d 0 0 %d 0 %d ] \n\
{currentfile str readhexstring pop} false 3 colorimage \n";

  static char trailer_string[]= "grestore restore showpage\n";

  fprintf(ofile,header_string,
	  xdim*3,
          xshift, yshift,
          xscale, yscale,
	  xdim, ydim, 
          xdim, -ydim, ydim);

  // Write the image
  ImVfbPtr p, p1, p2;
  p1= ImVfbQPtr( image, 0, 0 );
  p2= ImVfbQPtr( image, xdim-1, ydim-1 );
  int counter= 0;
  for ( p=p1; p<=p2; ImVfbSInc(image,p) ) {
    fprintf(ofile,"%02x%02x%02x",
	    ImVfbQRed(image,p),ImVfbQGreen(image,p),
	    ImVfbQBlue(image,p));
    counter += 6;
    if (counter>=72) {
      putc('\n',ofile);
      counter= 0;
    }
  }  

  fputs(trailer_string,ofile);

  // Close up
  if (fclose(ofile) == EOF) {
    fprintf(stderr,"rgbImage::save: error closing file <%s>!\n",fname);
    return(0);
  }
    
  return(1);
}

static int write_to_png(const char *fname, const int xdim, const int ydim,
			 ImVfb* image)
{
#ifdef INCL_PNG
  FILE *ofile= fopen(fname,"w");
  if (ofile == NULL) {
    fprintf(stderr,"rgbImage::save: cannot open <%s> for writing!\n",fname);
    return(0);
  }
  png_structp png_ptr= 
    png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fprintf(stderr,"rgbImage::save: unable to create png data structure!\n");
    fclose(ofile);
    return 0;
  }
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    fclose(ofile);
    fprintf(stderr,"rgbImage::save: unable to create png data structure 2!\n");
    return 0;
  }
  
  if (setjmp(png_ptr->jmpbuf)) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(ofile);
    fprintf(stderr,"rgbImage::save: fatal error in png libraries!\n");
    return 0;
  }

  png_init_io(png_ptr, ofile);

  info_ptr->width= xdim;
  info_ptr->height= ydim;
  info_ptr->color_type= PNG_COLOR_TYPE_RGBA;
  info_ptr->bit_depth= 8;
  info_ptr->gamma= 1.0;

  info_ptr->num_text= 0;
  info_ptr->max_text= 0;
  info_ptr->text= NULL;

  png_write_info(png_ptr, info_ptr);

  png_bytep row_ptr;
  if (!(row_ptr=(png_bytep)malloc(4*xdim))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(ofile);
    fprintf(stderr,"rgbImage::save: unable to allocate %d bytes!\n",
	    4*xdim);
    return 0;
  }

  for (int j=0; j<ydim; j++) {
    ImVfbPtr p1= ImVfbQPtr( image, 0, j );
    ImVfbPtr p2= ImVfbQPtr( image, xdim-1, j );
    png_bytep here= row_ptr;
    for (ImVfbPtr p= p1; p<=p2; ImVfbSInc(image,p)) {
      *here++= ImVfbQRed(image,p);
      *here++= ImVfbQGreen(image,p);
      *here++= ImVfbQBlue(image,p);
      *here++= ImVfbQAlpha(image,p);
    }
    png_write_row(png_ptr, row_ptr);
  }

  free(row_ptr);

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr); /* seems to free notes */

  // Close up
  if (fclose(ofile) == EOF) {
    fprintf(stderr,"rgbImage::save: error closing file <%s>!\n",fname);
    return(0);
  }
#endif    

  return(1);
}

#endif // ifdef AVOID_IMTOOLS

int rgbImage::save( char *fname, char *format )
{
  if (valid()) { 
    if (compressed()) uncompress();

#ifndef AVOID_IMTOOLS

    FILE *file;
    TagTable *dataTable;

    dataTable= TagTableAlloc();
    file= fopen(fname, "w");
    if (!file) {
      fprintf(stderr,
	      "rgbImage::save: cannot open file %s for writing!\n",
	      fname);
      return(0);
    }
    TagTableAppend( dataTable, TagEntryAlloc( "image vfb", POINTER, &buf ) ); 
    if ( ImFileFWrite( file, format, NULL, dataTable ) == -1 ) {
      fprintf(stderr,
	      "rgbImage::save: image save failed; error %d!\n",
	      ImErrNo);
      TagTableFree(dataTable);
      return(0);
    }
    if (fclose( file ) == EOF) {
      fprintf(stderr,"rgbImage::save: could not close file <%s>!\n",
	      fname);
      return(0);
    }
    return(1);

#else // ifndef AVOID_IMTOOLS
    if (!strcmp(format,"tiff"))
      return write_to_tiff(fname, xsize(), ysize(), buf);
    else if (!strcmp(format,"ps"))
      return write_to_ps(fname, xsize(), ysize(), buf);
#ifdef INCL_PNG
    else if (!strcmp(format,"png"))
      return write_to_png(fname, xsize(), ysize(), buf);
#endif
    else {
      fprintf(stderr,
  "rgbImage::save: only TIFF and PS formats supported; cannot handle %s!\n",
	      format);
      return(0);
    }
#endif // ifndef AVOID_IMTOOLS

  }
  else {
    fprintf(stderr,"rgbImage::save: can't save an invalid image!\n");
    return(0);
  }
}

void rgbImage::uncompress()
{
  // Compressed images are stored as an array of pixels, and an array
  // of runs.  A positive number in the runs array means that that
  // number of pixels from the pixels array should be used.  A negative
  // number means that the absolute value of that number of black 
  // transparent pixels should be used.

#ifndef AVOID_IMTOOLS
  buf= ImVfbAlloc(xsize(),ysize(), IMVFBRGB | IMVFBALPHA);
#else
  buf= ImVfbAlloc_private(xsize(),ysize());
#endif // ifndef AVOID_IMTOOLS

  owns_buffer= 1;

  register ImVfbPtr p= ImVfbQPtr( buf, 0, 0 );
  gBColor transparent_black; // initializes properly
  register int i;

  if (comp_pixels) {
    register gBColor* thispix= comp_pixels;
    for (int* thisrun= comp_runs; thisrun<comp_runs+n_comp_runs; thisrun++) {
      if (*thisrun >= 0) {
	for (i=0; i<*thisrun; i++) {
	  ImVfbSRed( buf, p, thispix->ir() );
	  ImVfbSGreen( buf, p, thispix->ig() );
	  ImVfbSBlue( buf, p, thispix->ib() );
	  ImVfbSAlpha( buf, p, thispix->ia() );
	  ImVfbSInc( buf, p );      
	  thispix++;
	}
      }
      else {
	for (i=0; i<(-1*(*thisrun)); i++) {
	  ImVfbSRed( buf, p, transparent_black.ir() );
	  ImVfbSGreen( buf, p, transparent_black.ig() );
	  ImVfbSBlue( buf, p, transparent_black.ib() );
	  ImVfbSAlpha( buf, p, transparent_black.ia() );
	  ImVfbSInc( buf, p );      
	}
      }
    }
  }
  else {
    register unsigned char* runner= byte_comp_pixels;
    for (int* thisrun= comp_runs; thisrun<comp_runs+n_comp_runs; thisrun++) {
      if (*thisrun >= 0) {
	for (i=0; i<*thisrun; i++) {
	  ImVfbSRed( buf, p, *runner ); runner++;
	  ImVfbSGreen( buf, p, *runner ); runner++;
	  ImVfbSBlue( buf, p, *runner ); runner++;
	  ImVfbSAlpha( buf, p, *runner ); runner++;
	  ImVfbSInc( buf, p );
	}
      }
      else {
	for (i=0; i<(-1*(*thisrun)); i++) {
	  ImVfbSRed( buf, p, transparent_black.ir() );
	  ImVfbSGreen( buf, p, transparent_black.ig() );
	  ImVfbSBlue( buf, p, transparent_black.ib() );
	  ImVfbSAlpha( buf, p, transparent_black.ia() );
	  ImVfbSInc( buf, p );      
	}
      }
    }
  }

  if (owns_compbuf) {
    delete [] comp_pixels;
    delete [] byte_comp_pixels;
  }
  comp_pixels= NULL;
  byte_comp_pixels= NULL;
  n_comp_pixels= 0;
  delete comp_runs;
  comp_runs= NULL;
  n_comp_runs= 0;
  compressed_flag= 0;
  owns_compbuf= 0;
}

#ifdef CRAY_T3D
unsigned char* rgbImage::get_matched_memory(const int xdim_in, 
					    const int ydim_in)
{
  if (mem_block_size < 4 * xdim_in * ydim_in) {
    fprintf(stderr,
	    "rgbImage::get_matched_memory: algorithm error (size %d)!\n",
	    mem_block_size);
    exit(-1);
  }
#ifdef never
char msgbuf[128];
sprintf(msgbuf,"get_matched_memory: returning %x=%d, %x=%d\n",
(int)mem_block_hook,(int)mem_block_hook,
(int)(mem_block_hook + mem_block_size + 4096), 
(int)mem_block_hook+mem_block_size+4096);
fprintf(stderr,msgbuf);
#endif
  return( mem_block_hook 
	 + mem_block_size // skip first image, already in use
	 + 4096 );  // 4096 bytes is offset of half the cache size
}
#endif
