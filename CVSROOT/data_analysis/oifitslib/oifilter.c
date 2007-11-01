/* $Id$ */

/**
 * @file oifilter.c
 * @ingroup oifilter
 *
 * Implementation of OIFITS filter.
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


#include <string.h>

#include "oifilter.h"


/** Internal use GString, defined in oifile.c */
extern GString *pGStr;

#ifdef HAVE_G_OPTION_GROUP

#define UNSET -9999.9

/** Filter specified on command-line via g_option_context_parse() */
static oi_filter_spec parsedFilter;
/** Values set by g_option_context_parse(), used to set parsedFilter */
char *arrname, *insname;
/** Values set by g_option_context_parse(), used to set parsedFilter */
static double mjdMin=UNSET, mjdMax, waveMin, waveMax;

/** Specification of command-line options for dataset filtering */
static GOptionEntry filterEntries[] = {
  {"arrname", 0, 0, G_OPTION_ARG_STRING, &arrname,
   "Accept only this ARRNAME", "NAME" },
  {"insname", 0, 0, G_OPTION_ARG_STRING, &insname,
   "Accept only this INSNAME", "NAME" },
  {"target-id", 0, 0, G_OPTION_ARG_INT, &parsedFilter.target_id,
   "Accept only this TARGET_ID", "ID" },
  {"mjd-min", 0, 0, G_OPTION_ARG_DOUBLE, &mjdMin,
   "Minimum MJD to accept", "MJD" },
  {"mjd-max", 0, 0, G_OPTION_ARG_DOUBLE, &mjdMax,
   "Maximum MJD to accept", "MJD" },
  {"wave-min", 0, 0, G_OPTION_ARG_DOUBLE, &waveMin,
   "Minimum wavelength to accept /nm", "WL" },
  {"wave-max", 0, 0, G_OPTION_ARG_DOUBLE, &waveMax,
   "Maximum wavelength to accept /nm", "WL" },
  {"accept-vis", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_vis,
   "If non-zero, accept complex visibilities (default 1)", "0/1" },
  {"accept-vis2", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_vis2,
   "If non-zero, accept squared visibilities (default 1)", "0/1" },
  {"accept-t3amp", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_t3amp,
   "If non-zero, accept triple amplitudes (default 1)", "0/1" },
  {"accept-t3phi", 0, 0, G_OPTION_ARG_INT, &parsedFilter.accept_t3phi,
   "If non-zero, accept closure phases (default 1)", "0/1" },
  { NULL }
};


/*
 * Private functions
 */

/**
 * Callback function invoked prior to parsing filter command-line options.
 */
static gboolean filter_pre_parse(GOptionContext *context, GOptionGroup *group,
				 gpointer unusedData, GError **error)
{
  init_oi_filter(&parsedFilter);
  arrname = NULL;
  insname = NULL;
  mjdMin = parsedFilter.mjd_range[0];
  mjdMax = parsedFilter.mjd_range[1];
  /* Convert m -> nm */
  waveMin = 1e9*parsedFilter.wave_range[0];
  waveMax = 1e9*parsedFilter.wave_range[1];
  return TRUE;
}

/**
 * Callback function invoked after parsing filter command-line options.
 */
static gboolean filter_post_parse(GOptionContext *context, GOptionGroup *group,
				  gpointer unusedData, GError **error)
{
  if (arrname != NULL)
    g_strlcpy(parsedFilter.arrname, arrname, FLEN_VALUE);
  if (insname != NULL)
    g_strlcpy(parsedFilter.insname, insname, FLEN_VALUE);
  parsedFilter.mjd_range[0] = mjdMin;
  parsedFilter.mjd_range[1] = mjdMax;
  /* Convert nm -> m */
  parsedFilter.wave_range[0] = 1e-9*waveMin;
  parsedFilter.wave_range[1] = 1e-9*waveMax;
  return TRUE;
}


/*
 * Public functions
 */

/**
 * Return a GOptionGroup for the filtering command-line options
 * recognized by oifitslib.
 *
 * You should add this group to your GOptionContext with
 * g_option_context_add_group(), if you are using
 * g_option_context_parse() to parse your command-line arguments.
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
 * Get command-line filter obtained by by g_option_context_parse().
 *
 * @return  Pointer to filter
 */
oi_filter_spec *get_user_oi_filter(void)
{
  g_assert(mjdMin != UNSET);
  return &parsedFilter;
}

/**
 * Filter OIFITS data using filter specified on commandline. Makes a
 * deep copy.
 *
 * @param pInput   pointer to input file data struct, see oifile.h
 * @param pOutput  pointer to uninitialised output data struct
 */
void apply_user_oi_filter(const oi_fits *pInput, oi_fits *pOutput)
{
  apply_oi_filter(pInput, get_user_oi_filter(), pOutput);
}

#endif /* #ifdef HAVE_G_OPTION_GROUP */

/**
 * Initialise filter specification to accept all data.
 *
 * @param pFilter  pointer to filter specification
 */
void init_oi_filter(oi_filter_spec *pFilter)
{
  strcpy(pFilter->arrname, "");
  strcpy(pFilter->insname, "");
  pFilter->target_id = -1;
  pFilter->mjd_range[0] = 0.;
  pFilter->mjd_range[1] = 1e7;
  pFilter->wave_range[0] = 0.;
  pFilter->wave_range[1] = 1e-4;
  pFilter->accept_vis = 1;
  pFilter->accept_vis2 = 1;
  pFilter->accept_t3amp = 1;
  pFilter->accept_t3phi = 1;
}

/**
 * Generate string representation of filter spec.
 *
 * @param pFilter  pointer to filter specification
 *
 * @return Human-readable string stating filter acceptance criteria
 */
const char *format_oi_filter(oi_filter_spec *pFilter)
{
  if (pGStr == NULL)
    pGStr = g_string_sized_new(256);

  g_string_printf(pGStr, "Filter accepts:\n");
  if(strlen(pFilter->arrname) > 0)
    g_string_append_printf(pGStr, "  ARRNAME='%s'\n", pFilter->arrname);
  else
    g_string_append_printf(pGStr, "  [Any ARRNAME]\n");
  if(strlen(pFilter->insname) > 0)
    g_string_append_printf(pGStr, "  INSNAME='%s'\n", pFilter->insname);
  else
    g_string_append_printf(pGStr, "  [Any INSNAME]\n");
  if(pFilter->target_id >= 0)
    g_string_append_printf(pGStr, "  TARGET_ID=%d\n", pFilter->target_id);
  else
    g_string_append_printf(pGStr, "  [Any TARGET_ID]\n");
  g_string_append_printf(pGStr, "  MJD: %.2f - %.2f\n",
			 pFilter->mjd_range[0], pFilter->mjd_range[1]);
  g_string_append_printf(pGStr, "  Wavelength: %.1f - %.1fnm\n",
			 1e9*pFilter->wave_range[0],
			 1e9*pFilter->wave_range[1]);
  if(pFilter->accept_vis)
    g_string_append_printf(pGStr, "  OI_VIS (complex visibilities)\n");
  else
    g_string_append_printf(pGStr, "  [OI_VIS not accepted]\n");
  if(pFilter->accept_vis2)
    g_string_append_printf(pGStr, "  OI_VIS2 (squared visibilities)\n");
  else
    g_string_append_printf(pGStr, "  [OI_VIS2 not accepted]\n");
  if(pFilter->accept_t3amp)
    g_string_append_printf(pGStr, "  OI_T3 T3AMP (triple amplitudes)\n");
  else
    g_string_append_printf(pGStr, "  [OI_T3 T3AMP not accepted]\n");
  if(pFilter->accept_t3phi)
    g_string_append_printf(pGStr, "  OI_T3 T3PHI (closure phases)\n");
  else
    g_string_append_printf(pGStr, "  [OI_T3 T3PHI not accepted]\n");

  return pGStr->str;
}

/**
 * Print filter spec to stdout.
 *
 * @param pFilter  pointer to filter specification
 */
void print_oi_filter(oi_filter_spec *pFilter)
{
  printf("%s", format_oi_filter(pFilter));
}


#define ACCEPT_ARRNAME(pObject, pFilter) \
  ( (strlen(pFilter->arrname) == 0 || \
     strcmp(pObject->arrname, pFilter->arrname) == 0) )

#define ACCEPT_INSNAME(pObject, pFilter) \
  ( (strlen(pFilter->insname) == 0 || \
     strcmp(pObject->insname, pFilter->insname) == 0) )

/**
 * Filter OI_TARGET table.
 *
 * @param pInTargets   pointer to input oi_target
 * @param pFilter      pointer to filter specification
 * @param pOutTargets  pointer to oi_target to write filtered records to
 */
void filter_oi_target(const oi_target *pInTargets,
		      const oi_filter_spec *pFilter, oi_target *pOutTargets)
{
  int i;

  if(pFilter->target_id >= 0) {
    /* New oi_target containing single record. TARGET_ID is set to 1 */
    pOutTargets->revision = pInTargets->revision;
    pOutTargets->ntarget = 0;
    for(i=0; i< pInTargets->ntarget; i++) {
      if(pInTargets->targ[i].target_id == pFilter->target_id) {
	pOutTargets->ntarget = 1;
	pOutTargets->targ = malloc(sizeof(target));
	memcpy(pOutTargets->targ, &pInTargets->targ[i], sizeof(target));
	pOutTargets->targ[0].target_id = 1;
      }
    }
  } else {
    /* Copy input table */
    memcpy(pOutTargets, pInTargets, sizeof(*pInTargets));
    MEMDUP(pOutTargets->targ, pInTargets->targ,
	   pInTargets->ntarget*sizeof(pInTargets->targ[0]));
  }
}

/**
 * Filter OI_ARRAY tables.
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
  while(link != NULL) {

    pInTab = (oi_array *) link->data;
    if(ACCEPT_ARRNAME(pInTab, pFilter)) {
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
 * channels have been accepted for each.
 *
 * @param pInput       pointer to input dataset
 * @param pFilter      pointer to filter specification
 * @param pOutput      pointer to oi_fits struct to write filtered tables to
 *
 * @return Hash table of boolean arrays giving accepted wavelength
 * channels, indexed by INSNAME
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
  while(link != NULL) {
    pInWave = (oi_wavelength *) link->data;
    if(ACCEPT_INSNAME(pInWave, pFilter)) {
      useWave = malloc(pInWave->nwave*sizeof(char));
      pOutWave = malloc(sizeof(oi_wavelength));
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
 * Filter specified OI_WAVELENGTH table.
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
  (void) g_strlcpy(pOutWave->insname, pInWave->insname, FLEN_VALUE);
  pOutWave->nwave = 0; /* use as counter */
  /* will reallocate eff_wave and eff_band later */
  pOutWave->eff_wave = malloc(pInWave->nwave*sizeof(pInWave->eff_wave[0]));
  pOutWave->eff_band = malloc(pInWave->nwave*sizeof(pInWave->eff_band[0]));

  for(i=0; i<pInWave->nwave; i++) {
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
				 pOutWave->nwave*sizeof(pInWave->eff_wave[0]));
    pOutWave->eff_band = realloc(pOutWave->eff_band,
				 pOutWave->nwave*sizeof(pInWave->eff_band[0]));
  }
}

/**
 * Filter all OI_VIS tables.
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
  oi_vis *pInTab, *pOutTab;
  char *useWave;

  if(!pFilter->accept_vis) return; /* don't copy any complex vis data */

  /* Filter OI_VIS tables in turn */
  link = pInput->visList;
  while(link != NULL) {

    pInTab = (oi_vis *) link->data;
    link = link->next; /* follow link now so we can use continue statements */

    /* If applicable, check whether INSNAME, ARRNAME match */
    if(!ACCEPT_INSNAME(pInTab, pFilter)) continue;
    if(!ACCEPT_ARRNAME(pInTab, pFilter)) continue;

    useWave = g_hash_table_lookup(useWaveHash, pInTab->insname);
    if (useWave != NULL) {
      pOutTab = malloc(sizeof(oi_vis));
      filter_oi_vis(pInTab, pFilter, useWave, pOutTab);
      if (pOutTab->nwave > 0 && pOutTab->numrec > 0) {
	pOutput->visList = g_list_append(pOutput->visList, pOutTab);
	++pOutput->numVis;
      } else {
	g_warning("Empty OI_VIS table removed from filter output");
	free(pOutTab);
      }
    }
  }
}

/**
 * Filter specified OI_VIS table by TARGET_ID, MJD, and wavelength.
 *
 * @param pInTab       pointer to input oi_vis
 * @param pFilter      pointer to filter specification
 * @param useWave      boolean array giving wavelength channels to accept
 * @param pOutTab      pointer to output oi_vis
 */
void filter_oi_vis(const oi_vis *pInTab, const oi_filter_spec *pFilter,
		   const char *useWave, oi_vis *pOutTab)
{
  int i, j, k, nrec;

  /* Copy table header items */
  memcpy(pOutTab, pInTab, sizeof(oi_vis));

  /* Filter records */
  nrec = 0; /* counter */
  pOutTab->record =
    malloc(pInTab->numrec*sizeof(oi_vis_record)); /* will reallocate */
  pOutTab->nwave = pInTab->nwave; /* upper limit */
  for(i=0; i<pInTab->numrec; i++) {
    if(pFilter->target_id >= 0 &&
       pInTab->record[i].target_id != pFilter->target_id)
      continue; /* skip record as TARGET_ID doesn't match */
    if(pInTab->record[i].mjd < pFilter->mjd_range[0] ||
       pInTab->record[i].mjd > pFilter->mjd_range[1])
      continue; /* skip record as MJD out of range */

    /* Create output record */
    memcpy(&pOutTab->record[nrec], &pInTab->record[i], sizeof(oi_vis_record));
    if(pFilter->target_id >= 0)
      pOutTab->record[nrec].target_id = 1;
    pOutTab->record[nrec].visamp = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].visamperr = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].visphi = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].visphierr = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].flag = malloc(pOutTab->nwave*sizeof(char));
    k = 0;
    for(j=0; j<pInTab->nwave; j++) {
      if(useWave[j]) {
	pOutTab->record[nrec].visamp[k] = pInTab->record[i].visamp[j];
	pOutTab->record[nrec].visamperr[k] = pInTab->record[i].visamperr[j];
	pOutTab->record[nrec].visphi[k] = pInTab->record[i].visphi[j];
	pOutTab->record[nrec].visphierr[k] = pInTab->record[i].visphierr[j];
	pOutTab->record[nrec++].flag[k++] = pInTab->record[i].flag[j];
      }
    }
    if(i == 0 && k < pOutTab->nwave) {
      /* For 1st output record, length of vectors wasn't known when
	 originally allocated, so reallocate */
      pOutTab->nwave = k;
      pOutTab->record[i].visamp = realloc(pOutTab->record[i].visamp,
					  k*sizeof(double));
      pOutTab->record[i].visamperr = realloc(pOutTab->record[i].visamperr,
					     k*sizeof(double));
      pOutTab->record[i].visphi = realloc(pOutTab->record[i].visphi,
					  k*sizeof(double));
      pOutTab->record[i].visphierr = realloc(pOutTab->record[i].visphierr,
					     k*sizeof(double));
      pOutTab->record[i].flag = realloc(pOutTab->record[i].flag,
					k*sizeof(char));
    }	
  }
  pOutTab->numrec = nrec;
  pOutTab->record = realloc(pOutTab->record, nrec*sizeof(oi_vis_record));
}

/**
 * Filter all OI_VIS tables.
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
  oi_vis2 *pInTab, *pOutTab;
  char *useWave;

  if(!pFilter->accept_vis2) return; /* don't copy any vis2 data */

  /* Filter OI_VIS2 tables in turn */
  link = pInput->vis2List;
  while(link != NULL) {

    pInTab = (oi_vis2 *) link->data;
    link = link->next; /* follow link now so we can use continue statements */

    /* If applicable, check whether INSNAME, ARRNAME match */
    if(!ACCEPT_INSNAME(pInTab, pFilter)) continue;
    if(!ACCEPT_ARRNAME(pInTab, pFilter)) continue;

    useWave = g_hash_table_lookup(useWaveHash, pInTab->insname);
    if (useWave != NULL) {
      pOutTab = malloc(sizeof(oi_vis2));
      filter_oi_vis2(pInTab, pFilter, useWave, pOutTab);
      if (pOutTab->nwave > 0 && pOutTab->numrec > 0) {
	pOutput->vis2List = g_list_append(pOutput->vis2List, pOutTab);
	++pOutput->numVis2;
      } else {
	g_warning("Empty OI_VIS2 table removed from filter output");
	free(pOutTab);
      }
    }
  }
}

/**
 * Filter specified OI_VIS2 table by TARGET_ID, MJD, and wavelength.
 *
 * @param pInTab       pointer to input oi_vis2
 * @param pFilter      pointer to filter specification
 * @param useWave      boolean array giving wavelength channels to accept
 * @param pOutTab      pointer to output oi_vis2
 */
void filter_oi_vis2(const oi_vis2 *pInTab, const oi_filter_spec *pFilter,
		    const char *useWave, oi_vis2 *pOutTab)
{
  int i, j, k, nrec;

  /* Copy table header items */
  memcpy(pOutTab, pInTab, sizeof(oi_vis2));

  /* Filter records */
  nrec = 0; /* counter */
  pOutTab->record =
    malloc(pInTab->numrec*sizeof(oi_vis2_record)); /* will reallocate */
  pOutTab->nwave = pInTab->nwave; /* upper limit */
  for(i=0; i<pInTab->numrec; i++) {
    if(pFilter->target_id >= 0 &&
       pInTab->record[i].target_id != pFilter->target_id)
      continue; /* skip record as TARGET_ID doesn't match */
    if(pInTab->record[i].mjd < pFilter->mjd_range[0] ||
       pInTab->record[i].mjd > pFilter->mjd_range[1])
      continue; /* skip record as MJD out of range */

    /* Create output record */
    memcpy(&pOutTab->record[nrec], &pInTab->record[i], sizeof(oi_vis2_record));
    if(pFilter->target_id >= 0)
      pOutTab->record[nrec].target_id = 1;
    pOutTab->record[nrec].vis2data = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].vis2err = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].flag = malloc(pOutTab->nwave*sizeof(char));
    k = 0;
    for(j=0; j<pInTab->nwave; j++) {
      if(useWave[j]) {
	pOutTab->record[nrec].vis2data[k] = pInTab->record[i].vis2data[j];
	pOutTab->record[nrec].vis2err[k] = pInTab->record[i].vis2err[j];
	pOutTab->record[nrec++].flag[k++] = pInTab->record[i].flag[j];
      }
    }
    if(i == 0 && k < pOutTab->nwave) {
      /* For 1st output record, length of vectors wasn't known when
	 originally allocated, so reallocate */
      pOutTab->nwave = k;
      pOutTab->record[i].vis2data = realloc(pOutTab->record[i].vis2data,
					    k*sizeof(double));
      pOutTab->record[i].vis2err = realloc(pOutTab->record[i].vis2err,
					   k*sizeof(double));
      pOutTab->record[i].flag = realloc(pOutTab->record[i].flag,
					k*sizeof(char));
    }	
  }
  pOutTab->numrec = nrec;
  pOutTab->record = realloc(pOutTab->record, nrec*sizeof(oi_vis2_record));
}

/**
 * Filter all OI_T3 tables.
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
  oi_t3 *pInTab, *pOutTab;
  char *useWave;

  if(!pFilter->accept_t3amp && !pFilter->accept_t3phi) return;

  /* Filter OI_T3 tables in turn */
  link = pInput->t3List;
  while(link != NULL) {

    pInTab = (oi_t3 *) link->data;
    link = link->next; /* follow link now so we can use continue statements */

    /* If applicable, check whether INSNAME, ARRNAME match */
    if(!ACCEPT_INSNAME(pInTab, pFilter)) continue;
    if(!ACCEPT_ARRNAME(pInTab, pFilter)) continue;

    useWave = g_hash_table_lookup(useWaveHash, pInTab->insname);
    if (useWave != NULL) {
      pOutTab = malloc(sizeof(oi_t3));
      filter_oi_t3(pInTab, pFilter, useWave, pOutTab);
      if (pOutTab->nwave > 0 && pOutTab->numrec > 0) {
	pOutput->t3List = g_list_append(pOutput->t3List, pOutTab);
	++pOutput->numT3;
      } else {
	g_warning("Empty OI_T3 table removed from filter output");
	free(pOutTab);
      }
    }
  }
}

/**
 * Filter specified OI_T3 table by TARGET_ID, MJD, and wavelength.
 *
 * @param pInTab       pointer to input oi_t3
 * @param pFilter      pointer to filter specification
 * @param useWave      boolean array giving wavelength channels to accept
 * @param pOutTab      pointer to output oi_t3
 */
void filter_oi_t3(const oi_t3 *pInTab, const oi_filter_spec *pFilter,
		  const char *useWave, oi_t3 *pOutTab)
{
  int i, j, k, nrec;
  double nan;

  /* If needed, make a NaN */
  if(!pFilter->accept_t3amp || !pFilter->accept_t3phi) {
    nan = 0.0;
    nan /= nan;
  }

  /* Copy table header items */
  memcpy(pOutTab, pInTab, sizeof(oi_t3));

  /* Filter records */
  nrec = 0; /* counter */
  pOutTab->record =
    malloc(pInTab->numrec*sizeof(oi_t3_record)); /* will reallocate */
  pOutTab->nwave = pInTab->nwave; /* upper limit */
  for(i=0; i<pInTab->numrec; i++) {
    if(pFilter->target_id >= 0 &&
       pInTab->record[i].target_id != pFilter->target_id)
      continue; /* skip record as TARGET_ID doesn't match */
    if(pInTab->record[i].mjd < pFilter->mjd_range[0] ||
       pInTab->record[i].mjd > pFilter->mjd_range[1])
      continue; /* skip record as MJD out of range */

    /* Create output record */
    memcpy(&pOutTab->record[nrec], &pInTab->record[i], sizeof(oi_t3_record));
    if(pFilter->target_id >= 0)
      pOutTab->record[nrec].target_id = 1;
    pOutTab->record[nrec].t3amp = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].t3amperr = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].t3phi = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].t3phierr = malloc(pOutTab->nwave*sizeof(double));
    pOutTab->record[nrec].flag = malloc(pOutTab->nwave*sizeof(char));
    k = 0;
    for(j=0; j<pInTab->nwave; j++) {
      if(useWave[j]) {
	if (pFilter->accept_t3amp) {
	  pOutTab->record[nrec].t3amp[k] = pInTab->record[i].t3amp[j];
	} else {
	  pOutTab->record[nrec].t3amp[k] = nan;
	}
	pOutTab->record[nrec].t3amperr[k] = pInTab->record[i].t3amperr[j];
	if (pFilter->accept_t3phi) {
	  pOutTab->record[nrec].t3phi[k] = pInTab->record[i].t3phi[j];
	} else {
	  pOutTab->record[nrec].t3phi[k] = nan;
	}
	pOutTab->record[nrec].t3phierr[k] = pInTab->record[i].t3phierr[j];
	pOutTab->record[nrec++].flag[k++] = pInTab->record[i].flag[j];
      }
    }
    if(i == 0 && k < pOutTab->nwave) {
      /* For 1st output record, length of vectors wasn't known when
	 originally allocated, so reallocate */
      pOutTab->nwave = k;
      pOutTab->record[i].t3amp = realloc(pOutTab->record[i].t3amp,
					 k*sizeof(double));
      pOutTab->record[i].t3amperr = realloc(pOutTab->record[i].t3amperr,
					    k*sizeof(double));
      pOutTab->record[i].t3phi = realloc(pOutTab->record[i].t3phi,
					 k*sizeof(double));
      pOutTab->record[i].t3phierr = realloc(pOutTab->record[i].t3phierr,
					    k*sizeof(double));
      pOutTab->record[i].flag = realloc(pOutTab->record[i].flag,
					k*sizeof(char));
    }	
  }
  pOutTab->numrec = nrec;
  pOutTab->record = realloc(pOutTab->record, nrec*sizeof(oi_t3_record));
}

/**
 * Filter OIFITS data. Makes a deep copy.
 *
 * @param pInput   pointer to input file data struct, see oifile.h
 * @param pFilter   pointer to filter specification
 * @param pOutput  pointer to uninitialised output data struct
 */
void apply_oi_filter(const oi_fits *pInput, const oi_filter_spec *pFilter,
		     oi_fits *pOutput)
{
  GHashTable *useWaveHash;

  init_oi_fits(pOutput);

  /* Filter OI_TARGET table and OI_ARRAY tables */
  filter_oi_target(&pInput->targets, pFilter, &pOutput->targets);
  filter_all_oi_array(pInput, pFilter, pOutput);

  /* Filter OI_WAVELENGTH tables, remembering which wavelengths have
     been accepted for each */
  useWaveHash = filter_all_oi_wavelength(pInput, pFilter, pOutput);

  /* Filter data tables */
  filter_all_oi_vis(pInput, pFilter, useWaveHash, pOutput);
  filter_all_oi_vis2(pInput, pFilter, useWaveHash, pOutput);
  filter_all_oi_t3(pInput, pFilter, useWaveHash, pOutput);
  /* :TODO: remove orphaned OI_ARRAY & OI_WAVELENGTH tables */

  g_hash_table_destroy(useWaveHash);
}
