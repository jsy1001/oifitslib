/**
 * @file
 * @ingroup oifile
 * Implementation of file-level API for OIFITS data.
 *
 * Copyright (C) 2007, 2015-2018 John Young
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

#include "oifile.h"
#include "chkmalloc.h"
#include "datemjd.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <fitsio.h>


/** GLib expanding string buffer, for use within OIFITSlib. */
GString *pGStr = NULL;


/** Typedef to specify pointer to a function that frees its argument. */
typedef void (*free_func)(gpointer);


/*
 * Private functions
 */

/** Find oi_array matching arrname in linked list. */
static oi_array *find_oi_array(const oi_fits *pOi,
                               const char *arrname)
{
  GList *link;
  oi_array *pArray;

  link = pOi->arrayList;
  while (link != NULL) {
    pArray = (oi_array *)link->data;
    if (strcmp(pArray->arrname, arrname) == 0)
      return pArray;
    link = link->next;
  }
  g_warning("Missing OI_ARRAY with ARRNAME=%s", arrname);
  return NULL;
}

/** Find oi_wavelength matching insname in linked list. */
static oi_wavelength *find_oi_wavelength(const oi_fits *pOi,
                                         const char *insname)
{
  GList *link;
  oi_wavelength *pWave;

  link = pOi->wavelengthList;
  while (link != NULL) {
    pWave = (oi_wavelength *)link->data;
    if (strcmp(pWave->insname, insname) == 0)
      return pWave;
    link = link->next;
  }
  g_warning("Missing OI_WAVELENGTH with INSNAME=%s", insname);
  return NULL;
}

/** Find oi_corr matching corrname in linked list. */
static oi_corr *find_oi_corr(const oi_fits *pOi, const char *corrname)
{
  GList *link;
  oi_corr *pCorr;

  link = pOi->corrList;
  while (link != NULL) {
    pCorr = (oi_corr *)link->data;
    if (strcmp(pCorr->corrname, corrname) == 0)
      return pCorr;
    link = link->next;
  }
  g_warning("Missing OI_CORR with CORRNAME=%s", corrname);
  return NULL;
}


/**
 * Return shortest wavelength in OI_WAVELENGTH table
 *
 * @param pWave  pointer to wavelength struct
 *
 * @return Minimum of eff_wave values /m
 */
static float get_min_wavelength(const oi_wavelength *pWave)
{
  int i;
  float minWave = 1.0e11;

  for (i = 0; i < pWave->nwave; i++) {
    if (pWave->eff_wave[i] < minWave) minWave = pWave->eff_wave[i];
  }
  return minWave;
}

/**
 * Return longest wavelength in OI_WAVELENGTH table
 *
 * @param pWave  pointer to wavelength struct
 *
 * @return Maximum of eff_wave values /m
 */
static float get_max_wavelength(const oi_wavelength *pWave)
{
  int i;
  float maxWave = 0.0;

  for (i = 0; i < pWave->nwave; i++) {
    if (pWave->eff_wave[i] > maxWave) maxWave = pWave->eff_wave[i];
  }
  return maxWave;
}

/** Generate summary string for each oi_array in GList. */
static void format_array_list_summary(GString *pGStr, GList *arrayList)
{
  int nn;
  GList *link;
  oi_array *pArray;

  nn = 1;
  link = arrayList;
  while (link != NULL) {
    pArray = (oi_array *)link->data;
    g_string_append_printf(pGStr,
                           "    #%-2d ARRNAME='%s'  %d elements\n",
                           nn++, pArray->arrname, pArray->nelement);
    link = link->next;
  }
}

/** Generate summary string for each oi_wavelength in GList. */
static void format_wavelength_list_summary(GString *pGStr, GList *waveList)
{
  int nn;
  GList *link;
  oi_wavelength *pWave;

  nn = 1;
  link = waveList;
  while (link != NULL) {
    pWave = (oi_wavelength *)link->data;
    g_string_append_printf(pGStr,
                           "    #%-2d INSNAME='%s'  %d channels  "
                           "%7.1f-%7.1fnm\n",
                           nn++, pWave->insname, pWave->nwave,
                           1e9 * get_min_wavelength(pWave),
                           1e9 * get_max_wavelength(pWave));
    link = link->next;
  }
}

/** Generate summary string for each oi_corr in GList. */
static void format_corr_list_summary(GString *pGStr, GList *corrList)
{
  int nn;
  GList *link;
  oi_corr *pCorr;

  nn = 1;
  link = corrList;
  while (link != NULL) {
    pCorr = (oi_corr *)link->data;
    g_string_append_printf(pGStr,
                           "    #%-2d CORRNAME='%s'  "
                           "%d/%d non-zero correlations\n",
                           nn++, pCorr->corrname, pCorr->ncorr, pCorr->ndata);
    link = link->next;
  }
}

/** Generate summary string for each oi_inspol in GList. */
static void format_inspol_list_summary(GString *pGStr, GList *inspolList)
{
  int nn;
  GList *link;
  oi_inspol *pInspol;

  nn = 1;
  link = inspolList;
  while (link != NULL) {
    pInspol = (oi_inspol *)link->data;
    //:TODO: add list of unique INSNAME values in this OI_INSPOL table
    g_string_append_printf(pGStr,
                           "    #%-2d ARRNAME='%s'\n", nn++, pInspol->arrname);
    link = link->next;
  }
}

/**
 * Return earliest of OI_VIS/VIS2/T3/FLUX MJD values.
 */
static double get_min_mjd(const oi_fits *pOi)
{
  const GList *link;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_flux *pFlux;
  int i;
  double minMjd;

  minMjd = 100000;
  link = pOi->visList;
  while (link != NULL) {
    pVis = link->data;
    for (i = 0; i < pVis->numrec; i++) {
      if (pVis->record[i].mjd < minMjd)
        minMjd = pVis->record[i].mjd;
    }
    link = link->next;
  }
  link = pOi->vis2List;
  while (link != NULL) {
    pVis2 = link->data;
    for (i = 0; i < pVis2->numrec; i++) {
      if (pVis2->record[i].mjd < minMjd)
        minMjd = pVis2->record[i].mjd;
    }
    link = link->next;
  }
  link = pOi->t3List;
  while (link != NULL) {
    pT3 = link->data;
    for (i = 0; i < pT3->numrec; i++) {
      if (pT3->record[i].mjd < minMjd)
        minMjd = pT3->record[i].mjd;
    }
    link = link->next;
  }
  link = pOi->fluxList;
  while (link != NULL) {
    pFlux = link->data;
    for (i = 0; i < pFlux->numrec; i++) {
      if (pFlux->record[i].mjd < minMjd)
        minMjd = pFlux->record[i].mjd;
    }
    link = link->next;
  }
  return minMjd;
}

/**
 * Return latest of OI_VIS/VIS2/T3/FLUX MJD values.
 */
static double get_max_mjd(const oi_fits *pOi)
{
  const GList *link;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_flux *pFlux;
  int i;
  double maxMjd;

  maxMjd = 0;
  link = pOi->visList;
  while (link != NULL) {
    pVis = link->data;
    for (i = 0; i < pVis->numrec; i++) {
      if (pVis->record[i].mjd > maxMjd)
        maxMjd = pVis->record[i].mjd;
    }
    link = link->next;
  }
  link = pOi->vis2List;
  while (link != NULL) {
    pVis2 = link->data;
    for (i = 0; i < pVis2->numrec; i++) {
      if (pVis2->record[i].mjd > maxMjd)
        maxMjd = pVis2->record[i].mjd;
    }
    link = link->next;
  }
  link = pOi->t3List;
  while (link != NULL) {
    pT3 = link->data;
    for (i = 0; i < pT3->numrec; i++) {
      if (pT3->record[i].mjd > maxMjd)
        maxMjd = pT3->record[i].mjd;
    }
    link = link->next;
  }
  link = pOi->fluxList;
  while (link != NULL) {
    pFlux = link->data;
    for (i = 0; i < pFlux->numrec; i++) {
      if (pFlux->record[i].mjd > maxMjd)
        maxMjd = pFlux->record[i].mjd;
    }
    link = link->next;
  }
  return maxMjd;
}

/*
 * Public functions
 */

/**
 * Initialise empty oi_fits struct
 *
 * @param pOi  pointer to file data struct, see oifile.h
 */
void init_oi_fits(oi_fits *pOi)
{
  pOi->header.origin[0] = '\0';
  pOi->header.date[0] = '\0';
  pOi->header.date_obs[0] = '\0';
  pOi->header.content[0] = '\0';
  pOi->header.telescop[0] = '\0';
  pOi->header.instrume[0] = '\0';
  pOi->header.observer[0] = '\0';
  pOi->header.insmode[0] = '\0';
  pOi->header.object[0] = '\0';
  pOi->header.referenc[0] = '\0';
  pOi->header.author[0] = '\0';
  pOi->header.prog_id[0] = '\0';
  pOi->header.procsoft[0] = '\0';
  pOi->header.obstech[0] = '\0';

  pOi->targets.revision = OI_REVN_V2_TARGET;
  pOi->targets.ntarget = 0;
  pOi->targets.targ = NULL;
  pOi->targets.usecategory = FALSE;

  pOi->numArray = 0;
  pOi->numWavelength = 0;
  pOi->numCorr = 0;
  pOi->numInspol = 0;
  pOi->numVis = 0;
  pOi->numVis2 = 0;
  pOi->numT3 = 0;
  pOi->numFlux = 0;
  pOi->arrayList = NULL;
  pOi->wavelengthList = NULL;
  pOi->corrList = NULL;
  pOi->inspolList = NULL;
  pOi->visList = NULL;
  pOi->vis2List = NULL;
  pOi->t3List = NULL;
  pOi->fluxList = NULL;
  pOi->arrayHash =
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
  pOi->wavelengthHash =
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
  pOi->corrHash =
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
}


#define RETURN_VAL_IF_BAD_TAB_REVISION(tabList, tabType, rev, val) \
  do {                                                             \
    tabType *tab;                                                  \
    GList *link;                                                   \
    link = (tabList);                                              \
    while (link != NULL) {                                         \
      tab = (tabType *)link->data;                                 \
      if (tab->revision != (rev)) return val;                      \
      link = link->next;                                           \
    }                                                              \
  } while (0)

/**
 * Do all table revision numbers match version 1 of the OIFITS standard?
 *
 * Ignores any tables defined in OIFITS version 2.
 *
 * @param pOi  pointer to file data struct, see oifile.h
 *
 * @return TRUE if OIFITS v1, FALSE otherwise
 */
int is_oi_fits_one(const oi_fits *pOi)
{
  g_assert(pOi != NULL);
  if (pOi->targets.revision != 1) return FALSE;
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->arrayList, oi_array, 1, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->wavelengthList, oi_wavelength, 1, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->visList, oi_vis, 1, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->vis2List, oi_vis2, 1, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->t3List, oi_t3, 1, FALSE);
  return TRUE;
}

/**
 * Do all table revision numbers match version 2 of the OIFITS standard?
 *
 * @param pOi  pointer to file data struct, see oifile.h
 *
 * @return TRUE if OIFITS v2, FALSE otherwise
 */
int is_oi_fits_two(const oi_fits *pOi)
{
  g_assert(pOi != NULL);
  if (pOi->targets.revision != 2) return FALSE;
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->arrayList, oi_array, 2, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->wavelengthList, oi_wavelength, 2, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->corrList, oi_corr, 1, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->inspolList, oi_inspol, 1, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->visList, oi_vis, 2, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->vis2List, oi_vis2, 2, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->t3List, oi_t3, 2, FALSE);
  RETURN_VAL_IF_BAD_TAB_REVISION(pOi->fluxList, oi_flux, 1, FALSE);
  return TRUE;
}

/**
 * Is the dataset one observation of single target with a single instrument?
 *
 * An empty dataset, as created by init_oi_fits(), is not considered atomic.
 *
 * @param pOi      pointer to file data struct, see oifile.h
 * @param maxDays  maximum time span to be considered atomic, in days
 *
 * @return TRUE if atomic, FALSE otherwise
 */
int is_atomic(const oi_fits *pOi, double maxDays)
{
  double minMjd, maxMjd;

  /* allow 0 or 1 array tables because optional for OIFITS v1 */
  if (pOi->numArray > 1)
    return FALSE;
  if (pOi->numWavelength != 1)
    return FALSE;
  if (pOi->targets.ntarget != 1)
    return FALSE;

  minMjd = get_min_mjd(pOi);
  maxMjd = get_max_mjd(pOi);
  if (fabs(maxMjd - minMjd) > maxDays)
    return FALSE;

  return TRUE;
}

/**
 * Count unflagged data
 *
 * @param pOi       pointer to file data struct, see oifile.h
 * @param pNumVis   return location for number of complex visibility data,
 *                  or NULL
 * @param pNumVis2  return location for number of squared visibility data,
 *                  or NULL
 * @param pNumT3    return location for number of bispectrum data, or NULL
 */
void count_oi_fits_data(const oi_fits *pOi, long *const pNumVis,
                        long *const pNumVis2, long *const pNumT3)
{
  long numVis, numVis2, numT3;
  GList *link;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  int i, j;

  /* Count unflagged complex visibilities */
  numVis = 0;
  link = pOi->visList;
  while (link != NULL)
  {
    pVis = (oi_vis *)link->data;
    for (j = 0; j < pVis->numrec; j++) {
      for (i = 0; i < pVis->nwave; i++) {
        if (!pVis->record[j].flag[i])
          ++numVis;
      }
    }
    link = link->next;
  }

  /* Count unflagged squared visibilities */
  numVis2 = 0;
  link = pOi->vis2List;
  while (link != NULL)
  {
    pVis2 = (oi_vis2 *)link->data;
    for (j = 0; j < pVis2->numrec; j++) {
      for (i = 0; i < pVis2->nwave; i++) {
        if (!pVis2->record[j].flag[i])
          ++numVis2;
      }
    }
    link = link->next;
  }

  /* Count unflagged bispectra */
  numT3 = 0;
  link = pOi->t3List;
  while (link != NULL)
  {
    pT3 = (oi_t3 *)link->data;
    for (j = 0; j < pT3->numrec; j++) {
      for (i = 0; i < pT3->nwave; i++) {
        if (!pT3->record[j].flag[i])
          ++numT3;
      }
    }
    link = link->next;
  }

  if (pNumVis)
    *pNumVis = numVis;
  if (pNumVis2)
    *pNumVis2 = numVis2;
  if (pNumT3)
    *pNumT3 = numT3;
}

/** Macro to write FITS table for each oi_* in GList. */
#define WRITE_OI_LIST(fptr, list, type, write_func, pStatus)      \
  do {                                                            \
    GList *link = (list);                                         \
    int extver = 1;                                               \
    while (link != NULL) {                                        \
      write_func(fptr, *((type *)link->data), extver++, pStatus); \
      link = link->next;                                          \
    }                                                             \
  } while (0)

/**
 * Set primary header keywords from table contents
 *
 * Sets values for DATE-OBS, TELESCOP, INSTRUME and OBJECT from
 * existing data.  ORIGIN, OBSERVER and INSMODE are set to "UNKNOWN".
 *
 * @param pOi  pointer to file data struct, see oifile.h
 */
void set_oi_header(oi_fits *pOi)
{
  const char multiple[] = "MULTIPLE";
  oi_array *pArray;
  oi_wavelength *pWave;
  long year, month, day;

  /* Set TELESCOP */
  if (pOi->numArray == 0) {
    g_strlcpy(pOi->header.telescop, "UNKNOWN", FLEN_VALUE);
  } else if (pOi->numArray == 1) {
    pArray = pOi->arrayList->data;
    g_strlcpy(pOi->header.telescop, pArray->arrname, FLEN_VALUE);
  } else {
    g_strlcpy(pOi->header.telescop, multiple, FLEN_VALUE);
  }

  /* Set INSTRUME */
  if (pOi->numWavelength == 1) {
    pWave = pOi->wavelengthList->data;
    g_strlcpy(pOi->header.instrume, pWave->insname, FLEN_VALUE);
  } else {
    g_strlcpy(pOi->header.instrume, multiple, FLEN_VALUE);
  }

  /* Set OBJECT */
  if (pOi->targets.ntarget == 1) {
    g_strlcpy(pOi->header.object, pOi->targets.targ[0].target, FLEN_VALUE);
  } else {
    g_strlcpy(pOi->header.object, multiple, FLEN_VALUE);
  }

  /* Set DATE-OBS */
  mjd2date(floor(get_min_mjd(pOi)), &year, &month, &day);
  g_snprintf(pOi->header.date_obs, FLEN_VALUE,
             "%4ld-%02ld-%02ld", year, month, day);

  /* Set other mandatory keywords */
  g_strlcpy(pOi->header.origin, "UNKNOWN", FLEN_VALUE);
  g_strlcpy(pOi->header.observer, "UNKNOWN", FLEN_VALUE);
  g_strlcpy(pOi->header.insmode, "UNKNOWN", FLEN_VALUE);
}

/**
 * Write OIFITS tables to new FITS file
 *
 * @param filename  name of file to create
 * @param oi        file data struct, see oifile.h
 * @param pStatus   pointer to status variable
 *
 * @return On error, returns non-zero cfitsio error code (also assigned to
 *         *pStatus)
 */
STATUS write_oi_fits(const char *filename, oi_fits oi, STATUS *pStatus)
{
  const char function[] = "write_oi_fits";
  fitsfile *fptr = NULL;

  if (*pStatus) return *pStatus;  /* error flag set - do nothing */

  /* Open new FITS file */
  fits_create_file(&fptr, filename, pStatus);
  if (*pStatus) goto except;

  /* Write primary header keywords */
  write_oi_header(fptr, oi.header, pStatus);

  /* Write OI_TARGET table */
  write_oi_target(fptr, oi.targets, pStatus);

  /* Write all OI_ARRAY tables */
  WRITE_OI_LIST(fptr, oi.arrayList, oi_array, write_oi_array, pStatus);

  /* Write all OI_WAVELENGTH tables */
  WRITE_OI_LIST(fptr, oi.wavelengthList, oi_wavelength, write_oi_wavelength,
                pStatus);

  /* Write all OI_CORR tables */
  WRITE_OI_LIST(fptr, oi.corrList, oi_corr, write_oi_corr, pStatus);

  /* Write all OI_INSPOL tables */
  WRITE_OI_LIST(fptr, oi.inspolList, oi_inspol, write_oi_inspol, pStatus);

  /* Write all data tables */
  WRITE_OI_LIST(fptr, oi.visList, oi_vis, write_oi_vis, pStatus);
  WRITE_OI_LIST(fptr, oi.vis2List, oi_vis2, write_oi_vis2, pStatus);
  WRITE_OI_LIST(fptr, oi.t3List, oi_t3, write_oi_t3, pStatus);
  WRITE_OI_LIST(fptr, oi.fluxList, oi_flux, write_oi_flux, pStatus);

except:
  if (fptr) fits_close_file(fptr, pStatus);
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/**
 * Read all OIFITS tables from FITS file
 *
 * @param filename  name of file to read
 * @param pOi       pointer to uninitialised file data struct, see oifile.h
 * @param pStatus   pointer to status variable
 *
 * @return On error, returns non-zero cfitsio error code (also assigned to
 *         *pStatus). Contents of file data struct are undefined
 */
STATUS read_oi_fits(const char *filename, oi_fits *pOi, STATUS *pStatus)
{
  const char function[] = "read_oi_fits";
  char desc[FLEN_STATUS];
  fitsfile *fptr = NULL;
  int hdutype;
  oi_array *pArray;
  oi_wavelength *pWave;
  oi_corr *pCorr;
  oi_inspol *pInspol;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_flux *pFlux;

  if (*pStatus) return *pStatus;  /* error flag set - do nothing */

  fits_open_file(&fptr, filename, READONLY, pStatus);
  if (*pStatus) goto except;

  /* Create empty data structures */
  pOi->arrayHash =
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
  pOi->wavelengthHash =
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
  pOi->corrHash =
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
  pOi->arrayList = NULL;
  pOi->wavelengthList = NULL;
  pOi->corrList = NULL;
  pOi->inspolList = NULL;
  pOi->visList = NULL;
  pOi->vis2List = NULL;
  pOi->t3List = NULL;
  pOi->fluxList = NULL;

  /* Read primary header keywords */
  read_oi_header(fptr, &pOi->header, pStatus);

  /* Read compulsory OI_TARGET table */
  read_oi_target(fptr, &pOi->targets, pStatus);
  if (*pStatus) goto except;

  /* Read all OI_ARRAY tables */
  pOi->numArray = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (TRUE) {
    pArray = chkmalloc(sizeof(oi_array));
    fits_write_errmark();
    if (read_next_oi_array(fptr, pArray, pStatus))
      break;  /* no more OI_ARRAY */
    pOi->arrayList = g_list_append(pOi->arrayList, pArray);
    ++pOi->numArray;
  }
  free(pArray);
  if (*pStatus != END_OF_FILE) goto except;
  *pStatus = 0; /* reset EOF */
  fits_clear_errmark();

  /* Read all OI_WAVELENGTH tables */
  pOi->numWavelength = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (TRUE) {
    pWave = chkmalloc(sizeof(oi_wavelength));
    fits_write_errmark();
    if (read_next_oi_wavelength(fptr, pWave, pStatus))
      break;  /* no more OI_WAVELENGTH */
    pOi->wavelengthList = g_list_append(pOi->wavelengthList, pWave);
    ++pOi->numWavelength;
  }
  free(pWave);
  if (*pStatus != END_OF_FILE) goto except;
  *pStatus = 0; /* reset EOF */
  fits_clear_errmark();

  /* Read all OI_CORR tables, skipping over failed tables */
  pOi->numCorr = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (TRUE) {
    pCorr = chkmalloc(sizeof(oi_corr));
    fits_write_errmark();
    if (read_next_oi_corr(fptr, pCorr, pStatus)) {
      free(pCorr);
      fits_clear_errmark();
      if (*pStatus == END_OF_FILE) {
        *pStatus = 0;
        break;  /* no more OI_CORR */
      }
      fits_get_errstatus(*pStatus, desc);
      fprintf(stderr, "\nSkipping bad OI_CORR (%s)\n", desc);
      *pStatus = 0;
      continue;
    }
    pOi->corrList = g_list_append(pOi->corrList, pCorr);
    ++pOi->numCorr;
  }

  /* Read all OI_INSPOL tables, skipping over failed tables */
  pOi->numInspol = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (TRUE) {
    pInspol = chkmalloc(sizeof(oi_inspol));
    fits_write_errmark();
    if (read_next_oi_inspol(fptr, pInspol, pStatus)) {
      free(pInspol);
      fits_clear_errmark();
      if (*pStatus == END_OF_FILE) {
        *pStatus = 0;
        break;  /* no more OI_INSPOL */
      }
      fits_get_errstatus(*pStatus, desc);
      fprintf(stderr, "\nSkipping bad OI_INSPOL (%s)\n", desc);
      *pStatus = 0;
      continue;
    }
    pOi->inspolList = g_list_append(pOi->inspolList, pInspol);
    ++pOi->numInspol;
  }

  /* Read all OI_VIS, hash-tabling corresponding array, wavelength and
   * corr tables, skipping over failed tables */
  pOi->numVis = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (TRUE) {
    pVis = chkmalloc(sizeof(oi_vis));
    fits_write_errmark();
    if (read_next_oi_vis(fptr, pVis, pStatus)) {
      free(pVis);
      fits_clear_errmark();
      if (*pStatus == END_OF_FILE) {
        *pStatus = 0;
        break;  /* no more OI_VIS */
      }
      fits_get_errstatus(*pStatus, desc);
      fprintf(stderr, "\nSkipping bad OI_VIS (%s)\n", desc);
      *pStatus = 0;
      continue;
    }
    pOi->visList = g_list_append(pOi->visList, pVis);
    ++pOi->numVis;
    if (strlen(pVis->arrname) > 0) {
      if (!g_hash_table_lookup(pOi->arrayHash, pVis->arrname))
        g_hash_table_insert(pOi->arrayHash, pVis->arrname,
                            find_oi_array(pOi, pVis->arrname));
    }
    if (!g_hash_table_lookup(pOi->wavelengthHash, pVis->insname))
      g_hash_table_insert(pOi->wavelengthHash, pVis->insname,
                          find_oi_wavelength(pOi, pVis->insname));
    if (strlen(pVis->corrname) > 0) {
      if (!g_hash_table_lookup(pOi->corrHash, pVis->corrname))
        g_hash_table_insert(pOi->corrHash, pVis->corrname,
                            find_oi_corr(pOi, pVis->corrname));
    }
  }

  /* Read all OI_VIS2, hash-tabling corresponding array, wavelength
   * and corr tables, skipping over failed tables */
  pOi->numVis2 = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (TRUE) {
    pVis2 = chkmalloc(sizeof(oi_vis2));
    fits_write_errmark();
    if (read_next_oi_vis2(fptr, pVis2, pStatus)) {
      free(pVis2);
      fits_clear_errmark();
      if (*pStatus == END_OF_FILE) {
        *pStatus = 0;
        break;  /* no more OI_VIS2 */
      }
      fits_get_errstatus(*pStatus, desc);
      fprintf(stderr, "\nSkipping bad OI_VIS2 (%s)\n", desc);
      *pStatus = 0;
      continue;
    }
    pOi->vis2List = g_list_append(pOi->vis2List, pVis2);
    ++pOi->numVis2;
    if (strlen(pVis2->arrname) > 0) {
      if (!g_hash_table_lookup(pOi->arrayHash, pVis2->arrname))
        g_hash_table_insert(pOi->arrayHash, pVis2->arrname,
                            find_oi_array(pOi, pVis2->arrname));
    }
    if (!g_hash_table_lookup(pOi->wavelengthHash, pVis2->insname))
      g_hash_table_insert(pOi->wavelengthHash, pVis2->insname,
                          find_oi_wavelength(pOi, pVis2->insname));
    if (strlen(pVis2->corrname) > 0) {
      if (!g_hash_table_lookup(pOi->corrHash, pVis2->corrname))
        g_hash_table_insert(pOi->corrHash, pVis2->corrname,
                            find_oi_corr(pOi, pVis2->corrname));
    }
  }

  /* Read all OI_T3, hash-tabling corresponding array, wavelength and
   * corr tables, skipping over failed tables */
  pOi->numT3 = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (TRUE) {
    pT3 = chkmalloc(sizeof(oi_t3));
    fits_write_errmark();
    if (read_next_oi_t3(fptr, pT3, pStatus)) {
      free(pT3);
      fits_clear_errmark();
      if (*pStatus == END_OF_FILE) {
        *pStatus = 0;
        break;  /* no more OI_T3 */
      }
      fits_get_errstatus(*pStatus, desc);
      fprintf(stderr, "\nSkipping bad OI_T3 (%s)\n", desc);
      *pStatus = 0;
      continue;
    }
    pOi->t3List = g_list_append(pOi->t3List, pT3);
    ++pOi->numT3;
    if (strlen(pT3->arrname) > 0) {
      if (!g_hash_table_lookup(pOi->arrayHash, pT3->arrname))
        g_hash_table_insert(pOi->arrayHash, pT3->arrname,
                            find_oi_array(pOi, pT3->arrname));
    }
    if (!g_hash_table_lookup(pOi->wavelengthHash, pT3->insname))
      g_hash_table_insert(pOi->wavelengthHash, pT3->insname,
                          find_oi_wavelength(pOi, pT3->insname));
    if (strlen(pT3->corrname) > 0) {
      if (!g_hash_table_lookup(pOi->corrHash, pT3->corrname))
        g_hash_table_insert(pOi->corrHash, pT3->corrname,
                            find_oi_corr(pOi, pT3->corrname));
    }
  }

  /* Read all OI_FLUX, hash-tabling corresponding array & wavelength
   * tables, skipping over failed tables */
  pOi->numFlux = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (TRUE) {
    pFlux = chkmalloc(sizeof(oi_flux));
    fits_write_errmark();
    if (read_next_oi_flux(fptr, pFlux, pStatus)) {
      free(pFlux);
      fits_clear_errmark();
      if (*pStatus == END_OF_FILE) {
        *pStatus = 0;
        break;  /* no more OI_FLUX */
      }
      fits_get_errstatus(*pStatus, desc);
      fprintf(stderr, "\nSkipping bad OI_FLUX (%s)\n", desc);
      *pStatus = 0;
      continue;
    }
    pOi->fluxList = g_list_append(pOi->fluxList, pFlux);
    ++pOi->numFlux;
    if (strlen(pFlux->arrname) > 0) {
      if (!g_hash_table_lookup(pOi->arrayHash, pFlux->arrname))
        g_hash_table_insert(pOi->arrayHash, pFlux->arrname,
                            find_oi_array(pOi, pFlux->arrname));
    }
    if (!g_hash_table_lookup(pOi->wavelengthHash, pFlux->insname))
      g_hash_table_insert(pOi->wavelengthHash, pFlux->insname,
                          find_oi_wavelength(pOi, pFlux->insname));
  }

  if (is_oi_fits_one(pOi))
    set_oi_header(pOi);

except:
  if (fptr) fits_close_file(fptr, pStatus);
  if (*pStatus && !oi_hush_errors) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/** Free linked list and contents. */
static void free_list(GList *list, free_func internalFree)
{
  GList *link;

  link = list;
  while (link != NULL) {
    if (internalFree)
      (*internalFree)(link->data);
    free(link->data);
    link = link->next;
  }
  g_list_free(list);
}

/**
 * Free storage used for OIFITS data
 *
 * @param pOi  pointer to file data struct, see oifile.h
 */
void free_oi_fits(oi_fits *pOi)
{
  g_hash_table_destroy(pOi->arrayHash);
  g_hash_table_destroy(pOi->wavelengthHash);
  g_hash_table_destroy(pOi->corrHash);
  free_oi_target(&pOi->targets);
  free_list(pOi->arrayList,
            (free_func)free_oi_array);
  free_list(pOi->wavelengthList,
            (free_func)free_oi_wavelength);
  free_list(pOi->corrList,
            (free_func)free_oi_corr);
  free_list(pOi->inspolList,
            (free_func)free_oi_inspol);
  free_list(pOi->visList,
            (free_func)free_oi_vis);
  free_list(pOi->vis2List,
            (free_func)free_oi_vis2);
  free_list(pOi->t3List,
            (free_func)free_oi_t3);
  free_list(pOi->fluxList,
            (free_func)free_oi_flux);
}

/**
 * Return oi_array corresponding to specified ARRNAME
 *
 * @param pOi      pointer to file data struct, see oifile.h
 * @param arrname  value of ARRNAME keyword
 *
 * @return pointer to oi_array matching arrname, or NULL if no match
 */
oi_array *oi_fits_lookup_array(const oi_fits *pOi, const char *arrname)
{
  return (oi_array *)g_hash_table_lookup(pOi->arrayHash, arrname);
}

/**
 * Lookup array element corresponding to specified ARRNAME & STA_INDEX
 *
 * @param pOi       pointer to file data struct, see oifile.h
 * @param arrname   value of ARRNAME keyword
 * @param staIndex  value of STA_INDEX from data table
 *
 * @return ptr to 1st element struct matching staIndex, or NULL if no match
 */
element *oi_fits_lookup_element(const oi_fits *pOi,
                                const char *arrname, int staIndex)
{
  int i;
  oi_array *pArray;

  pArray = oi_fits_lookup_array(pOi, arrname);
  if (pArray == NULL) return NULL;
  /* We don't assume records are ordered by STA_INDEX */
  for (i = 0; i < pArray->nelement; i++) {
    if (pArray->elem[i].sta_index == staIndex)
      return &pArray->elem[i];
  }
  return NULL;
}

/**
 * Lookup oi_wavelength corresponding to specified INSNAME
 *
 * @param pOi      pointer to file data struct, see oifile.h
 * @param insname  value of INSNAME keyword
 *
 * @return pointer to oi_wavelength matching insname, or NULL if no match
 */
oi_wavelength *oi_fits_lookup_wavelength(const oi_fits *pOi,
                                         const char *insname)
{
  return (oi_wavelength *)g_hash_table_lookup(pOi->wavelengthHash, insname);
}

/**
 * Lookup oi_corr corresponding to specified CORRNAME
 *
 * @param pOi       pointer to file data struct, see oifile.h
 * @param corrname  value of CORRNAME keyword
 *
 * @return pointer to oi_corr matching corrname, or NULL if no match
 */
oi_corr *oi_fits_lookup_corr(const oi_fits *pOi, const char *corrname)
{
  return (oi_corr *)g_hash_table_lookup(pOi->corrHash, corrname);
}

/**
 * Lookup target record by TARGET_ID
 *
 * @param pOi       pointer to file data struct, see oifile.h
 * @param targetId  value of TARGET_ID from data table
 *
 * @return ptr to 1st target struct matching targetId, or NULL if no match
 */
target *oi_fits_lookup_target(const oi_fits *pOi, int targetId)
{
  int i;

  /* We don't assume records are ordered by TARGET_ID */
  for (i = 0; i < pOi->targets.ntarget; i++) {
    if (pOi->targets.targ[i].target_id == targetId)
      return &pOi->targets.targ[i];
  }
  return NULL;
}

/**
 * Lookup target record by name
 *
 * @param pOi       pointer to file data struct, see oifile.h
 * @param target    value of TARGET (the name) to match
 *
 * @return ptr to 1st target struct matching target, or NULL if no match
 */
target *oi_fits_lookup_target_by_name(const oi_fits *pOi, const char *target)
{
  int i;

  for (i = 0; i < pOi->targets.ntarget; i++) {
    if (strcmp(pOi->targets.targ[i].target, target) == 0)
      return &pOi->targets.targ[i];
  }
  return NULL;
}

/** Generate summary string for each oi_vis/vis2/t3 in GList. */
#define FORMAT_OI_LIST_SUMMARY(pGStr, list, type)         \
  {                                                       \
    int nn = 1;                                           \
    GList *link = (list);                                 \
    while (link != NULL) {                                \
      g_string_append_printf(                             \
        pGStr,                                            \
        "    #%-2d DATE-OBS=%s\n"                         \
        "    INSNAME='%s'  ARRNAME='%s'  CORRNAME='%s'\n" \
        "     %5ld records x %3d wavebands\n",            \
        nn ++,                                            \
        ((type *)(link->data))->date_obs,                 \
        ((type *)(link->data))->insname,                  \
        ((type *)(link->data))->arrname,                  \
        ((type *)(link->data))->corrname,                 \
        ((type *)(link->data))->numrec,                   \
        ((type *)(link->data))->nwave);                   \
      link = link->next;                                  \
    }                                                     \
  }

/**
 * Generate file summary string
 *
 * @param pOi  pointer to oi_fits struct
 *
 * @return String summarising supplied dataset
 */
const char *format_oi_fits_summary(const oi_fits *pOi)
{
  if (pGStr == NULL)
    pGStr = g_string_sized_new(512);

  if (strlen(pOi->header.content) > 0)
    g_string_printf(pGStr, "'%s' data:\n", pOi->header.content);
  else
    g_string_printf(pGStr, "OIFITS data:\n");
  g_string_append_printf(pGStr, "  ORIGIN  = '%s'\n", pOi->header.origin);
  g_string_append_printf(pGStr, "  DATE    = '%s'\n", pOi->header.date);
  g_string_append_printf(pGStr, "  DATE-OBS= '%s'\n", pOi->header.date_obs);
  g_string_append_printf(pGStr, "  TELESCOP= '%s'\n", pOi->header.telescop);
  g_string_append_printf(pGStr, "  INSTRUME= '%s'\n", pOi->header.instrume);
  g_string_append_printf(pGStr, "  OBSERVER= '%s'\n", pOi->header.observer);
  g_string_append_printf(pGStr, "  OBJECT  = '%s'\n", pOi->header.object);
  g_string_append_printf(pGStr, "  INSMODE = '%s'\n", pOi->header.insmode);
  g_string_append_printf(pGStr, "  OBSTECH = '%s'\n\n", pOi->header.obstech);
  g_string_append_printf(pGStr, "  %d OI_ARRAY tables:\n", pOi->numArray);
  format_array_list_summary(pGStr, pOi->arrayList);
  g_string_append_printf(pGStr, "  %d OI_WAVELENGTH tables:\n",
                         pOi->numWavelength);
  format_wavelength_list_summary(pGStr, pOi->wavelengthList);
  g_string_append_printf(pGStr, "  %d OI_CORR tables:\n", pOi->numCorr);
  format_corr_list_summary(pGStr, pOi->corrList);
  g_string_append_printf(pGStr, "  %d OI_INSPOL tables:\n", pOi->numInspol);
  format_inspol_list_summary(pGStr, pOi->inspolList);
  g_string_append_printf(pGStr, "  %d OI_VIS tables:\n", pOi->numVis);
  FORMAT_OI_LIST_SUMMARY(pGStr, pOi->visList, oi_vis);
  g_string_append_printf(pGStr, "  %d OI_VIS2 tables:\n", pOi->numVis2);
  FORMAT_OI_LIST_SUMMARY(pGStr, pOi->vis2List, oi_vis2);
  g_string_append_printf(pGStr, "  %d OI_T3 tables:\n", pOi->numT3);
  FORMAT_OI_LIST_SUMMARY(pGStr, pOi->t3List, oi_t3);
  g_string_append_printf(pGStr, "  %d OI_FLUX tables:\n", pOi->numFlux);
  FORMAT_OI_LIST_SUMMARY(pGStr, pOi->fluxList, oi_flux);

  return pGStr->str;
}

/**
 * Print file summary to stdout
 *
 * @param pOi  pointer to oi_fits struct
 */
void print_oi_fits_summary(const oi_fits *pOi)
{
  printf("%s", format_oi_fits_summary(pOi));
}

/**
 * Make deep copy of a OI_TARGET table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_target *dup_oi_target(const oi_target *pInTab)
{
  oi_target *pOutTab;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->targ, pInTab->targ,
         pInTab->ntarget * sizeof(pInTab->targ[0]));
  return pOutTab;
}
/**
 * Make deep copy of a OI_ARRAY table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_array *dup_oi_array(const oi_array *pInTab)
{
  oi_array *pOutTab;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->elem, pInTab->elem,
         pInTab->nelement * sizeof(pInTab->elem[0]));
  return pOutTab;
}

/**
 * Make deep copy of a OI_WAVELENGTH table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_wavelength *dup_oi_wavelength(const oi_wavelength *pInTab)
{
  oi_wavelength *pOutTab;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->eff_wave, pInTab->eff_wave,
         pInTab->nwave * sizeof(pInTab->eff_wave[0]));
  MEMDUP(pOutTab->eff_band, pInTab->eff_band,
         pInTab->nwave * sizeof(pInTab->eff_band[0]));
  return pOutTab;
}

/**
 * Make deep copy of a OI_CORR table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_corr *dup_oi_corr(const oi_corr *pInTab)
{
  oi_corr *pOutTab;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->iindx, pInTab->iindx,
         pInTab->ncorr * sizeof(pInTab->iindx[0]));
  MEMDUP(pOutTab->jindx, pInTab->jindx,
         pInTab->ncorr * sizeof(pInTab->jindx[0]));
  MEMDUP(pOutTab->corr, pInTab->corr,
         pInTab->ncorr * sizeof(pInTab->corr[0]));
  return pOutTab;
}

/**
 * Make deep copy of a OI_INSPOL table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_inspol *dup_oi_inspol(const oi_inspol *pInTab)
{
  oi_inspol *pOutTab;
  oi_inspol_record *pInRec, *pOutRec;
  int i;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->record, pInTab->record,
         pInTab->numrec * sizeof(pInTab->record[0]));
  for (i = 0; i < pInTab->numrec; i++) {
    pOutRec = &pOutTab->record[i];
    pInRec = &pInTab->record[i];
    MEMDUP(pOutRec->jxx, pInRec->jxx,
           pInTab->nwave * sizeof(pInRec->jxx[0]));
    MEMDUP(pOutRec->jyy, pInRec->jyy,
           pInTab->nwave * sizeof(pInRec->jyy[0]));
    MEMDUP(pOutRec->jxy, pInRec->jxy,
           pInTab->nwave * sizeof(pInRec->jxy[0]));
    MEMDUP(pOutRec->jyx, pInRec->jyx,
           pInTab->nwave * sizeof(pInRec->jyx[0]));
  }
  return pOutTab;
}

/**
 * Make deep copy of a OI_VIS table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_vis *dup_oi_vis(const oi_vis *pInTab)
{
  oi_vis *pOutTab;
  oi_vis_record *pInRec, *pOutRec;
  int i;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->record, pInTab->record,
         pInTab->numrec * sizeof(pInTab->record[0]));
  for (i = 0; i < pInTab->numrec; i++) {
    pOutRec = &pOutTab->record[i];
    pInRec = &pInTab->record[i];
    MEMDUP(pOutRec->visamp, pInRec->visamp,
           pInTab->nwave * sizeof(pInRec->visamp[0]));
    MEMDUP(pOutRec->visamperr, pInRec->visamperr,
           pInTab->nwave * sizeof(pInRec->visamperr[0]));
    MEMDUP(pOutRec->visphi, pInRec->visphi,
           pInTab->nwave * sizeof(pInRec->visphi[0]));
    MEMDUP(pOutRec->visphierr, pInRec->visphierr,
           pInTab->nwave * sizeof(pInRec->visphierr[0]));
    MEMDUP(pOutRec->flag, pInRec->flag,
           pInTab->nwave * sizeof(pInRec->flag[0]));
    if (pInTab->usevisrefmap)
    {
      MEMDUP(pOutRec->visrefmap, pInRec->visrefmap,
             pInTab->nwave * pInTab->nwave * sizeof(pInRec->visrefmap[0]));
    }
    if (pInTab->usecomplex)
    {
      MEMDUP(pOutRec->rvis, pInRec->rvis,
             pInTab->nwave * sizeof(pInRec->rvis[0]));
      MEMDUP(pOutRec->rviserr, pInRec->rviserr,
             pInTab->nwave * sizeof(pInRec->rviserr[0]));
      MEMDUP(pOutRec->ivis, pInRec->ivis,
             pInTab->nwave * sizeof(pInRec->ivis[0]));
      MEMDUP(pOutRec->iviserr, pInRec->rviserr,
             pInTab->nwave * sizeof(pInRec->iviserr[0]));
    }
  }
  return pOutTab;
}

/**
 * Make deep copy of a OI_VIS2 table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_vis2 *dup_oi_vis2(const oi_vis2 *pInTab)
{
  oi_vis2 *pOutTab;
  oi_vis2_record *pInRec, *pOutRec;
  int i;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->record, pInTab->record,
         pInTab->numrec * sizeof(pInTab->record[0]));
  for (i = 0; i < pInTab->numrec; i++) {
    pOutRec = &pOutTab->record[i];
    pInRec = &pInTab->record[i];
    MEMDUP(pOutRec->vis2data, pInRec->vis2data,
           pInTab->nwave * sizeof(pInRec->vis2data[0]));
    MEMDUP(pOutRec->vis2err, pInRec->vis2err,
           pInTab->nwave * sizeof(pInRec->vis2err[0]));
    MEMDUP(pOutRec->flag, pInRec->flag,
           pInTab->nwave * sizeof(pInRec->flag[0]));
  }
  return pOutTab;
}

/**
 * Make deep copy of a OI_T3 table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_t3 *dup_oi_t3(const oi_t3 *pInTab)
{
  oi_t3 *pOutTab;
  oi_t3_record *pInRec, *pOutRec;
  int i;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->record, pInTab->record,
         pInTab->numrec * sizeof(pInTab->record[0]));
  for (i = 0; i < pInTab->numrec; i++) {
    pOutRec = &pOutTab->record[i];
    pInRec = &pInTab->record[i];
    MEMDUP(pOutRec->t3amp, pInRec->t3amp,
           pInTab->nwave * sizeof(pInRec->t3amp[0]));
    MEMDUP(pOutRec->t3amperr, pInRec->t3amperr,
           pInTab->nwave * sizeof(pInRec->t3amperr[0]));
    MEMDUP(pOutRec->t3phi, pInRec->t3phi,
           pInTab->nwave * sizeof(pInRec->t3phi[0]));
    MEMDUP(pOutRec->t3phierr, pInRec->t3phierr,
           pInTab->nwave * sizeof(pInRec->t3phierr[0]));
    MEMDUP(pOutRec->flag, pInRec->flag,
           pInTab->nwave * sizeof(pInRec->flag[0]));
  }
  return pOutTab;
}

/**
 * Make deep copy of a OI_FLUX table
 *
 * @param pInTab  pointer to input table
 *
 * @return Pointer to newly-allocated copy of input table
 */
oi_flux *dup_oi_flux(const oi_flux *pInTab)
{
  oi_flux *pOutTab;
  oi_flux_record *pInRec, *pOutRec;
  int i;

  MEMDUP(pOutTab, pInTab, sizeof(*pInTab));
  MEMDUP(pOutTab->record, pInTab->record,
         pInTab->numrec * sizeof(pInTab->record[0]));
  for (i = 0; i < pInTab->numrec; i++) {
    pOutRec = &pOutTab->record[i];
    pInRec = &pInTab->record[i];
    MEMDUP(pOutRec->fluxdata, pInRec->fluxdata,
           pInTab->nwave * sizeof(pInRec->fluxdata[0]));
    MEMDUP(pOutRec->fluxerr, pInRec->fluxerr,
           pInTab->nwave * sizeof(pInRec->fluxerr[0]));
    MEMDUP(pOutRec->flag, pInRec->flag,
           pInTab->nwave * sizeof(pInRec->flag[0]));
  }
  return pOutTab;
}

/**
 * Convert OI_VIS table to OIFITS v2.
 *
 * Zeros TIME values and changes OI_REVN.
 *
 * @param pTab  pointer to table to modify
 */
void upgrade_oi_vis(oi_vis *pTab)
{
  int i;
  
  if (pTab->revision = OI_REVN_V1_VIS) {
    pTab->revision = OI_REVN_V2_VIS;
    for (i = 0; i < pTab->numrec; i++)
      pTab->record[i].time = 0.0;
  }
}

/**
 * Convert OI_VIS table to OIFITS v2.
 *
 * Zeros TIME values and changes OI_REVN.
 *
 * @param pTab  pointer to table to modify
 */
void upgrade_oi_vis2(oi_vis2 *pTab)
{
  int i;
  
  if (pTab->revision = OI_REVN_V1_VIS2) {
    pTab->revision = OI_REVN_V2_VIS2;
    for (i = 0; i < pTab->numrec; i++)
      pTab->record[i].time = 0.0;
  }
}

/**
 * Convert OI_T3 table to OIFITS v2.
 *
 * Zeros TIME values and changes OI_REVN.
 *
 * @param pTab  pointer to table to modify
 */
void upgrade_oi_t3(oi_t3 *pTab)
{
  int i;
  
  if (pTab->revision = OI_REVN_V1_T3) {
    pTab->revision = OI_REVN_V2_T3;
    for (i = 0; i < pTab->numrec; i++)
      pTab->record[i].time = 0.0;
  }
}

