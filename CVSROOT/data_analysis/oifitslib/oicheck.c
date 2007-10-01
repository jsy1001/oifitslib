/* $Id$ */

/**
 * @file oicheck.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oicheck.h"


/** Internal use GString, defined in oifile.c */
extern GString *pGStr;

/** Descriptions for oi_breach_level values */
char *oi_breach_level_desc[] = {
  "No error",
  "Valid OIFITS, but may cause problems for some reading software",
  "Does not conform to the OIFITS standard",
  "Does not conform to the FITS standard"
};


/**
 * Initialise check result struct
 *
 * @param pResult  pointer to check result struct to initialise
 */
static void init_check_result(oi_check_result *pResult)
{
  int i;

  pResult->level = OI_BREACH_NONE;
  pResult->description = NULL;
  pResult->numBreach = 0;
  for(i=0; i<MAX_REPORT; i++)
    pResult->location[i] = NULL;
}

/** Record where a breach of the OIFITS standard has occurred. */
static void set_result(oi_check_result *pResult, oi_breach_level level,
		       const char *description, char *location)
{
  if(level > pResult->level)
    pResult->level = level;
  if(pResult->description == NULL)
    pResult->description = description;
  if(++pResult->numBreach < MAX_REPORT) {
    pResult->location[pResult->numBreach-1] = location;
  } else if (pResult->numBreach == MAX_REPORT) {
    pResult->location[MAX_REPORT-1] = g_strdup("[List truncated]");
  }
}

/**
 * Free strings in pResult->location.
 *
 * @param pResult  pointer to check result struct
 */
void free_check_result(oi_check_result *pResult)
{
  int n, i;

  n = (pResult->numBreach < MAX_REPORT) ? pResult->numBreach : MAX_REPORT;
  for(i=0; i<n; i++)
    free(pResult->location[i]);
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
 * @param pTable   pointer to oi_target struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach level indicating overall test result
 */
oi_breach_level check_unique_targets(oi_target *pTable,
				     oi_check_result *pResult)
{
  int i;
  GList *idList;
  target *pTarget;
  static const char desc[] = "Duplicate value of TARGET keyword in OI_TARGET";

  init_check_result(pResult);
  idList = NULL;
  for(i=0; i<pTable->ntarget; i++) {
    pTarget = &pTable->targ[i];
    if(g_list_find_custom(idList, pTarget->target,
			  (GCompareFunc) strcmp) != NULL) {
      /* Duplicate TARGET value */
      set_result(pResult, OI_BREACH_WARNING, desc,
		 g_strdup_printf("TARGET_ID=%d", pTarget->target_id));
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
  static const char desc[] = "Reference to missing target record";

  init_check_result(pResult);

  /* Check OI_VIS tables */
  link = pOi->visList;
  while(link != NULL) {
    pVis = link->data;
    for(i=0; i<pVis->numrec; i++) {
      if(oi_fits_lookup_target(pOi, pVis->record[i].target_id) == NULL)
	set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		   g_strdup_printf("OI_VIS #%d record %d",
				   g_list_position(pOi->visList, link)+1,
				   i+1));
    }
    link = link->next;
  }

  /* Check OI_VIS2 tables */
  link = pOi->vis2List;
  while(link != NULL) {
    pVis2 = link->data;
    for(i=0; i<pVis2->numrec; i++) {
      if(oi_fits_lookup_target(pOi, pVis2->record[i].target_id) == NULL)
	set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		   g_strdup_printf("OI_VIS2 #%d record %d",
				   g_list_position(pOi->vis2List, link)+1,
				   i+1));
    }
    link = link->next;
  }

  /* Check OI_T3 tables */
  link = pOi->t3List;
  while(link != NULL) {
    pT3 = link->data;
    for(i=0; i<pT3->numrec; i++) {
      if(oi_fits_lookup_target(pOi, pT3->record[i].target_id) == NULL)
	set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		   g_strdup_printf("OI_T3 #%d record %d",
				   g_list_position(pOi->t3List, link)+1,
				   i+1));
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
  static const char desc[] = "Reference to missing array element";

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
	    set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		       g_strdup_printf("OI_VIS #%d record %d",
				       g_list_position(pOi->visList, link)+1,
				       i+1));
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
	    set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		       g_strdup_printf("OI_VIS2 #%d record %d",
				       g_list_position(pOi->vis2List, link)+1,
				       i+1));
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
	    set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		       g_strdup_printf("OI_T3 #%d record %d",
				       g_list_position(pOi->t3List, link)+1,
				       i+1));
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
  static const char desc[] = "Data table contains negative error bar";

  init_check_result(pResult);

  /* Check OI_VIS tables */
  link = pOi->visList;
  while(link != NULL) {
    pVis = link->data;
    for(i=0; i<pVis->numrec; i++) {
      for(j=0; j<pVis->nwave; j++) {
	if(pVis->record[i].visamperr[j] < 0. ||
	   pVis->record[i].visphierr[j] < 0.)
	  set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		     g_strdup_printf("OI_VIS #%d record %d channel %d",
				     g_list_position(pOi->visList, link)+1,
				     i+1, j+1));
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
	if(pVis2->record[i].vis2err[j] < 0.)
	  set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		     g_strdup_printf("OI_VIS2 #%d record %d channel %d",
				     g_list_position(pOi->vis2List, link)+1,
				     i+1, j+1));
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
	if(pT3->record[i].t3amperr[j] < 0. ||
	   pT3->record[i].t3phierr[j] < 0.)
	  set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		     g_strdup_printf("OI_T3 #%d record %d channel %d",
				     g_list_position(pOi->t3List, link)+1,
				     i+1, j+1));
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
  static const char desc[] =
    "OI_T3 table may contain unnormalised triple product amplitude";

  init_check_result(pResult);

  link = pOi->t3List;
  while(link != NULL) {
    pT3 = link->data;
    for(i=0; i<pT3->numrec; i++) {
      t3Rec = pT3->record[i];
      for(j=0; j<pT3->nwave; j++) {
	/* use one sigma in case error bars are overestimated */
	if((t3Rec.t3amp[j] - 1.0) > 1*t3Rec.t3amperr[j])
	  set_result(pResult, OI_BREACH_NOT_OIFITS, desc,
		     g_strdup_printf("OI_T3 #%d record %d channel %d",
				     g_list_position(pOi->t3List, link)+1,
				     i+1, j+1));
      }
    }
    link = link->next;
  }

  return pResult->level;
}
