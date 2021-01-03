/****************************************************************************
 * utils.cc
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

#include <stdio.h>
#include <assert.h>
#include <string>

#include "starsplatter.h"

#define CHUNKSIZE 1024

// the gadget header struct
typedef struct io_gadget_header
{
  int npart[6];
  double mass[6];
  double time;
  double redshift;
  int flag_sfr;
  int flag_feedback;
  int npartTotal[6];
  int flag_cooling;
  int num_files;
  double BoxSize;
  double Omega0;
  double OmegaLambda;
  double HubbleParam;
  char fill[256-6*4-6*8-2*8-2*4-6*4-2*4-4*8];  /* fills to 256 Bytes */
} IO_Gadget_Header;

int ssplat_load_tipsy_box_ascii( FILE* infile, StarBunch* gas, 
				 StarBunch* stars, StarBunch* dark )
{
  int ntotal, ngas, nstar, ndark;
  if (fscanf(infile,"%d %d %d", &ntotal, &ngas, &nstar) != 3) {
    fprintf(stderr,"load_tipsy_box_ascii: Error reading particle numbers\n");
    return 0;
  }
  ndark= ntotal - (ngas + nstar);

  int ndims;
  if (fscanf(infile,"%d", &ndims) != 1) {
    fprintf(stderr,"load_tipsy_box_ascii: Error reading ndimensions\n");
    return 0;
  }
  if (ndims != 3) {
    fprintf(stderr,"load_tipsy_box_ascii: ndimensions = %d not supported\n",
	    ndims);
    return 0;
  }

  float time;
  if (fscanf(infile,"%f", &time) != 1) {
    fprintf(stderr,"load_tipsy_box_ascii: error reading time\n");
    return 0;
  }

  // Set bunch info
  gas->set_nstars( ngas );
  gas->set_time( time );
  stars->set_nstars( nstar );
  stars->set_time( time );
  dark->set_nstars( ndark );
  dark->set_time( time );

  // Load the particle info
  float *mass_data= new float[ntotal];
  float *x_coords= new float[ntotal];
  float *y_coords= new float[ntotal];
  float *z_coords= new float[ntotal];
  if (!mass_data || !x_coords || !y_coords || !z_coords) {
    fprintf(stderr,
  "load_tipsy_box_ascii: out of memory reading star coords (4*%d floats)!\n",
	    ntotal);
    delete [] mass_data;
    delete [] x_coords;
    delete [] y_coords;
    delete [] z_coords;
    return 0;
  }
  for (int i=0; i<ntotal; i++) {
    if (fscanf(infile,"%f",mass_data+i) != 1) {
      fprintf(stderr,"load_tipsy_box_ascii: error reading x coordinates\n");
      delete [] mass_data;
      delete [] x_coords;
      delete [] y_coords;
      delete [] z_coords;
      return 0;
    }
  }
  for (int i=0; i<ntotal; i++) {
    if (fscanf(infile,"%f",x_coords+i) != 1) {
      fprintf(stderr,"load_tipsy_box_ascii: error reading x coordinates\n");
      delete [] mass_data;
      delete [] x_coords;
      delete [] y_coords;
      delete [] z_coords;
      return 0;
    }
  }
  for (int i=0; i<ntotal; i++) {
    if (fscanf(infile,"%f",y_coords+i) != 1) {
      fprintf(stderr,"load_tipsy_box_ascii: error reading y coordinates\n");
      delete [] mass_data;
      delete [] x_coords;
      delete [] y_coords;
      delete [] z_coords;
      return 0;
    }
  }
  for (int i=0; i<ntotal; i++) {
    if (fscanf(infile,"%f",z_coords+i) != 1) {
      fprintf(stderr,"load_tipsy_box_ascii: error reading z coordinates\n");
      delete [] mass_data;
      delete [] x_coords;
      delete [] y_coords;
      delete [] z_coords;
      return 0;
    }
  }

  float* mrunner= mass_data;
  float* xrunner= x_coords;
  float* yrunner= y_coords;
  float* zrunner= z_coords;
  for (int i=0; i<ngas; i++) {
    gas->set_coords(i,gPoint( *xrunner++, *yrunner++, *zrunner++ ));
    gas->set_density(i,*mrunner++);
  }
  for (int i=0; i<ndark; i++) {
    dark->set_coords(i,gPoint( *xrunner++, *yrunner++, *zrunner++ ));
    dark->set_density(i,*mrunner++);
  }
  for (int i=0; i<nstar; i++) {
    stars->set_coords(i,gPoint( *xrunner++, *yrunner++, *zrunner++ ));
    stars->set_density(i,*mrunner++);
  }

  delete [] mass_data;
  delete [] x_coords;
  delete [] y_coords;
  delete [] z_coords;

  // Throw away velocity info
  float junk;
  for (int i=0; i<3*ntotal; i++) {
    if (fscanf(infile,"%f", &junk) != 1) {
      fprintf(stderr,
	      "load_tipsy_box_ascii: error while discarding velocities\n");
      return 0;
    }
  }

  // Load dark particle gravitational softening lengths
  float lscale;
  for (int i=0; i<ndark; i++) {
    if (fscanf(infile,"%f", &lscale) != 1) {
      fprintf(stderr,
	      "load_tipsy_box_ascii: error reading dark grav sft lengths\n");
      return 0;
    }
    dark->set_scale_length(i,lscale);
  }

  // Load star particle gravitational softening lengths
  for (int i=0; i<nstar; i++) {
    if (fscanf(infile,"%f", &lscale) != 1) {
      fprintf(stderr,
	      "load_tipsy_box_ascii: error reading star grav sft lengths\n");
      return 0;
    }
    stars->set_scale_length(i,lscale);
  }

  // Throw away gas density and temperature info
  for (int i=0; i<2*ngas; i++) {
    if (fscanf(infile,"%f", &junk) != 1) {
      fprintf(stderr,
  "load_tipsy_box_ascii: error while discarding gas density and temp info\n");
      return 0;
    }
  }

  // Load gas particle sph smoothing lengths
  for (int i=0; i<ngas; i++) {
    if (fscanf(infile,"%f", &lscale) != 1) {
      fprintf(stderr,
	  "load_tipsy_box_ascii: error reading gas sph smoothing lengths\n");
      return 0;
    }
    gas->set_scale_length(i,lscale);
  }

  return 1;
}

int ssplat_load_dubinski( FILE* infile, StarBunch** sbunch_tbl,
			  const int tbl_size, int* bunches_read )
{
  int nbodies;
  int ngroups;
  float time;

  if (fscanf(infile,"%d %d %f",&nbodies,&ngroups,&time) != 3) {
    fprintf(stderr,"ssplat_load_dubinski: error reading header line 1\n");
    *bunches_read= 0;
    return 0;
  }

  if (tbl_size < ngroups) {
    fprintf(stderr,"ssplat_load_dubinski: found %d groups, expected %d\n",
	    ngroups,tbl_size);
    *bunches_read= 0;
    return 0;
  }

  int istart, iend;
  float mass;
  int istart_expect= 1;
  for (int i=0; i<ngroups; i++) {
    if (fscanf(infile,"%d %d %f",&istart,&iend,&mass) != 3) {
      fprintf(stderr,
	      "ssplat_load_dubinski: error reading group info line %d\n",
	      i+1);
      *bunches_read= 0;
      return 0;
    }
    if (istart != istart_expect) {
      fprintf(stderr,
  "ssplat_load_dubinski: unexpected point order on group info line %d\n",
	      i+1);
      *bunches_read= 0;
      return 0;
    }
    sbunch_tbl[i]->set_time( time );
    // sbunch_tbl[i]->set_mass( mass );
    sbunch_tbl[i]->set_nstars( (iend+1-istart) );
    istart_expect= iend+1;
  }

  // Reset any groups for which there is no data
  for (int i= ngroups; i<tbl_size; i++) {
    sbunch_tbl[i]->set_time( time );
    // sbunch_tbl[i]->set_mass( 0.0 );
    sbunch_tbl[i]->set_nstars( 0 );
  }

  // Load the coordinate data
  for (int i=0; i<ngroups; i++) {
    if (!(sbunch_tbl[i]->load_ascii_xyzxyz(infile))) {
      fprintf(stderr,"ssplat_load_dubinski: error reading coordinate set %d\n",
	      i+1);
      *bunches_read= i;
      return 0;
    }
  }

  *bunches_read= ngroups;
  return 1;
}

int ssplat_load_dubinski_raw( FILE* infile, StarBunch** sbunch_tbl,
			      const int tbl_size, int* bunches_read )
{
  int nbodies;
  int ngroups;
  float time;

  if ((fread(&nbodies, sizeof(int), 1, infile) != 1)
      || (fread(&ngroups, sizeof(int), 1, infile) != 1)
      || (fread(&time, sizeof(float), 1, infile) != 1)) {
    fprintf(stderr,"ssplat_load_dubinski_raw: error reading header info\n");
    *bunches_read= 0;
    return 0;
  }

  if (tbl_size < ngroups) {
    fprintf(stderr,"ssplat_load_dubinski: found %d groups, expected %d\n",
	    ngroups,tbl_size);
    *bunches_read= 0;
    return 0;
  }

  int istart, iend, mass;
  int istart_expect= 1;
  for (int i=0; i<ngroups; i++) {
    if ((fread(&istart,sizeof(int),1,infile) != 1)
	|| (fread(&iend,sizeof(int),1,infile) != 1)
	|| (fread(&mass,sizeof(float),1,infile) != 1)) {
      fprintf(stderr,
	      "ssplat_load_dubinski: error reading group info line %d\n",
	      i+1);
      *bunches_read= 0;
      return 0;
    }
    if (istart != istart_expect) {
      fprintf(stderr,
  "ssplat_load_dubinski: unexpected point order on group info line %d\n",
	      i+1);
      *bunches_read= 0;
      return 0;
    }
    sbunch_tbl[i]->set_time( time );
    // sbunch_tbl[i]->set_mass( mass );
    sbunch_tbl[i]->set_nstars( (iend+1-istart) );
    istart_expect= iend+1;
  }

  // Reset any groups for which there is no data
  for (int i= ngroups; i<tbl_size; i++) {
    sbunch_tbl[i]->set_time( time );
    // sbunch_tbl[i]->set_mass( 0.0 );
    sbunch_tbl[i]->set_nstars( 0 );
  }

  // Load the coordinate data
  for (int i=0; i<ngroups; i++) {
    if (!(sbunch_tbl[i]->load_raw_xyzxyz(infile))) {
      fprintf(stderr,"ssplat_load_dubinski: error reading coordinate set %d\n",
	      i+1);
      *bunches_read= i;
      return 0;
    }
  }

  *bunches_read= ngroups;
  return 1;
}

static int typeHasVariableMass( IO_Gadget_Header* hdr, int type )
{
  return (hdr->mass[type]==0);
}

static int typeIsSPH( IO_Gadget_Header* hdr, int type )
{
  return (type==0); /* only gas is SPH, as I understand it */
}

static int typeDependsOnCoolingFlag( IO_Gadget_Header* hdr, int type )
{
  return (type==0 && hdr->flag_cooling);
}

static int loadOrDiscard( const char* propName, FILE* infile,
			  StarBunch** sbunch_tbl, const int ngroups,
			  IO_Gadget_Header* header,
			  int (*presence_test)(IO_Gadget_Header* header,
					       int type))
{
  int blockSizeBefore, blockSizeAfter;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
  (void)fread(&blockSizeBefore, sizeof(blockSizeBefore), 1, infile);
#pragma GCC diagnostic pop
  for (int i=0; i<ngroups; i++) {
    StarBunch* sb= sbunch_tbl[i];
    if (sb) {
      long nstars= sbunch_tbl[i]->nstars();
      if (nstars>0 && presence_test(header,i)) {
	float buf[CHUNKSIZE];
	long iStar= 0;
	int iProp= sb->allocate_next_free_prop_index(propName);
	while (nstars - iStar) {
	  size_t nThisChunk= 
	    (CHUNKSIZE>(nstars-iStar))?(nstars-iStar):CHUNKSIZE;
	  if (fread(buf,sizeof(float),nThisChunk,infile)<nThisChunk) {
	    fprintf(stderr,"ssplat_load_gadget: read error or premature EOF!\n");
	    return 0;
	  }
	  for (size_t iDatum=0; iDatum<nThisChunk; iDatum++) {
	    sbunch_tbl[i]->set_prop(iStar+iDatum,iProp,buf[iDatum]);
	  }
	  iStar += nThisChunk;
	}
      }
    }
    else {
      // There are header->npart[i] particles to be ignored
      if (presence_test(header,i)) 
	if (fseek(infile,header->npart[i]*sizeof(float),SEEK_CUR)) {
	  fprintf(stderr,"ssplat_load_gadget: seek error or premature EOF!\n");
	  return 0;
	}
    }
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
  (void)fread(&blockSizeAfter, sizeof(blockSizeAfter), 1, infile);
#pragma GCC diagnostic pop
  if (blockSizeAfter != blockSizeBefore) {
    fprintf(stderr,"ssplat_load_gadget: blocking error on %s!\n",propName);
    return 0;
  }
  return 1;
}

/** ckm: routine added to read gadget binary format **/
/** Note that some of the sbunch_tbl entries may be null! */
int ssplat_load_gadget ( FILE* infile, StarBunch** sbunch_tbl,
			      const int tbl_size, int* bunches_read )
{
  IO_Gadget_Header header;
  int blockSizeBefore, blockSizeAfter, ngroups;
  int numberOfBunchesWithVariableMass= 0;
  int numberOfSPHBunches= 0;
  int numberOfBunchesWithNeutralHydrogenDensity= 0;
  int numberOfBunchesWithElectronAbundance= 0;

  assert(sizeof(int)==4);
  assert(sizeof(float)==4);
  assert(sizeof(double)==8);
  assert(sizeof(char)==1);

  // read gadget header information
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 

  (void)fread(&blockSizeBefore, sizeof(blockSizeBefore), 1, infile);
  if (fread(&header, sizeof(header), 1, infile) != 1) {
    fprintf(stderr,"ssplat_load_gadget: error reading header info\n");
    *bunches_read= 0;
    return 0;
  }
  (void)fread(&blockSizeAfter, sizeof(blockSizeAfter), 1, infile);
  if (blockSizeAfter != blockSizeBefore) {
    fprintf(stderr,"ssplat_load_gadget: blocking error on block 0!\n");
    *bunches_read= 0;
    return 0;
  }
#pragma GCC diagnostic pop  
  // fprintf(stderr,"flags: sfr %d, feedback %d, cooling %d \n",
  //         header.flag_sfr, header.flag_feedback, header.flag_cooling);

  // Gadget files always supply info for 6 groups (some of which may
  // contain no particles).
  ngroups = 6;

  // error out if the number of groups passed do not match all occupied
  // particle types in the gadget binary file
  if (tbl_size < ngroups) {
    fprintf(stderr,"ssplat_load_gadget: found %d groups, expected %d\n",
	    ngroups,tbl_size);
    *bunches_read = 0;
    return 0;
  }

  for (int i=0; i<ngroups; i++) {

    if (sbunch_tbl[i]) {
	sbunch_tbl[i]->set_nstars( header.npart[i]);
    }

    if (header.redshift!=0.0) {
      // This is a cosmological simulation
      if (sbunch_tbl[i]) {
	sbunch_tbl[i]->set_time( 0.0 );
	sbunch_tbl[i]->set_a( header.time );
	sbunch_tbl[i]->set_z( header.redshift );
      }
    }
    else {
      // Plain old Euclidean space
      if (sbunch_tbl[i]) {
	sbunch_tbl[i]->set_time( header.time );
	sbunch_tbl[i]->set_a( 0.0 );
	sbunch_tbl[i]->set_z( 0.0 );
      }
    }
  }
 
  // Different particle types have different numbers of properties
  for (int i=0; i<ngroups; i++) {
    if (header.npart[i]) {
      int nprop= 4; // everyone has velocities and ids
      // +1 for mass for particles with variable mass
      if (typeHasVariableMass(&header,i)) {
	numberOfBunchesWithVariableMass++;
	nprop += 1;
      }
      // +3 for internal energy, density, smoothingLength for SPH particles
      if (typeIsSPH(&header,i)) { // gas particle
	numberOfSPHBunches++;
	nprop += 3;
      }
      // Optional fields
      if (typeDependsOnCoolingFlag(&header, i)) {
	numberOfBunchesWithElectronAbundance += 1; 
	nprop += 1;
      }
      if (typeDependsOnCoolingFlag(&header, i)) {
	numberOfBunchesWithNeutralHydrogenDensity += 1;
	nprop += 1;
      }
      if (sbunch_tbl[i]) {
	sbunch_tbl[i]->set_nprops(nprop); 
	for (int j=0; j<nprop; j++)
	  sbunch_tbl[i]->deallocate_prop_index(j); // remove any left-over prop defs
	if (sbunch_tbl[i]->debugLevel())
	  fprintf(stderr,"Bunch %d has %d props total\n",i,nprop);
      }
    }
  }

  // Load the coordinate data
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
  (void)fread(&blockSizeBefore, sizeof(blockSizeBefore), 1, infile);
#pragma GCC diagnostic pop
  for (int i=0; i<ngroups; i++) {
    StarBunch* sb= sbunch_tbl[i];
    if (sb) {
      if (sb->nstars()>0) {
	if (!(sb->load_raw_xyzxyz(infile))) {
	  fprintf(stderr,
		  "ssplat_load_gadget: error reading coordinate set %d\n",
		  i+1);
	  *bunches_read = i;
	  return 0;
	}
      }
    }
    else{
      if (fseek(infile,3*header.npart[i]*sizeof(float),SEEK_CUR)) {
	fprintf(stderr,
		"ssplat_load_gadget: error seeking past coordinate set %d\n",
		i+1);
	*bunches_read= i;
	return 0;
      }
    }
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
  (void)fread(&blockSizeAfter, sizeof(blockSizeAfter), 1, infile);
#pragma GCC diagnostic pop
  if (blockSizeAfter != blockSizeBefore) {
    fprintf(stderr,"ssplat_load_gadget: blocking error on block 1!\n");
    *bunches_read= 0;
    return 0;
  }

  // Load velocity data
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
  (void)fread(&blockSizeBefore, sizeof(blockSizeBefore), 1, infile);
#pragma GCC diagnostic pop
  for (int i=0; i<ngroups; i++) {
    StarBunch* sb= sbunch_tbl[i];
    if (sb) {
      long nstars= sb->nstars();
      if (nstars>0) {
	float buf[3*CHUNKSIZE];
	long iStar= 0;
	int ix= sb->allocate_next_free_prop_index(StarBunch::VEL_X_NAME);
	int iy= sb->allocate_next_free_prop_index(StarBunch::VEL_Y_NAME);
	int iz= sb->allocate_next_free_prop_index(StarBunch::VEL_Z_NAME);
	while (nstars - iStar) {
	  size_t nThisChunk= 
	    (CHUNKSIZE>(nstars-iStar))?(nstars-iStar):CHUNKSIZE;
	  if (fread(buf,sizeof(float),3*nThisChunk,infile)<3*nThisChunk) {
	    fprintf(stderr,
		    "ssplat_load_gadget: read error or premature EOF!\n");
	    return 0;
	  }
	  for (size_t iDatum=0; iDatum<nThisChunk; iDatum++) {
	    sb->set_prop(iStar+iDatum,ix,buf[3*iDatum]);
	    sb->set_prop(iStar+iDatum,iy,buf[3*iDatum+1]);
	    sb->set_prop(iStar+iDatum,iz,buf[3*iDatum+2]);
	  }
	  iStar += nThisChunk;
	}
      }
    }
    else {
      if (fseek(infile,3*header.npart[i]*sizeof(float),SEEK_CUR)) {
	fprintf(stderr,
		"ssplat_load_gadget: error seeking past velocity set %d\n",
		i+1);
	*bunches_read= i;
	return 0;
      }
    }
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
  (void)fread(&blockSizeAfter, sizeof(blockSizeAfter), 1, infile);
#pragma GCC diagnostic pop
  if (blockSizeAfter != blockSizeBefore) {
    fprintf(stderr,"ssplat_load_gadget: blocking error on block 2!\n");
    *bunches_read= 0;
    return 0;
  }

  // Load ID data
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
  (void)fread(&blockSizeBefore, sizeof(blockSizeBefore), 1, infile);
#pragma GCC diagnostic pop
  for (int i=0; i<ngroups; i++) {
    StarBunch* sb= sbunch_tbl[i];
    if (sb) {
      long nstars= sb->nstars();
      if (nstars>0) {
	unsigned int buf[CHUNKSIZE];
	long iStar= 0;
	while (nstars - iStar) {
	  size_t nThisChunk= 
	    (CHUNKSIZE>(nstars-iStar))?(nstars-iStar):CHUNKSIZE;
	  if (fread(buf,sizeof(unsigned int),nThisChunk,infile)<nThisChunk) {
	    fprintf(stderr,"ssplat_load_gadget: read error or premature EOF!\n");
	    return 0;
	  }
	  for (size_t iDatum=0; iDatum<nThisChunk; iDatum++) {
	    sb->set_id(iStar+iDatum,buf[iDatum]);
	  }
	  iStar += nThisChunk;
	}
      }
    }
    else {
      if (fseek(infile,header.npart[i]*sizeof(unsigned int),SEEK_CUR)) {
	fprintf(stderr,
		"ssplat_load_gadget: error seeking past index set %d\n",
		i+1);
	*bunches_read= i;
	return 0;
      }
    }
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result" 
  (void)fread(&blockSizeAfter, sizeof(blockSizeAfter), 1, infile);
#pragma GCC diagnostic pop
  if (blockSizeAfter != blockSizeBefore) {
    fprintf(stderr,"ssplat_load_gadget: blocking error on block 3!\n");
    *bunches_read= 0;
    return 0;
  }

  // Maybe load mass data
  if (numberOfBunchesWithVariableMass) {
    if (!loadOrDiscard( "Mass", infile, sbunch_tbl, ngroups, &header,
			typeHasVariableMass )) {
      *bunches_read= 0;
      return 0;
    }
  }

  // Maybe load InternalEnergy data
  if (numberOfSPHBunches) {
    if (!loadOrDiscard( "InternalEnergy", infile, sbunch_tbl, ngroups, &header,
			typeIsSPH )) {
      *bunches_read= 0;
      return 0;
    }
  }

  // Maybe load Density data
  if (numberOfSPHBunches) {
    if (!loadOrDiscard( "Density", infile, sbunch_tbl, ngroups, &header,
			typeIsSPH )) {
      *bunches_read= 0;
      return 0;
    }
  }

  // Maybe load electron abundance
  if (numberOfBunchesWithElectronAbundance) {
    if (!loadOrDiscard( "ElectronAbundance", infile, sbunch_tbl, ngroups, 
			&header, typeDependsOnCoolingFlag )) {
      *bunches_read= 0;
      return 0;
    }
  }

  // Maybe load neutral hydrogen density
  if (numberOfBunchesWithNeutralHydrogenDensity) {
    if (!loadOrDiscard( "NeutralHydrogenDensity", infile, sbunch_tbl, ngroups, 
			&header, typeDependsOnCoolingFlag )) {
      *bunches_read= 0;
      return 0;
    }
  }

  // Maybe load SmoothingLength data
  if (numberOfSPHBunches) {
    if (!loadOrDiscard( "SmoothingLength", infile, sbunch_tbl, ngroups, 
			&header, typeIsSPH )) {
      *bunches_read= 0;
      return 0;
    }
  }

  *bunches_read= 0;
  for (int i=0; i<ngroups; i++)
    if (sbunch_tbl[i]) *bunches_read += 1;
  return 1;
}

