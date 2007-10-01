/* $Id$ */

/**
 * @file write_fits.c
 * @ingroup oitable
 *
 * Implementation of functions to write FITS tables from data structures
 * in memory.
 *
 * Copyright (C) 2007 John Young
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

#include <stdlib.h>
#include <string.h>

#include "exchange.h"
#include "fitsio.h"


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
int write_oi_array(fitsfile *fptr, oi_array array, int extver, int *pStatus)
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
    printf("WARNING! array.revision != %d on entry to write_oi_array. Writing revision %d table\n", revision, revision);
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
  if (*pStatus) {
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
int write_oi_target(fitsfile *fptr, oi_target targets, int *pStatus)
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
    printf("WARNING! targets.revision != %d on entry to write_oi_target. Writing revision %d table\n", revision, revision);
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
  if (*pStatus) {
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
int write_oi_wavelength(fitsfile *fptr, oi_wavelength wave, int extver, 
			int *pStatus)
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
    printf("WARNING! wave.revision != %d on entry to write_oi_wavelength. Writing revision %d table\n", revision, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "INSNAME", wave.insname,
		 "Detector name", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_WAVELENGTH", pStatus);
  fits_write_col(fptr, TFLOAT, 1, 1, 1, wave.nwave, wave.eff_wave, pStatus);
  fits_write_col(fptr, TFLOAT, 2, 1, 1, wave.nwave, wave.eff_band, pStatus);

  if (*pStatus) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
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
int write_oi_vis(fitsfile *fptr, oi_vis vis, int extver, int *pStatus)
{
  const char function[] = "write_oi_vis";
  const int tfields = 12;
  char *ttype[] = {"TARGET_ID", "TIME", "MJD", "INT_TIME",
		    "VISAMP", "VISAMPERR", "VISPHI", "VISPHIERR",
		   "UCOORD", "VCOORD", "STA_INDEX", "FLAG"};
  char *tform[] = {"I", "D", "D", "D",
		   "?D", "?D", "?D", "?D",
		   "1D", "1D", "2I", "?L"};
  char *tunit[] = {"\0", "s", "day", "s",
		   "\0", "\0", "deg", "deg",
		   "m", "m", "\0", "\0"};
  char extname[] = "OI_VIS";
  int revision = 1, irow, i;
  char tmp[8];

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Create table structure: */
  /* - make up TFORM: substitute vis.nwave for '?' */
  for(i=0; i<tfields; i++) {
    if (tform[i][0] == '?') {
      sprintf(tmp, "%d%s", vis.nwave, &tform[i][1]);
      tform[i] = malloc((strlen(tmp)+1)*sizeof(char));
      strcpy(tform[i], tmp);
    }
  }
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);

  /* Write keywords */
  if (vis.revision != revision) {
    printf("WARNING! vis.revision != %d on entry to write_oi_vis. Writing revision %d table\n", revision, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "DATE-OBS", &vis.date_obs,
		 "UTC start date of observations", pStatus);
  if (strlen(vis.arrname) > 0)
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
  if (*pStatus) {
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
int write_oi_vis2(fitsfile *fptr, oi_vis2 vis2, int extver, int *pStatus)
{
  const char function[] = "write_oi_vis2";
  const int tfields = 10;
  char *ttype[] = {"TARGET_ID", "TIME", "MJD", "INT_TIME",
		   "VIS2DATA", "VIS2ERR", "UCOORD", "VCOORD",
		   "STA_INDEX", "FLAG"};
  char *tform[] = {"I", "D", "D", "D",
		   "?D", "?D", "1D", "1D",
		   "2I", "?L"};
  char *tunit[] = {"\0", "s", "day", "s",
		   "\0", "\0", "m", "m",
		   "\0", "\0"};
  char extname[] = "OI_VIS2";
  int revision = 1, irow, i;
  char tmp[8];

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Create table structure: */
  /* - make up TFORM: substitute vis2.nwave for '?' */
  for(i=0; i<tfields; i++) {
    if (tform[i][0] == '?') {
      sprintf(tmp, "%d%s", vis2.nwave, &tform[i][1]);
      tform[i] = malloc((strlen(tmp)+1)*sizeof(char));
      strcpy(tform[i], tmp);
    }
  }
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);

  /* Write keywords */
  if (vis2.revision != revision) {
    printf("WARNING! vis2.revision != %d on entry to write_oi_vis2. Writing revision %d table\n", revision, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "DATE-OBS", &vis2.date_obs,
		 "UTC start date of observations", pStatus);
  if (strlen(vis2.arrname) > 0)
    fits_write_key(fptr, TSTRING, "ARRNAME", &vis2.arrname,
		   "Array name", pStatus);
  fits_write_key(fptr, TSTRING, "INSNAME", &vis2.insname,
		 "Detector name", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_VIS2", pStatus);

  /* Write columns */
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
  if (*pStatus) {
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
int write_oi_t3(fitsfile *fptr, oi_t3 t3, int extver, int *pStatus)
{
  const char function[] = "write_oi_t3";
  const int tfields = 14;
  char *ttype[] = {"TARGET_ID", "TIME", "MJD", "INT_TIME",
		   "T3AMP", "T3AMPERR", "T3PHI", "T3PHIERR",
		   "U1COORD", "V1COORD", "U2COORD", "V2COORD", 
		   "STA_INDEX", "FLAG"};
  char *tform[] = {"I", "D", "D", "D",
		   "?D", "?D", "?D", "?D",
		   "1D", "1D", "1D", "1D",
		   "3I", "?L"};
  char *tunit[] = {"\0", "s", "day", "s",
		   "\0", "\0", "deg", "deg",
		   "m", "m", "m", "m",
		   "\0", "\0"};
  char extname[] = "OI_T3";
  int revision = 1, irow, i;
  char tmp[8];

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Create table structure: */
  /* - make up TFORM: substitute t3.nwave for '?' */
  for(i=0; i<tfields; i++) {
    if (tform[i][0] == '?') {
      sprintf(tmp, "%d%s", t3.nwave, &tform[i][1]);
      tform[i] = malloc((strlen(tmp)+1)*sizeof(char));
      strcpy(tform[i], tmp);
    }
  }
  fits_create_tbl(fptr, BINARY_TBL, 0, tfields, ttype, tform, tunit,
		  extname, pStatus);

  /* Write keywords */
  if (t3.revision != revision) {
    printf("WARNING! t3.revision != %d on entry to write_oi_t3. Writing revision %d table\n", revision, revision);
  }
  fits_write_key(fptr, TINT, "OI_REVN", &revision,
		 "Revision number of the table definition", pStatus);
  fits_write_key(fptr, TSTRING, "DATE-OBS", &t3.date_obs,
		 "UTC start date of observations", pStatus);
  if (strlen(t3.arrname) > 0)
    fits_write_key(fptr, TSTRING, "ARRNAME", &t3.arrname,
		   "Array name", pStatus);
  fits_write_key(fptr, TSTRING, "INSNAME", &t3.insname,
		 "Detector name", pStatus);
  fits_write_key(fptr, TINT, "EXTVER", &extver,
		 "ID number of this OI_T3", pStatus);

  /* Write columns */
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
  if (*pStatus) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}
