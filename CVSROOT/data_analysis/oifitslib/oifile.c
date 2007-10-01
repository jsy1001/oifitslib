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
  printf("Missing OI_ARRAY with ARRNAME=%s\n", arrname);
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
  printf("Missing OI_WAVELENGTH with INSNAME=%s\n", insname);
  return NULL;
}

/**
 * Read all OIFITS tables from FITS file.
 *
 *   @param filename  name of file to read
 *   @param pOi       pointer to file data struct, see oifile.h
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

/** Macro to generate info for each oi_vis/vis2/t3 in GList. */
#define OI_LIST_FORMAT_SUMMARY(pGStr, list, type, link) \
  { link = list; \
    while(link != NULL) { \
      g_string_append_printf( \
        pGStr, "  %5ld records x %3d wavebands  INSNAME=%s  ARRNAME=%s\n", \
       ((type *) (link->data))->numrec,   \
       ((type *) (link->data))->nwave,    \
       ((type *) (link->data))->insname,  \
       ((type *) (link->data))->arrname); \
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
char *oi_fits_format_summary(const oi_fits *pOi)
{
  GList *link;

  if (pGStr == NULL)
    pGStr = g_string_sized_new(256);

  g_string_printf(pGStr, "\n");
  /* :TODO: wavelength ranges */
  g_string_append_printf(pGStr, "%d OI_VIS tables:\n", pOi->numVis);
  OI_LIST_FORMAT_SUMMARY(pGStr, pOi->visList, oi_vis, link);
  g_string_append_printf(pGStr, "%d OI_VIS2 tables:\n", pOi->numVis2);
  OI_LIST_FORMAT_SUMMARY(pGStr, pOi->vis2List, oi_vis2, link);
  g_string_append_printf(pGStr, "%d OI_T3 tables:\n", pOi->numT3);
  OI_LIST_FORMAT_SUMMARY(pGStr, pOi->t3List, oi_t3, link);

  return pGStr->str;
}

/**
 * Print file summary to stdout.
 *
 * @param pOi  pointer to oi_fits struct
 */
void oi_fits_print_summary(const oi_fits *pOi)
{
  printf("%s", oi_fits_format_summary(pOi));
}
