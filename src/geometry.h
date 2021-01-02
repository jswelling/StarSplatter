/****************************************************************************
 * geometry.h
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

#ifndef GEOMETRY_H_INCLUDED
#define GEOMETRY_H_INCLUDED

#include <math.h>
#include <string.h>

#ifndef PI
#define PI M_PI
#endif

const double DegtoRad= PI/180.0;
const double RadtoDeg= 180.0/PI;

class gBVector;
class gTransfm;
class gColor;

class gVector {
friend class gPoint;
friend class gColor;
friend class gTransfm;
public:
  gVector( float xin, float yin, float zin ) 
  { data[0]= xin; data[1]= yin; data[2]= zin; data[3]= 1.0; }
  gVector( float xin, float yin, float zin, float win )
  { data[0]= xin; data[1]= yin; data[2]= zin; data[3]= win; }
  inline gVector( const gBVector& bvec );
  gVector() { data[0]= data[1]= data[2]= 0.0; data[3]= 1.0; }
  float x() const { return( data[0] ); }
  float y() const { return( data[1] ); }
  float z() const { return( data[2] ); }
  float w() const { return( data[3] ); }
  void homogenize()
    { 
      if ((data[3]!=1.0) && (data[3]!=0.0)) {
		float one_over_data3 = 1.0 / data[3];	
	data[0] *= one_over_data3;
	data[1] *= one_over_data3;
	data[2] *= one_over_data3;
	data[3]= 1.0;
      }
    }
  float lengthsqr() const
  { return( x()*x() + y()*y() + z()*z() ); }
  float length() const { return( sqrt( lengthsqr() ) ); }
  void normalize();
  gVector operator^( const gVector& other ) const // cross product
  { 
    return gVector( y()*other.z() - z()*other.y(),
		    z()*other.x() - x()*other.z(),
		    x()*other.y() - y()*other.x() ); 
  }
  float operator*( const gVector& other ) const // dot product
  { return( x()*other.x() + y()*other.y() + z()*other.z() ); }
  gVector operator*( const float factor ) const
  { return gVector( factor*x(), factor*y(), factor*z() ); }
  gVector operator+( const gVector& other ) const 
  { return gVector( x()+other.x(), y()+other.y(), z()+other.z() ); }
  gVector& operator+=( const gVector& other )
  {
    data[0] += other.data[0];
    data[1] += other.data[1];
    data[2] += other.data[2];
    data[3] += other.data[3];
    return *this;
  }
  gVector& operator*=( const float factor )
  {
    data[0] *= factor;
    data[1] *= factor;
    data[2] *= factor;
    data[3] *= factor;
    return *this;
  }    
  gVector operator-( const gVector& other ) const 
  { return gVector( x()-other.x(), y()-other.y(), z()-other.z() ); }
  int operator==( const gVector& other ) const
  { return( !memcmp( data, other.data, 4*sizeof(float) ) ); }
  int operator!=( const gVector& other ) const
  { return( memcmp( data, other.data, 4*sizeof(float) ) ); }
  gTransfm *flipping_transform();
  gTransfm *aligning_rotation( const gVector& other );
  gVector normal_component_wrt( const gVector& other );
private:
    float data[4];
};

class gBVector { 
  // compact, normalized vector as bytes, with relative length
  // data bytes are 7 bits magnitude, 1 (high-order) bit sign
public:
    gBVector( const gVector& vec, const float mag_in )
    {
	float one_over_tmag= mag_in >= 0.0 ? 1.0 / mag_in : 1.0 / -mag_in;
	int x= (int)(127*vec.x() * one_over_tmag);
	int y= (int)(127*vec.y() * one_over_tmag);
	int z= (int)(127*vec.z() * one_over_tmag);
	int relmag= (int)(255*vec.length() * one_over_tmag);
	if (x > 127) x= 127;
	if (y > 127) y= 127;
	if (z > 127) z= 127;
	if (x < -128) x= -128;
	if (y < -128) y= -128;
	if (z < -128) z= -128;
	if (relmag > 255) relmag= 255;  
	data[0]= x + 128;
	data[1]= y + 128;
	data[2]= z + 128;
	mag= relmag;
    }
    gBVector( float xin, float yin, float zin, float mag_in ) 
    {
	float one_over_tmag= mag_in >= 0.0 ? 1.0 / mag_in : 1.0 / -mag_in;
	int x= (int)(127*xin * one_over_tmag);
	int y= (int)(127*yin * one_over_tmag);
	int z= (int)(127*zin * one_over_tmag);
	int relmag= (int)(255*sqrt(xin*xin + yin*yin + zin*zin) * one_over_tmag);
	if (x > 127) x= 127;
	if (y > 127) y= 127;
	if (z > 127) z= 127;
	if (relmag > 255) relmag= 255;
	if (x < -128) x= -128;
	if (y < -128) y= -128;
	if (z < -128) z= -128;
	data[0]= x + 128;
	data[1]= y + 128;
	data[2]= z + 128;
	mag= relmag;
    }
    gBVector( int ix, int iy, int iz, int imag )
    {
	data[0]= ix + 128;
	data[1]= iy + 128;
	data[2]= iz + 128;
	mag= imag;
    }
    gBVector() { data[0]= data[1]= data[2]= 128; mag= 0; }
    int ix() const { return data[0] - 128; }
    int iy() const { return data[1] - 128; }
    int iz() const { return data[2] - 128; }
    int imag() const { return mag; }
    float x() const { return( (float)(ix())/127.0 ); }
    float y() const { return( (float)(iy())/127.0 ); }
    float z() const { return( (float)(iz())/127.0 ); }
    float x(float relmag) const 
    { return( ((ix()*mag)*relmag)/(127.0 * 255.0) ); }
    float y(float relmag) const 
    { return( ((iy()*mag)*relmag)/(127.0 * 255.0) ); }
    float z(float relmag) const 
    { return( ((iz()*mag)*relmag)/(127.0 * 255.0) ); }
    float length( const float relmag ) const 
    { return( mag * relmag / 255.0 ); }
    float length() const // Relative to unity
    { return( mag/255.0 ); }
    void normalize() { mag= 255; }
    gBVector operator^( const gBVector& other ) const // cross product
    { 
	return gBVector( (iy()*other.iz() - iz()*other.iy()) >> 7,
			 (iz()*other.ix() - ix()*other.iz()) >> 7,
			 (ix()*other.iy() - iy()*other.ix()) >> 7,
			 (mag*other.mag) >> 8 );
    }
    float operator*( const gBVector& other ) const // dot product
    {
	float cosfactor= (float)(ix()*other.ix()
				 + iy()*other.iy()
				 + iz()*other.iz())
	    / (float)(127*127);
	float result= cosfactor * (mag*other.mag) / (float)(255*255);
	return( result );
    }
    gBVector operator*( const float factor ) const
    {
	gBVector result;
	if (factor > 0.0) {
	  int tmag= (int)(mag*factor);
	  result= *this;
	  result.mag= (tmag <= 255) ? tmag : 255;
	}
	else if (factor < 0.0) {
	  result.data[0] += (data[0] >= 128) ? -128 : 128;
	  result.data[1] += (data[1] >= 128) ? -128 : 128;
	  result.data[2] += (data[2] >= 128) ? -128 : 128;
	  int tmag= -(int)(mag*factor);
	  result.mag= (tmag <= 255) ? tmag : 255;      
	}
	else { // factor is 0.0
	  result.data[0]= result.data[1]= result.data[2]= 128;
	  result.mag= 0;
	}
	return( result );
    }
    int operator==( const gBVector& other ) const
    { return( !memcmp( data, other.data, 4*sizeof(char) ) ); }
    int operator!=( const gBVector& other ) const
    { return( memcmp( data, other.data, 4*sizeof(char) ) ); }
private:
    unsigned char data[3];
    unsigned char mag;
};

gVector::gVector( const gBVector& bvec )
{
  data[0]= bvec.x(); data[1]= bvec.y(); data[2]= bvec.z(); data[3]= 1.0;
}

class gPoint {
friend class gVector;
friend class gColor;
friend class gTransfm;
public:
  gPoint( float xin, float yin, float zin ) 
    { data[0]= xin; data[1]= yin; data[2]= zin; data[3]= 1.0; }
  gPoint( float xin, float yin, float zin, float win )
    { data[0]= xin; data[1]= yin; data[2]= zin; data[3]= win; }
  gPoint() { data[0]= data[1]= data[2]= 0.0; data[3]= 1.0; }
  float x() const { return( data[0] ); }
  float y() const { return( data[1] ); }
  float z() const { return( data[2] ); }
  float w() const { return( data[3] ); }
  void homogenize()
    { 
      if ((data[3]!=1.0) && (data[3]!=0.0)) {
		 float one_over_data3 = 1.0 / data[3];
	data[0] *= one_over_data3;
	data[1] *= one_over_data3;
	data[2] *= one_over_data3;
	data[3]= 1.0;
      }
    }
  gPoint operator+( const gVector& vec ) const
    { return gPoint( x()+vec.x(), y()+vec.y(), z()+vec.z() ); }
  gVector operator-( const gPoint& other ) const 
    { return gVector( x()-other.x(), y()-other.y(), z()-other.z() ); }
  gPoint operator-( const gVector& vec ) const
    { return gPoint( x()-vec.x(), y()-vec.y(), z()-vec.z() ); }
  int operator==( const gPoint& other ) const
    { return( !memcmp( data, other.data, 4*sizeof(float) ) ); }
  int operator!=( const gPoint& other ) const
    { return( memcmp( data, other.data, 4*sizeof(float) ) ); }
private:
  float data[4];
};

class gBColor { // A more compact, byte rep of a color
friend class gColor;
public:
  gBColor( float rin, float gin, float bin, float ain= 1.0 ) 
  { 
    data[0]= (unsigned char)(255*rin + 0.5); 
    data[1]= (unsigned char)(255*gin + 0.5); 
    data[2]= (unsigned char)(255*bin + 0.5); 
    data[3]= (unsigned char)(255*ain + 0.5); 
  }
  gBColor( int rin, int gin, int bin, int ain= 255 )
  {
    data[0]= ( (rin > 255) ? 255 : rin );
    data[1]= ( (gin > 255) ? 255 : gin );
    data[2]= ( (bin > 255) ? 255 : bin );
    data[3]= ( (ain > 255) ? 255 : ain );
  }

#ifdef INT4BYTES
	gBColor (int *data_in) { intval = *data_in; }
#endif

  gBColor( unsigned char* data_in )
#ifdef INT4BYTES
  { intval= *(int*)data_in; }
#else
  { 
    data[0]= data_in[0]; data[1]= data_in[1]; data[2]= data_in[2];
    data[3]= data_in[3];
  }
#endif
  gBColor()
  { data[0]= data[1]= data[2]= data[3]= 0; }
  inline gBColor( const gColor& other ); // defined after gColor
  void clear() { data[0]= data[1]= data[2]= data[3]= 0; }

  float r() const { return( (float)data[0] / 255.0); }
  float g() const { return( (float)data[1] / 255.0); }
  float b() const { return( (float)data[2] / 255.0); }
  float a() const { return( (float)data[3] / 255.0); }

  int ir() const { return data[0]; }
  int ig() const { return data[1]; }
  int ib() const { return data[2]; }
  int ia() const { return data[3]; }
  int operator==( const gBColor& other ) const
#ifdef INT4BYTES
    { return (intval == other.intval); }
#else
  { return( !memcmp( data, other.data, 4*sizeof(unsigned char) ) ); }
#endif
  int operator!=( const gBColor& other ) const
#ifdef INT4BYTES
    { return (intval != other.intval); }
#else
  { return( memcmp( data, other.data, 4*sizeof(unsigned char) ) ); }
#endif
  gBColor operator*( const float fac )
  // fac had better be > 0, but we will not check for speed\'s sake
  {
    int ir= (int)(fac*data[0]);
    int ig= (int)(fac*data[1]);
    int ib= (int)(fac*data[2]);
    int ia= (int)(fac*data[3]);
    return gBColor (ir, ig, ib, ia);
  }
  gBColor operator+( const gBColor& other )
  {
    int ir= data[0] + other.data[0];
    int ig= data[1] + other.data[1];
    int ib= data[2] + other.data[2];
    int ia= data[3] + other.data[3];
    return gBColor(ir, ig, ib, ia);
  }
  gBColor operator+=( const gBColor& other ) {

#ifdef INT4BYTES
       register int accum;
	  register int space;

       accum = data[0] + other.data[0];
       if (accum > 255) accum = 255;
	  space = accum;

       accum = data[1] + other.data[1];
       if (accum > 255) accum = 255;
	  space = (space << 8) | accum;

       accum = data[2] + other.data[2];
       if (accum > 255) accum = 255;
	  space = (space << 8) | accum;

       accum = data[3] + other.data[3];
       if (accum > 255) accum = 255;
	  space = (space << 8) | accum;
       
	  this->intval = space;

	return *this;
       
#else
       register int lop;

	lop = this->data[0] + other.data[0];
	this->data[0] = (lop > 255 ? 255 : lop);
	lop = this->data[1] + other.data[1];
	this->data[1] = (lop > 255 ? 255 : lop);
	lop = this->data[2] + other.data[2];
	this->data[2] = (lop > 255 ? 255 : lop);
	lop = this->data[3] + other.data[3];
	this->data[3] = (lop > 255 ? 255 : lop);

	return *this;
#endif
  }
  gBColor operator*=( const float factor ) 
  {
    data[0] = (unsigned char)(factor*data[0]);
    data[1] = (unsigned char)(factor*data[1]);
    data[2] = (unsigned char)(factor*data[2]);
    data[3] = (unsigned char)(factor*data[3]);
    return *this;
  }
  gBColor alpha_weighted() const
  {
    return gBColor( ir()*ia() >> 8,
		    ig()*ia() >> 8,
		    ib()*ia() >> 8,
		    ia() );
  }
  unsigned char* bytes() const { return (unsigned char*)data; }
  gBColor scale_alpha( const float factor )
  {
    data[3] = (unsigned char)(factor*data[3]);
    return *this;
  }
private:
#ifdef INT4BYTES
  union {
    unsigned char data[4];
    int intval;
  };
#else
  unsigned char data[4];
#endif
};

class gHSVColor;

class gColor { // Floating point rep of a color
friend class gPoint;
friend class gVector;
friend class gTransfm;
friend class gBColor;
public:
    gColor( float rin, float gin, float bin, float ain= 1.0 ) 
    { data[0]= rin; data[1]= gin; data[2]= bin; data[3]= ain; }
    gColor()
    { data[0]= data[1]= data[2]= data[3]= 0.0; }
    gColor( const gColor& other )
    { memcpy( data, other.data, 4*sizeof(float) ); }
    gColor( const gBColor& other )
    { data[0]= other.r(); data[1]= other.g(); data[2]= other.b();
      data[3]= other.a(); }
    inline gColor( const gHSVColor& other ); // defined after gHSVColor
    float r() const { return( data[0] ); }
    float g() const { return( data[1] ); }
    float b() const { return( data[2] ); }
    float a() const { return( data[3] ); }
    int ir() const { return( (int)(255*data[0] + 0.5) ); }
    int ig() const { return( (int)(255*data[1] + 0.5) ); }
    int ib() const { return( (int)(255*data[2] + 0.5) ); }
    int ia() const { return( (int)(255*data[3] + 0.5) ); }
    void hsv( float& h, float& s, float& v ) const;
    gColor& operator=( const gBColor& other )
    { 
	data[0]= other.r(); data[1]= other.g();
	data[2]= other.b(); data[3]= other.a();
	return( *this );
    }
    gColor& operator=( const gColor& other )
    {
	if (this != &other ) memcpy( data, other.data, 4*sizeof(float) );
	return( *this );
    }
    inline gColor& operator=( const gHSVColor& other ); // defined later
    int operator==( const gColor& other ) const
    { return( !memcmp( data, other.data, 4*sizeof(float) ) ); }
    int operator!=( const gColor& other ) const
    { return( memcmp( data, other.data, 4*sizeof(float) ) ); }
    gColor clamp()
    {
	data[0]= data[0] >= 0.0 ? data[0] : 0.0;
	data[1]= data[1] >= 0.0 ? data[1] : 0.0;
	data[2]= data[2] >= 0.0 ? data[2] : 0.0;
	data[3]= data[3] >= 0.0 ? data[3] : 0.0;
	data[0]= data[0] <= 1.0 ? data[0] : 1.0;
	data[1]= data[1] <= 1.0 ? data[1] : 1.0;
	data[2]= data[2] <= 1.0 ? data[2] : 1.0;
	data[3]= data[3] <= 1.0 ? data[3] : 1.0;
	return *this;
    }
    gColor clamp_alpha()
    {
	data[3]= data[3] >= 0.0 ? data[3] : 0.0;
	data[3]= data[3] <= 1.0 ? data[3] : 1.0;
	return *this;
    }
    gColor operator+( const gColor& other )
    { return gColor( r()+other.r(), g()+other.g(), b()+other.b(),
		     a()+other.a() ).clamp(); }
    gColor operator*( const float fac ) const
    { return gColor( fac*r(), fac*g(), fac*b(), fac*a() ).clamp(); }
    gColor operator*( const gColor& other )
    { return gColor( r()*other.r(), g()*other.g(), b()*other.b(), 
		     a()*other.a() ); }
    gColor& operator+=( const gColor& other )
    {
      data[0] += other.data[0];
      data[1] += other.data[1];
      data[2] += other.data[2];
      data[3] += other.data[3];
      clamp();
      return *this;
    }    
    gColor& operator*=( const double fac )
    {
      data[0] *= fac;
      data[1] *= fac;
      data[2] *= fac;
      data[3] *= fac;
      clamp();
      return *this;
    }
    // Note that color values are assumed to be pre-multiplied by opacity!
    void add_under( const gColor & other) {
	float opac_dif= 1.0 - data[3];
	data[0]= data[0] + other.data[0]*opac_dif;
	data[1]= data[1] + other.data[1]*opac_dif;
	data[2]= data[2] + other.data[2]*opac_dif;
	data[3]= data[3] + other.data[3] - (data[3]*other.data[3]);
    }
    void add_noclamp( const gColor& other )
    {
      data[0] += other.data[0];
      data[1] += other.data[1];
      data[2] += other.data[2];
      data[3] += other.data[3];
    }
    void subtract_noclamp( const gColor& other )
    {
      data[0] -= other.data[0];
      data[1] -= other.data[1];
      data[2] -= other.data[2];
      data[3] -= other.data[3];
    }
    void mult_noclamp( const float fac )
    {
      data[0] *= fac;
      data[1] *= fac;
      data[2] *= fac;
      data[3] *= fac;
    }
    void scale_by_alpha()
    {
      data[0] *= data[3];
      data[1] *= data[3];
      data[2] *= data[3];
    }
private: 
    float data[4];
};

class gHSVColor { // Floating point rep of a color in HLS representation
public:
    gHSVColor( float hin, float sin, float vin, float ain= 1.0 ) 
    { data[0]= hin; data[1]= sin; data[2]= vin; data[3]= ain; }
    gHSVColor()
    { data[0]= data[1]= data[2]= data[3]= 0.0; }
    gHSVColor( const gHSVColor& other )
    { memcpy( data, other.data, sizeof(data) ); }
    gHSVColor( const gColor& other )
    { 
      float h, s, v;
      other.hsv(h,s,v);
      data[0]= h; data[1]= s; data[2]= v; data[3]= other.a();
    }
    float h() const { return( data[0] ); }
    float s() const { return( data[1] ); }
    float v() const { return( data[2] ); }
    float a() const { return( data[3] ); }
    gHSVColor& operator=( const gColor& other )
    { 
      float h, s, v;
      other.hsv(h,s,v);
      data[0]= h; data[1]= s; data[2]= v; data[3]= other.a();
      return *this;
    }
    gHSVColor& operator=( const gHSVColor& other )
    {
      if (this != &other ) memcpy( data, other.data, sizeof(data) );
      return( *this );
    }
    int operator==( const gHSVColor& other ) const
    { return( !memcmp( data, other.data, 4*sizeof(float) ) ); }
    int operator!=( const gHSVColor& other ) const
    { return( memcmp( data, other.data, 4*sizeof(float) ) ); }
    gHSVColor scale_value( const float fac )
    { data[2] *= fac; return *this; }
    void rgb( float& r, float& g, float& b ) const;
private: 
    float data[4];
};

inline gColor::gColor( const gHSVColor& other )
{ 
  float r, g, b;
  other.rgb( r, g, b );
  data[0]= r; data[1]= g; data[2]= b; data[3]= other.a();
}

inline gColor& gColor::operator=( const gHSVColor& other )
{
  float r,g,b;
  other.rgb(r,g,b);
  data[0]= r; data[1]= g; data[2]= b; data[3]= other.a();
  return( *this );
}

inline gBColor::gBColor( const gColor& other )
{ 
  gColor tmp= other;
  tmp.clamp();
  data[0]= (unsigned char)(255*tmp.r()); 
  data[1]= (unsigned char)(255*tmp.g()); 
  data[2]= (unsigned char)(255*tmp.b()); 
  data[3]= (unsigned char)(255*tmp.a()); 
}

class gTransfm {
friend class gPoint;
friend class gVector;
friend class gColor;
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
  gTransfm& operator=( const gTransfm& a );
  gTransfm& operator*( const gTransfm& a );
  gVector operator*( const gVector& vec ) const;
  gPoint operator*( const gPoint& pt ) const;
  void dump() const; // writes to stderr
  int operator==( const gTransfm& other ) const
  { return( !memcmp( data, other.data, 16*sizeof(float) ) ); }
  int operator!=( const gTransfm& other ) const
  { return( memcmp( data, other.data, 16*sizeof(float) ) ); }
  char *tostring() const; // allocates memory;  caller must free
  static gTransfm fromstring( char **buf );
  const float *floatrep() const { return data; }
private:
  float data[16];
};

class gBoundBox {
public:
    gBoundBox( float xin_llf, float yin_llf, float zin_llf,
	       float xin_trb, float yin_trb, float zin_trb );
    gBoundBox();
    ~gBoundBox();
    gBoundBox( const gBoundBox& a );
    gBoundBox& operator=( const gBoundBox& other );
    int inside( const gPoint& pt ) const
    {
	return( (pt.x() >= llf.x()) && (pt.x() <= trb.x())
		 && (pt.y() >= llf.y()) && (pt.y() <= trb.y())
		 && (pt.z() >= llf.z()) && (pt.z() <= trb.z())  );
    }
    int intersect( const gVector& dir, const gPoint& origin, 
		   const float mindist, float *maxdist ) const;
    int operator==( const gBoundBox& other ) const
    { return( (llf == other.llf) && (trb == other.trb) ); }
    int operator!=( const gBoundBox& other ) const
    { return( (llf != other.llf) || (trb != other.trb) ); }
    float xmin() const { return llf.x(); }
    float xmax() const { return trb.x(); }
    float ymin() const { return llf.y(); }
    float ymax() const { return trb.y(); }
    float zmin() const { return llf.z(); }
    float zmax() const { return trb.z(); }
    gPoint center() const
      { 
	return gPoint( 0.5*(xmin()+xmax()), 0.5*(ymin()+ymax()),
		      0.5*(zmin()+zmax()) ); 
      }
    void union_with( const gBoundBox& a );
    // Treats bbox as periodic boundary, and moves points as close as possible
    gPoint wrap_together( const gPoint& pt, const gPoint& fixedPt ) const;
    gPoint wrap( const gPoint& pt ) const;
private:
    gPoint llf;  // lower left front
    gPoint trb;  // top right back
};

#endif // GEOMETRY_H_INCLUDED
