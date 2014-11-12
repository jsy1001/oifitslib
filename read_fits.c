/**
 * @file
 * @ingroup oitable
 * Implementation of functions to read individual FITS tables and
 * write to data structures in memory.
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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/*
 * Private functions
 */


/**
 * Move to next binary table HDU with specified EXTNAME.
 *
 *   @param fptr     see cfitsio documentation
 *   @param reqName  required value of EXTNAME keyword
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus)
 */
static STATUS next_named_hdu(fitsfile *fptr, char *reqName, STATUS *pStatus)
{
  char comment[FLEN_COMMENT], extname[FLEN_VALUE];
  int hdutype;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Move to correct HDU - don't assume anything about EXTVERs */
  while (1==1) {
    fits_movrel_hdu(fptr, 1, &hdutype, pStatus);
    if (*pStatus) return *pStatus; /* no more HDUs */
    if (hdutype == BINARY_TBL) {
      fits_read_key(fptr, TSTRING, "EXTNAME", extname, comment, pStatus);
      if (strcmp(extname, reqName) == 0) break; /* current HDU matches */
    }
  }
  return *pStatus;
}

/**
 * Move to first binary table HDU with specified EXTNAME and keyword=value.
 *
 *   @param fptr     see cfitsio documentation
 *   @param reqName  required value of EXTNAME keyword
 *   @param keyword  keyword to check
 *   @param reqVal   required value of specified keyword
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus)
 */
static STATUS specific_named_hdu(fitsfile *fptr, char *reqName,
                                 char *keyword, char *reqVal, STATUS *pStatus)
{
  char comment[FLEN_COMMENT], extname[FLEN_VALUE], value[FLEN_VALUE];
  int ihdu, nhdu, hdutype;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Move to correct HDU - don't assume anything about EXTVERs */
  fits_get_num_hdus(fptr, &nhdu, pStatus);
  for (ihdu=2; ihdu<=nhdu; ihdu++) {
    fits_movabs_hdu(fptr, ihdu, &hdutype, pStatus);
    if (hdutype == BINARY_TBL) {
      fits_read_key(fptr, TSTRING, "EXTNAME", extname, comment, pStatus);
      fits_read_key(fptr, TSTRING, keyword, value, comment, pStatus);
      if (*pStatus) {
	*pStatus = 0;
	continue; /* next HDU */
      }
      if (strcmp(extname, reqName) != 0 || strcmp(value, reqVal) != 0)
	continue; /* next HDU */
    }
    break; /* current HDU matches */
  }
  if (ihdu > nhdu) {
    /* no matching HDU */
    *pStatus = BAD_HDU_NUM;
  }

  return *pStatus;
}

/**
 * Read optional header keyword.
 *
 * @return TRUE if keyword present, FALSE otherwise.
 */
static bool read_key_opt_string(fitsfile *fptr, char *keyname,
                                char *keyval, STATUS *pStatus)
{

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  if (fits_read_key(fptr, TSTRING, keyname, keyval, NULL, pStatus)) {
    *pStatus = 0;
    keyval[0] = '\0';
    return FALSE;
  }
  return TRUE;
}

/**
 * Read OI_ARRAY fits binary table at current HDU.
 *
 *   @param fptr     see cfitsio documentation
 *   @param pArray   ptr to array data struct, see exchange.h
 *   @param arrname  value of ARRNAME keyword if known, else NULL
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of array data struct are undefined
 */
static STATUS read_oi_array_chdu(fitsfile *fptr, oi_array *pArray,
                                 char *arrname, STATUS *pStatus)
{
  char comment[FLEN_COMMENT], name[FLEN_VALUE];
  char *p;
  char nullstring[] = "NULL";
  int nullint = 0;
  float nullfloat = 0.0F;
  double nulldouble = 0.0;
  const int revision = 1;
  int irow, colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pArray->revision, comment, pStatus);
  if (pArray->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword "
           "in OI_ARRAY table. Got %d\n", revision, pArray->revision);
  }
  if (arrname == NULL) {
    fits_read_key(fptr, TSTRING, "ARRNAME", name, comment, pStatus);
    strncpy(pArray->arrname, name, FLEN_VALUE-1);
  } else {
    strncpy(pArray->arrname, arrname, FLEN_VALUE-1);
  }
  fits_read_key(fptr, TSTRING, "FRAME", pArray->frame, comment, pStatus);
  fits_read_key(fptr, TDOUBLE, "ARRAYX", &pArray->arrayx, comment, pStatus);
  fits_read_key(fptr, TDOUBLE, "ARRAYY", &pArray->arrayy, comment, pStatus);
  fits_read_key(fptr, TDOUBLE, "ARRAYZ", &pArray->arrayz, comment, pStatus);
  /* get number of rows */
  fits_get_num_rows(fptr, &repeat, pStatus);
  pArray->nelement = repeat;
  pArray->elem = malloc(pArray->nelement*sizeof(element));
  /* read rows */
  for (irow=1; irow<=pArray->nelement; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TEL_NAME", &colnum, pStatus);
    p = pArray->elem[irow-1].tel_name;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, nullstring, &p, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_NAME", &colnum, pStatus);
    p = pArray->elem[irow-1].sta_name;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, nullstring, &p, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
		  &pArray->elem[irow-1].sta_index, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "DIAMETER", &colnum, pStatus);
    fits_read_col(fptr, TFLOAT, colnum, irow, 1, 1, &nullfloat,
		  &pArray->elem[irow-1].diameter, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STAXYZ", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 3, &nulldouble,
		  &pArray->elem[irow-1].staxyz, &anynull, pStatus);
    /*printf("%8s  %8s  %d  %5f  %10f %10f %10f\n",
	   pArray->elem[irow-1].tel_name,
	   pArray->elem[irow-1].sta_name, pArray->elem[irow-1].sta_index,
	   pArray->elem[irow-1].diameter, pArray->elem[irow-1].staxyz[0],
	   pArray->elem[irow-1].staxyz[1], pArray->elem[irow-1].staxyz[2]);*/
  }
  return *pStatus;
}


/**
 * Read OI_WAVELENGTH fits binary table at current HDU.
 *
 *   @param fptr     see cfitsio documentation
 *   @param pWave    ptr to wavelength data struct, see exchange.h
 *   @param insname  value of INSNAME keyword if known, else NULL
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of wavelength data struct are undefined
 */
static STATUS read_oi_wavelength_chdu(fitsfile *fptr, oi_wavelength *pWave,
                                      char *insname, STATUS *pStatus)
{
  char comment[FLEN_COMMENT], name[FLEN_VALUE];
  float nullfloat = 0.0F;
  const int revision = 1;
  int colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pWave->revision, comment, pStatus);
  if (pWave->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword in "
           "OI_WAVELENGTH table. Got %d\n", revision, pWave->revision);
  }
  if (insname == NULL) {
    fits_read_key(fptr, TSTRING, "INSNAME", name, comment, pStatus);
    strncpy(pWave->insname, name, FLEN_VALUE-1);
  } else {
    strncpy(pWave->insname, insname, FLEN_VALUE-1);
  }

  /* get number of rows */
  fits_get_num_rows(fptr, &repeat, pStatus);
  pWave->nwave = repeat;
  pWave->eff_wave = malloc(pWave->nwave*sizeof(float));
  pWave->eff_band = malloc(pWave->nwave*sizeof(float));
  /* read columns */
  fits_get_colnum(fptr, CASEINSEN, "EFF_WAVE", &colnum, pStatus);
  fits_read_col(fptr, TFLOAT, colnum, 1, 1, pWave->nwave, &nullfloat,
		pWave->eff_wave, &anynull, pStatus);
  fits_get_colnum(fptr, CASEINSEN, "EFF_BAND", &colnum, pStatus);
  fits_read_col(fptr, TFLOAT, colnum, 1, 1, pWave->nwave, &nullfloat,
		pWave->eff_band, &anynull, pStatus);
  return *pStatus;
}


/**
 * Read OI_CORR fits binary table at current HDU.
 *
 *   @param fptr      see cfitsio documentation
 *   @param pCorr     ptr to corr data struct, see exchange.h
 *   @param corrname  value of CORRNAME keyword if known, else NULL
 *   @param pStatus   pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of corr data struct are undefined
 */
static STATUS read_oi_corr_chdu(fitsfile *fptr, oi_corr *pCorr,
                                char *corrname, STATUS *pStatus)
{
  char comment[FLEN_COMMENT], name[FLEN_VALUE];
  int nullint = 0;
  double nulldouble = 0.0;
  const int revision = 1;
  int colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pCorr->revision, comment, pStatus);
  if (pCorr->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword in "
           "OI_CORR table. Got %d\n", revision, pCorr->revision);
  }
  if (corrname == NULL) {
    fits_read_key(fptr, TSTRING, "CORRNAME", name, comment, pStatus);
    strncpy(pCorr->corrname, name, FLEN_VALUE-1);
  } else {
    strncpy(pCorr->corrname, corrname, FLEN_VALUE-1);
  }
  fits_read_key(fptr, TINT, "NDATA", &pCorr->ndata, comment, pStatus);

  /* get number of rows */
  fits_get_num_rows(fptr, &repeat, pStatus);
  pCorr->ncorr = repeat;
  pCorr->iindx = malloc(pCorr->ncorr*sizeof(int));
  pCorr->jindx = malloc(pCorr->ncorr*sizeof(int));
  pCorr->corr = malloc(pCorr->ncorr*sizeof(double));
  /* read columns */
  fits_get_colnum(fptr, CASEINSEN, "IINDX", &colnum, pStatus);
  fits_read_col(fptr, TINT, colnum, 1, 1, pCorr->ncorr, &nullint,
                pCorr->iindx, &anynull, pStatus);
  fits_get_colnum(fptr, CASEINSEN, "JINDX", &colnum, pStatus);
  fits_read_col(fptr, TINT, colnum, 1, 1, pCorr->ncorr, &nullint,
                pCorr->jindx, &anynull, pStatus);
  fits_get_colnum(fptr, CASEINSEN, "CORR", &colnum, pStatus);
  fits_read_col(fptr, TDOUBLE, colnum, 1, 1, pCorr->ncorr, &nulldouble,
		pCorr->corr, &anynull, pStatus);
  return *pStatus;
}


/**
 * Read OI_POLAR fits binary table at current HDU.
 *
 *   @param fptr     see cfitsio documentation
 *   @param pPolar   ptr to polar struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of polar data struct are undefined
 */
static STATUS read_oi_polar_chdu(fitsfile *fptr, oi_polar *pPolar,
                                 STATUS *pStatus)
{
  char comment[FLEN_COMMENT];
  char *p;
  char nullstring[] = "NULL";
  int nullint = 0;
  double nulldouble = 0.0;
  float complex nullcomplex = 0.0 + 0.0*I;
  const int revision = 1;
  int irow, colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pPolar->revision, comment, pStatus);
  if (pPolar->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword in "
           "OI_POLAR table. Got %d\n", revision, pPolar->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pPolar->date_obs, comment, pStatus);
  fits_read_key(fptr, TINT, "NPOL", &pPolar->npol, comment, pStatus);
  /* note ARRNAME is mandatory */
  fits_read_key(fptr, TSTRING, "ARRNAME", pPolar->arrname, comment, pStatus);
  fits_read_key(fptr, TSTRING, "ORIENT", pPolar->orient, comment, pStatus);
  fits_read_key(fptr, TSTRING, "MODEL", pPolar->model, comment, pStatus);
  /* get number of rows */
  fits_get_num_rows(fptr, &pPolar->numrec, pStatus);
  pPolar->record = malloc(pPolar->numrec*sizeof(oi_polar_record));
  /* get value for nwave */
  /* format specifies same repeat count for L* columns */
  fits_get_colnum(fptr, CASEINSEN, "LXX", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  pPolar->nwave = repeat;
  /* read rows */
  for (irow=1; irow<=pPolar->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
		  &pPolar->record[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INSNAME", &colnum, pStatus);
    p = pPolar->record[irow-1].insname;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, nullstring, &p,
		  &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pPolar->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pPolar->record[irow-1].int_time, &anynull, pStatus);
    pPolar->record[irow-1].lxx = malloc(pPolar->nwave*sizeof(float complex));
    pPolar->record[irow-1].lyy = malloc(pPolar->nwave*sizeof(float complex));
    pPolar->record[irow-1].lxy = malloc(pPolar->nwave*sizeof(float complex));
    pPolar->record[irow-1].lyx = malloc(pPolar->nwave*sizeof(float complex));
    fits_get_colnum(fptr, CASEINSEN, "LXX", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pPolar->nwave,
		  &nullcomplex, pPolar->record[irow-1].lxx, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "LYY", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pPolar->nwave,
		  &nullcomplex, pPolar->record[irow-1].lyy, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "LXY", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pPolar->nwave,
		  &nullcomplex, pPolar->record[irow-1].lxy, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "LYX", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pPolar->nwave,
		  &nullcomplex, pPolar->record[irow-1].lyx, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 2, &nullint,
		  pPolar->record[irow-1].sta_index, &anynull, pStatus);
  }
  return *pStatus;
}


/*
 * Public functions
 */

/**
 * Read OIFITS primary header keywords.
 *
 * Moves to primary HDU.
 *
 *   @param fptr     see cfitsio documentation
 *   @param pHeader  ptr to header data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of header data struct are undefined
 */
STATUS read_oi_header(fitsfile *fptr, oi_header *pHeader, STATUS *pStatus)
{
  const char function[] = "read_oi_header";

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Move to primary HDU */
  fits_movabs_hdu(fptr, 1, NULL, pStatus);

  /* Note all header keywords (except SIMPLE etc.) are optional in OIFITS v1 */
  read_key_opt_string(fptr, "ORIGIN", pHeader->origin, pStatus);
  read_key_opt_string(fptr, "DATE-OBS", pHeader->date_obs, pStatus);
  read_key_opt_string(fptr, "TELESCOP", pHeader->telescop, pStatus);
  read_key_opt_string(fptr, "INSTRUME", pHeader->instrume, pStatus);
  read_key_opt_string(fptr, "INSMODE", pHeader->insmode, pStatus);
  read_key_opt_string(fptr, "OBJECT", pHeader->object, pStatus);

  read_key_opt_string(fptr, "REFERENC", pHeader->referenc, pStatus);
  read_key_opt_string(fptr, "PROG_ID", pHeader->prog_id, pStatus);
  read_key_opt_string(fptr, "PROCSOFT", pHeader->procsoft, pStatus);
  read_key_opt_string(fptr, "OBSTECH", pHeader->obstech, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/**
 * Read OI_TARGET fits binary table. Moves to first matching HDU
 *
 *   @param fptr      see cfitsio documentation
 *   @param pTargets  ptr to targets data struct, see exchange.h
 *   @param pStatus   pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of targets data struct are undefined
 */
STATUS read_oi_target(fitsfile *fptr, oi_target *pTargets, STATUS *pStatus)
{
  const char function[] = "read_oi_target";
  char comment[FLEN_COMMENT];
  char *p;
  char nullstring[] = "NULL";
  int nullint = 0;
  float nullfloat = 0.0F;
  double nulldouble = 0.0;
  const int revision = 1;
  int irow, colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  fits_movnam_hdu(fptr, BINARY_TBL, "OI_TARGET", 0, pStatus);
  fits_read_key(fptr, TINT, "OI_REVN", &pTargets->revision, comment, pStatus);
  if (pTargets->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword in "
           "OI_TARGETS table. Got %d\n", revision, pTargets->revision);
  }
  /* get number of rows */
  fits_get_num_rows(fptr, &repeat, pStatus);
  pTargets->ntarget = repeat;
  pTargets->targ = malloc(pTargets->ntarget*sizeof(target));
  /* read rows */
  for (irow=1; irow<=pTargets->ntarget; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
		  &pTargets->targ[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "TARGET", &colnum, pStatus);
    p = pTargets->targ[irow-1].target;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, nullstring, &p,
		  &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "RAEP0", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].raep0, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "DECEP0", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].decep0, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "EQUINOX", &colnum, pStatus);
    fits_read_col(fptr, TFLOAT, colnum, irow, 1, 1, &nullfloat,
		  &pTargets->targ[irow-1].equinox, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "RA_ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].ra_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "DEC_ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].dec_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "SYSVEL", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].sysvel, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VELTYP", &colnum, pStatus);
    p = pTargets->targ[irow-1].veltyp;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, nullstring, &p,
		  &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VELDEF", &colnum, pStatus);
    p = pTargets->targ[irow-1].veldef;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, nullstring, &p,
		  &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PMRA", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].pmra, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PMDEC", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].pmdec, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PMRA_ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].pmra_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PMDEC_ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pTargets->targ[irow-1].pmdec_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PARALLAX", &colnum, pStatus);
    fits_read_col(fptr, TFLOAT, colnum, irow, 1, 1, &nullfloat,
		  &pTargets->targ[irow-1].parallax, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PARA_ERR", &colnum, pStatus);
    fits_read_col(fptr, TFLOAT, colnum, irow, 1, 1, &nullfloat,
		  &pTargets->targ[irow-1].para_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "SPECTYP", &colnum, pStatus);
    p = pTargets->targ[irow-1].spectyp;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, nullstring, &p,
		  &anynull, pStatus);
    /*printf("%16s  %10f %10f  %8s\n",
	   pTargets->targ[irow-1].target,
	   pTargets->targ[irow-1].raep0, pTargets->targ[irow-1].decep0,
	   pTargets->targ[irow-1].spectyp);*/
  }

  /* Read optional column */
  fits_get_colnum(fptr, CASEINSEN, "CATEGORY", &colnum, pStatus);
  if(*pStatus == COL_NOT_FOUND) {
    pTargets->usecategory = FALSE;
    *pStatus = 0;
    //:TODO: set to -1?
  } else {
    pTargets->usecategory = TRUE;
    for (irow=1; irow<=pTargets->ntarget; irow++) {
      p = pTargets->targ[irow-1].category;
      fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, nullstring, &p,
                    &anynull, pStatus);
    }
  }

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Read OI_ARRAY fits binary table with specified ARRNAME
 *
 *   @param fptr     see cfitsio documentation
 *   @param arrname  read table with this value for ARRNAME
 *   @param pArray   ptr to array data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of array data struct are undefined
 */
STATUS read_oi_array(fitsfile *fptr, char *arrname, oi_array *pArray,
                     STATUS *pStatus)
{
  const char function[] = "read_oi_array";

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  specific_named_hdu(fptr, "OI_ARRAY", "ARRNAME", arrname, pStatus);
  read_oi_array_chdu(fptr, pArray, arrname, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/**
 * Read next OI_ARRAY fits binary table
 *
 *   @param fptr     see cfitsio documentation
 *   @param pArray   ptr to array data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of data struct are undefined
 */
STATUS read_next_oi_array(fitsfile *fptr, oi_array *pArray, STATUS *pStatus)
{
  const char function[] = "read_next_oi_array";

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_ARRAY", pStatus);
  if (*pStatus == END_OF_FILE) return *pStatus;
  read_oi_array_chdu(fptr, pArray, NULL, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Read OI_WAVELENGTH fits binary table with specified INSNAME
 *
 *   @param fptr     see cfitsio documentation
 *   @param insname  read table with this value for INSNAME
 *   @param pWave    ptr to wavelength data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of wavelength data struct are undefined
 */
STATUS read_oi_wavelength(fitsfile *fptr, char *insname, oi_wavelength *pWave,
                          STATUS *pStatus)
{
  const char function[] = "read_oi_wavelength";

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  specific_named_hdu(fptr, "OI_WAVELENGTH", "INSNAME", insname, pStatus);
  read_oi_wavelength_chdu(fptr, pWave, insname, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/**
 * Read next OI_WAVELENGTH fits binary table
 *
 *   @param fptr     see cfitsio documentation
 *   @param pWave    ptr to wavelength data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of data struct are undefined
 */
STATUS read_next_oi_wavelength(fitsfile *fptr, oi_wavelength *pWave,
                               STATUS *pStatus)
{
  const char function[] = "read_next_oi_wavelength";

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_WAVELENGTH", pStatus);
  if (*pStatus == END_OF_FILE) return *pStatus;
  read_oi_wavelength_chdu(fptr, pWave, NULL, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Read OI_CORR fits binary table with specified CORRNAME
 *
 *   @param fptr      see cfitsio documentation
 *   @param corrname  read table with this value for CORRNAME
 *   @param pCorr     ptr to corr data struct, see exchange.h
 *   @param pStatus   pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of corr data struct are undefined
 */
STATUS read_oi_corr(fitsfile *fptr, char *corrname, oi_corr *pCorr,
                    STATUS *pStatus)
{
  const char function[] = "read_oi_corr";

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  specific_named_hdu(fptr, "OI_CORR", "CORRNAME", corrname, pStatus);
  read_oi_corr_chdu(fptr, pCorr, corrname, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/**
 * Read next OI_CORR fits binary table
 *
 *   @param fptr     see cfitsio documentation
 *   @param pCorr    ptr to corr data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of data struct are undefined
 */
STATUS read_next_oi_corr(fitsfile *fptr, oi_corr *pCorr, STATUS *pStatus)
{
  const char function[] = "read_next_oi_corr";

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_CORR", pStatus);
  if (*pStatus == END_OF_FILE) return *pStatus;
  read_oi_corr_chdu(fptr, pCorr, NULL, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Read next OI_POLAR fits binary table
 *
 *   @param fptr     see cfitsio documentation
 *   @param pPolar   ptr to polar data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of polar data struct are undefined
 */
STATUS read_next_oi_polar(fitsfile *fptr, oi_polar *pPolar, STATUS *pStatus)
{
  const char function[] = "read_next_oi_polar";

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_POLAR", pStatus);
  if (*pStatus == END_OF_FILE) return *pStatus;
  read_oi_polar_chdu(fptr, pPolar, pStatus);

  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Read OI_VIS optional columns for complex visibility representation
 */
static STATUS read_oi_vis_complex(fitsfile *fptr, oi_vis *pVis,
                                  bool correlated, STATUS *pStatus)
{
  char comment[FLEN_COMMENT];
  char keyval[FLEN_VALUE];
  int nullint = 0;
  double nulldouble = 0.0;
  int irow, colnum, anynull;

  fits_get_colnum(fptr, CASEINSEN, "RVIS", &colnum, pStatus);
  if (*pStatus == COL_NOT_FOUND) {
    pVis->usecomplex = FALSE;
    *pStatus = 0;
    pVis->complexunit[0] = '\0';
    for (irow=1; irow<=pVis->numrec; irow++) {
      pVis->record[irow-1].rvis = NULL;
      pVis->record[irow-1].rviserr = NULL;
      pVis->record[irow-1].ivis = NULL;
      pVis->record[irow-1].iviserr = NULL;
    }
  } else {
    pVis->usecomplex = TRUE;
    /* read unit (mandatory if RVIS present) */
    fits_get_colnum(fptr, CASEINSEN, "RVIS", &colnum, pStatus);
    snprintf(keyval, FLEN_VALUE, "TUNIT%d", colnum);
    fits_read_key(fptr, TSTRING, keyval, pVis->complexunit, comment, pStatus);

    for (irow=1; irow<=pVis->numrec; irow++) {
      pVis->record[irow-1].rvis = malloc(pVis->nwave*sizeof(DATA));
      pVis->record[irow-1].rviserr = malloc(pVis->nwave*sizeof(DATA));
      pVis->record[irow-1].ivis = malloc(pVis->nwave*sizeof(DATA));
      pVis->record[irow-1].iviserr = malloc(pVis->nwave*sizeof(DATA));
      fits_get_colnum(fptr, CASEINSEN, "RVIS", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave,
                    &nulldouble, pVis->record[irow-1].rvis, &anynull,
                    pStatus);
      fits_get_colnum(fptr, CASEINSEN, "RVISERR", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave,
                    &nulldouble, pVis->record[irow-1].rviserr, &anynull,
                    pStatus);
      fits_get_colnum(fptr, CASEINSEN, "IVIS", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave,
                    &nulldouble, pVis->record[irow-1].ivis, &anynull,
                    pStatus);
      fits_get_colnum(fptr, CASEINSEN, "IVISERR", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave,
                    &nulldouble, pVis->record[irow-1].iviserr, &anynull,
                    pStatus);
      if (correlated) {
        fits_get_colnum(fptr, CASEINSEN, "CORRINDX_RVIS", &colnum, pStatus);
        fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
                      &pVis->record[irow-1].corrindx_rvis,
                      &anynull, pStatus);
        fits_get_colnum(fptr, CASEINSEN, "CORRINDX_IVIS", &colnum, pStatus);
        fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
                      &pVis->record[irow-1].corrindx_ivis,
                      &anynull, pStatus);
      }
    }
  }
  return *pStatus;
}

/**
 * Read OI_VIS optional content
 */
static STATUS read_oi_vis_opt(fitsfile *fptr, oi_vis *pVis, STATUS *pStatus)
{
  char comment[FLEN_COMMENT];
  char nullchar = 0;
  int nullint = 0;
  int irow, colnum, anynull;
  bool correlated;

  /* Read optional keywords */
  fits_read_key(fptr, TSTRING, "CORRNAME", pVis->corrname, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* CORRNAME is optional */
    pVis->corrname[0] = '\0';
    *pStatus = 0;
    correlated = FALSE;
  } else {
    correlated = TRUE;
  }
  fits_read_key(fptr, TSTRING, "AMPTYP", pVis->amptyp, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* AMPTYP is optional */
    pVis->amptyp[0] = '\0';
    *pStatus = 0;
  }
  fits_read_key(fptr, TSTRING, "PHITYP", pVis->phityp, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* PHITYP is optional */
    pVis->phityp[0] = '\0';
    *pStatus = 0;
  }
  fits_read_key(fptr, TINT, "AMPORDER", &pVis->amporder, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* AMPORDER is optional */
    pVis->amporder = -1;
    *pStatus = 0;
  }
  fits_read_key(fptr, TINT, "PHIORDER", &pVis->phiorder, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* PHIORDER is optional */
    pVis->phiorder = -1;
    *pStatus = 0;
  }

  /* Read optional columns */
  if (correlated) {
    for (irow=1; irow<=pVis->numrec; irow++) {
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_VISAMP", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
                    &pVis->record[irow-1].corrindx_visamp,
                    &anynull, pStatus);
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_VISPHI", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
                    &pVis->record[irow-1].corrindx_visphi,
                    &anynull, pStatus);
    }
  }
  fits_get_colnum(fptr, CASEINSEN, "VISREFMAP", &colnum, pStatus);
  if (*pStatus == COL_NOT_FOUND) {
    pVis->usevisrefmap = FALSE;
    *pStatus = 0;
    for (irow=1; irow<=pVis->numrec; irow++) {
      pVis->record[irow-1].visrefmap = NULL;
    }
  } else {
    pVis->usevisrefmap = TRUE;
    for (irow=1; irow<=pVis->numrec; irow++) {
      pVis->record[irow-1].visrefmap = malloc(pVis->nwave*pVis->nwave*
                                              sizeof(BOOL));
      fits_read_col(fptr, TLOGICAL, colnum, irow, 1, pVis->nwave*pVis->nwave,
                    &nullchar, pVis->record[irow-1].visrefmap,
                    &anynull, pStatus);
    }
  }
  read_oi_vis_complex(fptr, pVis, correlated, pStatus);

  return *pStatus;
}

/**
 * Read next OI_VIS fits binary table
 *
 *   @param fptr     see cfitsio documentation
 *   @param pVis     ptr to data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of data struct are undefined
 */
STATUS read_next_oi_vis(fitsfile *fptr, oi_vis *pVis, STATUS *pStatus)
{
  const char function[] = "read_next_oi_vis";
  char comment[FLEN_COMMENT];
  char keyval[FLEN_VALUE];
  char nullchar = 0;
  int nullint = 0;
  double nulldouble = 0.0;
  const int revision = 1;
  int irow, colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_VIS", pStatus);
  if (*pStatus == END_OF_FILE)
    return *pStatus;
  else if (*pStatus)
    goto except;

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pVis->revision, comment, pStatus);
  if (pVis->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword in "
           "OI_VIS table. Got %d\n", revision, pVis->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pVis->date_obs, comment, pStatus);
  fits_read_key(fptr, TSTRING, "ARRNAME", pVis->arrname, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* ARRNAME is optional */
    pVis->arrname[0] = '\0';
    *pStatus = 0;
  }
  fits_read_key(fptr, TSTRING, "INSNAME", pVis->insname, comment, pStatus);
  /* get number of rows */
  fits_get_num_rows(fptr, &pVis->numrec, pStatus);
  pVis->record = malloc(pVis->numrec*sizeof(oi_vis_record));
  /* get value for nwave */
  /* format specifies same repeat count for VIS* columns */
  fits_get_colnum(fptr, CASEINSEN, "VISAMP", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  pVis->nwave = repeat;
  /* read VISAMP unit (optional) */
  snprintf(keyval, FLEN_VALUE, "TUNIT%d", colnum);
  fits_read_key(fptr, TSTRING, keyval, pVis->ampunit, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) {
    pVis->ampunit[0] = '\0';
    *pStatus = 0;
  }
  /* read rows */
  for (irow=1; irow<=pVis->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
		  &pVis->record[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis->record[irow-1].time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis->record[irow-1].int_time, &anynull, pStatus);
    pVis->record[irow-1].visamp = malloc(pVis->nwave*sizeof(DATA));
    pVis->record[irow-1].visamperr = malloc(pVis->nwave*sizeof(DATA));
    pVis->record[irow-1].visphi = malloc(pVis->nwave*sizeof(DATA));
    pVis->record[irow-1].visphierr = malloc(pVis->nwave*sizeof(DATA));
    pVis->record[irow-1].flag = malloc(pVis->nwave*sizeof(BOOL));
    fits_get_colnum(fptr, CASEINSEN, "VISAMP", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave,
		  &nulldouble, pVis->record[irow-1].visamp, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VISAMPERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave,
		  &nulldouble, pVis->record[irow-1].visamperr, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VISPHI", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave,
		  &nulldouble, pVis->record[irow-1].visphi, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VISPHIERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave,
		  &nulldouble, pVis->record[irow-1].visphierr, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "UCOORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis->record[irow-1].ucoord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VCOORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis->record[irow-1].vcoord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 2, &nullint,
		  pVis->record[irow-1].sta_index, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLAG", &colnum, pStatus);
    fits_read_col(fptr, TLOGICAL, colnum, irow, 1, pVis->nwave, &nullchar,
		  pVis->record[irow-1].flag, &anynull, pStatus);
  }
  read_oi_vis_opt(fptr, pVis, pStatus);

 except:
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Read next OI_VIS2 fits binary table
 *
 *   @param fptr     see cfitsio documentation
 *   @param pVis2    ptr to data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of data struct are undefined
 */
STATUS read_next_oi_vis2(fitsfile *fptr, oi_vis2 *pVis2, STATUS *pStatus)
{
  const char function[] = "read_next_oi_vis2";
  char comment[FLEN_COMMENT];
  bool correlated;
  char nullchar = 0;
  int nullint = 0;
  double nulldouble = 0.0;
  const int revision = 1;
  int irow, colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_VIS2", pStatus);
  if (*pStatus == END_OF_FILE)
    return *pStatus;
  else if (*pStatus)
    goto except;

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pVis2->revision, comment, pStatus);
  if (pVis2->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword in "
           "OI_VIS2 table. Got %d\n", revision, pVis2->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pVis2->date_obs, comment, pStatus);
  fits_read_key(fptr, TSTRING, "ARRNAME", pVis2->arrname, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* ARRNAME is optional */
    pVis2->arrname[0] = '\0';
    *pStatus = 0;
  }
  fits_read_key(fptr, TSTRING, "INSNAME", pVis2->insname, comment, pStatus);
  fits_read_key(fptr, TSTRING, "CORRNAME", pVis2->corrname, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* CORRNAME is optional */
    pVis2->corrname[0] = '\0';
    *pStatus = 0;
    correlated = FALSE;
  } else {
    correlated = TRUE;
  }
  /* get number of rows */
  fits_get_num_rows(fptr, &pVis2->numrec, pStatus);
  pVis2->record = malloc(pVis2->numrec*sizeof(oi_vis2_record));
  /* get value for nwave */
  /* format specifies same repeat count for VIS2DATA & VIS2ERR columns */
  fits_get_colnum(fptr, CASEINSEN, "VIS2DATA", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  pVis2->nwave = repeat;
  /* read rows */
  for (irow=1; irow<=pVis2->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
		  &pVis2->record[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis2->record[irow-1].time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis2->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis2->record[irow-1].int_time, &anynull, pStatus);
    pVis2->record[irow-1].vis2data = malloc(pVis2->nwave*sizeof(DATA));
    pVis2->record[irow-1].vis2err = malloc(pVis2->nwave*sizeof(DATA));
    pVis2->record[irow-1].flag = malloc(pVis2->nwave*sizeof(BOOL));
    fits_get_colnum(fptr, CASEINSEN, "VIS2DATA", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis2->nwave,
		  &nulldouble, pVis2->record[irow-1].vis2data, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VIS2ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis2->nwave,
		  &nulldouble, pVis2->record[irow-1].vis2err, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "UCOORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis2->record[irow-1].ucoord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VCOORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pVis2->record[irow-1].vcoord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 2, &nullint,
		  pVis2->record[irow-1].sta_index, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLAG", &colnum, pStatus);
    fits_read_col(fptr, TLOGICAL, colnum, irow, 1, pVis2->nwave, &nullchar,
		  pVis2->record[irow-1].flag, &anynull, pStatus);

    /* read optional columns */
    if (correlated) {
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_VIS2DATA", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
                    &pVis2->record[irow-1].corrindx_vis2data,
                    &anynull, pStatus);
    }
  }

 except:
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Read next OI_T3 fits binary table
 *
 *   @param fptr     see cfitsio documentation
 *   @param pT3      ptr to data struct, see exchange.h
 *   @param pStatus  pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of data struct are undefined
 */
STATUS read_next_oi_t3(fitsfile *fptr, oi_t3 *pT3, STATUS *pStatus)
{
  const char function[] = "read_next_oi_t3";
  char comment[FLEN_COMMENT];
  bool correlated;
  char nullchar = 0;
  int nullint = 0;
  double nulldouble = 0.0;
  const int revision = 1;
  int irow, colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_T3", pStatus);
  if (*pStatus == END_OF_FILE)
    return *pStatus;
  else if (*pStatus)
    goto except;

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pT3->revision, comment, pStatus);
  if (pT3->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword in "
           "OI_T3 table. Got %d\n", revision, pT3->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pT3->date_obs, comment, pStatus);
  fits_read_key(fptr, TSTRING, "ARRNAME", pT3->arrname, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* ARRNAME is optional */
    pT3->arrname[0] = '\0';
    *pStatus = 0;
  }
  fits_read_key(fptr, TSTRING, "INSNAME", pT3->insname, comment, pStatus);
  fits_read_key(fptr, TSTRING, "CORRNAME", pT3->corrname, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* CORRNAME is optional */
    pT3->corrname[0] = '\0';
    *pStatus = 0;
    correlated = FALSE;
  } else {
    correlated = TRUE;
  }
  /* get number of rows & allocate storage */
  fits_get_num_rows(fptr, &pT3->numrec, pStatus);
  pT3->record = malloc(pT3->numrec*sizeof(oi_t3_record));
  /* get value for nwave */
  /* format specifies same repeat count for T3* columns */
  fits_get_colnum(fptr, CASEINSEN, "T3AMP", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  pT3->nwave = repeat;
  /* read rows */
  for (irow=1; irow<=pT3->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
		  &pT3->record[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pT3->record[irow-1].time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pT3->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pT3->record[irow-1].int_time, &anynull, pStatus);
    pT3->record[irow-1].t3amp = malloc(pT3->nwave*sizeof(DATA));
    pT3->record[irow-1].t3amperr = malloc(pT3->nwave*sizeof(DATA));
    pT3->record[irow-1].t3phi = malloc(pT3->nwave*sizeof(DATA));
    pT3->record[irow-1].t3phierr = malloc(pT3->nwave*sizeof(DATA));
    pT3->record[irow-1].flag = malloc(pT3->nwave*sizeof(BOOL));
    fits_get_colnum(fptr, CASEINSEN, "T3AMP", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pT3->nwave,
		  &nulldouble, pT3->record[irow-1].t3amp, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "T3AMPERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pT3->nwave,
		  &nulldouble, pT3->record[irow-1].t3amperr, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "T3PHI", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pT3->nwave,
		  &nulldouble, pT3->record[irow-1].t3phi, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "T3PHIERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pT3->nwave,
		  &nulldouble, pT3->record[irow-1].t3phierr, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "U1COORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1,
		  &nulldouble, &pT3->record[irow-1].u1coord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "V1COORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pT3->record[irow-1].v1coord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "U2COORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pT3->record[irow-1].u2coord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "V2COORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pT3->record[irow-1].v2coord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 3, &nullint,
		  pT3->record[irow-1].sta_index, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLAG", &colnum, pStatus);
    fits_read_col(fptr, TLOGICAL, colnum, irow, 1, pT3->nwave, &nullchar,
		  pT3->record[irow-1].flag, &anynull, pStatus);

    /* read optional columns */
    if (correlated) {
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_T3AMP", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
                    &pT3->record[irow-1].corrindx_t3amp, &anynull, pStatus);
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_T3PHI", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
                    &pT3->record[irow-1].corrindx_t3phi, &anynull, pStatus);
     }
  }

 except:
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}


/**
 * Read next OI_SPECTRUM fits binary table
 *
 *   @param fptr       see cfitsio documentation
 *   @param pSpectrum  ptr to data struct, see exchange.h
 *   @param pStatus    pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of data struct are undefined
 */
STATUS read_next_oi_spectrum(fitsfile *fptr, oi_spectrum *pSpectrum,
                             STATUS *pStatus)
{
  const char function[] = "read_next_oi_spectrum";
  char comment[FLEN_COMMENT];
  bool correlated;
  char keyval[FLEN_VALUE];
  int nullint = 0;
  double nulldouble = 0.0;
  const int revision = 1;
  int irow, colnum, anynull;
  long repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_SPECTRUM", pStatus);
  if (*pStatus == END_OF_FILE)
    return *pStatus;
  else if (*pStatus)
    goto except;

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pSpectrum->revision, comment, pStatus);
  if (pSpectrum->revision != revision) {
    printf("WARNING! Expecting value %d for OI_REVN keyword in "
           "OI_SPECTRUM table. Got %d\n", revision, pSpectrum->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pSpectrum->date_obs,
                comment, pStatus);
  fits_read_key(fptr, TSTRING, "ARRNAME", pSpectrum->arrname, comment, pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* ARRNAME is optional */
    pSpectrum->arrname[0] = '\0';
    *pStatus = 0;
  }
  fits_read_key(fptr, TSTRING, "INSNAME", pSpectrum->insname, comment, pStatus);
  fits_read_key(fptr, TSTRING, "CORRNAME", pSpectrum->corrname, comment,
                pStatus);
  if (*pStatus == KEY_NO_EXIST) { /* CORRNAME is optional */
    pSpectrum->corrname[0] = '\0';
    *pStatus = 0;
    correlated = FALSE;
  } else {
    correlated = TRUE;
  }
  fits_read_key(fptr, TDOUBLE, "FOV", &pSpectrum->fov, comment, pStatus);
  fits_read_key(fptr, TSTRING, "FOVTYPE", pSpectrum->fovtype, comment, pStatus); //:BUG: buffer overrun
  fits_read_key(fptr, TSTRING, "CALSTAT", keyval, comment, pStatus);
  pSpectrum->calstat = keyval[0];
  /* get number of rows & allocate storage */
  fits_get_num_rows(fptr, &pSpectrum->numrec, pStatus);
  pSpectrum->record = malloc(pSpectrum->numrec*sizeof(oi_spectrum_record));
  /* get value for nwave */
  /* format specifies same repeat count for FLUX* columns */
  fits_get_colnum(fptr, CASEINSEN, "FLUXDATA", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  pSpectrum->nwave = repeat;
  /* read unit (mandatory) */
  snprintf(keyval, FLEN_VALUE, "TUNIT%d", colnum);
  fits_read_key(fptr, TSTRING, keyval, pSpectrum->fluxunit, comment, pStatus);
    
  /* read rows */
  for (irow=1; irow<=pSpectrum->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
		  &pSpectrum->record[irow-1].target_id, &anynull, pStatus);
    /* no TIME column */
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pSpectrum->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, &nulldouble,
		  &pSpectrum->record[irow-1].int_time, &anynull, pStatus);
    pSpectrum->record[irow-1].fluxdata = malloc(pSpectrum->nwave*sizeof(DATA));
    pSpectrum->record[irow-1].fluxerr = malloc(pSpectrum->nwave*sizeof(DATA));
    fits_get_colnum(fptr, CASEINSEN, "FLUXDATA", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pSpectrum->nwave,
		  &nulldouble, pSpectrum->record[irow-1].fluxdata, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLUXERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pSpectrum->nwave,
		  &nulldouble, pSpectrum->record[irow-1].fluxerr, &anynull,
		  pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
		  &pSpectrum->record[irow-1].sta_index, &anynull, pStatus);

    /* read optional columns */
    if (correlated) {
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_FLUXDATA", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, &nullint,
                    &pSpectrum->record[irow-1].corrindx_fluxdata,
                    &anynull, pStatus);
    }
  }

 except:
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}
