/**
 * @file
 * @ingroup oicheck
 *
 * Implementation of OIFITS conformity checker.
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

/* :TODO: integrate fitsverify? */
/* :TODO: check_tables_present() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oicheck.h"


/** Internal use GString, defined in oifile.c */
extern GString *pGStr;

/** Descriptions for oi_breach_level values */
const char *const oi_breach_level_desc[] = {
  "No error",
  "Valid OIFITS, but may cause problems for some reading software",
  "Does not conform to the OIFITS standard",
  "Does not conform to the FITS standard"
};


/**
 * Initialise check result struct.
 *
 * @param pResult  pointer to check result struct to initialise
 */
void init_check_result(oi_check_result *pResult)
{
  int i;

  pResult->level = OI_BREACH_NONE;
  pResult->description = NULL;
  pResult->numBreach = 0;
  for(i=0; i<MAX_REPORT; i++)
    pResult->location[i] = NULL;
  pResult->chunk = g_string_chunk_new(100*MAX_REPORT);
}

/** Record where a breach of the OIFITS standard has occurred. */
static void set_result(oi_check_result *pResult, oi_breach_level level,
		       const char *description, const char *location)
{
  if(level > pResult->level)
    pResult->level = level;
  if(pResult->description == NULL)
    pResult->description = g_string_chunk_insert(pResult->chunk, description);
  if(++pResult->numBreach < MAX_REPORT) {
    pResult->location[pResult->numBreach-1] =
      g_string_chunk_insert(pResult->chunk, location);
  } else if (pResult->numBreach == MAX_REPORT) {
    pResult->location[MAX_REPORT-1] =
      g_string_chunk_insert(pResult->chunk, "[List truncated]");
  }
}

/**
 * Free dynamically-allocated storage within check result struct.
 *
 * @param pResult  pointer to check result struct
 */
void free_check_result(oi_check_result *pResult)
{
  g_string_chunk_free(pResult->chunk);
}

/**
 * Return string describing check result.
 *
 * @param pResult  pointer to check result struct
 *
 * @return String describing result of check, or NULL if no error
 */
char *format_check_result(oi_check_result *pResult)
{
  int n, i;

  if(pResult->level == OI_BREACH_NONE)
    return NULL;

  if (pGStr == NULL)
    pGStr = g_string_sized_new(256);

  g_string_printf(pGStr, "*** %s:\n%s, %d occurrences:-\n",
		  oi_breach_level_desc[pResult->level],
		  pResult->description, pResult->numBreach);
  n = (pResult->numBreach < MAX_REPORT) ? pResult->numBreach : MAX_REPORT;
  for(i=0; i<n; i++)
    g_string_append_printf(pGStr, "    %s\n", pResult->location[i]);

  return pGStr->str;
}

/**
 * Print check result to stdout.
 *
 * @param pResult  pointer to check result struct
 */
void print_check_result(oi_check_result *pResult)
{
  printf("%s", format_check_result(pResult));
}

/**
 * Check targets have unique identifiers
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach level indicating overall test result
 */
oi_breach_level check_unique_targets(oi_fits *pOi, oi_check_result *pResult)
{
  int i;
  GList *idList;
  target *pTarget;
  const char desc[] = "Duplicate value in TARGET column of OI_TARGET";
  char location[FLEN_VALUE];

  init_check_result(pResult);
  idList = NULL;
  for(i=0; i<pOi->targets.ntarget; i++) {
    pTarget = &pOi->targets.targ[i];
    if(g_list_find_custom(idList, pTarget->target,
			  (GCompareFunc) strcmp) != NULL) {
      /* Duplicate TARGET value */
      g_snprintf(location, FLEN_VALUE, "TARGET_ID=%d  TARGET='%s'",
		 pTarget->target_id, pTarget->target);
      set_result(pResult, OI_BREACH_WARNING, desc, location);
    } else {
      /* prepend to list as faster than appending and order doesn't matter */
      idList = g_list_prepend(idList, pTarget->target);
    }
  }

  g_list_free(idList);
  return pResult->level;
}

/**
 * Check all referenced targets are present in OI_TARGET.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach level indicating overall test result
 */
oi_breach_level check_targets_present(oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  int i;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  const char desc[] = "Reference to missing target record";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  /* Check OI_VIS tables */
  link = pOi->visList;
  while(link != NULL) {
    pVis = link->data;
    for(i=0; i<pVis->numrec; i++) {
      if(oi_fits_lookup_target(pOi, pVis->record[i].target_id) == NULL) {
	g_snprintf(location, FLEN_VALUE, "OI_VIS #%d record %d",
		   g_list_position(pOi->visList, link)+1, i+1);
	set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    }
    link = link->next;
  }

  /* Check OI_VIS2 tables */
  link = pOi->vis2List;
  while(link != NULL) {
    pVis2 = link->data;
    for(i=0; i<pVis2->numrec; i++) {
      if(oi_fits_lookup_target(pOi, pVis2->record[i].target_id) == NULL) {
	g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d record %d",
		   g_list_position(pOi->vis2List, link)+1, i+1);
	set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    }
    link = link->next;
  }

  /* Check OI_T3 tables */
  link = pOi->t3List;
  while(link != NULL) {
    pT3 = link->data;
    for(i=0; i<pT3->numrec; i++) {
      if(oi_fits_lookup_target(pOi, pT3->record[i].target_id) == NULL) {
	g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d",
		   g_list_position(pOi->t3List, link)+1, i+1);
	set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    }
    link = link->next;
  }

  return pResult->level;
}


/**
 * Check all referenced array elements are present.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach level indicating overall test result
 */
oi_breach_level check_elements_present(oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  int i, j;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  const char desc[] = "Reference to missing array element";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  /* Check OI_VIS tables */
  link = pOi->visList;
  while(link != NULL) {
    pVis = link->data;
    if(strlen(pVis->arrname) > 0) {
      for(i=0; i<pVis->numrec; i++) {
	for(j=0; j<2; j++) {
	  if(oi_fits_lookup_element(pOi, pVis->arrname,
				    pVis->record[i].sta_index[j]) == NULL) {
	    g_snprintf(location, FLEN_VALUE, "OI_VIS #%d record %d",
		       g_list_position(pOi->visList, link)+1, i+1);
	    set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
	  }
	}
      }
    }
    link = link->next;
  }

  /* Check OI_VIS2 tables */
  link = pOi->vis2List;
  while(link != NULL) {
    pVis2 = link->data;
    if(strlen(pVis2->arrname) > 0) {
      for(i=0; i<pVis2->numrec; i++) {
	for(j=0; j<2; j++) {
	  if(oi_fits_lookup_element(pOi, pVis2->arrname,
				    pVis2->record[i].sta_index[j]) == NULL) {
	    g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d record %d",
		       g_list_position(pOi->vis2List, link)+1, i+1);
	    set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
	  }
	}
      }
    }
    link = link->next;
  }

  /* Check OI_T3 tables */
  link = pOi->t3List;
  while(link != NULL) {
    pT3 = link->data;
    if(strlen(pT3->arrname) > 0) {
      for(i=0; i<pT3->numrec; i++) {
	for(j=0; j<3; j++) {
	  if(oi_fits_lookup_element(pOi, pT3->arrname,
				    pT3->record[i].sta_index[j]) == NULL) {
	    g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d",
		       g_list_position(pOi->t3List, link)+1, i+1);
	    set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
	  }
	}
      }
    }
    link = link->next;
  }

  return pResult->level;
}

/**
 * Check for negative error bars.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach level indicating overall test result
 */
oi_breach_level check_flagging(oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  int i, j;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  const char desc[] = "Data table contains negative error bar";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  /* Check OI_VIS tables */
  link = pOi->visList;
  while(link != NULL) {
    pVis = link->data;
    for(i=0; i<pVis->numrec; i++) {
      for(j=0; j<pVis->nwave; j++) {
        if(pVis->record[i].flag[j]) continue;
	if(pVis->record[i].visamperr[j] < 0. ||
	   pVis->record[i].visphierr[j] < 0.) {
	  g_snprintf(location, FLEN_VALUE, "OI_VIS #%d record %d channel %d",
		     g_list_position(pOi->visList, link)+1, i+1, j+1);
	  set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
	}
      }
    }
    link = link->next;
  }

  /* Check OI_VIS2 tables */
  link = pOi->vis2List;
  while(link != NULL) {
    pVis2 = link->data;
    for(i=0; i<pVis2->numrec; i++) {
      for(j=0; j<pVis2->nwave; j++) {
        if(pVis2->record[i].flag[j]) continue;
	if(pVis2->record[i].vis2err[j] < 0.) {
	  g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d record %d channel %d",
		     g_list_position(pOi->vis2List, link)+1, i+1, j+1);
	  set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
	}
      }
    }
    link = link->next;
  }

  /* Check OI_T3 tables */
  link = pOi->t3List;
  while(link != NULL) {
    pT3 = link->data;
    for(i=0; i<pT3->numrec; i++) {
      for(j=0; j<pT3->nwave; j++) {
        if(pT3->record[i].flag[j]) continue;
	if(pT3->record[i].t3amperr[j] < 0. ||
	   pT3->record[i].t3phierr[j] < 0.) {
	  g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d channel %d",
		     g_list_position(pOi->t3List, link)+1, i+1, j+1);
	  set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
	}
      }
    }
    link = link->next;
  }

  return pResult->level;
}

/**
 * Check for unnormalised (i.e. significantly > 1) T3AMP values.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach level indicating overall test result
 */
oi_breach_level check_t3amp(oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  int i, j;
  oi_t3 *pT3;
  oi_t3_record t3Rec;
  const char desc[] =
    "OI_T3 table may contain unnormalised triple product amplitude";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  link = pOi->t3List;
  while(link != NULL) {
    pT3 = link->data;
    for(i=0; i<pT3->numrec; i++) {
      t3Rec = pT3->record[i];
      for(j=0; j<pT3->nwave; j++) {
        if(t3Rec.flag[j]) continue;
	/* use one sigma in case error bars are overestimated */
	if((t3Rec.t3amp[j] - 1.0) > 1*t3Rec.t3amperr[j]) {
	  g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d channel %d",
		     g_list_position(pOi->t3List, link)+1, i+1, j+1);
	  set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
	}
      }
    }
    link = link->next;
  }

  return pResult->level;
}

/**
 * Check for un-ordered wavelength values in OI_WAVELENGTH.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach level indicating overall test result
 */
oi_breach_level check_waveorder(oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  int i;
  oi_wavelength *pWave;
  const char desc[] = "OI_WAVELENGTH has wavelengths not in ascending order";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  link = pOi->wavelengthList;
  while(link != NULL) {
    pWave = link->data;
    for(i=1; i<pWave->nwave; i++) {
      if (pWave->eff_wave[i] < pWave->eff_wave[i-1] ||
	  (i+1 < pWave->nwave && pWave->eff_wave[i] > pWave->eff_wave[i+1])) {
	g_snprintf(location, FLEN_VALUE, "OI_WAVELENGTH INSNAME=%s channel %d",
		   pWave->insname, i+1);
	set_result(pResult, OI_BREACH_WARNING, desc, location);
      }
    }
    link = link->next;
  }

  return pResult->level;
}
