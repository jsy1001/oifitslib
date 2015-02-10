/**
 * @file
 * @ingroup oitable
 * Implementation of functions to read individual FITS tables and
 * write to data structures in memory.
 *
 * Copyright (C) 2007, 2015 John Young
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
 * Read optional string-valued header keyword.
 *
 * @return TRUE if keyword read successfully, FALSE otherwise.
 */
static bool read_key_opt_string(fitsfile *fptr, char *keyname,
                                char *keyval, STATUS *pStatus)
{
  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  fits_write_errmark();
  if (fits_read_key(fptr, TSTRING, keyname, keyval, NULL, pStatus)) {
    keyval[0] = '\0';
    if (*pStatus == KEY_NO_EXIST) {
      *pStatus = 0;
      fits_clear_errmark();
    }
    return FALSE;
  }
  return TRUE;
}

/**
 * Read optional integer-valued header keyword.
 *
 * @return TRUE if keyword read successfully, FALSE otherwise.
 */
static bool read_key_opt_int(fitsfile *fptr, char *keyname,
                             int *keyval, STATUS *pStatus)
{
  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  fits_write_errmark();
  if (fits_read_key(fptr, TINT, keyname, keyval, NULL, pStatus)) {
    *keyval = -1;
    if (*pStatus == KEY_NO_EXIST) {
      *pStatus = 0;
      fits_clear_errmark();
    }
    return FALSE;
  }
  return TRUE;
}

/**
 * Verify current HDU against CHECKSUM and DATASUM keywords.
 *
 * The checksum keyword convention is described at
 * http://fits.gsfc.nasa.gov/registry/checksum.html
 *
 * The function writes a message to stdout if either checksum is
 * incorrect. Any missing checksum keyword is silently ignored.
 */
static STATUS verify_chksum(fitsfile *fptr, STATUS *pStatus)
{
  int dataok, hduok;
  char extname[FLEN_VALUE];
  int hdunum, extver;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  fits_verify_chksum(fptr, &dataok, &hduok, pStatus);
  if (dataok == -1 || hduok == -1) {
    fits_get_hdu_num(fptr, &hdunum);
    if (!read_key_opt_string(fptr, "EXTNAME", extname, pStatus)) {
      extver = 0;
    } else {
      fits_read_key(fptr, TINT, "EXTVER", &extver, NULL, pStatus);
    }
    if (dataok == -1)
      printf("WARNING! Data checksum verification failed "
              "for HDU #%d (EXTNAME='%s' EXTVER=%d)\n",
              hdunum, extname, extver);
    if (hduok == -1)
      printf("WARNING! HDU checksum verification failed "
              "for HDU #%d (EXTNAME='%s' EXTVER=%d)\n",
              hdunum, extname, extver);
  }
  return *pStatus;
}

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
  char extname[FLEN_VALUE];
  int hdutype;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Move to correct HDU - don't assume anything about EXTVERs */
  while (1==1) {
    fits_movrel_hdu(fptr, 1, &hdutype, pStatus);
    if (*pStatus) return *pStatus; /* no more HDUs */
    if (hdutype == BINARY_TBL) {
      fits_write_errmark();
      fits_read_key(fptr, TSTRING, "EXTNAME", extname, NULL, pStatus);
      if (*pStatus == KEY_NO_EXIST) {
        printf("WARNING! Skipping binary table HDU with no EXTNAME\n");
        *pStatus = 0;
        fits_clear_errmark();
      } else if (*pStatus) {
        return *pStatus;
      } else if (strcmp(extname, reqName) == 0) {
        break; /* current HDU matches */
      }
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
  char extname[FLEN_VALUE], value[FLEN_VALUE];
  int ihdu, nhdu, hdutype;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Move to correct HDU - don't assume anything about EXTVERs */
  fits_get_num_hdus(fptr, &nhdu, pStatus);
  if (*pStatus) return *pStatus;
  for (ihdu=2; ihdu<=nhdu; ihdu++) {
    fits_movabs_hdu(fptr, ihdu, &hdutype, pStatus);
    if (*pStatus) return *pStatus;
    if (hdutype == BINARY_TBL) {
      fits_write_errmark();
      fits_read_key(fptr, TSTRING, "EXTNAME", extname, NULL, pStatus);
      fits_read_key(fptr, TSTRING, keyword, value, NULL, pStatus);
      if (*pStatus) {
	*pStatus = 0;
        fits_clear_errmark();
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
  char name[FLEN_VALUE];
  char *p;
  double nan;
  const int revision = 2;
  int irow, colnum, anynull;
  long nrows;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Make a NaN */
  nan = 0.0;
  nan /= nan;

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pArray->revision, NULL, pStatus);
  if (*pStatus) return *pStatus;
  if (pArray->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_ARRAY table. Got %d\n",
           revision, pArray->revision);
  }
  if (arrname == NULL) {
    fits_read_key(fptr, TSTRING, "ARRNAME", name, NULL, pStatus);
    strncpy(pArray->arrname, name, FLEN_VALUE);
  } else {
    strncpy(pArray->arrname, arrname, FLEN_VALUE);
  }
  fits_read_key(fptr, TSTRING, "FRAME", pArray->frame, NULL, pStatus);
  fits_read_key(fptr, TDOUBLE, "ARRAYX", &pArray->arrayx, NULL, pStatus);
  fits_read_key(fptr, TDOUBLE, "ARRAYY", &pArray->arrayy, NULL, pStatus);
  fits_read_key(fptr, TDOUBLE, "ARRAYZ", &pArray->arrayz, NULL, pStatus);
  /* get number of rows and allocate storage */
  fits_get_num_rows(fptr, &nrows, pStatus);
  if (*pStatus) return *pStatus;
  alloc_oi_array(pArray, nrows);
  /* read rows */
  for (irow=1; irow<=pArray->nelement; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TEL_NAME", &colnum, pStatus);
    p = pArray->elem[irow-1].tel_name;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                  &p, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_NAME", &colnum, pStatus);
    p = pArray->elem[irow-1].sta_name;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                  &p, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
		  &pArray->elem[irow-1].sta_index, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "DIAMETER", &colnum, pStatus);
    fits_read_col(fptr, TFLOAT, colnum, irow, 1, 1, NULL,
		  &pArray->elem[irow-1].diameter, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STAXYZ", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 3, NULL,
		  &pArray->elem[irow-1].staxyz, &anynull, pStatus);
    if (pArray->revision >= 2) {
      fits_get_colnum(fptr, CASEINSEN, "FOV", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
                    &pArray->elem[irow-1].fov, &anynull, pStatus);
      fits_get_colnum(fptr, CASEINSEN, "FOVTYPE", &colnum, pStatus);
      p = pArray->elem[irow-1].fovtype;
      fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                    &p, &anynull, pStatus);
    } else {
      pArray->elem[irow-1].fov = nan;
      strncpy(pArray->elem[irow-1].fovtype, "FWHM", 7);
    }
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
  char name[FLEN_VALUE];
  const int revision = 2;
  int colnum, anynull;
  long nrows;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pWave->revision, NULL, pStatus);
  if (*pStatus) return *pStatus;
  if (pWave->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_WAVELENGTH table. Got %d\n",
           revision, pWave->revision);
  }
  if (insname == NULL) {
    fits_read_key(fptr, TSTRING, "INSNAME", name, NULL, pStatus);
    strncpy(pWave->insname, name, FLEN_VALUE);
  } else {
    strncpy(pWave->insname, insname, FLEN_VALUE);
  }

  /* get number of rows */
  fits_get_num_rows(fptr, &nrows, pStatus);
  if (*pStatus) return *pStatus;
  alloc_oi_wavelength(pWave, nrows);
  /* read columns */
  fits_get_colnum(fptr, CASEINSEN, "EFF_WAVE", &colnum, pStatus);
  fits_read_col(fptr, TFLOAT, colnum, 1, 1, pWave->nwave, NULL,
		pWave->eff_wave, &anynull, pStatus);
  fits_get_colnum(fptr, CASEINSEN, "EFF_BAND", &colnum, pStatus);
  fits_read_col(fptr, TFLOAT, colnum, 1, 1, pWave->nwave, NULL,
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
  char name[FLEN_VALUE];
  const int revision = 1;
  int colnum, anynull;
  long nrows;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pCorr->revision, NULL, pStatus);
  if (*pStatus) return *pStatus;
  if (pCorr->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_CORR table. Got %d\n",
           revision, pCorr->revision);
  }
  if (corrname == NULL) {
    fits_read_key(fptr, TSTRING, "CORRNAME", name, NULL, pStatus);
    strncpy(pCorr->corrname, name, FLEN_VALUE);
  } else {
    strncpy(pCorr->corrname, corrname, FLEN_VALUE);
  }
  fits_read_key(fptr, TINT, "NDATA", &pCorr->ndata, NULL, pStatus);

  /* get number of rows and allocate storage */
  fits_get_num_rows(fptr, &nrows, pStatus);
  if (*pStatus) return *pStatus;
  alloc_oi_corr(pCorr, nrows);
  /* read columns */
  fits_get_colnum(fptr, CASEINSEN, "IINDX", &colnum, pStatus);
  fits_read_col(fptr, TINT, colnum, 1, 1, pCorr->ncorr, NULL,
                pCorr->iindx, &anynull, pStatus);
  fits_get_colnum(fptr, CASEINSEN, "JINDX", &colnum, pStatus);
  fits_read_col(fptr, TINT, colnum, 1, 1, pCorr->ncorr, NULL,
                pCorr->jindx, &anynull, pStatus);
  fits_get_colnum(fptr, CASEINSEN, "CORR", &colnum, pStatus);
  fits_read_col(fptr, TDOUBLE, colnum, 1, 1, pCorr->ncorr, NULL,
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
  char *p;
  const int revision = 1;
  int irow, colnum, anynull;
  long nrows, repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pPolar->revision, NULL, pStatus);
  if (*pStatus) return *pStatus;
  if (pPolar->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_POLAR table. Got %d\n",
           revision, pPolar->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pPolar->date_obs, NULL, pStatus);
  fits_read_key(fptr, TINT, "NPOL", &pPolar->npol, NULL, pStatus);
  /* note ARRNAME is mandatory */
  fits_read_key(fptr, TSTRING, "ARRNAME", pPolar->arrname, NULL, pStatus);
  fits_read_key(fptr, TSTRING, "ORIENT", pPolar->orient, NULL, pStatus);
  fits_read_key(fptr, TSTRING, "MODEL", pPolar->model, NULL, pStatus);
  /* get dimensions and allocate storage */
  fits_get_num_rows(fptr, &nrows, pStatus);
  /* note format specifies same repeat count for L* columns = nwave */
  fits_get_colnum(fptr, CASEINSEN, "LXX", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  if (*pStatus) return *pStatus;
  alloc_oi_polar(pPolar, nrows, repeat);
  /* read rows */
  for (irow=1; irow<=pPolar->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
		  &pPolar->record[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INSNAME", &colnum, pStatus);
    p = pPolar->record[irow-1].insname;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                  &p, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pPolar->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pPolar->record[irow-1].int_time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "LXX", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pPolar->nwave, NULL,
		  pPolar->record[irow-1].lxx, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "LYY", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pPolar->nwave, NULL,
		  pPolar->record[irow-1].lyy, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "LXY", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pPolar->nwave,
		  NULL, pPolar->record[irow-1].lxy, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "LYX", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pPolar->nwave, NULL,
		  pPolar->record[irow-1].lyx, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
		  &pPolar->record[irow-1].sta_index, &anynull, pStatus);
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
  verify_chksum(fptr, pStatus);

  /* Note all header keywords (except SIMPLE etc.) are optional in OIFITS v1 */
  read_key_opt_string(fptr, "ORIGIN", pHeader->origin, pStatus);
  read_key_opt_string(fptr, "DATE-OBS", pHeader->date_obs, pStatus);
  read_key_opt_string(fptr, "TELESCOP", pHeader->telescop, pStatus);
  read_key_opt_string(fptr, "INSTRUME", pHeader->instrume, pStatus);
  read_key_opt_string(fptr, "OBSERVER", pHeader->observer, pStatus);
  read_key_opt_string(fptr, "INSMODE", pHeader->insmode, pStatus);
  read_key_opt_string(fptr, "OBJECT", pHeader->object, pStatus);

  read_key_opt_string(fptr, "REFERENC", pHeader->referenc, pStatus);
  read_key_opt_string(fptr, "AUTHOR", pHeader->author, pStatus);
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
  char *p;
  const int revision = 2;
  int irow, colnum, anynull;
  long nrows;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  fits_movnam_hdu(fptr, BINARY_TBL, "OI_TARGET", 0, pStatus);
  verify_chksum(fptr, pStatus);
  fits_read_key(fptr, TINT, "OI_REVN", &pTargets->revision, NULL, pStatus);
  if (*pStatus) goto except;
  if (pTargets->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_TARGET table. Got %d\n",
           revision, pTargets->revision);
  }
  /* get number of rows and allocate storage */
  fits_get_num_rows(fptr, &nrows, pStatus);
  if (*pStatus) goto except;
  alloc_oi_target(pTargets, nrows);
  /* read rows */
  for (irow=1; irow<=pTargets->ntarget; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "TARGET", &colnum, pStatus);
    p = pTargets->targ[irow-1].target;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                  &p, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "RAEP0", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].raep0, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "DECEP0", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].decep0, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "EQUINOX", &colnum, pStatus);
    fits_read_col(fptr, TFLOAT, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].equinox, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "RA_ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].ra_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "DEC_ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].dec_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "SYSVEL", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].sysvel, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VELTYP", &colnum, pStatus);
    p = pTargets->targ[irow-1].veltyp;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                  &p, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VELDEF", &colnum, pStatus);
    p = pTargets->targ[irow-1].veldef;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                  &p, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PMRA", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].pmra, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PMDEC", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].pmdec, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PMRA_ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].pmra_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PMDEC_ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].pmdec_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PARALLAX", &colnum, pStatus);
    fits_read_col(fptr, TFLOAT, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].parallax, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "PARA_ERR", &colnum, pStatus);
    fits_read_col(fptr, TFLOAT, colnum, irow, 1, 1, NULL,
		  &pTargets->targ[irow-1].para_err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "SPECTYP", &colnum, pStatus);
    p = pTargets->targ[irow-1].spectyp;
    fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                  &p, &anynull, pStatus);
    /*printf("%16s  %10f %10f  %8s\n",
	   pTargets->targ[irow-1].target,
	   pTargets->targ[irow-1].raep0, pTargets->targ[irow-1].decep0,
	   pTargets->targ[irow-1].spectyp);*/
  }

  /* Read optional column */
  pTargets->usecategory = FALSE;  /* default */
  if (pTargets->revision >= 2) {
    fits_write_errmark();
    fits_get_colnum(fptr, CASEINSEN, "CATEGORY", &colnum, pStatus);
    if(*pStatus == COL_NOT_FOUND) {
      *pStatus = 0;
      fits_clear_errmark();
    } else {
      pTargets->usecategory = TRUE;
      for (irow=1; irow<=pTargets->ntarget; irow++) {
        p = pTargets->targ[irow-1].category;
        fits_read_col(fptr, TSTRING, colnum, irow, 1, 1, NULL,
                      &p, &anynull, pStatus);
      }
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
  verify_chksum(fptr, pStatus);
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
  if (*pStatus == END_OF_FILE)
    return *pStatus;  /* don't report EOF to stderr */
  verify_chksum(fptr, pStatus);
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
  verify_chksum(fptr, pStatus);
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
  if (*pStatus == END_OF_FILE)
    return *pStatus;  /* don't report EOF to stderr */
  verify_chksum(fptr, pStatus);
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
  verify_chksum(fptr, pStatus);
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
  if (*pStatus == END_OF_FILE)
    return *pStatus;  /* don't report EOF to stderr */
  verify_chksum(fptr, pStatus);
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
  if (*pStatus == END_OF_FILE)
    return *pStatus;  /* don't report EOF to stderr */
  verify_chksum(fptr, pStatus);
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
  char keyword[FLEN_VALUE];
  int irow, colnum, anynull;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  fits_write_errmark();
  fits_get_colnum(fptr, CASEINSEN, "RVIS", &colnum, pStatus);
  if (*pStatus == COL_NOT_FOUND) {
    pVis->usecomplex = FALSE;
    pVis->complexunit[0] = '\0';
    for (irow=1; irow<=pVis->numrec; irow++) {
      pVis->record[irow-1].rvis = NULL;
      pVis->record[irow-1].rviserr = NULL;
      pVis->record[irow-1].ivis = NULL;
      pVis->record[irow-1].iviserr = NULL;
    }
    *pStatus = 0;
    fits_clear_errmark();
  } else {
    pVis->usecomplex = TRUE;
    /* read unit (mandatory if RVIS present) */
    fits_get_colnum(fptr, CASEINSEN, "RVIS", &colnum, pStatus);
    snprintf(keyword, FLEN_KEYWORD, "TUNIT%d", colnum);
    fits_read_key(fptr, TSTRING, keyword, pVis->complexunit, NULL, pStatus);

    for (irow=1; irow<=pVis->numrec; irow++) {
      pVis->record[irow-1].rvis = malloc(pVis->nwave *
                                         sizeof(pVis->record[0].rvis[0]));
      pVis->record[irow-1].rviserr = malloc(pVis->nwave *
                                            sizeof(pVis->record[0].rviserr[0]));
      pVis->record[irow-1].ivis = malloc(pVis->nwave *
                                         sizeof(pVis->record[0].ivis[0]));
      pVis->record[irow-1].iviserr = malloc(pVis->nwave *
                                            sizeof(pVis->record[0].iviserr[0]));
      fits_get_colnum(fptr, CASEINSEN, "RVIS", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave, NULL,
                    pVis->record[irow-1].rvis, &anynull, pStatus);
      fits_get_colnum(fptr, CASEINSEN, "RVISERR", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave, NULL,
                    pVis->record[irow-1].rviserr, &anynull, pStatus);
      fits_get_colnum(fptr, CASEINSEN, "IVIS", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave, NULL,
                    pVis->record[irow-1].ivis, &anynull, pStatus);
      fits_get_colnum(fptr, CASEINSEN, "IVISERR", &colnum, pStatus);
      fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave, NULL,
                    pVis->record[irow-1].iviserr, &anynull, pStatus);
      if (correlated) {
        fits_get_colnum(fptr, CASEINSEN, "CORRINDX_RVIS", &colnum, pStatus);
        fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
                      &pVis->record[irow-1].corrindx_rvis, &anynull, pStatus);
        fits_get_colnum(fptr, CASEINSEN, "CORRINDX_IVIS", &colnum, pStatus);
        fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
                      &pVis->record[irow-1].corrindx_ivis, &anynull, pStatus);
      }
    }
  }
  return *pStatus;
}

/**
 * Read OI_VIS optional content.
 */
static STATUS read_oi_vis_opt(fitsfile *fptr, oi_vis *pVis, STATUS *pStatus)
{
  int irow, colnum, anynull;
  bool correlated;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  if (pVis->revision == 1) {
    pVis->corrname[0] = '\0';
    pVis->amptyp[0] = '\0';
    pVis->phityp[0] = '\0';
    pVis->amporder = -1;
    pVis->phiorder = -1;
    pVis->usevisrefmap = FALSE;
    pVis->usecomplex = FALSE;
    return *pStatus;
  }

  /* Read optional keywords */
  correlated = read_key_opt_string(fptr, "CORRNAME", pVis->corrname, pStatus);
  read_key_opt_string(fptr, "AMPTYP", pVis->amptyp, pStatus);
  read_key_opt_string(fptr, "PHITYP", pVis->phityp, pStatus);
  read_key_opt_int(fptr, "AMPORDER", &pVis->amporder, pStatus);
  read_key_opt_int(fptr, "PHIORDER", &pVis->phiorder, pStatus);

  /* Read optional columns */
  if (correlated) {
    for (irow=1; irow<=pVis->numrec; irow++) {
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_VISAMP", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
                    &pVis->record[irow-1].corrindx_visamp, &anynull, pStatus);
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_VISPHI", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
                    &pVis->record[irow-1].corrindx_visphi, &anynull, pStatus);
    }
  }
  fits_write_errmark();
  fits_get_colnum(fptr, CASEINSEN, "VISREFMAP", &colnum, pStatus);
  if (*pStatus == COL_NOT_FOUND) {
    pVis->usevisrefmap = FALSE;
    for (irow=1; irow<=pVis->numrec; irow++) {
      pVis->record[irow-1].visrefmap = NULL;
    }
    *pStatus = 0;
    fits_clear_errmark();
  } else {
    pVis->usevisrefmap = TRUE;
    for (irow=1; irow<=pVis->numrec; irow++) {
      pVis->record[irow-1].visrefmap = malloc(pVis->nwave * pVis->nwave *
                                              sizeof(pVis->record[0].visrefmap[0]));
      fits_read_col(fptr, TLOGICAL, colnum, irow, 1, pVis->nwave*pVis->nwave,
                    NULL, pVis->record[irow-1].visrefmap, &anynull, pStatus);
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
  char keyword[FLEN_KEYWORD];
  const int revision = 2;
  int irow, colnum, anynull;
  long nrows, repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_VIS", pStatus);
  if (*pStatus == END_OF_FILE)
    return *pStatus;  /* don't report EOF to stderr */
  else if (*pStatus)
    goto except;
  verify_chksum(fptr, pStatus);

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pVis->revision, NULL, pStatus);
  if (*pStatus) goto except;
  if (pVis->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_VIS table. Got %d\n",
           revision, pVis->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pVis->date_obs, NULL, pStatus);
  read_key_opt_string(fptr, "ARRNAME", pVis->arrname, pStatus);
  fits_read_key(fptr, TSTRING, "INSNAME", pVis->insname, NULL, pStatus);
  /* get dimensions and allocate storage */
  fits_get_num_rows(fptr, &nrows, pStatus);
  /* note format specifies same repeat count for VIS* & FLAG columns = nwave */
  fits_get_colnum(fptr, CASEINSEN, "VISAMP", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  if (*pStatus) goto except;
  alloc_oi_vis(pVis, nrows, repeat);
  /* read VISAMP unit (optional) */
  snprintf(keyword, FLEN_KEYWORD, "TUNIT%d", colnum);
  read_key_opt_string(fptr, keyword, pVis->ampunit, pStatus);
  /* read rows */
  for (irow=1; irow<=pVis->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
		  &pVis->record[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis->record[irow-1].time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis->record[irow-1].int_time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VISAMP", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave, NULL,
                  pVis->record[irow-1].visamp, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VISAMPERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave, NULL,
                  pVis->record[irow-1].visamperr, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VISPHI", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave, NULL,
                  pVis->record[irow-1].visphi, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VISPHIERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis->nwave, NULL,
                  pVis->record[irow-1].visphierr, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "UCOORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis->record[irow-1].ucoord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VCOORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis->record[irow-1].vcoord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 2, NULL,
		  pVis->record[irow-1].sta_index, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLAG", &colnum, pStatus);
    fits_read_col(fptr, TLOGICAL, colnum, irow, 1, pVis->nwave, NULL,
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
  bool correlated;
  const int revision = 2;
  int irow, colnum, anynull;
  long nrows, repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_VIS2", pStatus);
  if (*pStatus == END_OF_FILE)
    return *pStatus;  /* don't report EOF to stderr */
  else if (*pStatus)
    goto except;
  verify_chksum(fptr, pStatus);

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pVis2->revision, NULL, pStatus);
  if (*pStatus) goto except;
  if (pVis2->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_VIS2 table. Got %d\n",
           revision, pVis2->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pVis2->date_obs, NULL, pStatus);
  read_key_opt_string(fptr, "ARRNAME", pVis2->arrname, pStatus);
  fits_read_key(fptr, TSTRING, "INSNAME", pVis2->insname, NULL, pStatus);

  if (pVis2->revision >= 2 &&
      read_key_opt_string(fptr, "CORRNAME", pVis2->corrname, pStatus)) {
    correlated = TRUE;
  } else {
    correlated = FALSE;
    pVis2->corrname[0] = '\0';
  }
  
  /* get dimensions and allocate storage */
  fits_get_num_rows(fptr, &nrows, pStatus);
  /* note format specifies same repeat count for VIS2* & FLAG columns = nwave*/
  fits_get_colnum(fptr, CASEINSEN, "VIS2DATA", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  if (*pStatus) goto except;
  alloc_oi_vis2(pVis2, nrows, repeat);
  /* read rows */
  for (irow=1; irow<=pVis2->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
		  &pVis2->record[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis2->record[irow-1].time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis2->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis2->record[irow-1].int_time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VIS2DATA", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis2->nwave, NULL,
                  pVis2->record[irow-1].vis2data, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VIS2ERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pVis2->nwave, NULL,
                  pVis2->record[irow-1].vis2err, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "UCOORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis2->record[irow-1].ucoord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "VCOORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pVis2->record[irow-1].vcoord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 2, NULL,
		  pVis2->record[irow-1].sta_index, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLAG", &colnum, pStatus);
    fits_read_col(fptr, TLOGICAL, colnum, irow, 1, pVis2->nwave, NULL,
		  pVis2->record[irow-1].flag, &anynull, pStatus);

    /* read optional columns */
    if (correlated) {
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_VIS2DATA", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
                    &pVis2->record[irow-1].corrindx_vis2data,
                    &anynull, pStatus);
    }
  }

 except:
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "FITSIO error in %s:\n", function);
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
  bool correlated;
  const int revision = 2;
  int irow, colnum, anynull;
  long nrows, repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_T3", pStatus);
  if (*pStatus == END_OF_FILE)
    return *pStatus;  /* don't report EOF to stderr */
  else if (*pStatus)
    goto except;
  verify_chksum(fptr, pStatus);

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pT3->revision, NULL, pStatus);
  if (*pStatus) goto except;
  if (pT3->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_T3 table. Got %d\n",
           revision, pT3->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pT3->date_obs, NULL, pStatus);
  fits_write_errmark();
  read_key_opt_string(fptr, "ARRNAME", pT3->arrname, pStatus);
  fits_read_key(fptr, TSTRING, "INSNAME", pT3->insname, NULL, pStatus);

  if (pT3->revision >= 2 &&
      read_key_opt_string(fptr, "CORRNAME", pT3->corrname, pStatus)) {
    correlated = TRUE;
  } else {
    correlated = FALSE;
    pT3->corrname[0] = '\0';
  }

  /* get number of rows & allocate storage */
  fits_get_num_rows(fptr, &nrows, pStatus);
  /* get value for nwave */
  /* format specifies same repeat count for T3* & FLAG columns */
  fits_get_colnum(fptr, CASEINSEN, "T3AMP", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  if (*pStatus) goto except;
  alloc_oi_t3(pT3, nrows, repeat);
  /* read rows */
  for (irow=1; irow<=pT3->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
		  &pT3->record[irow-1].target_id, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pT3->record[irow-1].time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pT3->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pT3->record[irow-1].int_time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "T3AMP", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pT3->nwave, NULL,
                  pT3->record[irow-1].t3amp, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "T3AMPERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pT3->nwave, NULL,
                  pT3->record[irow-1].t3amperr, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "T3PHI", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pT3->nwave, NULL,
                  pT3->record[irow-1].t3phi, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "T3PHIERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pT3->nwave, NULL,
                  pT3->record[irow-1].t3phierr, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "U1COORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
                  &pT3->record[irow-1].u1coord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "V1COORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pT3->record[irow-1].v1coord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "U2COORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pT3->record[irow-1].u2coord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "V2COORD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pT3->record[irow-1].v2coord, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 3, NULL,
		  pT3->record[irow-1].sta_index, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLAG", &colnum, pStatus);
    fits_read_col(fptr, TLOGICAL, colnum, irow, 1, pT3->nwave, NULL,
		  pT3->record[irow-1].flag, &anynull, pStatus);

    /* read optional columns */
    if (correlated) {
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_T3AMP", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
                    &pT3->record[irow-1].corrindx_t3amp, &anynull, pStatus);
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_T3PHI", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
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
  bool correlated;
  char keyword[FLEN_KEYWORD], value[FLEN_VALUE];
  const int revision = 1;
  int irow, colnum, anynull;
  long nrows, repeat;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  next_named_hdu(fptr, "OI_SPECTRUM", pStatus);
  if (*pStatus == END_OF_FILE)
    return *pStatus;  /* don't report EOF to stderr */
  else if (*pStatus)
    goto except;
  verify_chksum(fptr, pStatus);

  /* Read table */
  fits_read_key(fptr, TINT, "OI_REVN", &pSpectrum->revision, NULL, pStatus);
  if (*pStatus) goto except;
  if (pSpectrum->revision > revision) {
    printf("WARNING! Expecting OI_REVN <= %d in OI_SPECTRUM table. Got %d\n",
           revision, pSpectrum->revision);
  }
  fits_read_key(fptr, TSTRING, "DATE-OBS", pSpectrum->date_obs, NULL, pStatus);
  read_key_opt_string(fptr, "ARRNAME", pSpectrum->arrname, pStatus);
  fits_read_key(fptr, TSTRING, "INSNAME", pSpectrum->insname, NULL, pStatus);
  if(read_key_opt_string(fptr, "CORRNAME", pSpectrum->corrname, pStatus)) {
    correlated = TRUE;
  } else {
    pSpectrum->corrname[0] = '\0';
    correlated = FALSE;
  }
  fits_read_key(fptr, TDOUBLE, "FOV", &pSpectrum->fov, NULL, pStatus);
  fits_read_key(fptr, TSTRING, "FOVTYPE", pSpectrum->fovtype, NULL, pStatus); //:BUG: buffer overrun
  fits_read_key(fptr, TSTRING, "CALSTAT", value, NULL, pStatus);
  pSpectrum->calstat = value[0];
  /* get dimensions and allocate storage */
  fits_get_num_rows(fptr, &nrows, pStatus);
  /* note format specifies same repeat count for FLUX* columns = nwave */
  fits_get_colnum(fptr, CASEINSEN, "FLUXDATA", &colnum, pStatus);
  fits_get_coltype(fptr, colnum, NULL, &repeat, NULL, pStatus);
  if (*pStatus) goto except;
  alloc_oi_spectrum(pSpectrum, nrows, repeat);
  /* read unit (mandatory) */
  snprintf(keyword, FLEN_KEYWORD, "TUNIT%d", colnum);
  fits_read_key(fptr, TSTRING, keyword, pSpectrum->fluxunit, NULL, pStatus);
  /* read rows */
  for (irow=1; irow<=pSpectrum->numrec; irow++) {
    fits_get_colnum(fptr, CASEINSEN, "TARGET_ID", &colnum, pStatus);
    fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
		  &pSpectrum->record[irow-1].target_id, &anynull, pStatus);
    /* no TIME column */
    fits_get_colnum(fptr, CASEINSEN, "MJD", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pSpectrum->record[irow-1].mjd, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "INT_TIME", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, 1, NULL,
		  &pSpectrum->record[irow-1].int_time, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLUXDATA", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pSpectrum->nwave, NULL,
                  pSpectrum->record[irow-1].fluxdata, &anynull, pStatus);
    fits_get_colnum(fptr, CASEINSEN, "FLUXERR", &colnum, pStatus);
    fits_read_col(fptr, TDOUBLE, colnum, irow, 1, pSpectrum->nwave, NULL,
                  pSpectrum->record[irow-1].fluxerr, &anynull, pStatus);
    /* read optional columns */
    fits_write_errmark();
    fits_get_colnum(fptr, CASEINSEN, "STA_INDEX", &colnum, pStatus);
    if(*pStatus == COL_NOT_FOUND) {
      pSpectrum->record[irow-1].sta_index = -1;
      *pStatus = 0;
      fits_clear_errmark();
    } else {
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
                    &pSpectrum->record[irow-1].sta_index, &anynull, pStatus);
    }
    if (correlated) {
      fits_get_colnum(fptr, CASEINSEN, "CORRINDX_FLUXDATA", &colnum, pStatus);
      fits_read_col(fptr, TINT, colnum, irow, 1, 1, NULL,
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
