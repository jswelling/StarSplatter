/****************************************************************************
 * ssplat_tcl.cc
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tcl.h>

#include "starsplatter.h"

#if (TCL_MAJOR_VERSION<8)
#define OLD_TCL_API 1
#endif

#ifdef OLD_TCL_API
#define TCLCONST /* nothing */
#else
#define TCLCONST const
#endif

static Tcl_HashTable obj_hash;
static int cam_id= 0;
static int sbunch_id= 0;

enum Hash_Type { BASE_HASH, CAMERA_HASH, SBUNCH_HASH, SSREN_HASH };

class Hash_Value {
public:
  Hash_Value() {}
  ~Hash_Value() {}
  virtual Hash_Type type() const { return BASE_HASH; }
  virtual int docmd(ClientData clientdata, Tcl_Interp *interp, 
		    int argc, TCLCONST char* argv[])= 0;
};

class Cam_Hash : public Hash_Value {
public:
  Cam_Hash( Camera* cam_in ) { cam= cam_in; } 
  ~Cam_Hash() { delete cam; } // Note cam is deleted!
  Hash_Type type() const { return CAMERA_HASH; }
  int docmd( ClientData clientdata, Tcl_Interp *interp,
	     int argc, TCLCONST char* argv[]);
  Camera* camera() const { return cam; }
private:
  Camera* cam;
};

class SBunch_Hash : public Hash_Value {
public:
  SBunch_Hash( StarBunch* sbunch_in ) { sbunch= sbunch_in; }
  ~SBunch_Hash() { delete sbunch; } // Note sbunch is deleted!
  Hash_Type type() const { return SBUNCH_HASH; }
  int docmd( ClientData clientData, Tcl_Interp *interp,
	     int argc, TCLCONST char* argv[] );
  StarBunch* starbunch() const { return sbunch; }
private:
  StarBunch* sbunch;
};

class SSRen_Hash : public Hash_Value {
public:
  SSRen_Hash( StarSplatter* ssren_in ) { ssren= ssren_in; }
  ~SSRen_Hash() { delete ssren; } // Note ssren is deleted!
  Hash_Type type() const { return SSREN_HASH; }
  int docmd( ClientData clientData, Tcl_Interp *interp,
	     int argc, TCLCONST char* argv[] );
  StarSplatter* starsplatter() const { return ssren; }
private:
  StarSplatter* ssren;
};

static void append_cmd_to_result( Tcl_Interp *interp, int argc, 
				  TCLCONST char* argv[] )
{
  for (int i=0; i<argc; i++) Tcl_AppendElement( interp, argv[i] );
}

int arg_count_error( Tcl_Interp *interp, int argc, TCLCONST char* argv[] )
{
  Tcl_AppendResult(interp, "wrong # args in ", NULL);
  append_cmd_to_result( interp, argc, argv );
  return TCL_ERROR;
}

static int coords_from_list( Tcl_Interp *interp, TCLCONST char* list, 
			     double *x, double *y, double *z )
{
  int code;
  int argc;
  TCLCONST char** argv;

  if ( (code= Tcl_SplitList(interp, list, &argc, &argv)) != TCL_OK ) 
    return code;

  if (argc != 3) {
    Tcl_AppendResult( interp, "Wrong number of elements in coord list ",
		      list, NULL );
    return TCL_ERROR;
  }

  double x_d, y_d, z_d;
  code= Tcl_GetDouble(interp, argv[0], &x_d);
  if (code==TCL_OK) code= Tcl_GetDouble(interp, argv[1], &y_d);
  if (code==TCL_OK) code= Tcl_GetDouble(interp, argv[2], &z_d);

  if (code != TCL_OK) {
    Tcl_AppendResult(interp,"Bad coordinate list ", list, NULL);
    return TCL_ERROR;
  }

  *x= (double)x_d;
  *y= (double)y_d;
  *z= (double)z_d;
  free( argv );
  return TCL_OK;
}

static int color_from_list( Tcl_Interp *interp, char* list, 
			    double *r, double *g, double *b, double *a )
{
  int code;
  int argc;
  TCLCONST char**argv;

  if ( (code= Tcl_SplitList(interp, list, &argc, &argv)) != TCL_OK ) 
    return code;

  if ((argc != 3) && (argc != 4)) {
    Tcl_AppendResult( interp, "Wrong number of elements in coord list ",
		      list, NULL );
    return TCL_ERROR;
  }

  double r_d, g_d, b_d, a_d= 1.0;
  code= Tcl_GetDouble(interp, argv[0], &r_d);
  if (code==TCL_OK) code= Tcl_GetDouble(interp, argv[1], &g_d);
  if (code==TCL_OK) code= Tcl_GetDouble(interp, argv[2], &b_d);
  if ((code==TCL_OK) && (argc==4)) code= Tcl_GetDouble(interp, argv[3], &a_d);

  if (code != TCL_OK) {
    Tcl_AppendResult(interp,"Bad color list ", list, NULL);
    return TCL_ERROR;
  }

  *r= (double)r_d;
  *g= (double)g_d;
  *b= (double)b_d;
  *a= (double)a_d;
  free( argv );
  return TCL_OK;
}

static int ObjMthdCmd(ClientData clientdata, Tcl_Interp *interp, 
		      int argc, TCLCONST char* argv[])
{
  // Recover the object
  Tcl_HashEntry* entryPtr;
  entryPtr= Tcl_FindHashEntry(&obj_hash, argv[0]);
  if (!entryPtr) {
    Tcl_AppendResult(interp, "no object named \"", argv[0], "\"", (char*)NULL);
    return TCL_ERROR;
  }
  Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);

  return value->docmd( clientdata, interp, argc, argv );
}

static void tcl_return_cam_object( ClientData clientdata, Tcl_Interp *interp,
				   Camera* cam_in )
{
  Tcl_HashEntry *entryPtr= NULL;
  int newslot;
  do {
    // TCL_RESULT_SIZE should be at least 200, so this is safe
    sprintf(interp->result, "ssplat_cam_%d", cam_id++);
    entryPtr= Tcl_CreateHashEntry(&obj_hash, interp->result, &newslot);
  } while (!newslot);
  Cam_Hash* hashval= new Cam_Hash( cam_in );
  Tcl_SetHashValue(entryPtr, hashval);
  Tcl_CreateCommand(interp, interp->result, ObjMthdCmd,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
}

int SSRen_Hash::docmd( ClientData clientdata, Tcl_Interp *interp,
		       int argc, TCLCONST char* argv[] )
{
  if (!strcmp(argv[1],"delete")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    Tcl_DeleteCommand(interp, argv[0]);
    delete this;
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"dump")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    ssren->dump(stdout);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"image_size")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);
    int xdim, ydim;
    if ((Tcl_GetInt(interp, argv[2], &xdim) != TCL_OK)
	|| (Tcl_GetInt(interp, argv[3], &ydim) != TCL_OK)) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    ssren->set_image_dims( xdim, ydim );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"camera")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    Tcl_HashEntry* entryPtr;
    entryPtr= Tcl_FindHashEntry(&obj_hash, argv[2]);
    if (!entryPtr) {
      Tcl_AppendResult(interp, "no object named \"", argv[2], "\" in", NULL);
      append_cmd_to_result(interp, argc, argv);
      return TCL_ERROR;
    }
    Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
    if (value->type() != CAMERA_HASH) {
      Tcl_AppendResult(interp, argv[2], " is not a camera in ", NULL);
      append_cmd_to_result(interp, argc, argv);
      return TCL_ERROR;
    }
    ssren->set_camera( *((Cam_Hash*)value)->camera() );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"debug")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    int flag= 0;
    if (Tcl_GetBoolean(interp, argv[2], &flag) != TCL_OK) {
      Tcl_AppendResult(interp, "invalid flag in ",NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    ssren->set_debug(flag);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"exposure_rescale_type")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    StarSplatter::ExposureType type;
    if (!strcmp(argv[2],"linear")) type= StarSplatter::ET_LINEAR;
    else if (!strcmp(argv[2],"log_auto")) type= StarSplatter::ET_LOG_AUTO;
    else if (!strcmp(argv[2],"log")) type= StarSplatter::ET_LOG;
    else {
      Tcl_AppendResult(interp, "invalid type in ",NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    ssren->set_exposure_type( type );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"log_rescale_bounds")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);
    double bound_min, bound_max;
    if ((Tcl_GetDouble(interp, argv[2], &bound_min) != TCL_OK) 
	|| (Tcl_GetDouble(interp, argv[3], &bound_max) != TCL_OK)) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (bound_min >= bound_max) {
      Tcl_AppendResult(interp, "max not greater than min in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    ssren->set_log_rescale_bounds( bound_min, bound_max );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"splat_cutoff")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    double splat_cutoff;
    if (Tcl_GetDouble(interp, argv[2], &splat_cutoff) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    ssren->set_splat_cutoff_frac( splat_cutoff );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"exposure")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    double exposure;
    if (Tcl_GetDouble(interp, argv[2], &exposure) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    ssren->set_exposure_scale( exposure );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"rotate")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);

    double x, y, z;
    double angle;

    int code= 0;
    code= Tcl_GetDouble(interp, argv[2], &angle);
    if (code==TCL_OK) code= coords_from_list(interp, argv[3], &x, &y, &z);
    if (code!=TCL_OK) {
      Tcl_AppendResult(interp, "error in call ",NULL);
      append_cmd_to_result( interp, argc, argv );
      return code;
    }
    
    gVector axis= gVector(x,y,z);
    ssren->set_transform( *gTransfm::rotation(&axis, angle) 
			  * ssren->transform() );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"orient")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);

    double x, y, z;
    double angle;

    int code= 0;
    code= Tcl_GetDouble(interp, argv[2], &angle);
    if (code==TCL_OK) code= coords_from_list(interp, argv[3], &x, &y, &z);
    if (code!=TCL_OK) {
      Tcl_AppendResult(interp, "error in call ",NULL);
      append_cmd_to_result( interp, argc, argv );
      return code;
    }
    
    gVector axis= gVector(x,y,z);
    ssren->set_transform( *gTransfm::rotation(&axis, angle) );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"render")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);

    // Clear the renderer
    ssren->clear_stars();

    // argv[3] is the list of particle sets; add them to renderer
    int code;
    int particle_argc;
    TCLCONST char** particle_argv;
    if ( (code= Tcl_SplitList(interp, argv[3], 
			      &particle_argc, &particle_argv)) != TCL_OK ) 
      return code;
    for (int i=0; i<particle_argc; i++) {
      Tcl_HashEntry* entryPtr;
      entryPtr= Tcl_FindHashEntry(&obj_hash, particle_argv[i]);
      if (!entryPtr) {
	Tcl_AppendResult(interp, "no object named \"", 
			 particle_argv[i], "\" in", NULL);
	append_cmd_to_result(interp, argc, argv);
	return TCL_ERROR;
      }
      Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
      if (value->type() != SBUNCH_HASH) {
	Tcl_AppendResult(interp, argv[2], " is not a starbunch in ", NULL);
	append_cmd_to_result(interp, argc, argv);
	return TCL_ERROR;
      }
      ssren->add_stars( ((SBunch_Hash*)value)->starbunch() );
    }

    // Try to render the thing
    rgbImage* image= ssren->render();
    if (!image) {
      Tcl_AppendResult(interp, "render failed in ", NULL);
      append_cmd_to_result(interp,argc,argv);
      return TCL_ERROR;
    }

    // argv[2] is the filename;  save the image
    Tcl_DString nambuf;
    char* fname= Tcl_TildeSubst(interp, argv[2], &nambuf);
    if (!fname) return TCL_ERROR;
    char* extStartsHere= strrchr(fname,'.');
    char* typeString= "tiff"; // a default
    if (!strcasecmp(extStartsHere,".tif") 
	|| !strcasecmp(extStartsHere,".tiff")) {
      typeString= "tiff";
    }
    else if (!strcasecmp(extStartsHere,".ps")) {
      typeString= "ps";
    }
    else if (!strcasecmp(extStartsHere,".png")) {
      typeString= "png";
    }
    if (!image->save( fname, typeString )) {
      Tcl_AppendResult(interp, "failed to save image ", argv[2]," in ",NULL);
      append_cmd_to_result(interp,argc,argv);
      Tcl_DStringFree(&nambuf);
      delete image;
      return TCL_ERROR;
    }

    Tcl_DStringFree(&nambuf);
    delete image;
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"render_points")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);

    // Clear the renderer
    ssren->clear_stars();

    // argv[3] is the list of particle sets; add them to renderer
    int code;
    int particle_argc;
    TCLCONST char** particle_argv;
    if ( (code= Tcl_SplitList(interp, argv[3], 
			      &particle_argc, &particle_argv)) != TCL_OK ) 
      return code;
    for (int i=0; i<particle_argc; i++) {
      Tcl_HashEntry* entryPtr;
      entryPtr= Tcl_FindHashEntry(&obj_hash, particle_argv[i]);
      if (!entryPtr) {
	Tcl_AppendResult(interp, "no object named \"", 
			 particle_argv[i], "\" in", NULL);
	append_cmd_to_result(interp, argc, argv);
	return TCL_ERROR;
      }
      Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
      if (value->type() != SBUNCH_HASH) {
	Tcl_AppendResult(interp, argv[2], " is not a starbunch in ", NULL);
	append_cmd_to_result(interp, argc, argv);
	return TCL_ERROR;
      }
      ssren->add_stars( ((SBunch_Hash*)value)->starbunch() );
    }

    // Try to render the thing
    rgbImage* image= ssren->render_points();
    if (!image) {
      Tcl_AppendResult(interp, "render failed in ", NULL);
      append_cmd_to_result(interp,argc,argv);
      return TCL_ERROR;
    }

    // argv[2] is the filename;  save the image
    Tcl_DString nambuf;
    char* fname= Tcl_TildeSubst(interp, argv[2], &nambuf);
    if (!fname) return TCL_ERROR;
    if (!image->save( fname, "tiff" )) {
      Tcl_AppendResult(interp, "failed to save image ", argv[2]," in ",NULL);
      append_cmd_to_result(interp,argc,argv);
      Tcl_DStringFree(&nambuf);
      delete image;
      return TCL_ERROR;
    }

    Tcl_DStringFree(&nambuf);
    delete image;
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"force_slow_splat_method")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    int flag= 0;
    if (Tcl_GetBoolean(interp, argv[2], &flag) != TCL_OK) {
      Tcl_AppendResult(interp, "invalid flag in ",NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    // The set_force_slow_splat_method_flag method no longer exists as of
    // starsplatter 2.6.1, so this is now a no-op.
    //ssren->set_force_slow_splat_method_flag(flag);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"get")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    if (!strcmp(argv[2],"xsize")) {
      sprintf(interp->result,"%d ", ssren->image_xsize());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"ysize")) {
      sprintf(interp->result,"%d ", ssren->image_ysize());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"camera_set_flag")) {
      sprintf(interp->result,"%s", (ssren->camera_set()) ? "true" : "false");
      return TCL_OK;
    }
    if (!strcmp(argv[2],"camera")) {
      tcl_return_cam_object( clientdata, interp, 
			     new Camera( ssren->camera() ) );
      return TCL_OK;
    }
    if (!strcmp(argv[2],"debug")) {
      sprintf(interp->result,"%s", (ssren->debug()) ? "true" : "false");
      return TCL_OK;
    }
    if (!strcmp(argv[2],"exposure_rescale_type")) {
      StarSplatter::ExposureType type= ssren->get_exposure_type();
      char* typestr;
      switch (type) {
      case StarSplatter::ET_LINEAR:   typestr= "linear"; break;
      case StarSplatter::ET_LOG:      typestr= "log"; break;
      case StarSplatter::ET_LOG_AUTO: typestr= "log_auto"; break;
      }
      sprintf(interp->result,"%s", typestr);
      return TCL_OK;
    }
    if (!strcmp(argv[2],"log_rescale_min")) {
      double bound_min, bound_max;
      ssren->get_log_rescale_bounds(&bound_min,&bound_max);
      sprintf(interp->result,"%g", bound_min);
      return TCL_OK;
    }
    if (!strcmp(argv[2],"log_rescale_max")) {
      double bound_min, bound_max;
      ssren->get_log_rescale_bounds(&bound_min,&bound_max);
      sprintf(interp->result,"%g", bound_max);
      return TCL_OK;
    }
    if (!strcmp(argv[2],"splat_cutoff")) {
      sprintf(interp->result,"%g", ssren->splat_cutoff_frac());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"exposure")) {
      sprintf(interp->result,"%g", ssren->exposure_scale());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"force_slow_splat_method")) {
      // The get_force_slow_splat_method_flag method no longer exists as of
      // starsplatter 2.6.1. The 'slow' method is now fast and is always  
      // used, so we always return true.
      // sprintf(interp->result,"%s", 
      //      (ssren->get_force_slow_splat_method_flag()) ? "true" : "false");
      sprintf(interp->result,"%s", "true");
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(interp, "bad get parameter in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
  }
  else {
    Tcl_AppendResult(interp, "unrecognized command ", NULL);
    append_cmd_to_result( interp, argc, argv );
    return TCL_ERROR;
  }
}

int SBunch_Hash::docmd( ClientData clientdata, Tcl_Interp *interp,
			int argc, TCLCONST char* argv[] )
{
  if (!strcmp(argv[1],"delete")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    Tcl_DeleteCommand(interp, argv[0]);
    delete this;
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"dump")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    sbunch->dump(stdout,0);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"fulldump")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    sbunch->dump(stdout,1);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"nstars")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    int nstars;
    if (Tcl_GetInt(interp, argv[2], &nstars) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_nstars( nstars );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"nprops")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    int nprops;
    if (Tcl_GetInt(interp, argv[2], &nprops) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_nprops( nprops );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"attr")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);
    int whichAttr;
    int val;
    if (!strcmp(argv[2],"color_alg")) whichAttr= StarBunch::COLOR_ALG;
    else if (!strcmp(argv[2],"color_prop1")) whichAttr= StarBunch::COLOR_PROP1;
    else if (!strcmp(argv[2],"color_prop2")) whichAttr= StarBunch::COLOR_PROP2;
    else if (!strcmp(argv[2],"debug_level")) whichAttr= StarBunch::DEBUG_LEVEL;
    else {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &val) != TCL_OK) {
      if (whichAttr==StarBunch::COLOR_ALG) {
	if (!strcmp(argv[2],"constant")) val= StarBunch::CM_CONSTANT;
	else if (!strcmp(argv[3],"colormap_1d")) val= StarBunch::CM_COLORMAP_1D;
	else if (!strcmp(argv[3],"colormap_2d")) val= StarBunch::CM_COLORMAP_2D;
	else {
	  Tcl_AppendResult(interp, "bad value in ", NULL);
	  append_cmd_to_result( interp, argc, argv );
	  return TCL_ERROR;
	}
      }
      else {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
    }
    sbunch->set_attr( whichAttr, val );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"time")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    double time;
    if (Tcl_GetDouble(interp, argv[2], &time) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_time( time );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"density")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    double density;
    if (Tcl_GetDouble(interp, argv[2], &density) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_density( density );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"exponent_constant")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    double exp_const;
    if (Tcl_GetDouble(interp, argv[2], &exp_const) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_exp_constant( exp_const );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"scale_length")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    double scale_length;
    if (Tcl_GetDouble(interp, argv[2], &scale_length) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_scale_length( scale_length );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"bunch_color")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    double r, g, b, a;
    char* list= strdup(argv[2]);
    int code= color_from_list(interp, list, &r, &g, &b, &a);
    free(list);
    if (code != TCL_OK) return code;
    sbunch->set_bunch_color( gColor(r,g,b,a) );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"coords")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);
    int which_star;
    if (Tcl_GetInt(interp, argv[2], &which_star) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (which_star < 0 || which_star >= sbunch->nstars()) {
      Tcl_AppendResult(interp, "star ", argv[2], " does not exist in ", 
		       NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    double x, y, z;
    int code= coords_from_list(interp, argv[3], &x, &y, &z);
    if (code != TCL_OK) return code;
    sbunch->set_coords(which_star, gPoint(x,y,z));
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"particle_density")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);
    int which_star;
    if (Tcl_GetInt(interp, argv[2], &which_star) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (which_star < 0 || which_star >= sbunch->nstars()) {
      Tcl_AppendResult(interp, "star ", argv[2], " does not exist in ", 
		       NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    double density;
    if (Tcl_GetDouble(interp, argv[3], &density) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_density(which_star,(double)density);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"particle_exponent_constant")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);
    int which_star;
    if (Tcl_GetInt(interp, argv[2], &which_star) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (which_star < 0 || which_star >= sbunch->nstars()) {
      Tcl_AppendResult(interp, "star ", argv[2], " does not exist in ", 
		       NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    double exp_constant;
    if (Tcl_GetDouble(interp, argv[3], &exp_constant) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_exp_constant(which_star,(double)exp_constant);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"particle_scale_length")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);
    int which_star;
    if (Tcl_GetInt(interp, argv[2], &which_star) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (which_star < 0 || which_star >= sbunch->nstars()) {
      Tcl_AppendResult(interp, "star ", argv[2], " does not exist in ", 
		       NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    double scale_length;
    if (Tcl_GetDouble(interp, argv[3], &scale_length) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_scale_length(which_star,(double)scale_length);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"prop")) {
    if (argc != 5) return arg_count_error(interp, argc, argv);
    int which_star;
    int which_prop;
    if (Tcl_GetInt(interp, argv[2], &which_star) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &which_prop) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (which_star < 0 || which_star >= sbunch->nstars()) {
      Tcl_AppendResult(interp, "star ", argv[2], " does not exist in ", 
		       NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (which_prop < 0 || which_prop >= sbunch->nprops()) {
      Tcl_AppendResult(interp, "prop ", argv[3], " does not exist in ", 
		       NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    double val;
    if (Tcl_GetDouble(interp, argv[4], &val) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_prop(which_star,which_prop,val);
    return TCL_OK;

  }
  else if (!strcmp(argv[1],"propname")) {
    if (argc != 4) return arg_count_error(interp, argc, argv);
    int which_prop;
    if (Tcl_GetInt(interp, argv[2], &which_prop) != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (which_prop < 0 || which_prop >= sbunch->nprops()) {
      Tcl_AppendResult(interp, "prop ", argv[3], " does not exist in ", 
		       NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    sbunch->set_propName(which_prop,argv[3]);
    return TCL_OK;

  }
  else if (!strcmp(argv[1],"get")) {
    if (argc < 3) return arg_count_error(interp, argc, argv);
    if (!strcmp(argv[2],"coords")) {
      if (argc != 4) return arg_count_error(interp, argc, argv);
      int which_star;
      if (Tcl_GetInt(interp, argv[3], &which_star) != TCL_OK) {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      if (which_star < 0 || which_star >= sbunch->nstars()) {
	Tcl_AppendResult(interp, "star ", argv[3], " does not exist in ", 
			 NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      sprintf(interp->result,"%g %g %g ",
	      sbunch->coords(which_star).x(), sbunch->coords(which_star).y(), 
	      sbunch->coords(which_star).z() );
      return TCL_OK;
    }
    if (!strcmp(argv[2],"particle_density")) {
      if (argc != 4) return arg_count_error(interp, argc, argv);
      int which_star;
      if (Tcl_GetInt(interp, argv[3], &which_star) != TCL_OK) {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      if (which_star < 0 || which_star >= sbunch->nstars()) {
	Tcl_AppendResult(interp, "star ", argv[3], " does not exist in ", 
			 NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      sprintf(interp->result,"%g ",sbunch->density(which_star));
      return TCL_OK;
    }
    if (!strcmp(argv[2],"particle_exponent_constant")) {
      if (argc != 4) return arg_count_error(interp, argc, argv);
      int which_star;
      if (Tcl_GetInt(interp, argv[3], &which_star) != TCL_OK) {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      if (which_star < 0 || which_star >= sbunch->nstars()) {
	Tcl_AppendResult(interp, "star ", argv[3], " does not exist in ", 
			 NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      sprintf(interp->result,"%g ",sbunch->exp_constant(which_star));
      return TCL_OK;
    }
    if (!strcmp(argv[2],"particle_scale_length")) {
      if (argc != 4) return arg_count_error(interp, argc, argv);
      int which_star;
      if (Tcl_GetInt(interp, argv[3], &which_star) != TCL_OK) {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      if (which_star < 0 || which_star >= sbunch->nstars()) {
	Tcl_AppendResult(interp, "star ", argv[3], " does not exist in ", 
			 NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      sprintf(interp->result,"%g ",sbunch->scale_length(which_star));
      return TCL_OK;
    }
    if (!strcmp(argv[2],"nstars")) {
      if (argc != 3) return arg_count_error(interp, argc, argv);
      sprintf(interp->result,"%d ", sbunch->nstars());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"nprops")) {
      if (argc != 3) return arg_count_error(interp, argc, argv);
      sprintf(interp->result,"%d ", sbunch->nprops());
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"time")) {
      if (argc != 3) return arg_count_error(interp, argc, argv);
      sprintf(interp->result,"%g ", sbunch->time());
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"density")) {
      if (argc != 3) return arg_count_error(interp, argc, argv);
      sprintf(interp->result,"%g ", sbunch->density());
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"exponent_constant")) {
      if (argc != 3) return arg_count_error(interp, argc, argv);
      sprintf(interp->result,"%g ", sbunch->exp_constant());
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"scale_length")) {
      if (argc != 3) return arg_count_error(interp, argc, argv);
      sprintf(interp->result,"%g ", sbunch->scale_length());
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"bunch_color")) {
      if (argc != 3) return arg_count_error(interp, argc, argv);
      sprintf(interp->result,"%f %f %f %f", 
	      sbunch->bunch_color().r(), sbunch->bunch_color().g(), 
	      sbunch->bunch_color().b(), sbunch->bunch_color().a());
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"attr")) {
      if (argc != 4) return arg_count_error(interp, argc, argv);
      int whichAttr;
      int val;
      if (!strcmp(argv[3],"color_alg")) whichAttr= StarBunch::COLOR_ALG;
      else if (!strcmp(argv[3],"color_prop1")) whichAttr= StarBunch::COLOR_PROP1;
      else if (!strcmp(argv[3],"color_prop2")) whichAttr= StarBunch::COLOR_PROP2;
      else if (!strcmp(argv[3],"debug_level")) whichAttr= StarBunch::DEBUG_LEVEL;
      else {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      sprintf(interp->result,"%d ", sbunch->attr(whichAttr));
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"prop")) {
      if (argc != 5) return arg_count_error(interp, argc, argv);
      int which_star;
      int which_prop;
      if (Tcl_GetInt(interp, argv[3], &which_star) != TCL_OK) {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      if (Tcl_GetInt(interp, argv[4], &which_prop) != TCL_OK) {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      if (which_star < 0 || which_star >= sbunch->nstars()) {
	Tcl_AppendResult(interp, "star ", argv[3], " does not exist in ", 
			 NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      sprintf(interp->result,"%g ",sbunch->prop(which_star,which_prop));
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"propname")) {
      if (argc != 4) return arg_count_error(interp, argc, argv);
      int which_prop;
      if (Tcl_GetInt(interp, argv[3], &which_prop) != TCL_OK) {
	Tcl_AppendResult(interp, "bad value in ", NULL);
	append_cmd_to_result( interp, argc, argv );
	return TCL_ERROR;
      }
      sprintf(interp->result,"%s ",sbunch->propName(which_prop));
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(interp, "bad get parameter in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
  }
  else {
    Tcl_AppendResult(interp, "unrecognized command ", NULL);
    append_cmd_to_result( interp, argc, argv );
    return TCL_ERROR;
  }
}

int Cam_Hash::docmd( ClientData clientdata, Tcl_Interp *interp,
		     int argc, TCLCONST char* argv[] )
{
  if (!strcmp(argv[1],"delete")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    Tcl_DeleteCommand(interp, argv[0]);
    delete this;
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"dump")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    fprintf(stdout,"Camera <%s>:\n  from= ( %g %g %g )\n",
	    argv[0], cam->frompt().x(), cam->frompt().y(), cam->frompt().z());
    fprintf(stdout,"  at= ( %g %g %g )\n",
	    cam->atpt().x(),cam->atpt().y(),cam->atpt().z());
    fprintf(stdout,"  up= (%g %g %g )\n",
	    cam->updir().x(),cam->updir().y(),cam->updir().z());
    fprintf(stdout,"  fovea= %g, hither= %g, yon= %g\n",
	    cam->fov(), -cam->hither_dist(), -cam->yon_dist());
    fprintf(stdout,"  projection %s\n",
	    cam->parallel_proj() ? "parallel" : "perspective");
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"from")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    
    double x, y, z;
    int code= coords_from_list(interp, argv[2], &x, &y, &z);
    if (code != TCL_OK) return code;

    *cam= Camera( gPoint(x,y,z), cam->atpt(), cam->updir(),
		  cam->fov(), cam->hither_dist(), cam->yon_dist(),
		  cam->parallel_proj() );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"at")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    
    double x, y, z;
    int code= coords_from_list(interp, argv[2], &x, &y, &z);
    if (code != TCL_OK) return code;

    *cam= Camera( cam->frompt(), gPoint(x,y,z), cam->updir(),
		  cam->fov(), cam->hither_dist(), cam->yon_dist(),
		  cam->parallel_proj() );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"up")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    
    double x, y, z;
    int code= coords_from_list(interp, argv[2], &x, &y, &z);
    if (code != TCL_OK) return code;

    *cam= Camera( cam->frompt(), cam->atpt(), gVector(x,y,z),
		  cam->fov(), cam->hither_dist(), cam->yon_dist(),
		  cam->parallel_proj() );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"fovea")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);

    double newval;
    int code= Tcl_GetDouble(interp, argv[2], &newval);
    if (code != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ",NULL);
      append_cmd_to_result( interp, argc, argv );
      return code;
    }

    *cam= Camera( cam->frompt(), cam->atpt(), cam->updir(),
		  newval, cam->hither_dist(), cam->yon_dist(),
		  cam->parallel_proj() );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"hither")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);

    double newval;
    int code= Tcl_GetDouble(interp, argv[2], &newval);
    if (code != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ",NULL);
      append_cmd_to_result( interp, argc, argv );
      return code;
    }

    *cam= Camera( cam->frompt(), cam->atpt(), cam->updir(),
		  cam->fov(), -newval, cam->yon_dist(),
		  cam->parallel_proj() );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"yon")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);

    double newval;
    int code= Tcl_GetDouble(interp, argv[2], &newval);
    if (code != TCL_OK) {
      Tcl_AppendResult(interp, "bad value in ",NULL);
      append_cmd_to_result( interp, argc, argv );
      return code;
    }

    *cam= Camera( cam->frompt(), cam->atpt(), cam->updir(),
		  cam->fov(), cam->hither_dist(), -newval,
		  cam->parallel_proj() );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"projection")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    if (!strcmp(argv[2],"parallel")) {
      cam->set_parallel_proj();
      return TCL_OK;
    }
    else if (!strcmp(argv[2],"perspective")) {
      cam->set_perspective_proj();
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(interp, "bad projection type ", argv[2], " in ", NULL);
      append_cmd_to_result(interp, argc, argv);
      return TCL_ERROR;
    }
  }
  else if (!strcmp(argv[1],"get")) {
    if (argc != 3) return arg_count_error(interp, argc, argv);
    if (!strcmp(argv[2],"from")) {
      sprintf(interp->result,"%g %g %g ",
	      cam->frompt().x(),cam->frompt().y(),cam->frompt().z());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"at")) {
      sprintf(interp->result,"%g %g %g ",
	      cam->atpt().x(),cam->atpt().y(),cam->atpt().z());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"up")) {
      sprintf(interp->result,"%g %g %g ",
	      cam->updir().x(),cam->updir().y(),cam->updir().z());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"fovea")) {
      sprintf(interp->result,"%g",cam->fov());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"hither")) {
      sprintf(interp->result,"%g",-cam->hither_dist());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"yon")) {
      sprintf(interp->result,"%g",-cam->yon_dist());
      return TCL_OK;
    }
    if (!strcmp(argv[2],"projection")) {
      sprintf(interp->result,"%s",
	      (cam->parallel_proj()) ? "parallel" : "perspective");
      return TCL_OK;
    }
    else {
      Tcl_AppendResult(interp, "bad get parameter in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
  }
  else {
    Tcl_AppendResult(interp, "unrecognized command ", NULL);
    append_cmd_to_result( interp, argc, argv );
    return TCL_ERROR;
  }
}

static int tcl_load_tipsy_box( ClientData clientdata, Tcl_Interp *interp,
			       int argc, TCLCONST char* argv[] )
{
  if (argc != 6) return arg_count_error(interp, argc, argv);

  // argv[2] is the filename
  Tcl_DString nambuf;
  char* fname= Tcl_TildeSubst(interp, argv[2], &nambuf);
  if (!fname) return TCL_ERROR;
  FILE* infile= fopen(fname,"r");
  Tcl_DStringFree(&nambuf);
  if (!infile) {
    Tcl_AppendResult(interp, "could not open ", argv[2], " for reading. ",
		     NULL);
    return TCL_ERROR;
  }

  // argv[3], argv[4], argv[5] should be the gas, star, and dark particles 
  // respectively
  Tcl_HashEntry* entryPtr;
  entryPtr= Tcl_FindHashEntry(&obj_hash, argv[3]);
  if (!entryPtr) {
    Tcl_AppendResult(interp, "no object named \"", argv[3], "\" in", NULL);
    append_cmd_to_result(interp, argc, argv);
    return TCL_ERROR;
  }
  Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
  if (value->type() != SBUNCH_HASH) {
    Tcl_AppendResult(interp, argv[3], " is not a starbunch in ", NULL);
    append_cmd_to_result(interp, argc, argv);    
    return TCL_ERROR;
  }
  StarBunch* gas= ((SBunch_Hash*)value)->starbunch();

  entryPtr= Tcl_FindHashEntry(&obj_hash, argv[4]);
  if (!entryPtr) {
    Tcl_AppendResult(interp, "no object named \"", argv[4], "\" in", NULL);
    append_cmd_to_result(interp, argc, argv);
    return TCL_ERROR;
  }
  value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
  if (value->type() != SBUNCH_HASH) {
    Tcl_AppendResult(interp, argv[4], " is not a starbunch in ", NULL);
    append_cmd_to_result(interp, argc, argv);    
    return TCL_ERROR;
  }
  StarBunch* stars= ((SBunch_Hash*)value)->starbunch();

  entryPtr= Tcl_FindHashEntry(&obj_hash, argv[5]);
  if (!entryPtr) {
    Tcl_AppendResult(interp, "no object named \"", argv[5], "\" in", NULL);
    append_cmd_to_result(interp, argc, argv);
    return TCL_ERROR;
  }
  value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
  if (value->type() != SBUNCH_HASH) {
    Tcl_AppendResult(interp, argv[5], " is not a starbunch in ", NULL);
    append_cmd_to_result(interp, argc, argv);    
    return TCL_ERROR;
  }
  StarBunch* dark= ((SBunch_Hash*)value)->starbunch();

  if (!ssplat_load_tipsy_box_ascii( infile, gas, stars, dark )) {
    Tcl_AppendResult(interp, "load failed for ",NULL);
    append_cmd_to_result(interp, argc, argv);
    return TCL_ERROR;
  }
  
  if (fclose( infile ) == EOF) {
    fprintf(stderr,"Error closing %s after read (ignored)\n", argv[2]);
    return TCL_OK;
  }

  return TCL_OK;
}

static int tcl_load_dubinski( ClientData clientdata, Tcl_Interp *interp,
			      int argc, TCLCONST char* argv[] )
{
  if (argc != 4) return arg_count_error(interp, argc, argv);

  // argv[2] is the filename
  Tcl_DString nambuf;
  char* fname= Tcl_TildeSubst(interp, argv[2], &nambuf);
  if (!fname) return TCL_ERROR;
  FILE* infile= fopen(fname,"r");
  Tcl_DStringFree(&nambuf);
  if (!infile) {
    Tcl_AppendResult(interp, "could not open ", argv[2], " for reading. ",
		     NULL);
    return TCL_ERROR;
  }

  StarBunch** bunch_tbl= NULL;

  // argv[3] should be a list of StarBunches
  int code;
  int particle_argc;
  TCLCONST char** particle_argv;
  if ( (code= Tcl_SplitList(interp, argv[3], 
			    &particle_argc, &particle_argv)) != TCL_OK ) 
    return code;
  bunch_tbl= new StarBunch*[particle_argc];
  for (int i=0; i<particle_argc; i++) {
    Tcl_HashEntry* entryPtr;
    entryPtr= Tcl_FindHashEntry(&obj_hash, particle_argv[i]);
    if (!entryPtr) {
      Tcl_AppendResult(interp, "no object named \"", 
		       particle_argv[i], "\" in", NULL);
      append_cmd_to_result(interp, argc, argv);
      delete [] bunch_tbl;
      return TCL_ERROR;
    }
    Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
    if (value->type() != SBUNCH_HASH) {
      Tcl_AppendResult(interp, argv[2], " is not a starbunch in ", NULL);
      append_cmd_to_result(interp, argc, argv);
      delete [] bunch_tbl;
      return TCL_ERROR;
    }
    bunch_tbl[i]= ((SBunch_Hash*)value)->starbunch();
  }

  int bunches_read= 0;
  if (!ssplat_load_dubinski( infile, bunch_tbl, particle_argc, 
			     &bunches_read )) {
    Tcl_AppendResult(interp, "load failed for ",NULL);
    append_cmd_to_result(interp, argc, argv);
    delete [] bunch_tbl;
    return TCL_ERROR;
  }
  sprintf(interp->result, "%d", bunches_read);
  
  delete [] bunch_tbl;
  
  if (fclose( infile ) == EOF) {
    fprintf(stderr,"Error closing %s after read (ignored)\n", argv[2]);
    return TCL_OK;
  }

  return TCL_OK;
}

static int tcl_load_dubinski_raw( ClientData clientdata, Tcl_Interp *interp,
			      int argc, TCLCONST char* argv[] )
{
  if (argc != 4) return arg_count_error(interp, argc, argv);

  // argv[2] is the filename
  Tcl_DString nambuf;
  char* fname= Tcl_TildeSubst(interp, argv[2], &nambuf);
  if (!fname) return TCL_ERROR;
  FILE* infile= fopen(fname,"r");
  Tcl_DStringFree(&nambuf);
  if (!infile) {
    Tcl_AppendResult(interp, "could not open ", argv[2], " for reading. ",
		     NULL);
    return TCL_ERROR;
  }

  StarBunch** bunch_tbl= NULL;

  // argv[3] should be a list of StarBunches
  int code;
  int particle_argc;
  TCLCONST char** particle_argv;
  if ( (code= Tcl_SplitList(interp, argv[3], 
			    &particle_argc, &particle_argv)) != TCL_OK ) 
    return code;
  bunch_tbl= new StarBunch*[particle_argc];
  for (int i=0; i<particle_argc; i++) {
    Tcl_HashEntry* entryPtr;
    entryPtr= Tcl_FindHashEntry(&obj_hash, particle_argv[i]);
    if (!entryPtr) {
      Tcl_AppendResult(interp, "no object named \"", 
		       particle_argv[i], "\" in", NULL);
      append_cmd_to_result(interp, argc, argv);
      delete [] bunch_tbl;
      return TCL_ERROR;
    }
    Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
    if (value->type() != SBUNCH_HASH) {
      Tcl_AppendResult(interp, argv[2], " is not a starbunch in ", NULL);
      append_cmd_to_result(interp, argc, argv);
      delete [] bunch_tbl;
      return TCL_ERROR;
    }
    bunch_tbl[i]= ((SBunch_Hash*)value)->starbunch();
  }

  int bunches_read= 0;
  if (!ssplat_load_dubinski_raw( infile, bunch_tbl, particle_argc, 
				 &bunches_read )) {
    Tcl_AppendResult(interp, "load failed for ",NULL);
    append_cmd_to_result(interp, argc, argv);
    delete [] bunch_tbl;
    return TCL_ERROR;
  }
  sprintf(interp->result, "%d", bunches_read);
  
  delete [] bunch_tbl;
  
  if (fclose( infile ) == EOF) {
    fprintf(stderr,"Error closing %s after read (ignored)\n", argv[2]);
    return TCL_OK;
  }

  return TCL_OK;
}

static int tcl_load_gadget( ClientData clientdata, Tcl_Interp *interp,
			      int argc, TCLCONST char* argv[] )
{
  if (argc != 4) return arg_count_error(interp, argc, argv);

  // argv[2] is the filename
  Tcl_DString nambuf;
  char* fname= Tcl_TildeSubst(interp, argv[2], &nambuf);
  if (!fname) return TCL_ERROR;
  FILE* infile= fopen(fname,"r");
  Tcl_DStringFree(&nambuf);
  if (!infile) {
    Tcl_AppendResult(interp, "could not open ", argv[2], " for reading. ",
		     NULL);
    return TCL_ERROR;
  }

  StarBunch** bunch_tbl= NULL;

  // argv[3] should be a list of StarBunches
  int code;
  int particle_argc;
  TCLCONST char** particle_argv;
  if ( (code= Tcl_SplitList(interp, argv[3], 
			    &particle_argc, &particle_argv)) != TCL_OK ) 
    return code;
  bunch_tbl= new StarBunch*[particle_argc];
  for (int i=0; i<particle_argc; i++) {
    Tcl_HashEntry* entryPtr;
    entryPtr= Tcl_FindHashEntry(&obj_hash, particle_argv[i]);
    if (!entryPtr) {
      Tcl_AppendResult(interp, "no object named \"", 
		       particle_argv[i], "\" in", NULL);
      append_cmd_to_result(interp, argc, argv);
      delete [] bunch_tbl;
      return TCL_ERROR;
    }
    Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
    if (value->type() != SBUNCH_HASH) {
      Tcl_AppendResult(interp, argv[2], " is not a starbunch in ", NULL);
      append_cmd_to_result(interp, argc, argv);
      delete [] bunch_tbl;
      return TCL_ERROR;
    }
    bunch_tbl[i]= ((SBunch_Hash*)value)->starbunch();
  }

  int bunches_read= 0;
  if (!ssplat_load_gadget( infile, bunch_tbl, particle_argc, 
			     &bunches_read )) {
    Tcl_AppendResult(interp, "load failed for ",NULL);
    append_cmd_to_result(interp, argc, argv);
    delete [] bunch_tbl;
    return TCL_ERROR;
  }
  sprintf(interp->result, "%d", bunches_read);
  
  delete [] bunch_tbl;
  
  if (fclose( infile ) == EOF) {
    fprintf(stderr,"Error closing %s after read (ignored)\n", argv[2]);
    return TCL_OK;
  }

  return TCL_OK;
}

static int tcl_load_unstructured( ClientData clientdata, Tcl_Interp *interp,
			      int argc, TCLCONST char* argv[] )
{
  // format is 
  // "ssplat load $file [binary|ascii] [xyzxyz|xyzdkxyzdk] list-of-bunches
  //        [-skiplines n | -skipbytes n]
  if ((argc != 6) && (argc != 8)) return arg_count_error(interp, argc, argv);
  FILE *infile;
  if (Tcl_GetOpenFile(interp, argv[2], 0, 1, (ClientData*)&infile) 
      != TCL_OK) {
    Tcl_AppendResult(interp, "bad file in ", NULL);
    append_cmd_to_result( interp, argc, argv );
    return TCL_ERROR;
  }
  if (strcmp(argv[3],"binary") && strcmp(argv[3],"ascii")) {
    Tcl_AppendResult(interp, "format unknown in ", NULL);
    append_cmd_to_result( interp, argc, argv );
    return TCL_ERROR;
  }
  if (strcmp(argv[4],"xyzdkxyzdk") && strcmp(argv[4],"xyzxyz")) {
    Tcl_AppendResult(interp, "data layout unknown in ", NULL);
    append_cmd_to_result( interp, argc, argv );
    return TCL_ERROR;
  }
  // argv[5] should be a list of StarBunches
  int code;
  int particle_argc;
  TCLCONST char** particle_argv;
  if ( (code= Tcl_SplitList(interp, argv[5], 
			    &particle_argc, &particle_argv)) != TCL_OK ) 
    return code;
  StarBunch** bunch_tbl= new StarBunch*[particle_argc];
  for (int i=0; i<particle_argc; i++) {
    Tcl_HashEntry* entryPtr;
    entryPtr= Tcl_FindHashEntry(&obj_hash, particle_argv[i]);
    if (!entryPtr) {
      Tcl_AppendResult(interp, "no object named \"", 
		       particle_argv[i], "\" in", NULL);
      append_cmd_to_result(interp, argc, argv);
      delete [] bunch_tbl;
      return TCL_ERROR;
    }
    Hash_Value* value= (Hash_Value*)Tcl_GetHashValue(entryPtr);
    if (value->type() != SBUNCH_HASH) {
      Tcl_AppendResult(interp, argv[2], " is not a starbunch in ", NULL);
      append_cmd_to_result(interp, argc, argv);
      delete [] bunch_tbl;
      return TCL_ERROR;
    }
    bunch_tbl[i]= ((SBunch_Hash*)value)->starbunch();
  }
  // Check for skip count switches
  int skipcount= 0;
  if (argc==8) {
    if (strcmp(argv[6],"-skiplines") && strcmp(argv[6],"-skipbytes")) {
      Tcl_AppendResult(interp, "second from last arg unknown in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[7], &skipcount) != TCL_OK) {
      Tcl_AppendResult(interp, "invalid skip count in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
  }

  // Skip lines or bytes if requested
  if (skipcount) {
    if (!strcmp(argv[6],"-skiplines")) {
      char inbuf[256];
      for (int i=0; i<skipcount; i++) (void)fgets(inbuf,256,infile);
    }
    else if (!strcmp(argv[6],"-skipbytes")) {
      for (int i=0; i<skipcount; i++) (void)fgetc(infile);
    }
    if (ferror(infile)) {
      Tcl_AppendResult(interp, "error while skipping header in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
    if (feof(infile)) {
      Tcl_AppendResult(interp, "end of file while skipping header in ", NULL);
      append_cmd_to_result( interp, argc, argv );
      return TCL_ERROR;
    }
  }

  // Do the list of reads
  if (!strcmp(argv[3],"binary")) {
    if (!strcmp(argv[4],"xyzxyz")) {
      for (int i=0; i<particle_argc; i++)
	if (!bunch_tbl[i]->load_raw_xyzxyz(infile)) return TCL_ERROR;
    }
    else {
      for (int i=0; i<particle_argc; i++)
	if (!bunch_tbl[i]->load_raw_xyzdkxyzdk(infile)) return TCL_ERROR;
    }
  }
  else {
    if (!strcmp(argv[4],"xyzxyz")) {
      for (int i=0; i<particle_argc; i++)
	if (!bunch_tbl[i]->load_ascii_xyzxyz(infile)) return TCL_ERROR;
    }
    else {
      for (int i=0; i<particle_argc; i++)
	if (!bunch_tbl[i]->load_ascii_xyzdkxyzdk(infile)) return TCL_ERROR;
    }
  }

  delete [] bunch_tbl;

  return TCL_OK;
}

static int SSplatCmd(ClientData clientdata, Tcl_Interp *interp, int argc,
		     TCLCONST char *argv[] )
{
  if (argc < 2) return arg_count_error(interp, argc, argv);

  if (!strcmp(argv[1],"create_camera")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    tcl_return_cam_object( clientdata, interp,
			   new Camera( gPoint(0.0,0.0,1.0), 
				       gPoint(0.0,0.0,0.0),
				       gVector(0.0,1.0,0.0),
				       45.0, -0.5, -1.5 ) );
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"create_starbunch")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    Tcl_HashEntry *entryPtr= NULL;
    int newslot;
    do {
      // TCL_RESULT_SIZE should be at least 200, so this is safe
      sprintf(interp->result, "ssplat_starbunch_%d", sbunch_id++);
      entryPtr= Tcl_CreateHashEntry(&obj_hash, interp->result, &newslot);
    } while (!newslot);
    SBunch_Hash* hashval= new SBunch_Hash( new StarBunch );
    Tcl_SetHashValue(entryPtr, hashval);
    Tcl_CreateCommand(interp, interp->result, ObjMthdCmd,
		      (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"create_renderer")) {
    if (argc != 2) return arg_count_error(interp, argc, argv);
    Tcl_HashEntry *entryPtr= NULL;
    int newslot;
    do {
      // TCL_RESULT_SIZE should be at least 200, so this is safe
      sprintf(interp->result, "ssplat_starsplatter_%d", sbunch_id++);
      entryPtr= Tcl_CreateHashEntry(&obj_hash, interp->result, &newslot);
    } while (!newslot);
    SSRen_Hash* hashval= new SSRen_Hash( new StarSplatter );
    Tcl_SetHashValue(entryPtr, hashval);
    Tcl_CreateCommand(interp, interp->result, ObjMthdCmd,
		      (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
    return TCL_OK;
  }
  else if (!strcmp(argv[1],"load_tipsy_box")) {
    return tcl_load_tipsy_box( clientdata, interp, argc, argv );
  }
  else if (!strcmp(argv[1],"load_dubinski")) {
    return tcl_load_dubinski( clientdata, interp, argc, argv );
  }
  else if (!strcmp(argv[1],"load_dubinski_binary")) {
    return tcl_load_dubinski( clientdata, interp, argc, argv );
  }
  else if (!strcmp(argv[1],"load_gadget")) {
    return tcl_load_gadget( clientdata, interp, argc, argv );
  }
  else if (!strcmp(argv[1],"load")) {
    return tcl_load_unstructured( clientdata, interp, argc, argv );
  }
  else {
    Tcl_AppendResult(interp, "unrecognized command ", NULL);
    append_cmd_to_result( interp, argc, argv );
    return TCL_ERROR;
  }
}

int SSplat_Init(Tcl_Interp *interp)
{
  Tcl_InitHashTable(&obj_hash, TCL_STRING_KEYS);

  Tcl_CreateCommand(interp, "ssplat", SSplatCmd, 
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  return TCL_OK;
}

