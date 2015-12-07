/**
 * @file
 * @ingroup oifilter
 * Implementation of OIFITS filter.
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

#include "oifilter.h"
#include "chkmalloc.h"

#include <string.h>
#include <math.h>
#include <stdbool.h>


/** Internal use GString, defined in oifile.c */
extern GString *pGStr;

#define RAD2DEG (180.0 / 3.14159)

#define UNSET -9999.9


/** Filter specified on command-line via g_option_context_parse() */
static oi_filter_spec parsedFilter;
/** Values set by g_option_context_parse(), used to set parsedFilter */
char *arrname, *insname, *corrname;
/** Values set by g_option_context_parse(), used to set parsedFilter */
double waveMinNm, waveMaxNm, snrMin, snrMax;
/** Flag set by filter_post_parse */
static gboolean doneParse = FALSE;

/** Specification of command-line options for dataset filtering */
static GOptionEntry filterEntries[] = {
  {"arrname", 0, 0, G_OPTION_ARG_STRING, &arrname,
   "Accept ARRNAMEs matching this pattern (use * and ?)", "PATTERN" },
  {"insname", 0, 0, G_OPTION_ARG_STRING, &insname,
   "Accept INSNAMEs matching this pattern (use * and ?)", "PATTERN" },
  {"corrname", 0, 0, G_OPTION_ARG_STRING, &corrname,
   "Accept CORRNAMEs matching this pattern (use * and ?)", "PATTERN" },
  {"target-id", 0, 0, G_OPTION_ARG_INT, &parsedFilter.target_id,
   "Accept only this TARGET_ID", "ID" },
  {"mjd-min", 0, 0, G_OPTION_ARG_DOUBLE, &parsedFilter.mjd_range[0],
   "Minimum MJD to accept", "MJD" },
  {"mjd-max", 0, 0, G_OPTION_ARG_DOUBLE, &parsedFilter.mjd_range[1],
   "Maximum MJD to accept", "MJD" },
  {"wave-min", 0, 0, G_OPTION_ARG_DOUBLE, &waveMinNm,
   "Minimum wavelength to accept /nm", "WL" },
  {"wave-max", 0, 0, G_OPTION_ARG_DOUBLE, &waveMaxNm,
   "Maximum wavelength to accept /nm", "WL" },
  {"bas-min", 0, 0, G_OPTION_ARG_DOUBLE, &parsedFilter.bas_range[0],
   "Minimum baseline to accept /m", "BASE" },
  {"bas-max", 0, 0, G_OPTION_ARG_DOUBLE, &parsedFilter.bas_range[1],
   "Maximum baseline to accept /m", "BASE" },
  {"uvrad-min", 0, 0, G_OPTION_ARG_DOUBLE, &parsedFilter.uvrad_range[0],
   "Minimum UV radius to accept /wavelength", "UVRADIUS" },
  {"uvrad-max", 0, 0, G_OPTION_ARG_DOUBLE, &parsedFilter.uvrad_range[1],
   "Maximum UV radius to accept /wavelength", "UVRADIUS" },
  {"snr-min", 0, 0, G_OPTION_ARG_DOUBLE, &snrMin,
   "Minimum SNR to accept", "SNR" },
  {"snr-max", 0, 0, G_OPTION_ARG_DOUBLE, &snrMax,
   "Maximum SNR to accept", "SNR" },
  {"accept-vis", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_vis,
   "If non-zero, accept complex visibilities (default 1)", "0/1" },
  {"accept-vis2", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_vis2,
   "If non-zero, accept squared visibilities (default 1)", "0/1" },
  {"accept-t3amp", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_t3amp,
   "If non-zero, accept triple amplitudes (default 1)", "0/1" },
  {"accept-t3phi", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_t3phi,
   "If non-zero, accept closure phases (default 1)", "0/1" },
  {"accept-spectrum", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_spectrum,
   "If non-zero, accept spectra (default 1)", "0/1" },
  {"accept-flagged", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_flagged,
   "If non-zero, accept records with all data flagged (default 1)", "0/1" },
  { NULL }
};


/*
 * Private functions
 */

/**
 * Callback function invoked prior to parsing filter command-line options
 */
static gboolean filter_pre_parse(GOptionContext *context, GOptionGroup *group,
                                 gpointer unusedData, GError **error)
{
  init_oi_filter(&parsedFilter);
  arrname = NULL;
  insname = NULL;
  corrname = NULL;
  /* Convert m -> nm */
  waveMinNm = 1e9 * parsedFilter.wave_range[0];
  waveMaxNm = 1e9 * parsedFilter.wave_range[1];
  snrMin = (double)parsedFilter.snr_range[0];
  snrMax = (double)parsedFilter.snr_range[1];
  return TRUE;
}

/**
 * Callback function invoked after parsing filter command-line options
 */
static gboolean filter_post_parse(GOptionContext *context, GOptionGroup *group,
                                  gpointer unusedData, GError **error)
{
  doneParse = TRUE;
  if (arrname != NULL)
    g_strlcpy(parsedFilter.arrname, arrname, FLEN_VALUE);
  if (insname != NULL)
    g_strlcpy(parsedFilter.insname, insname, FLEN_VALUE);
  if (corrname != NULL)
    g_strlcpy(parsedFilter.corrname, corrname, FLEN_VALUE);
  /* Convert nm -> m */
  parsedFilter.wave_range[0] = (float)(1e-9 * waveMinNm);
  parsedFilter.wave_range[1] = (float)(1e-9 * waveMaxNm);
  parsedFilter.snr_range[0] = (float)snrMin;
  parsedFilter.snr_range[1] = (float)snrMax;
  return TRUE;
}

/**
 * Return new linked list of ARRNAMEs referenced by the
 * OI_VIS/VIS2/T3/SPECTRUM tables
 */
static GList *get_arrname_list(const oi_fits *pData)
{
  GList *arrnameList, *link;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_spectrum *pSpectrum;

  arrnameList = NULL;

  link = pData->visList;
  while (link != NULL) {
    pVis = (oi_vis *)link->data;
    if (pVis->arrname[0] != '\0' &&
        g_list_find_custom(arrnameList, pVis->arrname,
                           (GCompareFunc)strcmp) == NULL)
      arrnameList = g_list_prepend(arrnameList, pVis->arrname);
    link = link->next;
  }

  link = pData->vis2List;
  while (link != NULL) {
    pVis2 = (oi_vis2 *)link->data;
    if (pVis2->arrname[0] != '\0' &&
        g_list_find_custom(arrnameList, pVis2->arrname,
                           (GCompareFunc)strcmp) == NULL)
      arrnameList = g_list_prepend(arrnameList, pVis2->arrname);
    link = link->next;
  }

  link = pData->t3List;
  while (link != NULL) {
    pT3 = (oi_t3 *)link->data;
    if (pT3->arrname[0] != '\0' &&
        g_list_find_custom(arrnameList, pT3->arrname,
                           (GCompareFunc)strcmp) == NULL)
      arrnameList = g_list_prepend(arrnameList, pT3->arrname);
    link = link->next;
  }

  link = pData->spectrumList;
  while (link != NULL) {
    pSpectrum = (oi_spectrum *)link->data;
    if (pSpectrum->arrname[0] != '\0' &&
        g_list_find_custom(arrnameList, pSpectrum->arrname,
                           (GCompareFunc)strcmp) == NULL)
      arrnameList = g_list_prepend(arrnameList, pSpectrum->arrname);
    link = link->next;
  }
  return g_list_reverse(arrnameList);
}

/**
 * Return new linked list of INSNAMEs referenced by the
 * OI_VIS/VIS2/T3/SPECTRUM tables
 */
static GList *get_insname_list(const oi_fits *pData)
{
  GList *insnameList, *link;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_spectrum *pSpectrum;

  insnameList = NULL;

  link = pData->visList;
  while (link != NULL) {
    pVis = (oi_vis *)link->data;
    if (g_list_find_custom(insnameList, pVis->insname,
                           (GCompareFunc)strcmp) == NULL)
      insnameList = g_list_prepend(insnameList, pVis->insname);
    link = link->next;
  }

  link = pData->vis2List;
  while (link != NULL) {
    pVis2 = (oi_vis2 *)link->data;
    if (g_list_find_custom(insnameList, pVis2->insname,
                           (GCompareFunc)strcmp) == NULL)
      insnameList = g_list_prepend(insnameList, pVis2->insname);
    link = link->next;
  }

  link = pData->t3List;
  while (link != NULL) {
    pT3 = (oi_t3 *)link->data;
    if (g_list_find_custom(insnameList, pT3->insname,
                           (GCompareFunc)strcmp) == NULL)
      insnameList = g_list_prepend(insnameList, pT3->insname);
    link = link->next;
  }

  link = pData->spectrumList;
  while (link != NULL) {
    pSpectrum = (oi_spectrum *)link->data;
    if (g_list_find_custom(insnameList, pSpectrum->insname,
                           (GCompareFunc)strcmp) == NULL)
      insnameList = g_list_prepend(insnameList, pSpectrum->insname);
    link = link->next;
  }
  return g_list_reverse(insnameList);
}

/**
 * Return new linked list of CORRNAMEs referenced by the OI_VIS/VIS2/T3 tables
 */
static GList *get_corrname_list(const oi_fits *pData)
{
  GList *corrnameList, *link;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;

  corrnameList = NULL;

  link = pData->visList;
  while (link != NULL) {
    pVis = (oi_vis *)link->data;
    if (pVis->corrname[0] != '\0' &&
        g_list_find_custom(corrnameList, pVis->corrname,
                           (GCompareFunc)strcmp) == NULL)
      corrnameList = g_list_prepend(corrnameList, pVis->corrname);
    link = link->next;
  }

  link = pData->vis2List;
  while (link != NULL) {
    pVis2 = (oi_vis2 *)link->data;
    if (pVis2->corrname[0] != '\0' &&
        g_list_find_custom(corrnameList, pVis2->corrname,
                           (GCompareFunc)strcmp) == NULL)
      corrnameList = g_list_prepend(corrnameList, pVis2->corrname);
    link = link->next;
  }

  link = pData->t3List;
  while (link != NULL) {
    pT3 = (oi_t3 *)link->data;
    if (pT3->corrname[0] != '\0' &&
        g_list_find_custom(corrnameList, pT3->corrname,
                           (GCompareFunc)strcmp) == NULL)
      corrnameList = g_list_prepend(corrnameList, pT3->corrname);
    link = link->next;
  }
  /* OI_SPECTRUM does not use CORRNAME */
  return g_list_reverse(corrnameList);
}

/**
 * Remove first OI_ARRAY table with ARRNAME not in @a arrnameList
 *
 * @return gboolean  TRUE if a table was removed, FALSE if all checked
 */
static gboolean prune_oi_array(oi_fits *pData, GList *arrnameList)
{
  GList *link;
  oi_array *pArray;

  link = pData->arrayList;
  while (link != NULL)
  {
    pArray = (oi_array *)link->data;
    if (g_list_find_custom(arrnameList, pArray->arrname,
                           (GCompareFunc)strcmp) == NULL)
    {
      g_warning("Unreferenced OI_ARRAY table with ARRNAME=%s "
                "removed from filter output", pArray->arrname);
      g_hash_table_remove(pData->arrayHash, pArray->arrname);
      pData->arrayList = g_list_remove(pData->arrayList, pArray);
      --pData->numArray;
      free_oi_array(pArray);
      free(pArray);
      return TRUE;
    }
    link = link->next;
  }
  return FALSE;
}

/**
 * Remove first OI_WAVELENGTH table with INSNAME not in @a insnameList
 *
 * @return gboolean  TRUE if a table was removed, FALSE if all checked
 */
static gboolean prune_oi_wavelength(oi_fits *pData, GList *insnameList)
{
  GList *link;
  oi_wavelength *pWave;

  link = pData->wavelengthList;
  while (link != NULL)
  {
    pWave = (oi_wavelength *)link->data;
    if (g_list_find_custom(insnameList, pWave->insname,
                           (GCompareFunc)strcmp) == NULL)
    {
      g_warning("Unreferenced OI_WAVELENGTH table with INSNAME=%s "
                "removed from filter output", pWave->insname);
      g_hash_table_remove(pData->wavelengthHash, pWave->insname);
      pData->wavelengthList = g_list_remove(pData->wavelengthList, pWave);
      --pData->numWavelength;
      free_oi_wavelength(pWave);
      free(pWave);
      return TRUE;
    }
    link = link->next;
  }
  return FALSE;
}

/**
 * Remove first OI_CORR table with CORRNAME not in @a corrnameList
 *
 * @return gboolean  TRUE if a table was removed, FALSE if all checked
 */
static gboolean prune_oi_corr(oi_fits *pData, GList *corrnameList)
{
  GList *link;
  oi_corr *pCorr;

  link = pData->corrList;
  while (link != NULL)
  {
    pCorr = (oi_corr *)link->data;
    if (g_list_find_custom(corrnameList, pCorr->corrname,
                           (GCompareFunc)strcmp) == NULL)
    {
      g_warning("Unreferenced OI_CORR table with CORRNAME=%s "
                "removed from filter output", pCorr->corrname);
      g_hash_table_remove(pData->corrHash, pCorr->corrname);
      pData->corrList = g_list_remove(pData->corrList, pCorr);
      --pData->numCorr;
      free_oi_corr(pCorr);
      free(pCorr);
      return TRUE;
    }
    link = link->next;
  }
  return FALSE;
}

/**
 * Remove first OI_INSPOL table with ARRNAME not in @a arrnameList
 *
 * @return gboolean  TRUE if a table was removed, FALSE if all checked
 */
static gboolean prune_oi_inspol(oi_fits *pData, GList *arrnameList)
{
  GList *link;
  oi_inspol *pInspol;

  link = pData->inspolList;
  while (link != NULL)
  {
    pInspol = (oi_inspol *)link->data;
    if (g_list_find_custom(arrnameList, pInspol->arrname,
                           (GCompareFunc)strcmp) == NULL)
    {
      g_warning("Unreferenced OI_INSPOL table with ARRNAME=%s "
                "removed from filter output", pInspol->arrname);
      pData->inspolList = g_list_remove(pData->inspolList, pInspol);
      --pData->numInspol;
      free_oi_inspol(pInspol);
      free(pInspol);
      return TRUE;
    }
    link = link->next;
  }
  return FALSE;
}


/*
 * Public functions
 */

/**
 * Return a GOptionGroup for the filtering command-line options
 * recognized by oifitslib
 *
 * You should add this group to your GOptionContext with
 * g_option_context_add_group() or g_option_context_set_main_group(),
 * if you are using g_option_context_parse() to parse your
 * command-line arguments.
 */
GOptionGroup *get_oi_filter_option_group(void)
{
  GOptionGroup *group;

  group = g_option_group_new("filter", "Dataset filtering options:",
                             "Show help for filtering options", NULL, NULL);
  g_option_group_add_entries(group, filterEntries);
  g_option_group_set_parse_hooks(group, filter_pre_parse, filter_post_parse);
  return group;
}

/**
 * Get command-line filter obtained by by g_option_context_parse()
 *
 * @return  Pointer to filter
 */
oi_filter_spec *get_user_oi_filter(void)
{
  g_assert(doneParse);
  return &parsedFilter;
}

/**
 * Filter OIFITS data using filter specified on commandline. Makes a
 * deep copy
 *
 * @param pInput   pointer to input file data struct, see oifile.h
 * @param pOutput  pointer to uninitialised output data struct
 */
void apply_user_oi_filter(const oi_fits *pInput, oi_fits *pOutput)
{
  apply_oi_filter(pInput, get_user_oi_filter(), pOutput);
}

/**
 * Initialise filter specification to accept all data
 *
 * @param pFilter  pointer to filter specification
 */
void init_oi_filter(oi_filter_spec *pFilter)
{
  strcpy(pFilter->arrname, "*");
  strcpy(pFilter->insname, "*");
  strcpy(pFilter->corrname, "*");
  pFilter->target_id = -1;
  pFilter->mjd_range[0] = 0.;
  pFilter->mjd_range[1] = 1e7;
  pFilter->wave_range[0] = 0.;
  pFilter->wave_range[1] = 1e-4;
  pFilter->bas_range[0] = 0.;
  pFilter->bas_range[1] = 1e4;
  pFilter->uvrad_range[0] = 0.;
  pFilter->uvrad_range[1] = 1e11;
  pFilter->snr_range[0] = -5.;
  pFilter->snr_range[1] = 1e10;
  pFilter->accept_vis = 1;
  pFilter->accept_vis2 = 1;
  pFilter->accept_t3amp = 1;
  pFilter->accept_t3phi = 1;
  pFilter->accept_spectrum = 1;
  pFilter->accept_flagged = 1;

  pFilter->arrname_pttn = NULL;
  pFilter->insname_pttn = NULL;
  pFilter->corrname_pttn = NULL;
}

/**
 * Generate string representation of filter spec
 *
 * @param pFilter  pointer to filter specification
 *
 * @return Human-readable string stating filter acceptance criteria
 */
const char *format_oi_filter(const oi_filter_spec *pFilter)
{
  if (pGStr == NULL)
    pGStr = g_string_sized_new(256);

  g_string_printf(pGStr, "Filter accepts:\n");
  g_string_append_printf(pGStr, "  ARRNAME='%s'\n", pFilter->arrname);
  g_string_append_printf(pGStr, "  INSNAME='%s'\n", pFilter->insname);
  g_string_append_printf(pGStr, "  CORRNAME='%s'\n", pFilter->corrname);
  if (pFilter->target_id >= 0)
    g_string_append_printf(pGStr, "  TARGET_ID=%d\n", pFilter->target_id);
  else
    g_string_append_printf(pGStr, "  [Any TARGET_ID]\n");
  g_string_append_printf(pGStr, "  MJD: %.2lf - %.2lf\n",
                         pFilter->mjd_range[0], pFilter->mjd_range[1]);
  g_string_append_printf(pGStr, "  Wavelength: %.1f - %.1fnm\n",
                         1e9 * pFilter->wave_range[0],
                         1e9 * pFilter->wave_range[1]);
  g_string_append_printf(pGStr, "  Baseline: %.1lf - %.1lfm\n",
                         pFilter->bas_range[0], pFilter->bas_range[1]);
  g_string_append_printf(pGStr, "  UV Radius: %.1lg - %.1lg waves\n",
                         pFilter->uvrad_range[0], pFilter->uvrad_range[1]);
  g_string_append_printf(pGStr, "  SNR: %.1f - %.1f\n",
                         pFilter->snr_range[0], pFilter->snr_range[1]);
  if (pFilter->accept_vis)
    g_string_append_printf(pGStr, "  OI_VIS (complex visibilities)\n");
  else
    g_string_append_printf(pGStr, "  [OI_VIS not accepted]\n");
  if (pFilter->accept_vis2)
    g_string_append_printf(pGStr, "  OI_VIS2 (squared visibilities)\n");
  else
    g_string_append_printf(pGStr, "  [OI_VIS2 not accepted]\n");
  if (pFilter->accept_t3amp)
    g_string_append_printf(pGStr, "  OI_T3 T3AMP (triple amplitudes)\n");
  else
    g_string_append_printf(pGStr, "  [OI_T3 T3AMP not accepted]\n");
  if (pFilter->accept_t3phi)
    g_string_append_printf(pGStr, "  OI_T3 T3PHI (closure phases)\n");
  else
    g_string_append_printf(pGStr, "  [OI_T3 T3PHI not accepted]\n");
  if (pFilter->accept_spectrum)
    g_string_append_printf(pGStr, "  OI_SPECTRUM (incoherent spectra)\n");
  else
    g_string_append_printf(pGStr, "  [OI_SPECTRUM not accepted]\n");
  if (pFilter->accept_flagged)
    g_string_append_printf(pGStr, "  All-flagged records\n");
  else
    g_string_append_printf(pGStr, "  [All-flagged records not accepted]\n");

  return pGStr->str;
}

/**
 * Print filter spec to stdout
 *
 * @param pFilter  pointer to filter specification
 */
void print_oi_filter(const oi_filter_spec *pFilter)
{
  printf("%s", format_oi_filter(pFilter));
}


#define ACCEPT_ARRNAME(pObject, pFilter) \
  ( (pFilter)->arrname_pttn == NULL ||   \
    g_pattern_match_string((pFilter)->arrname_pttn, (pObject)->arrname) )

#define ACCEPT_INSNAME(pObject, pFilter) \
  ( (pFilter)->insname_pttn == NULL ||   \
    g_pattern_match_string((pFilter)->insname_pttn, (pObject)->insname) )

#define ACCEPT_CORRNAME(pObject, pFilter) \
  ( (pFilter)->corrname_pttn == NULL ||   \
    g_pattern_match_string((pFilter)->corrname_pttn, (pObject)->corrname) )

/**
 * Filter primary header keywords
 *
 * @param pInHeader   pointer to input oi_header
 * @param pFilter     pointer to filter specification
 * @param pOutHeader  pointer to oi_header to write filtered keywords to
 */
void filter_oi_header(const oi_header *pInHeader,
                      const oi_filter_spec *pFilter, oi_header *pOutHeader)
{
  memcpy(pOutHeader, pInHeader, sizeof(*pInHeader));
  //:TODO: overwrite header values if dataset becomes atomic?
  // TELESCOP, INSTRUME, OBJECT (OBSERVER, REFERENC)
}

/**
 * Filter OI_TARGET table
 *
 * @param pInTargets   pointer to input oi_target
 * @param pFilter      pointer to filter specification
 * @param pOutTargets  pointer to oi_target to write filtered records to
 */
void filter_oi_target(const oi_target *pInTargets,
                      const oi_filter_spec *pFilter, oi_target *pOutTargets)
{
  int i;

  if (pFilter->target_id >= 0) {
    /* New oi_target containing single record. TARGET_ID is set to 1 */
    pOutTargets->revision = pInTargets->revision;
    pOutTargets->ntarget = 0;
    for (i = 0; i < pInTargets->ntarget; i++) {
      if (pInTargets->targ[i].target_id == pFilter->target_id) {
        alloc_oi_target(pOutTargets, 1);
        memcpy(pOutTargets->targ, &pInTargets->targ[i], sizeof(target));
        pOutTargets->targ[0].target_id = 1;
      }
    }
  } else {
    /* Copy input table */
    memcpy(pOutTargets, pInTargets, sizeof(*pInTargets));
    MEMDUP(pOutTargets->targ, pInTargets->targ,
           pInTargets->ntarget * sizeof(pInTargets->targ[0]));
  }
}

/**
 * Filter OI_ARRAY tables
 *
 * Tables are either removed or copied verbatim.
 *
 * @param pInput       pointer to input dataset
 * @param pFilter      pointer to filter specification
 * @param pOutput      pointer to oi_fits struct to write filtered tables to
 */
void filter_all_oi_array(const oi_fits *pInput, const oi_filter_spec *pFilter,
                         oi_fits *pOutput)
{
  GList *link;
  oi_array *pInTab, *pOutTab;

  /* Filter OI_ARRAY tables in turn */
  link = pInput->arrayList;
  while (link != NULL) {
    pInTab = (oi_array *)link->data;
    if (ACCEPT_ARRNAME(pInTab, pFilter)) {
      /* Copy this table */
      pOutTab = dup_oi_array(pInTab);
      pOutput->arrayList = g_list_append(pOutput->arrayList, pOutTab);
      ++pOutput->numArray;
      g_hash_table_insert(pOutput->arrayHash, pOutTab->arrname, pOutTab);
    }
    link = link->next;
  }
}

/**
 * Filter all OI_WAVELENGTH tables, remembering which wavelength
 * channels have been accepted for each
 *
 * @param pInput   pointer to input dataset
 * @param pFilter  pointer to filter specification
 * @param pOutput  pointer to oi_fits struct to write filtered tables to
 *
 * @return Hash table of boolean arrays giving accepted wavelength
 *         channels, indexed by INSNAME
 */
GHashTable *filter_all_oi_wavelength(const oi_fits *pInput,
                                     const oi_filter_spec *pFilter,
                                     oi_fits *pOutput)
{
  GHashTable *useWaveHash;
  oi_wavelength *pInWave, *pOutWave;
  char *useWave;
  GList *link;

  useWaveHash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free);
  link = pInput->wavelengthList;
  while (link != NULL) {
    pInWave = (oi_wavelength *)link->data;
    if (ACCEPT_INSNAME(pInWave, pFilter)) {
      useWave = chkmalloc(pInWave->nwave * sizeof(useWave[0]));
      pOutWave = chkmalloc(sizeof(oi_wavelength));
      filter_oi_wavelength(pInWave, pFilter->wave_range, pOutWave, useWave);
      if (pOutWave->nwave > 0) {
        pOutput->wavelengthList = g_list_append(pOutput->wavelengthList,
                                                pOutWave);
        ++pOutput->numWavelength;
        g_hash_table_insert(pOutput->wavelengthHash,
                            pOutWave->insname, pOutWave);
        g_hash_table_insert(useWaveHash, g_strdup(pInWave->insname), useWave);
      } else {
        g_hash_table_insert(useWaveHash, g_strdup(pInWave->insname), NULL);
        g_warning("Empty tables with INSNAME=%s removed from filter output",
                  pInWave->insname);
        free(useWave);
        free(pOutWave);
      }
    }
    link = link->next;
  }
  return useWaveHash;
}

/**
 * Filter an OI_WAVELENGTH table
 *
 * @param pInWave    pointer to input oi_wavelength
 * @param waveRange  minimum and maximum wavelengths to accept /m
 * @param pOutWave   pointer to output oi_wavelength (uninitialised)
 * @param useWave    on output, boolean array giving wavelength channels
 *                   that were accepted by the filter
 */
void filter_oi_wavelength(const oi_wavelength *pInWave,
                          const float waveRange[2], oi_wavelength *pOutWave,
                          char *useWave)
{
  int i;

  pOutWave->revision = pInWave->revision;
  (void)g_strlcpy(pOutWave->insname, pInWave->insname, FLEN_VALUE);
  pOutWave->nwave = 0; /* use as counter */
  /* will reallocate eff_wave and eff_band later */
  pOutWave->eff_wave = chkmalloc(pInWave->nwave * sizeof(pInWave->eff_wave[0]));
  pOutWave->eff_band = chkmalloc(pInWave->nwave * sizeof(pInWave->eff_band[0]));

  for (i = 0; i < pInWave->nwave; i++) {
    if (pInWave->eff_wave[i] < waveRange[0] ||
        pInWave->eff_wave[i] > waveRange[1]) {
      useWave[i] = FALSE;
    } else {
      useWave[i] = TRUE;
      pOutWave->eff_wave[pOutWave->nwave] = pInWave->eff_wave[i];
      pOutWave->eff_band[pOutWave->nwave++] = pInWave->eff_band[i];
    }
  }
  if (pOutWave->nwave < pInWave->nwave) {
    pOutWave->eff_wave = realloc(pOutWave->eff_wave,
                                 pOutWave->nwave * sizeof(pInWave->eff_wave[0]));
    pOutWave->eff_band = realloc(pOutWave->eff_band,
                                 pOutWave->nwave * sizeof(pInWave->eff_band[0]));
  }
}

/**
 * Filter OI_CORR tables
 *
 * Tables are either removed or copied verbatim.
 *
 * @param pInput       pointer to input dataset
 * @param pFilter      pointer to filter specification
 * @param pOutput      pointer to oi_fits struct to write filtered tables to
 */
void filter_all_oi_corr(const oi_fits *pInput, const oi_filter_spec *pFilter,
                        oi_fits *pOutput)
{
  GList *link;
  oi_corr *pInTab, *pOutTab;

  /* Filter OI_CORR tables in turn */
  link = pInput->corrList;
  while (link != NULL) {
    pInTab = (oi_corr *)link->data;
    if (ACCEPT_CORRNAME(pInTab, pFilter)) {
      /* Copy this table */
      pOutTab = dup_oi_corr(pInTab);
      pOutput->corrList = g_list_append(pOutput->corrList, pOutTab);
      ++pOutput->numCorr;
      g_hash_table_insert(pOutput->corrHash, pOutTab->corrname, pOutTab);
    }
    link = link->next;
  }
}

/**
 * Filter all OI_INSPOL tables
 *
 * @param pInput       pointer to input dataset
 * @param pFilter      pointer to filter specification
 * @param useWaveHash  hash table with INSAME values as keys and char[]
 *                     specifying wavelength channels to accept as values
 * @param pOutput      pointer to output oi_fits struct
 */
void filter_all_oi_inspol(const oi_fits *pInput, const oi_filter_spec *pFilter,
                          GHashTable *useWaveHash, oi_fits *pOutput)
{
  GList *link;
  oi_inspol *pInTab, *pOutTab;

  /* Filter OI_INSPOL tables in turn */
  link = pInput->inspolList;
  while (link != NULL) {
    pInTab = (oi_inspol *)link->data;
    link = link->next; /* follow link now so we can use continue statements */

    /* If applicable, check whether ARRNAME matches */
    if (!ACCEPT_ARRNAME(pInTab, pFilter)) continue;

    pOutTab = chkmalloc(sizeof(oi_inspol));
    filter_oi_inspol(pInTab, pFilter, useWaveHash, pOutTab);
    if (pOutTab->nwave > 0 && pOutTab->numrec > 0) {
      pOutput->inspolList = g_list_append(pOutput->inspolList, pOutTab);
      ++pOutput->numInspol;
    } else {
      g_warning("Empty OI_INSPOL table removed from filter output");
      g_debug("Removed empty OI_INSPOL with ARRNAME=%s", pOutTab->arrname);
      free(pOutTab);
    }
  }
}

/**
 * Filter an OI_INSPOL table by TARGET_ID, INSNAME, and MJD
 *
 * @param pInTab   pointer to input oi_inspol
 * @param pFilter  pointer to filter specification
 * @param useWaveHash  hash table with INSAME values as keys and char[]
 *                     specifying wavelength channels to accept as values
 * @param pOutTab  pointer to output oi_inspol
 */
void filter_oi_inspol(const oi_inspol *pInTab, const oi_filter_spec *pFilter,
                      GHashTable *useWaveHash, oi_inspol *pOutTab)
{
  int i, j, k, nrec;
  char *useWave;

  /* Copy table header items */
  memcpy(pOutTab, pInTab, sizeof(oi_inspol));

  /* Filter records */
  nrec = 0; /* counter */
  pOutTab->record =
    chkmalloc(pInTab->numrec * sizeof(oi_inspol_record)); /* will reallocate */
  pOutTab->nwave = pInTab->nwave; /* upper limit */
  for (i = 0; i < pInTab->numrec; i++) {
    if (pFilter->target_id >= 0 &&
        pInTab->record[i].target_id != pFilter->target_id)
      continue;  /* skip record as TARGET_ID doesn't match */
    if (pFilter->insname_pttn != NULL &&
        !g_pattern_match_string(pFilter->insname_pttn,
                                pInTab->record[i].insname))
      continue;  /* skip record as INSNAME doesn't match */
    if ((pInTab->record[i].mjd_end < pFilter->mjd_range[0]) ||
        (pInTab->record[i].mjd_obs > pFilter->mjd_range[1]))
      continue;  /* skip record as MJD ranges don't overlap */
    useWave = g_hash_table_lookup(useWaveHash, pInTab->record[i].insname);
    if (useWave == NULL)
      continue;  /* skip record as INSNAME filtered out */

    /* Create output record */
    memcpy(&pOutTab->record[nrec], &pInTab->record[i],
           sizeof(oi_inspol_record));
    if (pFilter->target_id >= 0)
      pOutTab->record[nrec].target_id = 1;
    pOutTab->record[nrec].lxx = chkmalloc
      (
        pOutTab->nwave * sizeof(pOutTab->record[0].lxx[0])
      );
    pOutTab->record[nrec].lyy = chkmalloc
      (
        pOutTab->nwave * sizeof(pOutTab->record[0].lyy[0])
      );
    pOutTab->record[nrec].lxy = chkmalloc
      (
        pOutTab->nwave * sizeof(pOutTab->record[0].lxy[0])
      );
    pOutTab->record[nrec].lyx = chkmalloc
      (
        pOutTab->nwave * sizeof(pOutTab->record[0].lyx[0])
      );
    k = 0;
    g_assert(useWave != NULL);
    for (j = 0; j < pInTab->nwave; j++) {
      if (useWave[j]) {
        pOutTab->record[nrec].lxx[k] = pInTab->record[i].lxx[j];
        pOutTab->record[nrec].lyy[k] = pInTab->record[i].lyy[j];
        pOutTab->record[nrec].lxy[k] = pInTab->record[i].lxy[j];
        pOutTab->record[nrec].lyx[k] = pInTab->record[i].lyx[j];
        ++k;
      }
    }
    if (nrec == 0 && k < pOutTab->nwave) {
      /* For 1st output record, length of vectors wasn't known when
         originally allocated, so reallocate */
      pOutTab->nwave = k;
      pOutTab->record[nrec].lxx = realloc(pOutTab->record[nrec].lxx,
                                          k * sizeof(pOutTab->record[0].lxx[0]));
      pOutTab->record[nrec].lyy = realloc(pOutTab->record[nrec].lyy,
                                          k * sizeof(pOutTab->record[0].lyy[0]));
      pOutTab->record[nrec].lxy = realloc(pOutTab->record[nrec].lxy,
                                          k * sizeof(pOutTab->record[0].lxy[0]));
      pOutTab->record[nrec].lyx = realloc(pOutTab->record[nrec].lyx,
                                          k * sizeof(pOutTab->record[0].lyx[0]));
    }
    ++nrec;
  }
  pOutTab->numrec = nrec;
  pOutTab->record = realloc(pOutTab->record, nrec * sizeof(oi_inspol_record));
}

/**
 * Filter all OI_VIS tables
 *
 * @param pInput       pointer to input dataset
 * @param pFilter      pointer to filter specification
 * @param useWaveHash  hash table with INSAME values as keys and char[]
 *                     specifying wavelength channels to accept as values
 * @param pOutput      pointer to output oi_fits struct
 */
void filter_all_oi_vis(const oi_fits *pInput, const oi_filter_spec *pFilter,
                       GHashTable *useWaveHash, oi_fits *pOutput)
{
  GList *link;
  oi_wavelength *pWave;
  oi_vis *pInTab, *pOutTab;
  char *useWave;

  if (!pFilter->accept_vis) return;  /* don't copy any complex vis data */

  /* Filter OI_VIS tables in turn */
  link = pInput->visList;
  while (link != NULL) {
    pInTab = (oi_vis *)link->data;
    link = link->next; /* follow link now so we can use continue statements */

    /* If applicable, check whether INSNAME, ARRNAME match */
    if (!ACCEPT_INSNAME(pInTab, pFilter)) continue;
    if (!ACCEPT_ARRNAME(pInTab, pFilter)) continue;
    if (!ACCEPT_CORRNAME(pInTab, pFilter)) continue;

    useWave = g_hash_table_lookup(useWaveHash, pInTab->insname);
    if (useWave != NULL) {
      pOutTab = chkmalloc(sizeof(oi_vis));
      pWave = oi_fits_lookup_wavelength(pInput, pInTab->insname);
      if (pWave == NULL)
        g_warning("OI_WAVELENGTH with INSNAME=%s missing", pInTab->insname);
      filter_oi_vis(pInTab, pFilter, pWave, useWave, pOutTab);
      if (pOutTab->nwave > 0 && pOutTab->numrec > 0) {
        pOutput->visList = g_list_append(pOutput->visList, pOutTab);
        ++pOutput->numVis;
      } else {
        g_warning("Empty OI_VIS table removed from filter output");
        g_debug("Removed empty OI_VIS with DATE-OBS=%s INSNAME=%s",
                pOutTab->date_obs, pOutTab->insname);
        free(pOutTab);
      }
    }
  }
}

/**
 * Do any of selected channels in @a pRec have acceptable UV radius and SNR?
 */
static bool any_vis_chan_ok(const oi_vis_record *pRec,
                            const oi_filter_spec *pFilter,
                            const oi_wavelength *pWave, const char *useWave,
                            int nwave)
{
  int j;
  double bas, uvrad;
  float snrAmp, snrPhi;

  bas = pow(pRec->ucoord * pRec->ucoord + pRec->vcoord * pRec->vcoord, 0.5);
  for (j = 0; j < nwave; j++) {
    if (useWave[j]) {
      if (pWave != NULL) {
        uvrad = bas / pWave->eff_wave[j];
        if (uvrad < pFilter->uvrad_range[0] ||
            uvrad > pFilter->uvrad_range[1])
          continue;
      }  /* else accept uv radius */
      snrAmp = pRec->visamp[j] / pRec->visamperr[j];
      snrPhi = RAD2DEG / pRec->visphierr[j];
      if (snrAmp < pFilter->snr_range[0] || snrAmp > pFilter->snr_range[1] ||
          snrPhi < pFilter->snr_range[0] || snrPhi > pFilter->snr_range[1])
        continue;
      return TRUE;  /* this channel is OK */
    }
  }
  return FALSE;
}

/**
 * Filter OI_VIS table row by wavelength, UV radius and SNR
 */
static void filter_oi_vis_record(const oi_vis_record *pInRec,
                                 const oi_filter_spec *pFilter,
                                 const oi_wavelength *pWave,
                                 const char *useWave,
                                 int nwaveIn, int nwaveOut,
                                 BOOL usevisrefmap, BOOL usecomplex,
                                 oi_vis_record *pOutRec)
{
  bool someUnflagged;
  int j, k, l, m;
  float snrAmp, snrPhi;
  double uvrad;

  memcpy(pOutRec, pInRec, sizeof(oi_vis_record));
  if (pFilter->target_id >= 0)
    pOutRec->target_id = 1;
  pOutRec->visamp = chkmalloc(nwaveOut * sizeof(pOutRec->visamp[0]));
  pOutRec->visamperr = chkmalloc(nwaveOut * sizeof(pOutRec->visamperr[0]));
  pOutRec->visphi = chkmalloc(nwaveOut * sizeof(pOutRec->visphi[0]));
  pOutRec->visphierr = chkmalloc(nwaveOut * sizeof(pOutRec->visphierr[0]));
  pOutRec->flag = chkmalloc(nwaveOut * sizeof(pOutRec->flag[0]));
  if (usevisrefmap)
    pOutRec->visrefmap = chkmalloc
      (
        nwaveOut * nwaveOut * sizeof(pOutRec->visrefmap[0])
      );
  else
    pOutRec->visrefmap = NULL;
  if (usecomplex) {
    pOutRec->rvis = chkmalloc(nwaveOut * sizeof(pOutRec->rvis[0]));
    pOutRec->rviserr = chkmalloc(nwaveOut * sizeof(pOutRec->rviserr[0]));
    pOutRec->ivis = chkmalloc(nwaveOut * sizeof(pOutRec->ivis[0]));
    pOutRec->iviserr = chkmalloc(nwaveOut * sizeof(pOutRec->iviserr[0]));
  } else {
    pOutRec->rvis = NULL;
    pOutRec->rviserr = NULL;
    pOutRec->ivis = NULL;
    pOutRec->iviserr = NULL;
  }
  k = 0;
  someUnflagged = FALSE;
  for (j = 0; j < nwaveIn; j++) {
    if (useWave[j]) {
      pOutRec->visamp[k] = pInRec->visamp[j];
      pOutRec->visamperr[k] = pInRec->visamperr[j];
      pOutRec->visphi[k] = pInRec->visphi[j];
      pOutRec->visphierr[k] = pInRec->visphierr[j];
      pOutRec->flag[k] = pInRec->flag[j];
      if (pWave != NULL) {
        uvrad = pow(pInRec->ucoord * pInRec->ucoord +
                    pInRec->vcoord * pInRec->vcoord, 0.5) / pWave->eff_wave[j];
        if (uvrad < pFilter->uvrad_range[0] || uvrad > pFilter->uvrad_range[1])
          pOutRec->flag[k] = 1; /* UV radius out of range, flag datum */
      }
      snrAmp = pInRec->visamp[j] / pInRec->visamperr[j];
      snrPhi = RAD2DEG / pInRec->visphierr[j];
      if (snrAmp < pFilter->snr_range[0] || snrAmp > pFilter->snr_range[1] ||
          snrPhi < pFilter->snr_range[0] || snrPhi > pFilter->snr_range[1])
        pOutRec->flag[k] = 1; /* SNR out of range, flag datum */
      if (!pOutRec->flag[k]) someUnflagged = TRUE;
      if (usevisrefmap) {
        m = 0;
        for (l = 0; l < nwaveIn; l++) {
          if (useWave[l]) {
            pOutRec->visrefmap[m + k *
                               nwaveOut] = pInRec->visrefmap[l + j * nwaveIn];
            ++m;
          }
        }
      }
      if (usecomplex) {
        pOutRec->rvis[k] = pInRec->rvis[j];
        pOutRec->rviserr[k] = pInRec->rviserr[j];
        pOutRec->ivis[k] = pInRec->ivis[j];
        pOutRec->iviserr[k] = pInRec->iviserr[j];
      }
      ++k;
    }
  }
  g_assert(pFilter->accept_flagged || someUnflagged);
}

/**
 * Filter an OI_VIS table by TARGET_ID, MJD, wavelength, UV radius, and SNR
 *
 * @param pInTab   pointer to input oi_vis
 * @param pFilter  pointer to filter specification
 * @param pWave    pointer to oi_wavelength referenced by this table
 * @param useWave  boolean array giving wavelength channels to accept
 * @param pOutTab  pointer to output oi_vis
 */
void filter_oi_vis(const oi_vis *pInTab, const oi_filter_spec *pFilter,
                   const oi_wavelength *pWave, const char *useWave,
                   oi_vis *pOutTab)
{
  int i, j, nrec;
  double u1, v1, bas;

  /* Copy table header items */
  memcpy(pOutTab, pInTab, sizeof(oi_vis));
  pOutTab->nwave = 0;
  for (j = 0; j < pInTab->nwave; j++)
    if (useWave[j]) ++pOutTab->nwave;

  /* Filter records */
  nrec = 0; /* counter */
  pOutTab->record =
    chkmalloc(pInTab->numrec * sizeof(oi_vis_record)); /* will reallocate */
  for (i = 0; i < pInTab->numrec; i++) {
    if (pFilter->target_id >= 0 &&
        pInTab->record[i].target_id != pFilter->target_id)
      continue;  /* skip record as TARGET_ID doesn't match */
    if (pInTab->record[i].mjd < pFilter->mjd_range[0] ||
                                pInTab->record[i].mjd > pFilter->mjd_range[1])
      continue;  /* skip record as MJD out of range */
    u1 = pInTab->record[i].ucoord;
    v1 = pInTab->record[i].vcoord;
    bas = pow(u1 * u1 + v1 * v1, 0.5);
    if (bas < pFilter->bas_range[0] || bas > pFilter->bas_range[1])
      continue;  /* skip record as projected baseline out of range */
    if (!pFilter->accept_flagged &&
        !any_vis_chan_ok(&pInTab->record[i], pFilter,
                         pWave, useWave, pInTab->nwave))
      continue;  /* filter out all-flagged record */

    /* Create output record */
    filter_oi_vis_record(&pInTab->record[i], pFilter, pWave, useWave,
                         pInTab->nwave, pOutTab->nwave,
                         pInTab->usevisrefmap, pInTab->usecomplex,
                         &pOutTab->record[nrec++]);
  }
  pOutTab->numrec = nrec;
  pOutTab->record = realloc(pOutTab->record, nrec * sizeof(oi_vis_record));
}

/**
 * Filter all OI_VIS2 tables
 *
 * @param pInput       pointer to input dataset
 * @param pFilter      pointer to filter specification
 * @param useWaveHash  hash table with INSAME values as keys and char[]
 *                     specifying wavelength channels to accept as values
 * @param pOutput      pointer to output oi_fits struct
 */
void filter_all_oi_vis2(const oi_fits *pInput, const oi_filter_spec *pFilter,
                        GHashTable *useWaveHash, oi_fits *pOutput)
{
  GList *link;
  oi_wavelength *pWave;
  oi_vis2 *pInTab, *pOutTab;
  char *useWave;

  if (!pFilter->accept_vis2) return;  /* don't copy any vis2 data */

  /* Filter OI_VIS2 tables in turn */
  link = pInput->vis2List;
  while (link != NULL) {
    pInTab = (oi_vis2 *)link->data;
    link = link->next; /* follow link now so we can use continue statements */

    /* If applicable, check whether INSNAME, ARRNAME match */
    if (!ACCEPT_INSNAME(pInTab, pFilter)) continue;
    if (!ACCEPT_ARRNAME(pInTab, pFilter)) continue;
    if (!ACCEPT_CORRNAME(pInTab, pFilter)) continue;

    useWave = g_hash_table_lookup(useWaveHash, pInTab->insname);
    if (useWave != NULL) {
      pOutTab = chkmalloc(sizeof(oi_vis2));
      pWave = oi_fits_lookup_wavelength(pInput, pInTab->insname);
      if (pWave == NULL)
        g_warning("OI_WAVELENGTH with INSNAME=%s missing", pInTab->insname);
      filter_oi_vis2(pInTab, pFilter, pWave, useWave, pOutTab);
      if (pOutTab->nwave > 0 && pOutTab->numrec > 0) {
        pOutput->vis2List = g_list_append(pOutput->vis2List, pOutTab);
        ++pOutput->numVis2;
      } else {
        g_warning("Empty OI_VIS2 table removed from filter output");
        g_debug("Removed empty OI_VIS2 with DATE-OBS=%s INSNAME=%s",
                pOutTab->date_obs, pOutTab->insname);
        free(pOutTab);
      }
    }
  }
}

/**
 * Do any of selected channels in @a pRec have acceptable UV radius and SNR?
 */
static bool any_vis2_chan_ok(const oi_vis2_record *pRec,
                             const oi_filter_spec *pFilter,
                             const oi_wavelength *pWave, const char *useWave,
                             int nwave)
{
  int j;
  double bas, uvrad;
  float snr;

  bas = pow(pRec->ucoord * pRec->ucoord + pRec->vcoord * pRec->vcoord, 0.5);
  for (j = 0; j < nwave; j++) {
    if (useWave[j]) {
      if (pWave != NULL) {
        uvrad = bas / pWave->eff_wave[j];
        if (uvrad < pFilter->uvrad_range[0] ||
            uvrad > pFilter->uvrad_range[1])
          continue;
      }  /* else accept uv radius */
      snr = pRec->vis2data[j] / pRec->vis2err[j];
      if (snr < pFilter->snr_range[0] || snr > pFilter->snr_range[1])
        continue;
      return TRUE;  /* this channel is OK */
    }  
  }
  return FALSE;
}

/**
 * Filter OI_VIS2 table row by wavelength, UV radius and SNR
 */
static void filter_oi_vis2_record(const oi_vis2_record *pInRec,
                                  const oi_filter_spec *pFilter,
                                  const oi_wavelength *pWave,
                                  const char *useWave,
                                  int nwaveIn, int nwaveOut,
                                  oi_vis2_record *pOutRec)
{
  bool someUnflagged;
  int j, k;
  float snr;
  double uvrad;

  memcpy(pOutRec, pInRec, sizeof(oi_vis2_record));
  if (pFilter->target_id >= 0)
    pOutRec->target_id = 1;
  pOutRec->vis2data = chkmalloc(nwaveOut * sizeof(pOutRec->vis2data[0]));
  pOutRec->vis2err = chkmalloc(nwaveOut * sizeof(pOutRec->vis2err[0]));
  pOutRec->flag = chkmalloc(nwaveOut * sizeof(pOutRec->flag[0]));
  k = 0;
  someUnflagged = FALSE;
  for (j = 0; j < nwaveIn; j++) {
    if (useWave[j]) {
      pOutRec->vis2data[k] = pInRec->vis2data[j];
      pOutRec->vis2err[k] = pInRec->vis2err[j];
      pOutRec->flag[k] = pInRec->flag[j];
      if (pWave != NULL) {
        uvrad = pow(pInRec->ucoord * pInRec->ucoord +
                    pInRec->vcoord * pInRec->vcoord, 0.5) / pWave->eff_wave[j];
        if (uvrad < pFilter->uvrad_range[0] || uvrad > pFilter->uvrad_range[1])
          pOutRec->flag[k] = 1; /* UV radius out of range, flag datum */
      }
      snr = pInRec->vis2data[j] / pInRec->vis2err[j];
      if (snr < pFilter->snr_range[0] || snr > pFilter->snr_range[1])
        pOutRec->flag[k] = 1; /* SNR out of range, flag datum */
      if (!pOutRec->flag[k]) someUnflagged = TRUE;
      ++k;
    }
  }
  g_assert(pFilter->accept_flagged || someUnflagged);
}

/**
 * Filter an OI_VIS2 table by TARGET_ID, MJD, wavelength, UV radius and SNR
 *
 * @param pInTab   pointer to input oi_vis2
 * @param pFilter  pointer to filter specification
 * @param pWave    pointer to oi_wavelength referenced by this table
 * @param useWave  boolean array giving wavelength channels to accept
 * @param pOutTab  pointer to output oi_vis2
 */
void filter_oi_vis2(const oi_vis2 *pInTab, const oi_filter_spec *pFilter,
                    const oi_wavelength *pWave, const char *useWave,
                    oi_vis2 *pOutTab)
{
  int i, j, nrec;
  double bas, u1, v1;

  /* Copy table header items */
  memcpy(pOutTab, pInTab, sizeof(oi_vis2));
  pOutTab->nwave = 0;
  for (j = 0; j < pInTab->nwave; j++)
    if (useWave[j]) ++pOutTab->nwave;

  /* Filter records */
  nrec = 0; /* counter */
  pOutTab->record =
    chkmalloc(pInTab->numrec * sizeof(oi_vis2_record)); /* will reallocate */
  for (i = 0; i < pInTab->numrec; i++) {
    if (pFilter->target_id >= 0 &&
        pInTab->record[i].target_id != pFilter->target_id)
      continue;  /* skip record as TARGET_ID doesn't match */
    if ((pInTab->record[i].mjd < pFilter->mjd_range[0]) ||
        (pInTab->record[i].mjd > pFilter->mjd_range[1]))
      continue;  /* skip record as MJD out of range */
    u1 = pInTab->record[i].ucoord;
    v1 = pInTab->record[i].vcoord;
    bas = pow(u1 * u1 + v1 * v1, 0.5);
    if (bas < pFilter->bas_range[0] || bas > pFilter->bas_range[1])
      continue;  /* skip record as projected baseline out of range */
    if (!pFilter->accept_flagged &&
        !any_vis2_chan_ok(&pInTab->record[i], pFilter, pWave, useWave,
                          pInTab->nwave))
      continue;  /* filter out all-flagged record */

    /* Create output record */
    filter_oi_vis2_record(&pInTab->record[i], pFilter, pWave, useWave,
                          pInTab->nwave, pOutTab->nwave,
                          &pOutTab->record[nrec++]);
  }
  pOutTab->numrec = nrec;
  pOutTab->record = realloc(pOutTab->record, nrec * sizeof(oi_vis2_record));
}

/**
 * Filter all OI_T3 tables
 *
 * @param pInput       pointer to input dataset
 * @param pFilter      pointer to filter specification
 * @param useWaveHash  hash table with INSAME values as keys and char[]
 *                     specifying wavelength channels to accept as values
 * @param pOutput      pointer to output oi_fits struct
 */
void filter_all_oi_t3(const oi_fits *pInput, const oi_filter_spec *pFilter,
                      GHashTable *useWaveHash, oi_fits *pOutput)
{
  GList *link;
  oi_wavelength *pWave;
  oi_t3 *pInTab, *pOutTab;
  char *useWave;

  if (!pFilter->accept_t3amp && !pFilter->accept_t3phi) return;

  /* Filter OI_T3 tables in turn */
  link = pInput->t3List;
  while (link != NULL) {
    pInTab = (oi_t3 *)link->data;
    link = link->next; /* follow link now so we can use continue statements */

    /* If applicable, check whether INSNAME, ARRNAME match */
    if (!ACCEPT_INSNAME(pInTab, pFilter)) continue;
    if (!ACCEPT_ARRNAME(pInTab, pFilter)) continue;
    if (!ACCEPT_CORRNAME(pInTab, pFilter)) continue;

    useWave = g_hash_table_lookup(useWaveHash, pInTab->insname);
    if (useWave != NULL) {
      pOutTab = chkmalloc(sizeof(oi_t3));
      pWave = oi_fits_lookup_wavelength(pInput, pInTab->insname);
      if (pWave == NULL)
        g_warning("OI_WAVELENGTH with INSNAME=%s missing", pInTab->insname);
      filter_oi_t3(pInTab, pFilter, pWave, useWave, pOutTab);
      if (pOutTab->nwave > 0 && pOutTab->numrec > 0) {
        pOutput->t3List = g_list_append(pOutput->t3List, pOutTab);
        ++pOutput->numT3;
      } else {
        g_warning("Empty OI_T3 table removed from filter output");
        g_debug("Removed empty OI_T3 with DATE-OBS=%s INSNAME=%s",
                pOutTab->date_obs, pOutTab->insname);
        free(pOutTab);
      }
    }
  }
}
/**
 * Do any of selected channels in @a pRec have acceptable UV radii and SNR?
 */
static bool any_t3_chan_ok(const oi_t3_record *pRec,
                           const oi_filter_spec *pFilter,
                           const oi_wavelength *pWave, const char *useWave,
                           int nwave)
{
  int j;
  double u1, v1, u2, v2, abRad, bcRad, acRad;
  float snrAmp, snrPhi;

  if (pWave == NULL)
    return TRUE;  /* cannot filter by UV radius, so keep record */
  u1 = pRec->u1coord;
  v1 = pRec->v1coord;
  u2 = pRec->u2coord;
  v2 = pRec->v2coord;
  for (j = 0; j < nwave; j++) {
    if (useWave[j]) {
      if (pWave != NULL) {
        abRad = pow(u1 * u1 + v1 * v1, 0.5) / pWave->eff_wave[j];
        bcRad = pow(u2 * u2 + v2 * v2, 0.5) / pWave->eff_wave[j];
        acRad = (pow((u1 + u2) * (u1 + u2) + (v1 + v2) * (v1 + v2), 0.5) /
                 pWave->eff_wave[j]);
        if (abRad < pFilter->uvrad_range[0] ||
            abRad > pFilter->uvrad_range[1] ||
            bcRad < pFilter->uvrad_range[0] ||
            bcRad > pFilter->uvrad_range[1] ||
            acRad < pFilter->uvrad_range[0] ||
            acRad > pFilter->uvrad_range[1])
          continue;
      }  /* else accept uv radius */
      if (pFilter->accept_t3amp) {
        snrAmp = pRec->t3amp[j] / pRec->t3amperr[j];
        if (snrAmp < pFilter->snr_range[0] || snrAmp > pFilter->snr_range[1])
          continue;
      }
      if (pFilter->accept_t3phi) {
        snrPhi = RAD2DEG / pRec->t3phierr[j];
        if (snrPhi < pFilter->snr_range[0] || snrPhi > pFilter->snr_range[1])
          continue;
      }
      return TRUE;  /* this channel is OK */
    }
  }
  return FALSE;
}

/**
 * Filter OI_T3 table row by wavelength and SNR
 */
static void filter_oi_t3_record(const oi_t3_record *pInRec,
                                const oi_filter_spec *pFilter,
                                const oi_wavelength *pWave,
                                const char *useWave,
                                int nwaveIn, int nwaveOut,
                                oi_t3_record *pOutRec)
{
  bool someUnflagged;
  int j, k;
  double nan, u1, v1, u2, v2, abRad, bcRad, acRad;
  float snrAmp, snrPhi;

  /* If needed, make a NaN */
  if (!pFilter->accept_t3amp || !pFilter->accept_t3phi) {
    nan = 0.0;
    nan /= nan;
  }

  memcpy(pOutRec, pInRec, sizeof(oi_t3_record));
  if (pFilter->target_id >= 0)
    pOutRec->target_id = 1;
  pOutRec->t3amp = chkmalloc(nwaveOut * sizeof(pOutRec->t3amp[0]));
  pOutRec->t3amperr = chkmalloc(nwaveOut * sizeof(pOutRec->t3amperr[0]));
  pOutRec->t3phi = chkmalloc(nwaveOut * sizeof(pOutRec->t3phi[0]));
  pOutRec->t3phierr = chkmalloc(nwaveOut * sizeof(pOutRec->t3phierr[0]));
  pOutRec->flag = chkmalloc(nwaveOut * sizeof(pOutRec->flag[0]));
  k = 0;
  someUnflagged = FALSE;
  u1 = pInRec->u1coord;
  v1 = pInRec->v1coord;
  u2 = pInRec->u2coord;
  v2 = pInRec->v2coord;
  for (j = 0; j < nwaveIn; j++) {
    if (useWave[j]) {
      if (pFilter->accept_t3amp) {
        pOutRec->t3amp[k] = pInRec->t3amp[j];
      } else {
        pOutRec->t3amp[k] = nan;
      }
      pOutRec->t3amperr[k] = pInRec->t3amperr[j];
      if (pFilter->accept_t3phi) {
        pOutRec->t3phi[k] = pInRec->t3phi[j];
      } else {
        pOutRec->t3phi[k] = nan;
      }
      pOutRec->t3phierr[k] = pInRec->t3phierr[j];
      pOutRec->flag[k] = pInRec->flag[j];
      snrAmp = pInRec->t3amp[j] / pInRec->t3amperr[j];
      snrPhi = RAD2DEG / pInRec->t3phierr[j];
      if (pWave != NULL) {
        abRad = pow(u1 * u1 + v1 * v1, 0.5) / pWave->eff_wave[j];
        if (abRad < pFilter->uvrad_range[0] || abRad > pFilter->uvrad_range[1])
          pOutRec->flag[k] = 1; /* UV radius ab out of range, flag datum */
        bcRad = pow(u2 * u2 + v2 * v2, 0.5) / pWave->eff_wave[j];
        if (bcRad < pFilter->uvrad_range[0] || bcRad > pFilter->uvrad_range[1])
          pOutRec->flag[k] = 1; /* UV radius bc out of range, flag datum */
        acRad = pow((u1 + u2) * (u1 + u2) +
                    (v1 + v2) * (v1 + v2), 0.5) / pWave->eff_wave[j];
        if (acRad < pFilter->uvrad_range[0] || acRad > pFilter->uvrad_range[1])
          pOutRec->flag[k] = 1; /* UV radius ac out of range, flag datum */
      }
      if (pFilter->accept_t3amp && (snrAmp < pFilter->snr_range[0] ||
                                    snrAmp > pFilter->snr_range[1]))
        pOutRec->flag[k] = 1; /* SNR out of range, flag datum */
      else if (pFilter->accept_t3phi && (snrPhi < pFilter->snr_range[0] ||
                                         snrPhi > pFilter->snr_range[1]))
        pOutRec->flag[k] = 1; /* SNR out of range, flag datum */
      if (!pOutRec->flag[k]) someUnflagged = TRUE;
      ++k;
    }
  }
  g_assert(pFilter->accept_flagged || someUnflagged);
}

/**
 * Filter an OI_T3 table by TARGET_ID, MJD, wavelength, UV radius and SNR
 *
 * @param pInTab   pointer to input oi_t3
 * @param pFilter  pointer to filter specification
 * @param pWave    pointer to oi_wavelength referenced by this table
 * @param useWave  boolean array giving wavelength channels to accept
 * @param pOutTab  pointer to output oi_t3
 */
void filter_oi_t3(const oi_t3 *pInTab, const oi_filter_spec *pFilter,
                  const oi_wavelength *pWave, const char *useWave,
                  oi_t3 *pOutTab)
{
  int i, j, nrec;
  double u1, v1, u2, v2, bas;

  /* Copy table header items */
  memcpy(pOutTab, pInTab, sizeof(oi_t3));
  pOutTab->nwave = 0;
  for (j = 0; j < pInTab->nwave; j++)
    if (useWave[j]) ++pOutTab->nwave;

  /* Filter records */
  nrec = 0; /* counter */
  pOutTab->record =
    chkmalloc(pInTab->numrec * sizeof(oi_t3_record)); /* will reallocate */
  for (i = 0; i < pInTab->numrec; i++) {
    if (pFilter->target_id >= 0 &&
        pInTab->record[i].target_id != pFilter->target_id)
      continue;  /* skip record as TARGET_ID doesn't match */
    if ((pInTab->record[i].mjd < pFilter->mjd_range[0]) ||
        (pInTab->record[i].mjd > pFilter->mjd_range[1]))
      continue;  /* skip record as MJD out of range */
    u1 = pInTab->record[i].u1coord;
    v1 = pInTab->record[i].v1coord;
    u2 = pInTab->record[i].u2coord;
    v2 = pInTab->record[i].v2coord;
    bas = pow(u1 * u1 + v1 * v1, 0.5);
    if (bas < pFilter->bas_range[0] || bas > pFilter->bas_range[1])
      continue;  /* skip record as projected baseline ab out of range */
    bas = pow(u2 * u2 + v2 * v2, 0.5);
    if (bas < pFilter->bas_range[0] || bas > pFilter->bas_range[1])
      continue;  /* skip record as projected baseline bc out of range */
    bas = pow((u1 + u2) * (u1 + u2) + (v1 + v2) * (v1 + v2), 0.5);
    if (bas < pFilter->bas_range[0] || bas > pFilter->bas_range[1])
      continue;  /* skip record as projected baseline ac out of range */
    if (!pFilter->accept_flagged &&
        !any_t3_chan_ok(&pInTab->record[i], pFilter, pWave, useWave,
                        pInTab->nwave))
      continue;  /* filter out all-flagged record */

    /* Create output record */
    filter_oi_t3_record(&pInTab->record[i], pFilter, pWave, useWave,
                        pInTab->nwave, pOutTab->nwave,
                        &pOutTab->record[nrec++]);
  }
  pOutTab->numrec = nrec;
  pOutTab->record = realloc(pOutTab->record, nrec * sizeof(oi_t3_record));
}

/**
 * Filter all OI_SPECTRUM tables
 *
 * @param pInput       pointer to input dataset
 * @param pFilter      pointer to filter specification
 * @param useWaveHash  hash table with INSAME values as keys and char[]
 *                     specifying wavelength channels to accept as values
 * @param pOutput      pointer to output oi_fits struct
 */
void filter_all_oi_spectrum(const oi_fits *pInput,
                            const oi_filter_spec *pFilter,
                            GHashTable *useWaveHash, oi_fits *pOutput)
{
  GList *link;
  oi_spectrum *pInTab, *pOutTab;
  char *useWave;

  if (!pFilter->accept_spectrum) return;  /* don't copy any spectra */

  /* Filter OI_SPECTRUM tables in turn */
  link = pInput->spectrumList;
  while (link != NULL) {
    pInTab = (oi_spectrum *)link->data;
    link = link->next; /* follow link now so we can use continue statements */

    /* If applicable, check whether INSNAME, ARRNAME, CORRNAME match */
    if (!ACCEPT_INSNAME(pInTab, pFilter)) continue;
    if (!ACCEPT_ARRNAME(pInTab, pFilter)) continue;
    if (!ACCEPT_CORRNAME(pInTab, pFilter)) continue;

    useWave = g_hash_table_lookup(useWaveHash, pInTab->insname);
    if (useWave != NULL) {
      pOutTab = chkmalloc(sizeof(oi_spectrum));
      filter_oi_spectrum(pInTab, pFilter, useWave, pOutTab);
      if (pOutTab->nwave > 0 && pOutTab->numrec > 0) {
        pOutput->spectrumList = g_list_append(pOutput->spectrumList, pOutTab);
        ++pOutput->numSpectrum;
      } else {
        g_warning("Empty OI_SPECTRUM table removed from filter output");
        g_debug("Removed empty OI_SPECTRUM with DATE-OBS=%s INSNAME=%s",
                pOutTab->date_obs, pOutTab->insname);
        free(pOutTab);
      }
    }
  }
}

/**
 * Filter OI_SPECTRUM table row by wavelength and SNR
 */
static void filter_oi_spectrum_record(const oi_spectrum_record *pInRec,
                                      const oi_filter_spec *pFilter,
                                      const char *useWave,
                                      int nwaveIn, int nwaveOut,
                                      oi_spectrum_record *pOutRec)
{
  int j, k;
  double nan;
  float snr;

  /* Make a NaN, for fluxdata rejected on SNR */
  nan = 0.0;
  nan /= nan;

  memcpy(pOutRec, pInRec, sizeof(oi_spectrum_record));
  if (pFilter->target_id >= 0)
    pOutRec->target_id = 1;
  pOutRec->fluxdata = chkmalloc(nwaveOut * sizeof(pOutRec->fluxdata[0]));
  pOutRec->fluxerr = chkmalloc(nwaveOut * sizeof(pOutRec->fluxerr[0]));
  k = 0;
  for (j = 0; j < nwaveIn; j++) {
    if (useWave[j]) {
      snr = pInRec->fluxdata[j] / pInRec->fluxerr[j];
      if (snr < pFilter->snr_range[0] || snr > pFilter->snr_range[1]) {
        /* SNR out of range, null datum */
        pOutRec->fluxdata[k] = nan;
      } else {
        pOutRec->fluxdata[k] = pInRec->fluxdata[j];
      }
      pOutRec->fluxerr[k] = pInRec->fluxerr[j];
      ++k;
    }
  }
}

/**
 * Filter an OI_SPECTRUM table by TARGET_ID, MJD, wavelength and SNR
 *
 * @param pInTab       pointer to input oi_spectrum
 * @param pFilter      pointer to filter specification
 * @param useWave      boolean array giving wavelength channels to accept
 * @param pOutTab      pointer to output oi_spectrum
 */
void filter_oi_spectrum(const oi_spectrum *pInTab,
                        const oi_filter_spec *pFilter,
                        const char *useWave, oi_spectrum *pOutTab)
{
  int i, j, nrec;

  /* Copy table header items */
  memcpy(pOutTab, pInTab, sizeof(oi_spectrum));
  pOutTab->nwave = 0;
  for (j = 0; j < pInTab->nwave; j++)
    if (useWave[j]) ++pOutTab->nwave;

  /* Filter records */
  nrec = 0; /* counter */
  pOutTab->record =
    chkmalloc(pInTab->numrec * sizeof(oi_spectrum_record)); /* will reallocate */
  for (i = 0; i < pInTab->numrec; i++) {
    if (pFilter->target_id >= 0 &&
        pInTab->record[i].target_id != pFilter->target_id)
      continue;  /* skip record as TARGET_ID doesn't match */
    if (pInTab->record[i].mjd < pFilter->mjd_range[0] ||
                                pInTab->record[i].mjd > pFilter->mjd_range[1])
      continue;  /* skip record as MJD out of range */

    /* Create output record */
    filter_oi_spectrum_record(&pInTab->record[i], pFilter, useWave,
                              pInTab->nwave, pOutTab->nwave,
                              &pOutTab->record[nrec++]);
  }
  pOutTab->numrec = nrec;
  pOutTab->record = realloc(pOutTab->record, nrec * sizeof(oi_spectrum_record));
}

/**
 * Filter OIFITS data. Makes a deep copy
 *
 * @param pInput   pointer to input file data struct, see oifile.h
 * @param pFilter  pointer to filter specification
 * @param pOutput  pointer to uninitialised output data struct
 */
void apply_oi_filter(const oi_fits *pInput, oi_filter_spec *pFilter,
                     oi_fits *pOutput)
{
  GHashTable *useWaveHash;
  GList *list;

  init_oi_fits(pOutput);

  /* Compile glob-style patterns for efficiency */
  g_assert(pFilter->arrname_pttn == NULL);
  pFilter->arrname_pttn = g_pattern_spec_new(pFilter->arrname);
  g_assert(pFilter->insname_pttn == NULL);
  pFilter->insname_pttn = g_pattern_spec_new(pFilter->insname);
  g_assert(pFilter->corrname_pttn == NULL);
  pFilter->corrname_pttn = g_pattern_spec_new(pFilter->corrname);

  /* Filter primary header keywords */
  filter_oi_header(&pInput->header, pFilter, &pOutput->header);

  /* Filter OI_TARGET, OI_ARRAY, and OI_CORR tables */
  filter_oi_target(&pInput->targets, pFilter, &pOutput->targets);
  filter_all_oi_array(pInput, pFilter, pOutput);
  filter_all_oi_corr(pInput, pFilter, pOutput);

  /* Filter OI_WAVELENGTH tables, remembering which wavelengths have
     been accepted for each */
  useWaveHash = filter_all_oi_wavelength(pInput, pFilter, pOutput);

  /* Filter tables with spectral data */
  filter_all_oi_inspol(pInput, pFilter, useWaveHash, pOutput);
  filter_all_oi_vis(pInput, pFilter, useWaveHash, pOutput);
  filter_all_oi_vis2(pInput, pFilter, useWaveHash, pOutput);
  filter_all_oi_t3(pInput, pFilter, useWaveHash, pOutput);
  filter_all_oi_spectrum(pInput, pFilter, useWaveHash, pOutput);

  /* Remove orphaned OI_ARRAY, OI_INSPOL, OI_WAVELENGTH and OI_CORR tables */
  list = get_arrname_list(pOutput);
  while (prune_oi_array(pOutput, list)) ;
  while (prune_oi_inspol(pOutput, list)) ;
  g_list_free(list);
  list = get_insname_list(pOutput);
  while (prune_oi_wavelength(pOutput, list)) ;
  g_list_free(list);
  list = get_corrname_list(pOutput);
  while (prune_oi_corr(pOutput, list)) ;
  g_list_free(list);

  //:TODO: remove orphaned OI_INSPOL records?
  // Note these do not invalidate the OIFITS file

  /* Free compiled patterns */
  g_pattern_spec_free(pFilter->arrname_pttn);
  g_pattern_spec_free(pFilter->insname_pttn);
  g_pattern_spec_free(pFilter->corrname_pttn);
  pFilter->arrname_pttn = NULL;
  pFilter->insname_pttn = NULL;
  pFilter->corrname_pttn = NULL;

  g_hash_table_destroy(useWaveHash);
}
