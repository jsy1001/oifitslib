/**
 * @file
 * @ingroup oimerge
 * Implementation of merge component of OIFITSlib.
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

#include "oimerge.h"
#include "datemjd.h"

#include <string.h>
#include <assert.h>
#include <math.h>


/*
 * Private functions
 */

/**
 * Return pointer to first oi_wavelength in list that contains
 * identical wavebands (in same order) to pWave.
 */
static oi_wavelength *match_oi_wavelength(const oi_wavelength *pWave,
                                          const GList *list)
{
  const GList *link;
  oi_wavelength *pCmp;
  double tol = 1e-10;
  int i;

  link = list;
  while(link != NULL) {
    pCmp = (oi_wavelength *) link->data;
    if (pCmp->nwave == pWave->nwave) {
      for(i=0; i<pWave->nwave; i++) {
	if (fabs(pCmp->eff_wave[i] - pWave->eff_wave[i]) >= tol ||
	    fabs(pCmp->eff_band[i] - pWave->eff_band[i]) >= tol)
	  break;
      }
      if(i == pWave->nwave)
        return pCmp;  /* all wavebands match */
    }
    link = link->next;
  }
  return NULL;
}

/**
 * Return midpoint of primary header DATE-OBS range in @a list as MJD.
 */
static long mid_mjd(const GList *list)
{
  const GList *link;
  oi_header *pHeader;
  long year, month, day, mjd, minMjd, maxMjd;

  minMjd = 100000;
  maxMjd = 0;
  link = list;
  while (link != NULL) {
    pHeader = &((oi_fits *) link->data)->header;
    if (sscanf(pHeader->date_obs, "%4ld-%2ld-%2ld", &year, &month, &day) == 3)
    {
      mjd = date2mjd(year, month, day);
      if (mjd < minMjd)
        minMjd = mjd;
      if (mjd > maxMjd)
        maxMjd = mjd;
    }
    //:TODO: handle files without DATE-OBS
    link = link->next;
  }
  return (minMjd + maxMjd)/2;
}


/*
 * Public functions
 */

/** Merge specified primary header keyword */
#define MERGE_HEADER_KEY(inList, keyattr, pOutput)                      \
  {                                                                     \
    int nuniq;                                                          \
    const GList *link;                                                  \
    oi_header *pInHeader;                                               \
    nuniq = 0;                                                          \
    link = inList;                                                      \
    while (link != NULL) {                                              \
      pInHeader = &((oi_fits *) link->data)->header;                    \
      if (strlen(pInHeader->keyattr) > 0 &&                             \
          strcmp(pInHeader->keyattr, pOutput->header.keyattr) != 0)     \
      {                                                                 \
        if (++nuniq > 1)                                                \
          break;                                                        \
        g_strlcpy(pOutput->header.keyattr, pInHeader->keyattr,          \
                  FLEN_VALUE);                                          \
      }                                                                 \
      link = link->next;                                                \
    }                                                                   \
    if (nuniq > 1)                                                      \
      g_strlcpy(pOutput->header.keyattr, "MULTIPLE", FLEN_VALUE);       \
  }

/**
 * Merge primary header keywords
 *
 * @param inList   linked list of oi_fits structs to merge
 * @param pOutput  pointer to initialised struct to write merged data to
 */
void merge_oi_header(const GList *inList, oi_fits *pOutput)
{
  long year, month, day;

  assert(pOutput->header.origin[0] == '\0');

  mjd2date(mid_mjd(inList), &year, &month, &day);
  g_snprintf(pOutput->header.date_obs, FLEN_VALUE,
             "%4ld-%02ld-%02ld", year, month, day);

  /* Copy unique keywords or replace with "MULTIPLE" */
  MERGE_HEADER_KEY(inList, origin, pOutput);  //:TODO: MULTIPLE ok for ORIGIN?
  MERGE_HEADER_KEY(inList, telescop, pOutput);
  MERGE_HEADER_KEY(inList, instrume, pOutput);
  MERGE_HEADER_KEY(inList, insmode, pOutput);
  MERGE_HEADER_KEY(inList, object, pOutput);
  MERGE_HEADER_KEY(inList, referenc, pOutput);
  MERGE_HEADER_KEY(inList, prog_id, pOutput);
  MERGE_HEADER_KEY(inList, procsoft, pOutput);
  MERGE_HEADER_KEY(inList, obstech, pOutput);
}

/**
 * Copy records for uniquely-named targets into output target table
 *
 * @param inList   linked list of oi_fits structs to merge
 * @param pOutput  pointer to oi_fits struct to write merged data to
 *
 * @return Hash table giving new TARGET_ID indexed by target name
 */
GHashTable *merge_oi_target(const GList *inList, oi_fits *pOutput)
{
  #define MAX_TARGET 100
  GHashTable *targetIdHash;
  const GList *link;
  oi_target *pInTab, *pOutTab;
  int i, *pValue;

  pOutTab = &pOutput->targets;
  pOutTab->revision = 1;
  pOutTab->ntarget = 0;
  pOutTab->targ = malloc(MAX_TARGET*sizeof(target));
  targetIdHash = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free);
  link = inList;
  while(link != NULL) {
    pInTab = &((oi_fits *) link->data)->targets;
    if(pInTab->revision > pOutTab->revision)
      pOutTab->revision = pInTab->revision;
    for(i=0; i<pInTab->ntarget; i++) {
      if(!g_hash_table_lookup(targetIdHash, pInTab->targ[i].target)) {
	pValue = malloc(sizeof(int));
	*pValue = ++pOutTab->ntarget;
	assert(pOutTab->ntarget < MAX_TARGET);
	g_hash_table_insert(targetIdHash, pInTab->targ[i].target, pValue);
	memcpy(&pOutTab->targ[pOutTab->ntarget-1], &pInTab->targ[i],
	       sizeof(target));
	pOutTab->targ[pOutTab->ntarget-1].target_id = pOutTab->ntarget;
      }
    }
    link = link->next;
  }
  pOutTab->targ = realloc(pOutTab->targ, pOutTab->ntarget*sizeof(target));
  return targetIdHash;
}

/**
 * Copy unique wavelength tables into output dataset
 *
 * @param inList   linked list of oi_fits structs to merge
 * @param pOutput  pointer to oi_fits struct to write merged data to
 *
 * @return Linked list of hash tables, giving mapping from old to new
 *         INSNAMEs for each input dataset
 */
GList *merge_all_oi_wavelength(const GList *inList, oi_fits *pOutput)
{
  GList *insnameHashList;
  const GList *waveList, *ilink, *jlink;
  GHashTable *hash;
  oi_wavelength *pInTab, *pOutTab;
  char newName[FLEN_VALUE];
  
  insnameHashList = NULL;
  assert(pOutput->wavelengthList == NULL);
  
  /* Loop over input datasets */
  ilink = inList;
  while(ilink != NULL) {
    /* Append hash table for this dataset to output list */
    hash = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    insnameHashList = g_list_append(insnameHashList, hash);
    waveList = ((oi_fits *) ilink->data)->wavelengthList;
    /* Loop over wavelength tables in current dataset */
    jlink = waveList;
    while(jlink != NULL) {
      pInTab = (oi_wavelength *) jlink->data;
      pOutTab = match_oi_wavelength(pInTab, pOutput->wavelengthList);
      if (pOutTab == NULL) {
	/* Add copy of pInTab to output, changing INSNAME if it clashes */
	pOutTab = dup_oi_wavelength(pInTab);
	if (g_hash_table_lookup(pOutput->wavelengthHash,
				pOutTab->insname) != NULL) {
          /* Avoid truncation at FLEN_VALUE - 1 */
          if(strlen(pOutTab->insname) < (FLEN_VALUE - 5))
            g_snprintf(newName, FLEN_VALUE,
                       "%s_%03d", pOutTab->insname, pOutput->numWavelength + 1);
          else
            g_snprintf(newName, FLEN_VALUE,
                       "ins%03d", pOutput->numWavelength + 1);
          g_strlcpy(pOutTab->insname, newName, FLEN_VALUE);
	}
	g_hash_table_insert(pOutput->wavelengthHash, pOutTab->insname,
			    pOutTab);
	pOutput->wavelengthList = g_list_append(pOutput->wavelengthList,
						pOutTab);
	++pOutput->numWavelength;
      }
      g_hash_table_insert(hash, pInTab->insname, pOutTab->insname);
      jlink = jlink->next;
    }
    ilink = ilink->next;
  }
  return insnameHashList;
}

/**
 * Copy corr tables into output dataset
 *
 * @param inList   linked list of oi_fits structs to merge
 * @param pOutput  pointer to oi_fits struct to write merged data to
 *
 * @return Linked list of hash tables, giving mapping from old to new
 *         CORRNAMEs for each input dataset
 */
GList *merge_all_oi_corr(const GList *inList, oi_fits *pOutput)
{
  GList *corrnameHashList;
  const GList *corrList, *ilink, *jlink;
  GHashTable *hash;
  oi_corr *pInTab, *pOutTab;
  char newName[FLEN_VALUE];
  
  corrnameHashList = NULL;
  assert(pOutput->corrList == NULL);
  
  /* Loop over input datasets */
  ilink = inList;
  while(ilink != NULL) {
    /* Append hash table for this dataset to output list */
    hash = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    corrnameHashList = g_list_append(corrnameHashList, hash);
    corrList = ((oi_fits *) ilink->data)->corrList;
    /* Loop over corr tables in current dataset */
    jlink = corrList;
    while(jlink != NULL) {
      pInTab = (oi_corr *) jlink->data;

      /* Add copy of pInTab to output, changing CORRNAME if it clashes */
      pOutTab = dup_oi_corr(pInTab);
      if (g_hash_table_lookup(pOutput->corrHash, pOutTab->corrname) != NULL) {
        /* Avoid truncation at FLEN_VALUE - 1 */
        if(strlen(pOutTab->corrname) < (FLEN_VALUE - 5))
          g_snprintf(newName, FLEN_VALUE,
                     "%s_%03d", pOutTab->corrname, pOutput->numCorr + 1);
        else
          g_snprintf(newName, FLEN_VALUE, "ins%03d", pOutput->numCorr + 1);
        g_strlcpy(pOutTab->corrname, newName, FLEN_VALUE);
      }
      g_hash_table_insert(pOutput->corrHash, pOutTab->corrname, pOutTab);
      pOutput->corrList = g_list_append(pOutput->corrList, pOutTab);
      ++pOutput->numCorr;

      g_hash_table_insert(hash, pInTab->corrname, pOutTab->corrname);
      jlink = jlink->next;
    }
    ilink = ilink->next;
  }
  return corrnameHashList;
}

/** Replace ARRNAME with empty string */
#define UNSET_ARRNAME(pTab) { pTab->arrname[0] = '\0'; }

/** Replace INSNAME with string from hash table */
#define REPLACE_INSNAME(pTab, oldInsname, insnameHash)                  \
  {                                                                     \
    (void) g_strlcpy(pTab->insname,                                     \
                     g_hash_table_lookup(insnameHash, oldInsname),      \
                     FLEN_VALUE);                                       \
  }

/** Replace CORRNAME with string from hash table */
#define REPLACE_CORRNAME(pTab, oldCorrname, corrnameHash)               \
  {                                                                     \
    if (strlen(oldCorrname) > 0)                                        \
    {                                                                   \
      (void) g_strlcpy(pTab->corrname,                                  \
                       g_hash_table_lookup(corrnameHash, oldCorrname),  \
                       FLEN_VALUE);                                     \
    }                                                                   \
  }

/** Replace TARGET_IDs with values from hash table */
#define REPLACE_TARGET_ID(pTab, pInOi, targetIdHash)                    \
  {                                                                     \
    int ii;                                                             \
    for(ii=0; ii<pTab->numrec; ii++) {                                  \
      /* hash table is indexed by target name */                        \
      pTab->record[ii].target_id =                                      \
      *((int *) g_hash_table_lookup(                                    \
          targetIdHash,                                                 \
          oi_fits_lookup_target(pInOi,                                  \
                                pTab->record[ii].target_id)->target));  \
    }                                                                   \
  }

/**
 * Copy all input OI_VIS tables into output dataset.
 *
 * Modifies INSNAME, CORRNAME and TARGET_ID to maintain correct
 * cross-references.
 *
 * @param inList        linked list of oi_fits structs to merge
 * @param targetIdHash  hash table giving new TARGET_ID indexed by target name
 * @param insnameHashList   list of hash tables giving new INSNAME indexed
 *                          by old
 * @param corrnameHashList  list of hash tables giving new CORRNAME indexed
 *                          by old
 * @param pOutput       pointer to oi_fits struct to write merged data to
 */
void merge_all_oi_vis(const GList *inList, GHashTable *targetIdHash,
		      const GList *insnameHashList,
                      const GList *corrnameHashList, oi_fits *pOutput)
{
  const GList *ilink, *jlink, *insHashLink, *corrHashLink;
  oi_fits *pInput;
  oi_vis *pOutTab;
  GHashTable *insnameHash, *corrnameHash;
  
  /* Loop over input datasets */
  ilink = inList;
  insHashLink = insnameHashList;
  corrHashLink = corrnameHashList;
  while(ilink != NULL) {
    insnameHash = (GHashTable *) insHashLink->data;
    corrnameHash = (GHashTable *) corrHashLink->data;
    pInput = (oi_fits *) ilink->data;
    /* Loop over data tables in dataset */
    jlink = pInput->visList;
    while(jlink != NULL) {
      pOutTab = dup_oi_vis((oi_vis *) jlink->data);
      UNSET_ARRNAME(pOutTab);
      REPLACE_INSNAME(pOutTab, pOutTab->insname, insnameHash);
      REPLACE_CORRNAME(pOutTab, pOutTab->corrname, corrnameHash);
      REPLACE_TARGET_ID(pOutTab, pInput, targetIdHash);
      /* Append modified copy of table to output */
      pOutput->visList = g_list_append(pOutput->visList, pOutTab); 
      ++pOutput->numVis;
      jlink = jlink->next;
    }
    ilink = ilink->next;
    insHashLink = insHashLink->next;
    corrHashLink = corrHashLink->next;
  }
}

/**
 * Copy all input OI_VIS2 tables into output dataset.
 *
 * Modifies INSNAME, CORRNAME and TARGET_ID to maintain correct
 * cross-references.
 *
 * @param inList        linked list of oi_fits structs to merge
 * @param targetIdHash  hash table giving new TARGET_ID indexed by target name
 * @param insnameHashList   list of hash tables giving new INSNAME indexed
 *                          by old
 * @param corrnameHashList  list of hash tables giving new CORRNAME indexed
 *                          by old
 * @param pOutput       pointer to oi_fits struct to write merged data to
 */
void merge_all_oi_vis2(const GList *inList, GHashTable *targetIdHash,
		       const GList *insnameHashList,
                       const GList *corrnameHashList, oi_fits *pOutput)
{
  const GList *ilink, *jlink, *insHashLink, *corrHashLink;
  oi_fits *pInput;
  oi_vis2 *pOutTab;
  GHashTable *insnameHash, *corrnameHash;
  
  /* Loop over input datasets */
  ilink = inList;
  insHashLink = insnameHashList;
  corrHashLink = corrnameHashList;
  while(ilink != NULL) {
    insnameHash = (GHashTable *) insHashLink->data;
    corrnameHash = (GHashTable *) corrHashLink->data;
    pInput = (oi_fits *) ilink->data;
    /* Loop over data tables in dataset */
    jlink = pInput->vis2List;
    while(jlink != NULL) {
      pOutTab = dup_oi_vis2((oi_vis2 *) jlink->data);
      UNSET_ARRNAME(pOutTab);
      REPLACE_INSNAME(pOutTab, pOutTab->insname, insnameHash);
      REPLACE_CORRNAME(pOutTab, pOutTab->corrname, corrnameHash);
      REPLACE_TARGET_ID(pOutTab, pInput, targetIdHash);
      /* Append modified copy of table to output */
      pOutput->vis2List = g_list_append(pOutput->vis2List, pOutTab); 
      ++pOutput->numVis2;
      jlink = jlink->next;
    }
    ilink = ilink->next;
    insHashLink = insHashLink->next;
    corrHashLink = corrHashLink->next;
  }
}

/**
 * Copy all input OI_T3 tables into output dataset.
 *
 * Modifies INSNAME, CORRNAME and TARGET_ID to maintain correct
 * cross-references.
 *
 * @param inList        linked list of oi_fits structs to merge
 * @param targetIdHash  hash table giving new TARGET_ID indexed by target name
 * @param insnameHashList   list of hash tables giving new INSNAME indexed
 *                          by old
 * @param corrnameHashList  list of hash tables giving new CORRNAME indexed
 *                          by old
 * @param pOutput       pointer to oi_fits struct to write merged data to
 */
void merge_all_oi_t3(const GList *inList, GHashTable *targetIdHash,
		     const GList *insnameHashList,
                     const GList *corrnameHashList, oi_fits *pOutput)
{
  const GList *ilink, *jlink, *insHashLink, *corrHashLink;
  oi_fits *pInput;
  oi_t3 *pOutTab;
  GHashTable *insnameHash, *corrnameHash;
  
  /* Loop over input datasets */
  ilink = inList;
  insHashLink = insnameHashList;
  corrHashLink = corrnameHashList;
  while(ilink != NULL) {
    insnameHash = (GHashTable *) insHashLink->data;
    corrnameHash = (GHashTable *) corrHashLink->data;
    pInput = (oi_fits *) ilink->data;
    /* Loop over data tables in dataset */
    jlink = pInput->t3List;
    while(jlink != NULL) {
      pOutTab = dup_oi_t3((oi_t3 *) jlink->data);
      UNSET_ARRNAME(pOutTab);
      REPLACE_INSNAME(pOutTab, pOutTab->insname, insnameHash);
      REPLACE_CORRNAME(pOutTab, pOutTab->corrname, corrnameHash);
      REPLACE_TARGET_ID(pOutTab, pInput, targetIdHash);
      /* Append modified copy of table to output */
      pOutput->t3List = g_list_append(pOutput->t3List, pOutTab); 
      ++pOutput->numT3;
      jlink = jlink->next;
    }
    ilink = ilink->next;
    insHashLink = insHashLink->next;
    corrHashLink = corrHashLink->next;
  }
}

/**
 * Copy all input OI_SPECTRUM tables into output dataset.
 *
 * Modifies INSNAME and TARGET_ID to maintain correct cross-references.
 *
 * @param inList        linked list of oi_fits structs to merge
 * @param targetIdHash  hash table giving new TARGET_ID indexed by target name
 * @param insnameHashList list of hash tables giving new INSNAME indexed by old
 * @param pOutput       pointer to oi_fits struct to write merged data to
 */
void merge_all_oi_spectrum(const GList *inList, GHashTable *targetIdHash,
                           const GList *insnameHashList, oi_fits *pOutput)
{
  const GList *ilink, *jlink, *insHashLink;
  oi_fits *pInput;
  oi_spectrum *pOutTab;
  GHashTable *insnameHash;
  
  /* Loop over input datasets */
  ilink = inList;
  insHashLink = insnameHashList;
  while(ilink != NULL) {
    insnameHash = (GHashTable *) insHashLink->data;
    pInput = (oi_fits *) ilink->data;
    /* Loop over data tables in dataset */
    jlink = pInput->spectrumList;
    while(jlink != NULL) {
      pOutTab = dup_oi_spectrum((oi_spectrum *) jlink->data);
      UNSET_ARRNAME(pOutTab);
      REPLACE_INSNAME(pOutTab, pOutTab->insname, insnameHash);
      REPLACE_TARGET_ID(pOutTab, pInput, targetIdHash);
      /* Append modified copy of table to output */
      pOutput->spectrumList = g_list_append(pOutput->spectrumList, pOutTab); 
      ++pOutput->numSpectrum;
      jlink = jlink->next;
    }
    ilink = ilink->next;
    insHashLink = insHashLink->next;
  }
}

/**
 * Merge list of oi_fits structs into single dataset.
 *
 * @param inList   linked list of oi_fits structs to merge
 * @param pOutput  pointer to oi_fits struct to write merged data to
 */
void merge_oi_fits_list(const GList *inList, oi_fits *pOutput)
{
  GHashTable *targetIdHash;
  GList *insnameHashList, *corrnameHashList, *link;

  init_oi_fits(pOutput);
  merge_oi_header(inList, pOutput);
  targetIdHash = merge_oi_target(inList, pOutput);
  insnameHashList = merge_all_oi_wavelength(inList, pOutput);
  corrnameHashList = merge_all_oi_corr(inList, pOutput);
  merge_all_oi_vis(inList, targetIdHash,
                   insnameHashList, corrnameHashList, pOutput);
  merge_all_oi_vis2(inList, targetIdHash,
                    insnameHashList, corrnameHashList, pOutput);
  merge_all_oi_t3(inList, targetIdHash,
                  insnameHashList, corrnameHashList, pOutput);
  merge_all_oi_spectrum(inList, targetIdHash, insnameHashList, pOutput);

  g_hash_table_destroy(targetIdHash);
  link = insnameHashList;
  while(link != NULL) {
    g_hash_table_destroy((GHashTable *) link->data);
    link = link->next;
  }
  g_list_free(insnameHashList);
  link = corrnameHashList;
  while(link != NULL) {
    g_hash_table_destroy((GHashTable *) link->data);
    link = link->next;
  }
  g_list_free(corrnameHashList);
}

/**
 * Merge supplied oi_fits structs into single dataset.
 *
 * @param pOutput  pointer to oi_fits struct to write merged data to
 * @param pInput1  pointer to first oi_fits struct to merge
 * @param pInput2  pointer to second oi_fits struct to merge,
 *                 followed by pointers to any further structs to merge,
 *                 followed by NULL
 */
void merge_oi_fits(oi_fits *pOutput,
		   oi_fits *pInput1, oi_fits *pInput2,...)
{
  GList *inList;
  va_list ap;
  oi_fits *pInput;

  inList = NULL;
  inList = g_list_append(inList, pInput1);
  inList = g_list_append(inList, pInput2);
  va_start(ap, pInput2);
  pInput = va_arg(ap, oi_fits *);
  while(pInput != NULL) {
    inList = g_list_append(inList, pInput);
    pInput = va_arg(ap, oi_fits *);
  }
  va_end(ap);
  merge_oi_fits_list(inList, pOutput);
  g_list_free(inList);
}
