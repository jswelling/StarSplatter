/****************************************************************************
 * tclrunner.cc
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

/* This is a main program for running tcl apps. It borrows heavily from
 *  tclMain.c in the tcl7.5 distribution.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <tcl.h>

#include "starsplatter.h"

static char rcsid[] = "$Id: tclrunner.cc,v 1.17 2008-07-25 21:08:24 welling Exp $";

/* Notes-
 */

int Tcl_AppInit(Tcl_Interp *interp);

char* progname;

const int STDIN_LINE_LENGTH= 200;

int main( int argc, char* argv[] ) 
{
  char* initial_script= NULL;

  Tcl_Interp* tcl_interp= NULL;

  progname= argv[0];

  extern char *optarg;
  extern int optind;
  int c;
  int errflg= 0;
  int exitflg= 0;
  while ((c= getopt(argc, argv, "VCH")) != -1)
    switch (c) {
    case 'V':
      fprintf(stdout,"%s: %s\n",argv[0], ssplat_version_string);
      exitflg++;
      break;
    case 'C':
      fputs(ssplat_copyright_string, stdout);
      exitflg++;
      break;
    case 'H':
      fputs(ssplat_home_page_string, stdout);
      fputs("\n", stdout);
      exitflg++;
      break;
    case '?':
      errflg++;
      break;
    default:
      errflg++;
      break;
    }
  if (errflg) {
    fprintf(stderr,"usage: %s [-V] [-C] [-H] [tclscript [arg1 arg2 ...]]\n",
	    progname);
    fprintf(stderr,"    -V prints version info\n");
    fprintf(stderr,"    -C prints copyright info\n");
    fprintf(stderr,"    -H prints home page URL\n");
    fprintf(stderr,"    tclscript and arg1, arg2, etc. are as per tclsh\n");
    fprintf(stderr,"    see %s for more info\n",
	    ssplat_home_page_string);
    exit(2);
  }
  if (exitflg) exit(0);

#ifndef AVOID_TCL_MAIN
  Tcl_Main(argc, argv, Tcl_AppInit);
#else
  tcl_interp= Tcl_CreateInterp();
  
  // Handle additional command line arguments
  if (optind < argc) {
    // File was given as a script
    initial_script= new char[strlen(argv[optind])+1];
    strcpy(initial_script, argv[optind]);
    char buf[256];
    sprintf(buf,"%d",argc-(optind+1));
    Tcl_SetVar(tcl_interp, "argc", buf, TCL_GLOBAL_ONLY);
    Tcl_SetVar(tcl_interp, "argv0", initial_script, TCL_GLOBAL_ONLY);
    Tcl_SetVar(tcl_interp, "tcl_interactive","0", TCL_GLOBAL_ONLY);
    char* args= Tcl_Merge(argc-(optind+1), argv+(optind+1));
    Tcl_SetVar(tcl_interp, "argv", args, TCL_GLOBAL_ONLY);
    (void)free(args);
  }
  else {
    // Set the "tcl_interactive" variable if appropriate
    Tcl_SetVar(tcl_interp, "tcl_interactive",
	       (isatty(0)) ? "1" : "0", TCL_GLOBAL_ONLY);
    Tcl_SetVar(tcl_interp, "argc", "0", TCL_GLOBAL_ONLY);
    Tcl_SetVar(tcl_interp, "argv0", argv[0], TCL_GLOBAL_ONLY);
    char* args= Tcl_Merge(argc-optind, argv+optind);
    Tcl_SetVar(tcl_interp, "argv", args, TCL_GLOBAL_ONLY);
    (void)free(args);
  }
  
  if (Tcl_AppInit(tcl_interp) != TCL_OK) {
    fprintf(stderr,"Tcl error in Tcl_AppInit!\n");
    if (tcl_interp->result) 
      fprintf(stderr,"%s\n",tcl_interp->result);
    exit(-1);
  }

  int code=0;
  if (initial_script) {
    code= Tcl_EvalFile(tcl_interp, initial_script);
  }
  else {
    // Interpret stdin
    char line[STDIN_LINE_LENGTH];
    FILE* prompt_file= NULL;
    if (isatty(STDIN_FILENO)) prompt_file= fopen("/dev/tty","w");
    while (!feof(stdin)) {
      Tcl_DString cmd;
      Tcl_DStringInit(&cmd);
      while (1) {
	if (prompt_file) fprintf(prompt_file,"%s> ",progname);
	if (fgets(line, STDIN_LINE_LENGTH, stdin) == NULL) break;
	Tcl_DStringAppend(&cmd, line, -1);
	if (Tcl_CommandComplete(Tcl_DStringValue(&cmd))) break;
      }
      code= Tcl_RecordAndEval(tcl_interp, Tcl_DStringValue(&cmd), 0);
      Tcl_DStringFree(&cmd);
      if (code != TCL_OK) {
	if (initial_script)
	  fprintf(stderr,"Tcl error at line %d of script <%s>!\n",
		  tcl_interp->errorLine, initial_script);
	else fprintf(stderr,"Tcl error at line %d!\n",
		     tcl_interp->errorLine);
      }
    }
    if (prompt_file) fclose(prompt_file);
  }

  if (tcl_interp->result) {
    printf("%s\n",tcl_interp->result);
  }
  if (code != TCL_OK) {
    if (initial_script)
      fprintf(stderr,"Tcl error at line %d of script <%s>!\n",
	      tcl_interp->errorLine, initial_script);
    else fprintf(stderr,"Tcl error at line %d!\n",
		 tcl_interp->errorLine);
  }

  delete [] initial_script;
  Tcl_DeleteInterp(tcl_interp);
#endif /* ifdef AVOID_TCL_MAIN */
}

int Tcl_AppInit(Tcl_Interp *interp)
{
  if (Tcl_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  if (SSplat_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  Tcl_SetVar(interp, "tcl_rcFileName", "~/.ssplatrc", TCL_GLOBAL_ONLY);

  return TCL_OK;
}
