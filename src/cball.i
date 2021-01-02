%module cball

%{
#include "geometry.h"
#include "cball.h"
%}

%include "typemaps.i"

/* Struct to hold mouse position information */
struct MousePosition { int x, y, maxx, maxy; };

%typemap(in) MousePosition () {   // temp becomes local var
  int i;
  if (PyTuple_Check($input)) {
    if (!PyArg_ParseTuple($input,"iiii",
	                  &($1.x),&($1.y),&($1.maxx),$($1.maxy))) {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 int elements");
      return NULL;
    }
    
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple of 4 ints.");
    return NULL;
  }
}

%typemap(in) MousePosition& (MousePosition tmp) {
  if (PyTuple_Check($input)) {
    if (!PyArg_ParseTuple($input,"iiii",
	                  &(tmp.x),&(tmp.y),&(tmp.maxx),&(tmp.maxy))) {
      PyErr_SetString(PyExc_TypeError,"tuple must have 4 int elements");
      return NULL;
    }
    $1 = &tmp;
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a tuple of 4 ints.");
    return NULL;
  }
}

class CBall {
public:
  CBall( void (*motion_callback_in)(gTransfm *, void *, CBall *),
	 void *cb_info_in, double scale_in= 1.0 );
  ~CBall();
  void set_view( gTransfm& new_viewmatrix );
  const gTransfm *view();
  const gTransfm *getCumulativeViewTrans();
  void roll( const MousePosition& mousedown, const MousePosition& mouseup );
  void slide( const MousePosition& mousedown, const MousePosition& mouseup );
  void reset();
  void set_scale( double scale_in );
  double scale() const;
};
