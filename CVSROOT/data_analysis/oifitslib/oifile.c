/* $Id$ */

/**
 * @file oifile.c
 * @ingroup oifile
 *
 * Implementation of file-level API for OIFITS data.
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

#include <fitsio.h>
#include "oifile.h"


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
  while(link != NULL) {
    pArray = (oi_array *) link->data;
    if(strcmp(pArray->arrname, arrname) == 0)
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
  while(link != NULL) {
    pWave = (oi_wavelength *) link->data;
    if(strcmp(pWave->insname, insname) == 0)
      return pWave;
    link = link->next;
  }
  g_warning("Missing OI_WAVELENGTH with INSNAME=%s", insname);
  return NULL;
}

/**
 * Return shortest wavelength in OI_WAVELENGTH table.
 *
 *   @param pWave  pointer to wavelength struct
 *
 *   @return Minimum of eff_wave values /m
 */
static float get_min_wavelength(const oi_wavelength *pWave)
{
  int i;
  float minWave = 1.0e11;

  for(i=0; i<pWave->nwave; i++) {
    if (pWave->eff_wave[i] < minWave) minWave = pWave->eff_wave[i];
  }
  return minWave;
}

/**
 * Return longest wavelength in OI_WAVELENGTH table.
 *
 *   @param pWave  pointer to wavelength struct
 *
 *   @return Maximum of eff_wave values /m
 */
static float get_max_wavelength(const oi_wavelength *pWave)
{
  int i;
  float maxWave = 0.0;

  for(i=0; i<pWave->nwave; i++) {
    if (pWave->eff_wave[i] > maxWave) maxWave = pWave->eff_wave[i];
  }
  return maxWave;
}

/** Generate summary string for each oi_array in GList. */
static void format_array_list_summary(GString *pGStr, GList *arrayList)
{
  GList *link;
  oi_array *pArray;

  link = arrayList;
  while(link != NULL) {
    pArray = (oi_array *) link->data;
    g_string_append_printf(pGStr,
			   "    ARRNAME='%s'  %d elements\n",
			   pArray->arrname, pArray->nelement);
    link = link->next;
  }
}    

/** Generate summary string for each oi_wavelength in GList. */
static void format_wavelength_list_summary(GString *pGStr, GList *waveList)
{
  GList *link;
  oi_wavelength *pWave;

  link = waveList;
  while(link != NULL) {
    pWave = (oi_wavelength *) link->data;
    g_string_append_printf(pGStr,
			   "    INSNAME='%s'  %d channels  %7.1f-%7.1fnm\n",
			   pWave->insname, pWave->nwave,
			   1e9*get_min_wavelength(pWave),
			   1e9*get_max_wavelength(pWave));
    link = link->next;
  }
}    


/*
 * Public functions
 */

/**
 * Initialise empty oi_fits struct.
 *
 *   @param pOi  pointer to file data struct, see oifile.h
 */
void init_oi_fits(oi_fits *pOi)
{
  pOi->numArray = 0;
  pOi->numWavelength = 0;
  pOi->numVis = 0;
  pOi->numVis2 = 0;
  pOi->numT3 = 0;
  pOi->arrayList = NULL;
  pOi->wavelengthList = NULL;
  pOi->visList = NULL;
  pOi->vis2List = NULL;
  pOi->t3List = NULL;
  pOi->arrayHash = 
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
  pOi->wavelengthHash =
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
}

/** Macro to write FITS table for each oi_* in GList. */
#define WRITE_OI_LIST(fptr, list, type, link, write_func, \
                      extver, pStatus) \
  link = list; \
  extver = 1; \
  while(link != NULL) { \
    write_func(fptr, *((type *) link->data), extver++, pStatus); \
    link = link->next; \
  }

/**
 * Write OIFITS tables to new FITS file.
 *
 *   @param filename  name of file to create
 *   @param oi        file data struct, see oifile.h
 *   @param pStatus   pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus)
 */
int write_oi_fits(const char *filename, oi_fits oi, int *pStatus)
{
  const char function[] = "write_oi_fits";
  fitsfile *fptr;
  GList *link;
  int extver;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  /* Open new FITS file */
  fits_create_file(&fptr, filename, pStatus);
  if(*pStatus) goto except;

  /* Write OI_TARGET table */
  write_oi_target(fptr, oi.targets, pStatus);

  /* Write all OI_ARRAY tables */
  WRITE_OI_LIST(fptr, oi.arrayList, oi_array, link,
		write_oi_array, extver, pStatus);

  /* Write all OI_WAVELENGTH tables */
  WRITE_OI_LIST(fptr, oi.wavelengthList, oi_wavelength, link,
		write_oi_wavelength, extver, pStatus);

  /* Write all data tables */
  WRITE_OI_LIST(fptr, oi.visList, oi_vis, link,
		write_oi_vis, extver, pStatus);
  WRITE_OI_LIST(fptr, oi.vis2List, oi_vis2, link,
		write_oi_vis2, extver, pStatus);
  WRITE_OI_LIST(fptr, oi.t3List, oi_t3, link,
		write_oi_t3, extver, pStatus);
  
  fits_close_file(fptr, pStatus);

 except:
  if (*pStatus) {
    fprintf(stderr, "CFITSIO error in %s:\n", function);
    fits_report_error(stderr, *pStatus);
  }
  return *pStatus;
}

/**
 * Read all OIFITS tables from FITS file.
 * :TODO: mechanism to hush error reports - global variable?
 *
 *   @param filename  name of file to read
 *   @param pOi       pointer to uninitialised file data struct, see oifile.h
 *   @param pStatus   pointer to status variable
 *
 *   @return On error, returns non-zero cfitsio error code (also assigned to
 *           *pStatus). Contents of file data struct are undefined
 */
int read_oi_fits(const char *filename, oi_fits *pOi, int *pStatus)
{
  const char function[] = "read_oi_fits";
  fitsfile *fptr;
  int hdutype;
  oi_array *pArray;
  oi_wavelength *pWave;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;

  if (*pStatus) return *pStatus; /* error flag set - do nothing */

  fits_open_file(&fptr, filename, READONLY, pStatus);
  if (*pStatus) goto except;

  /* Create empty data structures */
  pOi->arrayHash = 
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
  pOi->wavelengthHash =
    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
  pOi->arrayList = NULL;
  pOi->wavelengthList = NULL;
  pOi->visList = NULL;
  pOi->vis2List = NULL;
  pOi->t3List = NULL;

  /* Read compulsory OI_TARGET table */
  read_oi_target(fptr, &pOi->targets, pStatus);
  if (*pStatus) goto except;

  /* Read all OI_ARRAY tables */
  pOi->numArray = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (1==1) {
    pArray = malloc(sizeof(oi_array));
    if (read_next_oi_array(fptr, pArray, pStatus))
      break; /* no more OI_ARRAY */
    pOi->arrayList = g_list_append(pOi->arrayList, pArray);
    ++pOi->numArray;
  }
  free(pArray);
  if(*pStatus != END_OF_FILE) goto except;
  *pStatus = 0; /* reset EOF */

  /* Read all OI_WAVELENGTH tables */
  pOi->numWavelength = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (1==1) {
    pWave = malloc(sizeof(oi_wavelength));
    if (read_next_oi_wavelength(fptr, pWave, pStatus))
      break; /* no more OI_WAVELENGTH */
    pOi->wavelengthList = g_list_append(pOi->wavelengthList, pWave);
    ++pOi->numWavelength;
  }
  free(pWave);
  if(*pStatus != END_OF_FILE) goto except;
  *pStatus = 0; /* reset EOF */

  /* Read all OI_VIS, hash-tabling corresponding array & wavelength tables */
  pOi->numVis = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (1==1) {
    pVis = malloc(sizeof(oi_vis));
    if (read_next_oi_vis(fptr, pVis, pStatus)) break; /* no more OI_VIS */
    pOi->visList = g_list_append(pOi->visList, pVis);
    ++pOi->numVis;
    if (strlen(pVis->arrname) > 0) {
      if(!g_hash_table_lookup(pOi->arrayHash, pVis->arrname))
	g_hash_table_insert(pOi->arrayHash, pVis->arrname,
			    find_oi_array(pOi, pVis->arrname));
    }
    if(!g_hash_table_lookup(pOi->wavelengthHash, pVis->insname))
      g_hash_table_insert(pOi->wavelengthHash, pVis->insname,
			  find_oi_wavelength(pOi, pVis->insname));
  }
  free(pVis);
  if(*pStatus != END_OF_FILE) goto except;
  *pStatus = 0; /* reset EOF */

  /* Read all OI_VIS2, hash-tabling corresponding array & wavelength tables */
  pOi->numVis2 = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (1==1) {
    pVis2 = malloc(sizeof(oi_vis2));
    if (read_next_oi_vis2(fptr, pVis2, pStatus)) break; /* no more OI_VIS2 */
    pOi->vis2List = g_list_append(pOi->vis2List, pVis2);
    ++pOi->numVis2;
    if (strlen(pVis2->arrname) > 0) {
      if(!g_hash_table_lookup(pOi->arrayHash, pVis2->arrname))
	g_hash_table_insert(pOi->arrayHash, pVis2->arrname,
			    find_oi_array(pOi, pVis2->arrname));
    }
    if(!g_hash_table_lookup(pOi->wavelengthHash, pVis2->insname))
      g_hash_table_insert(pOi->wavelengthHash, pVis2->insname,
			  find_oi_wavelength(pOi, pVis2->insname));
  }
  free(pVis2);
  if(*pStatus != END_OF_FILE) goto except;
  *pStatus = 0; /* reset EOF */

  /* Read all OI_T3, hash-tabling corresponding array & wavelength tables */
  pOi->numT3 = 0;
  fits_movabs_hdu(fptr, 1, &hdutype, pStatus); /* back to start */
  while (1==1) {
    pT3 = malloc(sizeof(oi_t3));
    if (read_next_oi_t3(fptr, pT3, pStatus)) break; /* no more OI_T3 */
    pOi->t3List = g_list_append(pOi->t3List, pT3);
    ++pOi->numT3;
    if (strlen(pT3->arrname) > 0) {
      if(!g_hash_table_lookup(pOi->arrayHash, pT3->arrname))
	g_hash_table_insert(pOi->arrayHash, pT3->arrname,
			    find_oi_array(pOi, pT3->arrname));
    }
    if(!g_hash_table_lookup(pOi->wavelengthHash, pT3->insname))
      g_hash_table_insert(pOi->wavelengthHash, pT3->insname,
			  find_oi_wavelength(pOi, pT3->insname));
  }
  free(pT3);
  if(*pStatus != END_OF_FILE) goto except;
  *pStatus = 0; /* reset EOF */

  fits_close_file(fptr, pStatus);

 except:
  if (*pStatus) {
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
  while(link != NULL) {
    if(internalFree)
      (*internalFree)(link->data);
    free(link->data);
    link = link->next;
  }
  g_list_free(list);

}

/**
 * Free storage used for OIFITS data.
 *
 *   @param pOi     pointer to file data struct, see oifile.h
 */
void free_oi_fits(oi_fits *pOi)
{
  g_hash_table_destroy(pOi->arrayHash);
  g_hash_table_destroy(pOi->wavelengthHash);
  free_oi_target(&pOi->targets);
  free_list(pOi->arrayList,
	    (free_func) free_oi_array);
  free_list(pOi->wavelengthList,
	    (free_func) free_oi_wavelength);
  free_list(pOi->visList,
	    (free_func) free_oi_vis);
  free_list(pOi->vis2List,
	    (free_func) free_oi_vis2);
  free_list(pOi->t3List,
	    (free_func) free_oi_t3);
}

/**
 * Return oi_array corresponding to specified ARRNAME.
 *
 *   @param pOi      pointer to file data struct, see oifile.h
 *   @param arrname  value of ARRNAME keyword
 *
 *   @return pointer to oi_array matching arrname, or NULL if no match
 */
oi_array *oi_fits_lookup_array(const oi_fits *pOi, const char *arrname)
{
  return (oi_array *) g_hash_table_lookup(pOi->arrayHash, arrname);
}

/**
 * Lookup array element corresponding to specified ARRNAME & STA_INDEX.
 *
 *   @param pOi       pointer to file data struct, see oifile.h
 *   @param arrname   value of ARRNAME keyword
 *   @param staIndex  value of STA_INDEX from data table
 *
 *   @return ptr to 1st target struct matching targetId, or NULL if no match
 */
element *oi_fits_lookup_element(const oi_fits *pOi,
				const char *arrname, int staIndex)
{
  int i;
  oi_array *pArray;

  pArray = oi_fits_lookup_array(pOi, arrname);
  if (pArray == NULL) return NULL;
  /* We don't assume records are ordered by STA_INDEX */
  for(i=0; i<pArray->nelement; i++) {
    if(pArray->elem[i].sta_index == staIndex)
      return &pArray->elem[i];
  }
  return NULL;
}

/**
 * Lookup oi_wavelength corresponding to specified INSNAME.
 *
 *   @param pOi      pointer to file data struct, see oifile.h
 *   @param insname  value of INSNAME keyword
 *
 *   @return pointer to oi_wavelength matching insname, or NULL if no match
 */
oi_wavelength *oi_fits_lookup_wavelength(const oi_fits *pOi,
					 const char *insname)
{
  return (oi_wavelength *) g_hash_table_lookup(pOi->wavelengthHash, insname);
}

/**
 * Lookup target record corresponding to specified TARGET_ID.
 *
 *   @param pOi       pointer to file data struct, see oifile.h
 *   @param targetId  value of TARGET_ID from data table
 *
 *   @return ptr to 1st target struct matching targetId, or NULL if no match
 */
target *oi_fits_lookup_target(const oi_fits *pOi, int targetId)
{
  int i;

  /* We don't assume records are ordered by TARGET_ID */
  for(i=0; i<pOi->targets.ntarget; i++) {
    if(pOi->targets.targ[i].target_id == targetId)
      return &pOi->targets.targ[i];
  }
  return NULL;
}

  
/** Macro to generate summary string for each oi_vis/vis2/t3 in GList. */
#define FORMAT_OI_LIST_SUMMARY(pGStr, list, type, link) \
  { link = list; \
    while(link != NULL) { \
      g_string_append_printf( \
       pGStr, \
       "    INSNAME='%s'  ARRNAME='%s'  DATE_OBS=%s\n" \
       "     %5ld records x %3d wavebands\n", \
       ((type *) (link->data))->insname,  \
       ((type *) (link->data))->arrname,  \
       ((type *) (link->data))->date_obs, \
       ((type *) (link->data))->numrec,   \
       ((type *) (link->data))->nwave);   \
      link = link->next; \
    } \
  }

/**
 * Generate file summary string.
 *
 * @param pOi  pointer to oi_fits struct
 *
 * @return String summarising supplied dataset
 */
const char *format_oi_fits_summary(const oi_fits *pOi)
{
  GList *link;

  if (pGStr == NULL)
    pGStr = g_string_sized_new(512);

  g_string_printf(pGStr, "OIFITS data:\n");
  g_string_append_printf(pGStr, "  %d OI_ARRAY tables:\n", pOi->numArray);
  format_array_list_summary(pGStr, pOi->arrayList);
  g_string_append_printf(pGStr, "  %d OI_WAVELENGTH tables:\n",
			 pOi->numWavelength);
  format_wavelength_list_summary(pGStr, pOi->wavelengthList);
  g_string_append_printf(pGStr, "  %d OI_VIS tables:\n", pOi->numVis);
  FORMAT_OI_LIST_SUMMARY(pGStr, pOi->visList, oi_vis, link);
  g_string_append_printf(pGStr, "  %d OI_VIS2 tables:\n", pOi->numVis2);
  FORMAT_OI_LIST_SUMMARY(pGStr, pOi->vis2List, oi_vis2, link);
  g_string_append_printf(pGStr, "  %d OI_T3 tables:\n", pOi->numT3);
  FORMAT_OI_LIST_SUMMARY(pGStr, pOi->t3List, oi_t3, link);

  return pGStr->str;
}

/**
 * Print file summary to stdout.
 *
 * @param pOi  pointer to oi_fits struct
 */
void print_oi_fits_summary(const oi_fits *pOi)
{
  printf("%s", format_oi_fits_summary(pOi));
}

/**
 * Make deep copy of a OI_TARGET table.
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
	 pInTab->ntarget*sizeof(pInTab->targ[0]));
  return pOutTab;
}
/**
 * Make deep copy of a OI_ARRAY table.
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
	 pInTab->nelement*sizeof(pInTab->elem[0]));
  return pOutTab;
}

/**
 * Make deep copy of a OI_WAVELENGTH table.
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
	 pInTab->nwave*sizeof(pInTab->eff_wave[0]));
  MEMDUP(pOutTab->eff_band, pInTab->eff_band,
	 pInTab->nwave*sizeof(pInTab->eff_band[0]));
  return pOutTab;
}

/**
 * Make deep copy of a OI_VIS table.
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
	 pInTab->numrec*sizeof(pInTab->record[0]));
  for(i=0; i<pInTab->numrec; i++) {
    pOutRec = &pOutTab->record[i];
    pInRec = &pInTab->record[i];
    MEMDUP(pOutRec->visamp, pInRec->visamp,
	   pInTab->nwave*sizeof(pInRec->visamp[0]));
    MEMDUP(pOutRec->visamperr, pInRec->visamperr,
	   pInTab->nwave*sizeof(pInRec->visamperr[0]));
    MEMDUP(pOutRec->visphi, pInRec->visphi,
	   pInTab->nwave*sizeof(pInRec->visphi[0]));
    MEMDUP(pOutRec->visphierr, pInRec->visphierr,
	   pInTab->nwave*sizeof(pInRec->visphierr[0]));
    MEMDUP(pOutRec->flag, pInRec->flag,
	   pInTab->nwave*sizeof(pInRec->flag[0]));
  }
  return pOutTab;
}

/**
 * Make deep copy of a OI_VIS2 table.
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
	 pInTab->numrec*sizeof(pInTab->record[0]));
  for(i=0; i<pInTab->numrec; i++) {
    pOutRec = &pOutTab->record[i];
    pInRec = &pInTab->record[i];
    MEMDUP(pOutRec->vis2data, pInRec->vis2data,
	   pInTab->nwave*sizeof(pInRec->vis2data[0]));
    MEMDUP(pOutRec->vis2err, pInRec->vis2err,
	   pInTab->nwave*sizeof(pInRec->vis2err[0]));
    MEMDUP(pOutRec->flag, pInRec->flag,
	   pInTab->nwave*sizeof(pInRec->flag[0]));
  }
  return pOutTab;
}

/**
 * Make deep copy of a OI_T3 table.
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
	 pInTab->numrec*sizeof(pInTab->record[0]));
  for(i=0; i<pInTab->numrec; i++) {
    pOutRec = &pOutTab->record[i];
    pInRec = &pInTab->record[i];
    MEMDUP(pOutRec->t3amp, pInRec->t3amp,
	   pInTab->nwave*sizeof(pInRec->t3amp[0]));
    MEMDUP(pOutRec->t3amperr, pInRec->t3amperr,
	   pInTab->nwave*sizeof(pInRec->t3amperr[0]));
    MEMDUP(pOutRec->t3phi, pInRec->t3phi,
	   pInTab->nwave*sizeof(pInRec->t3phi[0]));
    MEMDUP(pOutRec->t3phierr, pInRec->t3phierr,
	   pInTab->nwave*sizeof(pInRec->t3phierr[0]));
    MEMDUP(pOutRec->flag, pInRec->flag,
	   pInTab->nwave*sizeof(pInRec->flag[0]));
  }
  return pOutTab;
}
