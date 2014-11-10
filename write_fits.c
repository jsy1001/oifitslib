/**
 * @file
 * @ingroup oitable
 * Implementation of functions to write FITS tables from data structures
 * in memory.
 *
 * Copyright (C) 2007, 2014 John Young
 *
 *
 * This file is part of OIFITSlib.
 *
 * OIFITSlib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OIFITSlib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with OIFITSlib.  If not, see
 * http://www.gnu.org/licenses/
 */

#include "exchange.h"

#include <fitsio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>


int oi_hush_errors = 0;


/*
 * Private functions
 */

/** Make deep copy of array of n strings, substituting @a value for any
    initial '?' */
char **make_tform(const char **template, int n, int value)
{
  char **tform;
  int i, size, needed;

  tform = malloc(n*sizeof(char *));
  for(i=0; i<n; i++) {
    if (template[i][0] == '?') {
      size = strlen(template[i]) + 4; /* allow for extra digits + \0 */
      tform[i] = malloc(size);
      needed = snprintf(tform[i], size, "%d%s", value, &template[i][1]);
      assert(needed < size); /* fails if string was truncated */
    } else {
      tform[i] = malloc((strlen(template[i])+1)); /* +1 for \0 */
      strcpy(tform[i], template[i]);
    }
  }
  return tform;
}

/** Free array of n strings returned by make_tform() */
void free_tform(char **tform, int n)
{
  int i;

  for(i=0; i<n; i++)
    free(tform[i]);
  free(tform);
}


/*
 * Public functions
 */

/**
 * Write OI_ARRAY fits binary table
 *
 *   @param fptr    see cfitsio documentation
 *   @param array   array data struct, see exchange.h
 *   @param extver  value for EXTVER keyword
 *   @param pStatus pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */
STATUS write_oi_array(fitsfile *fptr, oi_array array, int extver,
                      STATUS *pStatus)
{
  const char function[] = "write_oi_array";
  const int tfields = 5;
  char *ttype[] = {"TEL_NAME", "STA_NAME", "STA_INDEX", "DIAMETER", "STAXYZ"};
  char *tform[] = {"16A", "16A", "I", "E", "3D"};
  char *tunit[] = {"\0", "\0", "\0", "m", "m"};
  char extname[] = "OI_ARRAY";
  char *str;
  int revision = 1, irow;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  if (array.revision != revision) {
    printf("WARNING! array.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "ARRNAME", array.arrname,
		 "Array name", pStatus);
  fits_write_key(fptr, TSTRING, "FRAME", array.frame,
		 "Coordinate frame", pStatus);
  fits_write_key(fptr, TDOUBLE, "ARRAYX", &array.arrayx,
		 "Array centre x coordinate", pStatus);
  fits_write_key_unit(fptr, "ARRAYX", "m", pStatus);
  fits_write_key(fptr, TDOUBLE, "ARRAYY", &array.arrayy,
		 "Array centre y coordinate", pStatus);
  fits_write_key_unit(fptr, "ARRAYY", "m", pStatus);
  fits_write_key(fptr, TDOUBLE, "ARRAYZ", &array.arrayz,
		 "Array centre z coordinate", pStatus);
  fits_write_key_unit(fptr, "ARRAYZ", "m", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_ARRAY", pStatus);
  for(irow=1; irow<=array.nelement; irow++) {
    str = array.elem[irow-1].tel_name;
    fits_write_col(fptr, TSTRING, 1, irow, 1, 1, &str, pStatus);
    str = array.elem[irow-1].sta_name;
    fits_write_col(fptr, TSTRING, 2, irow, 1, 1, &str, pStatus);
    /* make cfitsio convert int->short here */
    fits_write_col(fptr, TINT, 3, irow, 1, 1, &array.elem[irow-1].sta_index,
		   pStatus);
    fits_write_col(fptr, TFLOAT, 4, irow, 1, 1, &array.elem[irow-1].diameter,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 5, irow, 1, 3, &array.elem[irow-1].staxyz,
		   pStatus);
  }
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Write OI_TARGET fits binary table
 *
 *   @param fptr     see cfitsio documentation
 *   @param targets  targets data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */
STATUS write_oi_target(fitsfile *fptr, oi_target targets, STATUS *pStatus)
{
  const char function[] = "write_oi_target";
  const int tfields = 17;
  char *ttype[] = {"TARGET_ID", "TARGET", "RAEP0", "DECEP0", "EQUINOX",
		   "RA_ERR", "DEC_ERR", "SYSVEL", "VELTYP",
		   "VELDEF", "PMRA", "PMDEC", "PMRA_ERR", "PMDEC_ERR",
		   "PARALLAX", "PARA_ERR", "SPECTYP"};
  char *tform[] = {"I", "16A", "D", "D", "E",
		   "D", "D", "D", "8A",
		   "8A", "D", "D", "D", "D",
		   "E", "E", "16A"};
  char *tunit[] = {"\0", "\0", "deg", "deg", "yr",
		   "deg", "deg", "m/s", "\0",
		   "\0", "deg/yr", "deg/yr", "deg/yr", "deg/yr",
		   "deg", "deg", "\0"};
  char extname[] = "OI_TARGET";
  char *str;
  int revision = 1, irow;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  if (targets.revision != revision) {
    printf("WARNING! targets.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  for(irow=1; irow<=targets.ntarget; irow++) {
    /* make cfitsio convert int->short here */
    fits_write_col(fptr, TINT, 1, irow, 1, 1,
		   &targets.targ[irow-1].target_id, pStatus);
    str = targets.targ[irow-1].target;
    fits_write_col(fptr, TSTRING, 2, irow, 1, 1, &str, pStatus);
    fits_write_col(fptr, TDOUBLE, 3, irow, 1, 1, &targets.targ[irow-1].raep0,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 4, irow, 1, 1,
		   &targets.targ[irow-1].decep0, pStatus);
    fits_write_col(fptr, TFLOAT, 5, irow, 1, 1,
		   &targets.targ[irow-1].equinox, pStatus);
    fits_write_col(fptr, TDOUBLE, 6, irow, 1, 1,
		   &targets.targ[irow-1].ra_err, pStatus);
    fits_write_col(fptr, TDOUBLE, 7, irow, 1, 1,
		   &targets.targ[irow-1].dec_err, pStatus);
    fits_write_col(fptr, TDOUBLE, 8, irow, 1, 1,
		   &targets.targ[irow-1].sysvel, pStatus);
    str = targets.targ[irow-1].veltyp;
    fits_write_col(fptr, TSTRING, 9, irow, 1, 1, &str, pStatus);
    str = targets.targ[irow-1].veldef;
    fits_write_col(fptr, TSTRING, 10, irow, 1, 1, &str, pStatus);
    fits_write_col(fptr, TDOUBLE, 11, irow, 1, 1,
		   &targets.targ[irow-1].pmra, pStatus);
    fits_write_col(fptr, TDOUBLE, 12, irow, 1, 1,
		   &targets.targ[irow-1].pmdec, pStatus);
    fits_write_col(fptr, TDOUBLE, 13, irow, 1, 1,
		   &targets.targ[irow-1].pmra_err, pStatus);
    fits_write_col(fptr, TDOUBLE, 14, irow, 1, 1,
		   &targets.targ[irow-1].pmdec_err, pStatus);
    fits_write_col(fptr, TFLOAT, 15, irow, 1, 1,
		   &targets.targ[irow-1].parallax, pStatus);
    fits_write_col(fptr, TFLOAT, 16, irow, 1, 1,
		   &targets.targ[irow-1].para_err, pStatus);
    str = targets.targ[irow-1].spectyp;
    fits_write_col(fptr, TSTRING, 17, irow, 1, 1, &str, pStatus);
  }

  /* Write optional columns */
  if (targets.usecategory)
  {
    fits_insert_col(fptr, 18, "CATEGORY", "3A", pStatus);
    for(irow=1; irow<=targets.ntarget; irow++) {
      str = targets.targ[irow-1].category;
      fits_write_col(fptr, TSTRING, 18, irow, 1, 1, &str, pStatus);
    }
  }

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Write OI_WAVELENGTH fits binary table 
 *
 *   @param fptr     see cfitsio documentation
 *   @param wave     wavelength data struct, see exchange.h
 *   @param extver   value for EXTVER keyword
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */
STATUS write_oi_wavelength(fitsfile *fptr, oi_wavelength wave, int extver, 
                           STATUS *pStatus)
{
  const char function[] = "write_oi_wavelength";
  const int tfields = 2;
  char *ttype[] = {"EFF_WAVE", "EFF_BAND"};
  char *tform[] = {"E", "E"};
  char *tunit[] = {"m", "m"};
  char extname[] = "OI_WAVELENGTH";
  int revision = 1;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  if (wave.revision != revision) {
    printf("WARNING! wave.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "INSNAME", wave.insname,
		 "Detector name", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_WAVELENGTH", pStatus);
  fits_write_col(fptr, TFLOAT, 1, 1, 1, wave.nwave, wave.eff_wave, pStatus);
  fits_write_col(fptr, TFLOAT, 2, 1, 1, wave.nwave, wave.eff_band, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Write OI_CORR fits binary table 
 *
 *   @param fptr     see cfitsio documentation
 *   @param corr     corr data struct, see exchange.h
 *   @param extver   value for EXTVER keyword
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */
STATUS write_oi_corr(fitsfile *fptr, oi_corr corr, int extver, STATUS *pStatus)
{
  const char function[] = "write_oi_corr";
  const int tfields = 3;
  char *ttype[] = {"IINDX", "JINDX", "CORR"};
  char *tform[] = {"J", "J", "D"};
  char *tunit[] = {"\0", "\0", "\0"};
  char extname[] = "OI_CORR";
  int revision = 1;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  if (corr.revision != revision) {
    printf("WARNING! corr.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "CORRNAME", corr.corrname,
		 "Name of correlated data set", pStatus);
  fits_write_key(fptr, TINT, "NDATA", &corr.ndata,
		 "Number of correlated data", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_CORR", pStatus);
  fits_write_col(fptr, TINT, 1, 1, 1, corr.ncorr, corr.iindx, pStatus);
  fits_write_col(fptr, TINT, 2, 1, 1, corr.ncorr, corr.jindx, pStatus);
  fits_write_col(fptr, TDOUBLE, 3, 1, 1, corr.ncorr, corr.corr, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/**
 * Write OI_POLAR fits binary table 
 *
 *   @param fptr     see cfitsio documentation
 *   @param polar    polar struct, see exchange.h
 *   @param extver   value for EXTVER keyword
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */
STATUS write_oi_polar(fitsfile *fptr, oi_polar polar, int extver,
                      STATUS *pStatus)
{
  const char function[] = "write_oi_polar";
  const int tfields = 9;
  char *ttype[] = {"TARGET_ID", "INSNAME", "MJD", "INT_TIME",
                   "LXX", "LYY", "LXY", "LYX", "STA_INDEX"};
  //:TODO: follow standard in choosing repeat count for INSNAME
  const char *tformTpl[] = {"I", "70A", "D", "D",
			    "?C", "?C", "?C", "?C", "2I"};
  char **tform;
  char *tunit[] = {"\0", "\0", "day", "s",
		   "\0", "\0", "\0", "\0", "\0"};
  char extname[] = "OI_POLAR";
  char *str;
  int revision = 1, irow;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Create table structure */
  tform = make_tform(tformTpl, tfields, polar.nwave);
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  free_tform(tform, tfields);

  /* Write keywords */
  if (polar.revision != revision) {
    printf("WARNING! polar.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "DATE-OBS", &polar.date_obs,
		 "UTC start date of observations", pStatus);
  fits_write_key(fptr, TINT, "NPOL", &polar.npol,
                 "Number of polarization types", pStatus);
  /* note ARRNAME is mandatory */
  fits_write_key(fptr, TSTRING, "ARRNAME", &polar.arrname,
                 "Array name", pStatus);
  fits_write_key(fptr, TSTRING, "ORIENT", &polar.orient,
                 "Orientation of the Jones matrix L..", pStatus);
  fits_write_key(fptr, TSTRING, "MODEL", &polar.model,
		 "How Jones matrix L.. was estimated", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_POLAR", pStatus);

  /* Write columns */
  for(irow=1; irow<=polar.numrec; irow++) {

    fits_write_col(fptr, TINT, 1, irow, 1, 1,
                   &polar.record[irow-1].target_id, pStatus);
    str = polar.record[irow-1].insname;
    fits_write_col(fptr, TSTRING, 2, irow, 1, 1, &str, pStatus);
    fits_write_col(fptr, TDOUBLE, 3, irow, 1, 1,
                   &polar.record[irow-1].mjd, pStatus);
    fits_write_col(fptr, TDOUBLE, 4, irow, 1, 1,
                   &polar.record[irow-1].int_time, pStatus);
    fits_write_col(fptr, TCOMPLEX, 5, irow, 1, polar.nwave,
		   polar.record[irow-1].lxx, pStatus);
    fits_write_col(fptr, TCOMPLEX, 6, irow, 1, polar.nwave,
		   polar.record[irow-1].lyy, pStatus);
    fits_write_col(fptr, TCOMPLEX, 7, irow, 1, polar.nwave,
		   polar.record[irow-1].lxy, pStatus);
    fits_write_col(fptr, TCOMPLEX, 8, irow, 1, polar.nwave,
		   polar.record[irow-1].lyx, pStatus);
    fits_write_col(fptr, TINT, 9, irow, 1, 2, polar.record[irow-1].sta_index,
		   pStatus);
  }
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/**
 * Write OI_VIS optional content
 */
static STATUS write_oi_vis_opt(fitsfile *fptr, oi_vis vis, STATUS *pStatus)
{
  const int tfields = 4;
  char *ttype[] = {"RVIS", "RVISERR", "IVIS", "IVISERR"};
  const char *tformTpl[] = {"?D", "?D", "?D", "?D"};
  char **tform;
  int irow;
  bool correlated;
  char keyval[FLEN_VALUE];

  /* Write optional keywords */
  correlated = (strlen(vis.corrname) > 0);
  if (correlated)
    fits_write_key(fptr, TSTRING, "CORRNAME", &vis.corrname,
		   "Correlated data set name", pStatus);
  if (strlen(vis.amptyp) > 0)
    fits_write_key(fptr, TSTRING, "AMPTYP", &vis.amptyp,
		   "Class of amplitude data", pStatus);
  if (strlen(vis.phityp) > 0)
    fits_write_key(fptr, TSTRING, "PHITYP", &vis.phityp,
		   "Class of phase data", pStatus);
  if (vis.amporder >= 0)
    fits_write_key(fptr, TINT, "AMPORDER", &vis.amporder,
		   "Polynomial fit order for differential amp", pStatus);
  if (vis.phiorder >= 0)
    fits_write_key(fptr, TINT, "PHIORDER", &vis.phiorder,
		   "Polynomial fit order for differential phi", pStatus);

  /* Write optional columns */
  if (correlated)
  {
    fits_insert_col(fptr, 7, "CORRINDX_VISAMP", "J", pStatus);
    fits_insert_col(fptr, 10, "CORRINDX_VISPHI", "J", pStatus);
    for(irow=1; irow<=vis.numrec; irow++) {
      fits_write_col(fptr, TINT, 7, irow, 1, 1,
                     &vis.record[irow-1].corrindx_visamp, pStatus);
      fits_write_col(fptr, TINT, 10, irow, 1, 1,
                     &vis.record[irow-1].corrindx_visphi, pStatus);
    }
  }
  if (vis.usevisrefmap) {
    snprintf(keyval, FLEN_VALUE, "%dL", vis.nwave*vis.nwave);
    fits_insert_col(fptr, 11, "VISREFMAP", keyval, pStatus);
    snprintf(keyval, FLEN_VALUE, "(%d,%d)", vis.nwave, vis.nwave);
    fits_write_key(fptr, TSTRING, "TDIM11", &keyval,
                   "Dimensions of field  11", pStatus);
    for(irow=1; irow<=vis.numrec; irow++) {
      fits_write_col(fptr, TLOGICAL, 11, irow, 1, vis.nwave*vis.nwave,
                     vis.record[irow-1].visrefmap, pStatus);
    }
  }
  if (vis.usecomplex) {
    tform = make_tform(tformTpl, tfields, vis.nwave);
    fits_insert_cols(fptr, 9, tfields, ttype, tform, pStatus);
    free_tform(tform, tfields);
  
    //:TODO: write units

    for(irow=1; irow<=vis.numrec; irow++) {

      assert(vis.record[irow-1].rvis != NULL);
      assert(vis.record[irow-1].rviserr != NULL);
      assert(vis.record[irow-1].ivis != NULL);
      assert(vis.record[irow-1].iviserr != NULL);
      fits_write_col(fptr, TDOUBLE, 9, irow, 1, vis.nwave,
                     vis.record[irow-1].rvis, pStatus);
      fits_write_col(fptr, TDOUBLE, 10, irow, 1, vis.nwave,
                     vis.record[irow-1].rviserr, pStatus);
      fits_write_col(fptr, TDOUBLE, 11, irow, 1, vis.nwave,
                     vis.record[irow-1].ivis, pStatus);
      fits_write_col(fptr, TDOUBLE, 12, irow, 1, vis.nwave,
                     vis.record[irow-1].iviserr, pStatus);
    }
    if (correlated)
    {
      fits_insert_col(fptr, 11, "CORRINDX_RVIS", "J", pStatus);
      fits_insert_col(fptr, 14, "CORRINDX_IVIS", "J", pStatus);
      for(irow=1; irow<=vis.numrec; irow++) {

        fits_write_col(fptr, TINT, 11, irow, 1, 1,
                       &vis.record[irow-1].corrindx_rvis, pStatus);
        fits_write_col(fptr, TINT, 14, irow, 1, 1,
                       &vis.record[irow-1].corrindx_ivis, pStatus);
      }
    }
  }
  return *pStatus;
}

/**
 * Write OI_VIS fits binary table 
 *
 *   @param fptr     see cfitsio documentation
 *   @param vis      data struct, see exchange.h
 *   @param extver   value for EXTVER keyword
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */
STATUS write_oi_vis(fitsfile *fptr, oi_vis vis, int extver, STATUS *pStatus)
{
  const char function[] = "write_oi_vis";
  const int tfields = 12;
  char *ttype[] = {"TARGET_ID", "TIME", "MJD", "INT_TIME",
                   "VISAMP", "VISAMPERR", "VISPHI", "VISPHIERR",
		   "UCOORD", "VCOORD", "STA_INDEX", "FLAG"};
  const char *tformTpl[] = {"I", "D", "D", "D",
			    "?D", "?D", "?D", "?D",
			    "1D", "1D", "2I", "?L"};
  char **tform;
  char *tunit[] = {"\0", "s", "day", "s",
		   "\0", "\0", "deg", "deg",
		   "m", "m", "\0", "\0"};
  char extname[] = "OI_VIS";
  int revision = 1, irow;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Create table structure */
  tform = make_tform(tformTpl, tfields, vis.nwave);
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  free_tform(tform, tfields);

  /* Write keywords */
  if (vis.revision != revision) {
    printf("WARNING! vis.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "DATE-OBS", &vis.date_obs,
		 "UTC start date of observations", pStatus);
  if (strlen(vis.arrname) > 0) //:TODO: mandatory in OIFITS2
    fits_write_key(fptr, TSTRING, "ARRNAME", &vis.arrname,
		   "Array name", pStatus);
  fits_write_key(fptr, TSTRING, "INSNAME", &vis.insname,
		 "Detector name", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_VIS", pStatus);

  /* Write columns */
  for(irow=1; irow<=vis.numrec; irow++) {

    fits_write_col(fptr, TINT, 1, irow, 1, 1, &vis.record[irow-1].target_id,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 2, irow, 1, 1, &vis.record[irow-1].time,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 3, irow, 1, 1, &vis.record[irow-1].mjd,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 4, irow, 1, 1, &vis.record[irow-1].int_time,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 5, irow, 1, vis.nwave,
		   vis.record[irow-1].visamp, pStatus);
    fits_write_col(fptr, TDOUBLE, 6, irow, 1, vis.nwave,
		   vis.record[irow-1].visamperr, pStatus);
    fits_write_col(fptr, TDOUBLE, 7, irow, 1, vis.nwave,
		   vis.record[irow-1].visphi, pStatus);
    fits_write_col(fptr, TDOUBLE, 8, irow, 1, vis.nwave,
		   vis.record[irow-1].visphierr, pStatus);
    fits_write_col(fptr, TDOUBLE, 9, irow, 1, 1, &vis.record[irow-1].ucoord,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 10, irow, 1, 1, &vis.record[irow-1].vcoord,
		   pStatus);
    fits_write_col(fptr, TINT, 11, irow, 1, 2, vis.record[irow-1].sta_index,
		   pStatus);
    fits_write_col(fptr, TLOGICAL, 12, irow, 1, vis.nwave,
		   vis.record[irow-1].flag, pStatus);
  }
  write_oi_vis_opt(fptr, vis, pStatus);
  
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Write OI_VIS2 fits binary table 
 *
 *   @param fptr     see cfitsio documentation
 *   @param vis2     data struct, see exchange.h
 *   @param extver   value for EXTVER keyword
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */
STATUS write_oi_vis2(fitsfile *fptr, oi_vis2 vis2, int extver, STATUS *pStatus)
{
  const char function[] = "write_oi_vis2";
  const int tfields = 10;  /* mandatory columns */
  char *ttype[] = {"TARGET_ID", "TIME", "MJD", "INT_TIME",
		   "VIS2DATA", "VIS2ERR", "UCOORD", "VCOORD",
		   "STA_INDEX", "FLAG"};
  const char *tformTpl[] = {"I", "D", "D", "D",
			    "?D", "?D", "1D", "1D",
			    "2I", "?L"};
  char **tform;
  char *tunit[] = {"\0", "s", "day", "s",
		   "\0", "\0", "m", "m",
		   "\0", "\0"};
  char extname[] = "OI_VIS2";
  int revision = 1, irow;
  bool correlated;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Create table structure */
  tform = make_tform(tformTpl, tfields, vis2.nwave);
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  free_tform(tform, tfields);

  /* Write mandatory keywords */
  if (vis2.revision != revision) {
    printf("WARNING! vis2.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "DATE-OBS", &vis2.date_obs,
		 "UTC start date of observations", pStatus);
  if (strlen(vis2.arrname) > 0) //:TODO: mandatory in OIFITS2
    fits_write_key(fptr, TSTRING, "ARRNAME", &vis2.arrname,
		   "Array name", pStatus);
  fits_write_key(fptr, TSTRING, "INSNAME", &vis2.insname,
		 "Detector name", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_VIS2", pStatus);

  /* Write mandatory columns */
  for(irow=1; irow<=vis2.numrec; irow++) {

    fits_write_col(fptr, TINT, 1, irow, 1, 1, &vis2.record[irow-1].target_id,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 2, irow, 1, 1, &vis2.record[irow-1].time,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 3, irow, 1, 1, &vis2.record[irow-1].mjd,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 4, irow, 1, 1, &vis2.record[irow-1].int_time,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 5, irow, 1, vis2.nwave,
		   vis2.record[irow-1].vis2data, pStatus);
    fits_write_col(fptr, TDOUBLE, 6, irow, 1, vis2.nwave,
		   vis2.record[irow-1].vis2err, pStatus);
    fits_write_col(fptr, TDOUBLE, 7, irow, 1, 1, &vis2.record[irow-1].ucoord,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 8, irow, 1, 1, &vis2.record[irow-1].vcoord,
		   pStatus);
    fits_write_col(fptr, TINT, 9, irow, 1, 2, vis2.record[irow-1].sta_index,
		   pStatus);
    fits_write_col(fptr, TLOGICAL, 10, irow, 1, vis2.nwave,
		   vis2.record[irow-1].flag, pStatus);
  }

  /* Write optional keywords */
  correlated = (strlen(vis2.corrname) > 0);
  if (correlated)
    fits_write_key(fptr, TSTRING, "CORRNAME", &vis2.corrname,
		   "Correlated data set name", pStatus);

  /* Write optional columns */
  if (correlated)
  {
    fits_insert_col(fptr, 7, "CORRINDX_VIS2DATA", "J", pStatus);
    for(irow=1; irow<=vis2.numrec; irow++) {
      fits_write_col(fptr, TINT, 7, irow, 1, 1,
                     &vis2.record[irow-1].corrindx_vis2data, pStatus);
    }
  }

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Write OI_T3 fits binary table 
 *
 *   @param fptr     see cfitsio documentation
 *   @param t3       data struct, see exchange.h
 *   @param extver   value for EXTVER keyword
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */ 
STATUS write_oi_t3(fitsfile *fptr, oi_t3 t3, int extver, STATUS *pStatus)
{
  const char function[] = "write_oi_t3";
  const int tfields = 14;
  char *ttype[] = {"TARGET_ID", "TIME", "MJD", "INT_TIME",
		   "T3AMP", "T3AMPERR", "T3PHI", "T3PHIERR",
		   "U1COORD", "V1COORD", "U2COORD", "V2COORD", 
		   "STA_INDEX", "FLAG"};
  const char *tformTpl[] = {"I", "D", "D", "D",
			    "?D", "?D", "?D", "?D",
			    "1D", "1D", "1D", "1D",
			    "3I", "?L"};
  char **tform;
  char *tunit[] = {"\0", "s", "day", "s",
		   "\0", "\0", "deg", "deg",
		   "m", "m", "m", "m",
		   "\0", "\0"};
  char extname[] = "OI_T3";
  int revision = 1, irow;
  bool correlated;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Create table structure */
  tform = make_tform(tformTpl, tfields, t3.nwave);
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  free_tform(tform, tfields);

  /* Write mandatory keywords */
  if (t3.revision != revision) {
    printf("WARNING! t3.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "DATE-OBS", &t3.date_obs,
		 "UTC start date of observations", pStatus);
  if (strlen(t3.arrname) > 0) //:TODO: mandatory in OIFITS2
    fits_write_key(fptr, TSTRING, "ARRNAME", &t3.arrname,
		   "Array name", pStatus);
  fits_write_key(fptr, TSTRING, "INSNAME", &t3.insname,
		 "Detector name", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_T3", pStatus);

  /* Write mandatory columns */
  for(irow=1; irow<=t3.numrec; irow++) {

    fits_write_col(fptr, TINT, 1, irow, 1, 1, &t3.record[irow-1].target_id,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 2, irow, 1, 1, &t3.record[irow-1].time,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 3, irow, 1, 1, &t3.record[irow-1].mjd,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 4, irow, 1, 1, &t3.record[irow-1].int_time,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 5, irow, 1, t3.nwave,
		   t3.record[irow-1].t3amp, pStatus);
    fits_write_col(fptr, TDOUBLE, 6, irow, 1, t3.nwave,
		   t3.record[irow-1].t3amperr, pStatus);
    fits_write_col(fptr, TDOUBLE, 7, irow, 1, t3.nwave,
		   t3.record[irow-1].t3phi, pStatus);
    fits_write_col(fptr, TDOUBLE, 8, irow, 1, t3.nwave,
		   t3.record[irow-1].t3phierr, pStatus);
    fits_write_col(fptr, TDOUBLE, 9, irow, 1, 1, &t3.record[irow-1].u1coord,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 10, irow, 1, 1, &t3.record[irow-1].v1coord,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 11, irow, 1, 1, &t3.record[irow-1].u2coord,
		   pStatus);
    fits_write_col(fptr, TDOUBLE, 12, irow, 1, 1, &t3.record[irow-1].v2coord,
		   pStatus);
    fits_write_col(fptr, TINT, 13, irow, 1, 3, t3.record[irow-1].sta_index,
		   pStatus);
    fits_write_col(fptr, TLOGICAL, 14, irow, 1, t3.nwave,
		   t3.record[irow-1].flag, pStatus);
  }

  /* Write optional keywords */
  correlated = (strlen(t3.corrname) > 0);
  if (correlated)
    fits_write_key(fptr, TSTRING, "CORRNAME", &t3.corrname,
		   "Correlated data set name", pStatus);

  /* Write optional columns */
  if (correlated)
  {
    fits_insert_col(fptr, 7, "CORRINDX_T3AMP", "J", pStatus);
    fits_insert_col(fptr, 10, "CORRINDX_T3PHI", "J", pStatus);
    for(irow=1; irow<=t3.numrec; irow++) {
      fits_write_col(fptr, TINT, 7, irow, 1, 1,
                     &t3.record[irow-1].corrindx_t3amp, pStatus);
      fits_write_col(fptr, TINT, 10, irow, 1, 1,
                     &t3.record[irow-1].corrindx_t3phi, pStatus);
    }
  }

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Write OI_SPECTRUM fits binary table 
 *
 *   @param fptr      see cfitsio documentation
 *   @param spectrum  data struct, see exchange.h
 *   @param extver    value for EXTVER keyword
 *   @param pStatus   pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code, and sets *pStatus
 */ 
STATUS write_oi_spectrum(fitsfile *fptr, oi_spectrum spectrum, int extver,
                         STATUS *pStatus)
{
  const char function[] = "write_oi_spectrum";
  //:TODO: maybe omit STA_INDEX column
  const int tfields = 6;
  char *ttype[] = {"TARGET_ID", "MJD", "INT_TIME",
		   "FLUXDATA", "FLUXERR", "STA_INDEX"};
  const char *tformTpl[] = {"I", "D", "D",
			    "?D", "?D", "I"};
  char **tform;
  //:TODO: flux units from struct?
  char *tunit[] = {"\0", "day", "s",
		   "\0", "\0", "\0"};
  char extname[] = "OI_SPECTRUM";
  char keyval[FLEN_VALUE];
  int revision = 1, irow;
  bool correlated;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Create table structure */
  tform = make_tform(tformTpl, tfields, spectrum.nwave);
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);
  free_tform(tform, tfields);

  /* Write mandatory keywords */
  if (spectrum.revision != revision) {
    printf("WARNING! spectrum.revision != %d on entry to %s. "
           "Writing revision %d table\n", revision, function, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "DATE-OBS", &spectrum.date_obs,
		 "UTC start date of observations", pStatus);
  if (strlen(spectrum.arrname) > 0)
    fits_write_key(fptr, TSTRING, "ARRNAME", &spectrum.arrname,
		   "Array name", pStatus);
  fits_write_key(fptr, TSTRING, "INSNAME", &spectrum.insname,
		 "Detector name", pStatus);
  fits_write_key(fptr, TDOUBLE, "FOV", &spectrum.fov,
                 "[arcsec] Field Of View on sky for FLUXDATA", pStatus);
  //:TODO: alt function to write unit?
  fits_write_key(fptr, TSTRING, "FOVTYPE", &spectrum.fovtype,
                 "Model for FOV", pStatus);
  keyval[0] = spectrum.calstat;
  keyval[1] = '\0';
  fits_write_key(fptr, TSTRING, "CALSTAT", &keyval,
                 "Calibration state (U or C)", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_SPECTRUM", pStatus);

  /* Write mandatory columns */
  for(irow=1; irow<=spectrum.numrec; irow++) {

    fits_write_col(fptr, TINT, 1, irow, 1, 1,
                   &spectrum.record[irow-1].target_id, pStatus);
    fits_write_col(fptr, TDOUBLE, 2, irow, 1, 1,
                   &spectrum.record[irow-1].mjd, pStatus);
    fits_write_col(fptr, TDOUBLE, 3, irow, 1, 1,
                   &spectrum.record[irow-1].int_time, pStatus);
    fits_write_col(fptr, TDOUBLE, 4, irow, 1, spectrum.nwave,
		   spectrum.record[irow-1].fluxdata, pStatus);
    fits_write_col(fptr, TDOUBLE, 5, irow, 1, spectrum.nwave,
		   spectrum.record[irow-1].fluxerr, pStatus);
    fits_write_col(fptr, TINT, 6, irow, 1, 1,
                   &spectrum.record[irow-1].sta_index, pStatus);
  }

  /* Write optional keywords */
  correlated = (strlen(spectrum.corrname) > 0);
  if (correlated)
    fits_write_key(fptr, TSTRING, "CORRNAME", &spectrum.corrname,
		   "Correlated data set name", pStatus);

  /* Write optional columns */
  if (correlated)
  {
    fits_insert_col(fptr, 6, "CORRINDX_FLUXDATA", "J", pStatus);
    for(irow=1; irow<=spectrum.numrec; irow++) {
      fits_write_col(fptr, TINT, 6, irow, 1, 1,
                     &spectrum.record[irow-1].corrindx_fluxdata, pStatus);
    }
  }

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}
