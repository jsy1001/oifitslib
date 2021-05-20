/**
 * @file
 * @ingroup oicheck
 * Implementation of OIFITS conformity checker.
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

#include "oicheck.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


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
  for (i = 0; i < MAX_REPORT; i++)
    pResult->location[i] = NULL;
  pResult->chunk = g_string_chunk_new(100 * MAX_REPORT);
}

/** Record where a breach of the OIFITS standard has occurred. */
static void set_result(oi_check_result *pResult, oi_breach_level level,
                       const char *description, const char *location)
{
  if (level > pResult->level)
    pResult->level = level;
  if (pResult->description == NULL)
    pResult->description = g_string_chunk_insert(pResult->chunk, description);
  else
    g_assert_cmpstr(description, ==, pResult->description);
  if (++pResult->numBreach < MAX_REPORT) {
    pResult->location[pResult->numBreach - 1] =
      g_string_chunk_insert(pResult->chunk, location);
  } else if (pResult->numBreach == MAX_REPORT) {
    pResult->location[MAX_REPORT - 1] =
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

  if (pResult->level == OI_BREACH_NONE)
    return NULL;

  if (pGStr == NULL)
    pGStr = g_string_sized_new(256);

  g_string_printf(pGStr, "*** %s:\n%s, %d occurrences:-\n",
                  oi_breach_level_desc[pResult->level],
                  pResult->description, pResult->numBreach);
  n = (pResult->numBreach < MAX_REPORT) ? pResult->numBreach : MAX_REPORT;
  for (i = 0; i < n; i++)
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

#define CHECK_BAD_TAB_REVISION(tabList, tabType, tabName, rev)          \
  do {                                                                  \
    char location[FLEN_VALUE];                                          \
    tabType *tab;                                                       \
    GList *link;                                                        \
    link = (tabList);                                                   \
    while (link != NULL) {                                              \
      tab = (tabType *)link->data;                                      \
      if (tab->revision != (rev)) {                                     \
        g_snprintf(location, FLEN_VALUE, "%s #%d", tabName,             \
                   g_list_position((tabList), link) + 1);               \
        set_result(pResult, OI_BREACH_NOT_OIFITS,                       \
                   "Invalid OI_REVN", location);                        \
      }                                                                 \
      link = link->next;                                                \
    }                                                                   \
  } while (0)

/**
 * Check tables present and their revision numbers.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_tables(const oi_fits *pOi, oi_check_result *pResult)
{
  const char desc1[] = "Mandatory table missing";
  char location[FLEN_VALUE];

  init_check_result(pResult);
  if (is_oi_fits_two(pOi)) {

    if (pOi->numArray == 0) {
      g_snprintf(location, FLEN_VALUE, "No OI_ARRAY table - "
                 "at least one required");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc1, location);
    }
    if (pOi->numWavelength == 0) {
      g_snprintf(location, FLEN_VALUE, "No OI_WAVELENGTH table - "
                 "at least one required");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc1, location);
    }
    if (pOi->numVis == 0 && pOi->numVis2 == 0 && pOi->numT3 == 0 &&
        pOi->numFlux == 0) {
      g_snprintf(location, FLEN_VALUE, "No data table - "
                 "at least one OI_VIS/VIS2/T3/FLUX required");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc1, location);
    }

    if (pOi->targets.revision != OI_REVN_V2_TARGET) {
      set_result(pResult, OI_BREACH_NOT_OIFITS, "Invalid OI_REVN", "OI_TARGET");
    }
    CHECK_BAD_TAB_REVISION(pOi->arrayList, oi_array, "OI_ARRAY", OI_REVN_V2_ARRAY);
    CHECK_BAD_TAB_REVISION(pOi->wavelengthList, oi_wavelength, "OI_WAVELENGTH",
                           OI_REVN_V2_WAVELENGTH);
    CHECK_BAD_TAB_REVISION(pOi->visList, oi_vis, "OI_VIS", OI_REVN_V2_VIS);
    CHECK_BAD_TAB_REVISION(pOi->vis2List, oi_vis2, "OI_VIS2", OI_REVN_V2_VIS2);
    CHECK_BAD_TAB_REVISION(pOi->t3List, oi_t3, "OI_T3", OI_REVN_V2_T3);
    CHECK_BAD_TAB_REVISION(pOi->fluxList, oi_flux, "OI_FLUX", OI_REVN_V2_FLUX);
    CHECK_BAD_TAB_REVISION(pOi->corrList, oi_corr, "OI_CORR", OI_REVN_V2_CORR);
    CHECK_BAD_TAB_REVISION(pOi->inspolList, oi_inspol, "OI_INSPOL", OI_REVN_V2_INSPOL);

  } else { /* is_oi_fits_one(pOi)) */

    if (pOi->numWavelength == 0) {
      g_snprintf(location, FLEN_VALUE, "No OI_WAVELENGTH table - "
                 "at least one required");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc1, location);
    }
    if (pOi->numVis == 0 && pOi->numVis2 == 0 && pOi->numT3 == 0) {
      g_snprintf(location, FLEN_VALUE, "No data table - "
                 "at least one OI_VIS/VIS2/T3 required");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc1, location);
    }

    if (pOi->targets.revision != OI_REVN_V1_TARGET) {
      set_result(pResult, OI_BREACH_NOT_OIFITS, "Invalid OI_REVN", "OI_TARGET");
    }
    CHECK_BAD_TAB_REVISION(pOi->arrayList, oi_array, "OI_ARRAY", OI_REVN_V1_ARRAY);
    CHECK_BAD_TAB_REVISION(pOi->wavelengthList, oi_wavelength, "OI_WAVELENGTH",
                           OI_REVN_V1_WAVELENGTH);
    CHECK_BAD_TAB_REVISION(pOi->visList, oi_vis, "OI_VIS", OI_REVN_V1_VIS);
    CHECK_BAD_TAB_REVISION(pOi->vis2List, oi_vis2, "OI_VIS2", OI_REVN_V1_VIS2);
    CHECK_BAD_TAB_REVISION(pOi->t3List, oi_t3, "OI_T3", OI_REVN_V1_T3);

  }
  return pResult->level;
}

/**
 * Check mandatory primary header keywords are present.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_header(const oi_fits *pOi, oi_check_result *pResult)
{
  const char desc[] = "Invalid/missing primary header keyword value";
  char location[FLEN_VALUE];

  init_check_result(pResult);
  if (is_oi_fits_two(pOi)) {
    if (strlen(pOi->header.origin) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "ORIGIN value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pOi->header.date) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "DATE value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pOi->header.date_obs) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "DATE-OBS value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pOi->header.content) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "CONTENT value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    } else if (strcmp(pOi->header.content, "OIFITS2") != 0) {
      g_snprintf(location, FLEN_VALUE,
                 "Value of CONTENT in primary header is '%s' not 'OIFITS2'",
                 pOi->header.content);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pOi->header.telescop) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "TELESCOP value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pOi->header.instrume) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "INSTRUME value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pOi->header.observer) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "OBSERVER value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pOi->header.insmode) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "INSMODE value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pOi->header.object) == 0) {
      g_snprintf(location, FLEN_VALUE,
                 "OBJECT value missing from primary header");
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
  }
  return pResult->level;
}

/**
 * Check string keywords have allowed values.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_keywords(const oi_fits *pOi, oi_check_result *pResult)
{
  int ver2;
  GList *link;
  oi_array *pArray;
  oi_vis *pVis;
  oi_flux *pFlux;
  const char desc[] = "Invalid keyword value";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  ver2 = is_oi_fits_two(pOi);

  /* Check OI_ARRAY keywords */
  link = pOi->arrayList;
  while (link != NULL) {
    pArray = link->data;
    if (strcmp(pArray->frame, "GEOCENTRIC") != 0 &&
        strcmp(pArray->frame, "SKY") != 0) {
      g_snprintf(location, FLEN_VALUE,
                 "OI_ARRAY #%d FRAME='%s' ('GEOCENTRIC'/'SKY')",
                 g_list_position(pOi->arrayList, link) + 1, pArray->frame);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    // TODO: warn if SKY used in revision 1
    link = link->next;
  }

  /* Check optional OI_VIS keywords */
  link = pOi->visList;
  while (link != NULL) {
    pVis = link->data;
    if (ver2 && strlen(pVis->amptyp) > 0 &&
        strcmp(pVis->amptyp, "absolute") != 0 &&
        strcmp(pVis->amptyp, "differential") != 0 &&
        strcmp(pVis->amptyp, "correlated flux") != 0) {
      g_snprintf(location, FLEN_VALUE, "OI_VIS #%d AMPTYP='%s' "
                 "('absolute'/'differential'/'correlated flux')",
                 g_list_position(pOi->visList, link) + 1, pVis->amptyp);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (ver2 && strlen(pVis->phityp) > 0 &&
        strcmp(pVis->phityp, "absolute") != 0 &&
        strcmp(pVis->phityp, "differential") != 0) {
      g_snprintf(location, FLEN_VALUE,
                 "OI_VIS #%d PHITYP='%s' ('absolute'/'differential')",
                 g_list_position(pOi->visList, link) + 1, pVis->phityp);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    link = link->next;
  }

  /* Check OI_FLUX keywords */
  link = pOi->fluxList;
  while (link != NULL) {
    pFlux = link->data;
    if (pFlux->calstat != 'C' && pFlux->calstat != 'U') {
      g_snprintf(location, FLEN_VALUE,
                 "OI_FLUX #%d CALSTAT='%c' ('C'/'U')",
                 g_list_position(pOi->fluxList, link) + 1,
                 pFlux->calstat);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    if (strlen(pFlux->fovtype) > 0 &&
        strcmp(pFlux->fovtype, "FWHM") != 0 &&
        strcmp(pFlux->fovtype, "RADIUS") != 0) {
      g_snprintf(location, FLEN_VALUE,
                 "OI_FLUX #%d FOVTYPE='%s' ('FWHM', 'RADIUS')",
                 g_list_position(pOi->fluxList, link) + 1,
                 pFlux->fovtype);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    link = link->next;
  }

  return pResult->level;
}

/**
 * Check optional OI_VIS VISREFMAP column present when needed.
 *
 * @sa check_keywords() which checks that AMPTYP and PHITYP each have
 * an allowed value.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_visrefmap(const oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  oi_vis *pVis;
  const char desc[] =
    "VISREFMAP present (missing) for absolute (differential) vis";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  /* Check OI_VIS keywords */
  link = pOi->visList;
  while (link != NULL) {
    pVis = link->data;
    if (strcmp(pVis->amptyp, "differential") == 0 ||
        strcmp(pVis->phityp, "differential") == 0)
    {
      /* differential data => VISREFMAP mandatory */
      if (!pVis->usevisrefmap) {
        g_snprintf(location, FLEN_VALUE,
                   "OI_VIS #%d AMPTYP='%s' PHITYP='%s' has no VISREFMAP",
                   g_list_position(pOi->visList, link) + 1,
                   pVis->amptyp, pVis->phityp);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    } else if (pVis->usevisrefmap) {
      /* VISREFMAP not applicable */
      g_snprintf(location, FLEN_VALUE,
                 "OI_VIS #%d AMPTYP='%s' PHITYP='%s' has VISREFMAP",
                 g_list_position(pOi->visList, link) + 1,
                 pVis->amptyp, pVis->phityp);
      set_result(pResult, OI_BREACH_WARNING, desc, location);
    }
    link = link->next;
  }

  return pResult->level;
}

/**
 * Check targets have unique identifiers.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_unique_targets(const oi_fits *pOi,
                                     oi_check_result *pResult)
{
  int i;
  GList *idList;
  target *pTarget;
  const char desc[] = "Duplicate value in TARGET column of OI_TARGET";
  char location[FLEN_VALUE];

  init_check_result(pResult);
  idList = NULL;
  for (i = 0; i < pOi->targets.ntarget; i++) {
    pTarget = &pOi->targets.targ[i];
    if (g_list_find_custom(idList, pTarget->target,
                           (GCompareFunc)strcmp) != NULL) {
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
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_targets_present(const oi_fits *pOi,
                                      oi_check_result *pResult)
{
  GList *link;
  int i;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_flux *pFlux;
  const char desc[] = "Reference to missing target record";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  /* Check OI_VIS tables */
  link = pOi->visList;
  while (link != NULL) {
    pVis = link->data;
    for (i = 0; i < pVis->numrec; i++) {
      if (oi_fits_lookup_target(pOi, pVis->record[i].target_id) == NULL) {
        g_snprintf(location, FLEN_VALUE, "OI_VIS #%d record %d",
                   g_list_position(pOi->visList, link) + 1, i + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    }
    link = link->next;
  }

  /* Check OI_VIS2 tables */
  link = pOi->vis2List;
  while (link != NULL) {
    pVis2 = link->data;
    for (i = 0; i < pVis2->numrec; i++) {
      if (oi_fits_lookup_target(pOi, pVis2->record[i].target_id) == NULL) {
        g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d record %d",
                   g_list_position(pOi->vis2List, link) + 1, i + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    }
    link = link->next;
  }

  /* Check OI_T3 tables */
  link = pOi->t3List;
  while (link != NULL) {
    pT3 = link->data;
    for (i = 0; i < pT3->numrec; i++) {
      if (oi_fits_lookup_target(pOi, pT3->record[i].target_id) == NULL) {
        g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d",
                   g_list_position(pOi->t3List, link) + 1, i + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    }
    link = link->next;
  }

  /* Check OI_FLUX tables */
  link = pOi->fluxList;
  while (link != NULL) {
    pFlux = link->data;
    for (i = 0; i < pFlux->numrec; i++) {
      if (oi_fits_lookup_target(pOi, pFlux->record[i].target_id) == NULL) {
        g_snprintf(location, FLEN_VALUE, "OI_FLUX #%d record %d",
                   g_list_position(pOi->fluxList, link) + 1, i + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    }
    link = link->next;
  }

  return pResult->level;
}

/**
 * Check ARRNAME is set (mandatory in OIFITS v2).
 *
 * @sa check_elements_present() which checks that the OI_ARRAY table
 * referenced by ARRNAME contains the elements referenced by
 * STA_INDEX.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_arrname(const oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  oi_inspol *pInspol;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_flux *pFlux;
  const char desc[] = "ARRNAME missing";
  char location[FLEN_VALUE];

  init_check_result(pResult);
  if(is_oi_fits_two(pOi))
  {
    /* Check OI_INSPOL tables */
    link = pOi->inspolList;
    while (link != NULL) {
      pInspol = link->data;
      if (strlen(pInspol->arrname) == 0) {
        g_snprintf(location, FLEN_VALUE, "OI_INSPOL #%d",
                   g_list_position(pOi->visList, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
      link = link->next;
    }

    /* Check OI_VIS tables (file is v2) */
    link = pOi->visList;
    while (link != NULL) {
      pVis = link->data;
      if (strlen(pVis->arrname) == 0) {
        g_snprintf(location, FLEN_VALUE, "OI_VIS #%d",
                   g_list_position(pOi->visList, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
      link = link->next;
    }

    /* Check OI_VIS2 tables (file is v2) */
    link = pOi->vis2List;
    while (link != NULL) {
      pVis2 = link->data;
      if (strlen(pVis2->arrname) == 0) {
        g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d",
                   g_list_position(pOi->vis2List, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
      link = link->next;
    }

    /* Check OI_T3 tables (file is v2) */
    link = pOi->t3List;
    while (link != NULL) {
      pT3 = link->data;
      if (strlen(pT3->arrname) == 0) {
        g_snprintf(location, FLEN_VALUE, "OI_T3 #%d",
                   g_list_position(pOi->t3List, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
      link = link->next;
    }

    /* Check OI_FLUX tables */
    link = pOi->fluxList;
    while (link != NULL) {
      pFlux = link->data;
      if (pFlux->calstat == 'U' && strlen(pFlux->arrname) == 0) {
        /* ARRNAME required in OI_FLUX only if uncalibrated */
        g_snprintf(location, FLEN_VALUE, "OI_FLUX #%d",
                   g_list_position(pOi->fluxList, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
      link = link->next;
    }
  }
  
  return pResult->level;
}

/**
 * Check all referenced array elements are present.
 *
 * @sa check_arrname() which checks that the ARRNAME keyword is present.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_elements_present(const oi_fits *pOi,
                                       oi_check_result *pResult)
{
  GList *link;
  int i, j;
  oi_inspol *pInspol;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_flux *pFlux;
  const char desc[] = "Reference to missing array element";
  const char desc2[] = "ARRNAME missing"; //:BUG:
  char location[FLEN_VALUE];

  init_check_result(pResult);

  /* Check OI_INSPOL tables */
  link = pOi->inspolList;
  while (link != NULL) {
    pInspol = link->data;
    if (strlen(pInspol->arrname) > 0) {
      for (i = 0; i < pInspol->numrec; i++) {
        if (oi_fits_lookup_element(pOi, pInspol->arrname,
                                   pInspol->record[i].sta_index) == NULL) {
          g_snprintf(location, FLEN_VALUE, "OI_INSPOL #%d record %d",
                     g_list_position(pOi->inspolList, link) + 1, i + 1);
          set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
        }
      }
    } else {
      g_snprintf(location, FLEN_VALUE, "OI_INSPOL #%d",
                 g_list_position(pOi->visList, link) + 1);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc2, location);
    }
    link = link->next;
  }

  /* Check OI_VIS tables (ARRNAME optional in v1) */
  link = pOi->visList;
  while (link != NULL) {
    pVis = link->data;
    if (strlen(pVis->arrname) > 0) {
      for (i = 0; i < pVis->numrec; i++) {
        for (j = 0; j < 2; j++) {
          if (oi_fits_lookup_element(pOi, pVis->arrname,
                                     pVis->record[i].sta_index[j]) == NULL) {
            g_snprintf(location, FLEN_VALUE, "OI_VIS #%d record %d",
                       g_list_position(pOi->visList, link) + 1, i + 1);
            set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
          }
        }
      }
    }
    link = link->next;
  }

  /* Check OI_VIS2 tables (ARRNAME optional in v1) */
  link = pOi->vis2List;
  while (link != NULL) {
    pVis2 = link->data;
    if (strlen(pVis2->arrname) > 0) {
      for (i = 0; i < pVis2->numrec; i++) {
        for (j = 0; j < 2; j++) {
          if (oi_fits_lookup_element(pOi, pVis2->arrname,
                                     pVis2->record[i].sta_index[j]) == NULL) {
            g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d record %d",
                       g_list_position(pOi->vis2List, link) + 1, i + 1);
            set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
          }
        }
      }
    }
    link = link->next;
  }

  /* Check OI_T3 tables (ARRNAME optional in v1) */
  link = pOi->t3List;
  while (link != NULL) {
    pT3 = link->data;
    if (strlen(pT3->arrname) > 0) {
      for (i = 0; i < pT3->numrec; i++) {
        for (j = 0; j < 3; j++) {
          if (oi_fits_lookup_element(pOi, pT3->arrname,
                                     pT3->record[i].sta_index[j]) == NULL) {
            g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d",
                       g_list_position(pOi->t3List, link) + 1, i + 1);
            set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
          }
        }
      }
    }
    link = link->next;
  }

  /* Check OI_FLUX tables */
  link = pOi->fluxList;
  while (link != NULL) {
    pFlux = link->data;
    if (strlen(pFlux->arrname) > 0) {
      for (i = 0; i < pFlux->numrec; i++) {
        if (pFlux->record[i].sta_index == -1) continue;
        if (oi_fits_lookup_element(pOi, pFlux->arrname,
                                   pFlux->record[i].sta_index) == NULL) {
          g_snprintf(location, FLEN_VALUE, "OI_FLUX #%d record %d",
                     g_list_position(pOi->fluxList, link) + 1, i + 1);
          set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
        }
      }
    }
    link = link->next;
  }

  return pResult->level;
}

/**
 * Check all referenced OI_CORR tables are present.
 *
 * Does not check OI_CORR contents as insignificant correlations may
 * be omitted.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_corr_present(const oi_fits *pOi,
                                   oi_check_result *pResult)
{
  GList *link;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_flux *pFlux;
  const char desc[] = "Reference to missing OI_CORR table";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  /* Check OI_VIS tables */
  link = pOi->visList;
  while (link != NULL) {
    pVis = link->data;
    if (strlen(pVis->corrname) > 0 &&
        oi_fits_lookup_corr(pOi, pVis->corrname) == NULL) {
      g_snprintf(location, FLEN_VALUE, "OI_VIS #%d",
                 g_list_position(pOi->visList, link) + 1);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    link = link->next;
  }

  /* Check OI_VIS2 tables */
  link = pOi->vis2List;
  while (link != NULL) {
    pVis2 = link->data;
    if (strlen(pVis2->corrname) > 0 &&
        oi_fits_lookup_corr(pOi, pVis2->corrname) == NULL) {
      g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d",
                 g_list_position(pOi->vis2List, link) + 1);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    link = link->next;
  }

  /* Check OI_T3 tables */
  link = pOi->t3List;
  while (link != NULL) {
    pT3 = link->data;
    if (strlen(pT3->corrname) > 0 &&
        oi_fits_lookup_corr(pOi, pT3->corrname) == NULL) {
      g_snprintf(location, FLEN_VALUE, "OI_T3 #%d",
                 g_list_position(pOi->t3List, link) + 1);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
    }
    link = link->next;
  }

  /* Check OI_FLUX tables */
  link = pOi->fluxList;
  while (link != NULL) {
    pFlux = link->data;
    if (strlen(pFlux->corrname) > 0 &&
        oi_fits_lookup_corr(pOi, pFlux->corrname) == NULL) {
      g_snprintf(location, FLEN_VALUE, "OI_FLUX #%d",
                 g_list_position(pOi->fluxList, link) + 1);
      set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
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
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_flagging(const oi_fits *pOi, oi_check_result *pResult)
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
  while (link != NULL) {
    pVis = link->data;
    for (i = 0; i < pVis->numrec; i++) {
      for (j = 0; j < pVis->nwave; j++) {
        if (pVis->record[i].flag[j]) continue;
        if (pVis->record[i].visamperr[j] < 0. ||
            pVis->record[i].visphierr[j] < 0.) {
          g_snprintf(location, FLEN_VALUE, "OI_VIS #%d record %d channel %d",
                     g_list_position(pOi->visList, link) + 1, i + 1, j + 1);
          set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
        }
      }
    }
    link = link->next;
  }

  /* Check OI_VIS2 tables */
  link = pOi->vis2List;
  while (link != NULL) {
    pVis2 = link->data;
    for (i = 0; i < pVis2->numrec; i++) {
      for (j = 0; j < pVis2->nwave; j++) {
        if (pVis2->record[i].flag[j]) continue;
        if (pVis2->record[i].vis2err[j] < 0.) {
          g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d record %d channel %d",
                     g_list_position(pOi->vis2List, link) + 1, i + 1, j + 1);
          set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
        }
      }
    }
    link = link->next;
  }

  /* Check OI_T3 tables */
  link = pOi->t3List;
  while (link != NULL) {
    pT3 = link->data;
    for (i = 0; i < pT3->numrec; i++) {
      for (j = 0; j < pT3->nwave; j++) {
        if (pT3->record[i].flag[j]) continue;
        if (pT3->record[i].t3amperr[j] < 0. ||
            pT3->record[i].t3phierr[j] < 0.) {
          g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d channel %d",
                     g_list_position(pOi->t3List, link) + 1, i + 1, j + 1);
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
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_t3amp(const oi_fits *pOi, oi_check_result *pResult)
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
  while (link != NULL) {
    pT3 = link->data;
    for (i = 0; i < pT3->numrec; i++) {
      t3Rec = pT3->record[i];
      for (j = 0; j < pT3->nwave; j++) {
        if (t3Rec.flag[j]) continue;
        /* use one sigma in case error bars are overestimated */
        if ((t3Rec.t3amp[j] - 1.0) > 1 * t3Rec.t3amperr[j]) {
          g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d channel %d",
                     g_list_position(pOi->t3List, link) + 1, i + 1, j + 1);
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
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_waveorder(const oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  int i;
  oi_wavelength *pWave;
  const char desc[] = "OI_WAVELENGTH has wavelengths not in ascending order";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  link = pOi->wavelengthList;
  while (link != NULL) {
    pWave = link->data;
    for (i = 1; i < pWave->nwave; i++) {
      if (pWave->eff_wave[i] < pWave->eff_wave[i - 1] ||
          (i + 1 < pWave->nwave &&
           pWave->eff_wave[i] > pWave->eff_wave[i + 1])) {
        g_snprintf(location, FLEN_VALUE, "OI_WAVELENGTH INSNAME=%s channel %d",
                   pWave->insname, i + 1);
        set_result(pResult, OI_BREACH_WARNING, desc, location);
      }
    }
    link = link->next;
  }

  return pResult->level;
}

/**
 * Check for non-zero TIME values in OI_VIS/VIS2/T3.
 *
 * Use of TIME is deprecated in OIFITS v2.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_time(const oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  int i;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  const double tol = 1e-10;
  const char desc[] = "Non-zero TIME values in OIFITS v2 data table";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  if (is_oi_fits_two(pOi))
  {
    /* Check OI_VIS tables */
    link = pOi->visList;
    while (link != NULL) {
      pVis = link->data;
      for (i = 0; i < pVis->numrec; i++) {
        if (fabs(pVis->record[i].time) > tol) {
          g_snprintf(location, FLEN_VALUE, "OI_VIS #%d record %d",
                     g_list_position(pOi->visList, link) + 1, i + 1);
          set_result(pResult, OI_BREACH_WARNING, desc, location);
        }
      }
      link = link->next;
    }

    /* Check OI_VIS2 tables */
    link = pOi->vis2List;
    while (link != NULL) {
      pVis2 = link->data;
      for (i = 0; i < pVis2->numrec; i++) {
        if (fabs(pVis2->record[i].time) > tol) {
          g_snprintf(location, FLEN_VALUE, "OI_VIS2 #%d record %d",
                     g_list_position(pOi->vis2List, link) + 1, i + 1);
          set_result(pResult, OI_BREACH_WARNING, desc, location);
        }
      }
      link = link->next;
    }

    /* Check OI_T3 tables */
    link = pOi->t3List;
    while (link != NULL) {
      pT3 = link->data;
      for (i = 0; i < pT3->numrec; i++) {
        if (fabs(pT3->record[i].time) > tol) {
          g_snprintf(location, FLEN_VALUE, "OI_T3 #%d record %d",
                     g_list_position(pOi->t3List, link) + 1, i + 1);
          set_result(pResult, OI_BREACH_WARNING, desc, location);
        }
      }
      link = link->next;
    }
  }
  return pResult->level;
}

/**
 * Check presence of ARRNAME, STA_INDEX and FOVTYPE in OI_FLUX tables.
 *
 * ARRNAME and STA_INDEX must be present if the fluxes are
 * uncalibrated, but absent if they are calibrated. FOVTYPE must be
 * absent if the fluxes are uncalibrated.
 *
 * @param pOi      pointer to oi_fits struct to check
 * @param pResult  pointer to oi_check_result struct to store result in
 *
 * @return oi_breach_level indicating overall test result
 */
oi_breach_level check_flux(const oi_fits *pOi, oi_check_result *pResult)
{
  GList *link;
  oi_flux *pFlux;
  const char desc[] =
    "ARRNAME/STA_INDEX/FOVTYPE present (missing) in (un)calibrated fluxes";
  char location[FLEN_VALUE];

  init_check_result(pResult);

  link = pOi->fluxList;
  while (link != NULL) {
    pFlux = link->data;
    if (pFlux->calstat == 'C') {
      if (strlen(pFlux->arrname) > 0) {
        g_snprintf(location, FLEN_VALUE,
                   "OI_FLUX #%d Calibrated but ARRNAME='%s'",
                   g_list_position(pOi->fluxList, link) + 1,
                   pFlux->arrname);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
      if (pFlux->record[0].sta_index != -1) {
        g_snprintf(location, FLEN_VALUE,
                   "OI_FLUX #%d Calibrated but STA_INDEX present",
                   g_list_position(pOi->fluxList, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    } else if (pFlux->calstat == 'U') {
      if (strlen(pFlux->arrname) == 0) {
        g_snprintf(location, FLEN_VALUE,
                   "OI_FLUX #%d Uncalibrated but ARRNAME missing",
                   g_list_position(pOi->fluxList, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
      if (pFlux->record[0].sta_index == -1) {
        g_snprintf(location, FLEN_VALUE,
                   "OI_FLUX #%d Uncalibrated but STA_INDEX missing",
                   g_list_position(pOi->fluxList, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
      if (strlen(pFlux->fovtype) > 0) {
        g_snprintf(location, FLEN_VALUE,
                   "OI_FLUX #%d Uncalibrated but FOVTYPE present",
                   g_list_position(pOi->fluxList, link) + 1);
        set_result(pResult, OI_BREACH_NOT_OIFITS, desc, location);
      }
    }
    /* else do nothing, will fail check_keywords() */
    link = link->next;
  }

  return pResult->level;
}
