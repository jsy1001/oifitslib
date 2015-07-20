/**
 * @file
 * @ingroup oiiter
 * Implementation of OIFITS iterator interface.
 *
 * Copyright (C) 2015 John Young
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

#include "oiiter.h"

#include <math.h>

/**
 * Initialise squared visibility iterator.
 *
 * @param pIter    Iterator struct to initialise.
 * @param pData    OIFITS dataset to iterate over.
 * @param pFilter  Filter to apply, or NULL.
 */
void oi_vis2_iter_init(oi_vis2_iter *pIter, oi_fits *pData,
                       oi_filter_spec *const pFilter)
{
  g_assert(pIter != NULL);
  g_assert(pData != NULL);

  pIter->pData = pData;
  if (pFilter != NULL)
    pIter->filter = *pFilter;
  else
    init_oi_filter(&pIter->filter);
  pIter->link = pData->vis2List;
  if (pIter->link != NULL) {
    pIter->pTable = (oi_vis2 *)pIter->link->data;
    pIter->pWave = oi_fits_lookup_wavelength(pIter->pData,
                                             pIter->pTable->insname);
  } else {
    pIter->pTable = NULL;
    pIter->pWave = NULL;
  }
  pIter->extver = 1;
  pIter->irec = 0;
  pIter->iwave = -1;
}

#define ACCEPT_ARRNAME(pTable, pFilter)                                 \
  ( (pFilter)->arrname_pttn == NULL ||                                  \
    g_pattern_match_string((pFilter)->arrname_pttn, (pTable)->arrname) )

#define ACCEPT_INSNAME(pTable, pFilter)                                 \
  ( (pFilter)->insname_pttn == NULL ||                                  \
    g_pattern_match_string((pFilter)->insname_pttn, (pTable)->insname) )

#define ACCEPT_CORRNAME(pTable, pFilter)                                \
  ( (pFilter)->corrname_pttn == NULL ||                                 \
    g_pattern_match_string((pFilter)->corrname_pttn, (pTable)->corrname) )

/**
 * Advance iterator to next wavelength channel in current OI_VIS2 record.
 */
static bool oi_vis2_iter_next_channel(oi_vis2_iter *pIter)
{
  if (pIter->link == NULL)
    return false;  /* no OI_VIS2 tables */
  if (pIter->iwave < pIter->pTable->nwave - 1) {
    ++pIter->iwave;
    return true;
  } else {
    return false;
  }
}

/**
 * Does current OI_VIS2 datum pass filter?
 */
static bool oi_vis2_iter_accept_channel(oi_vis2_iter *pIter)
{
  double uvrad;
  float snr;
  oi_vis2_record *pRec = &pIter->pTable->record[pIter->irec];

  if (pIter->pWave->eff_wave[pIter->iwave] < pIter->filter.wave_range[0] ||
      pIter->pWave->eff_wave[pIter->iwave] > pIter->filter.wave_range[1])
    return false;
  snr = pRec->vis2data[pIter->iwave] / pRec->vis2err[pIter->iwave];
  if (snr < pIter->filter.snr_range[0] ||
      snr > pIter->filter.snr_range[1])
    return false;
  uvrad = (pow(pRec->ucoord * pRec->ucoord +
               pRec->vcoord * pRec->vcoord, 0.5) /
           pIter->pWave->eff_wave[pIter->iwave]);
  if (uvrad < pIter->filter.uvrad_range[0] ||
      uvrad > pIter->filter.uvrad_range[1])
    return false;
  if (pRec->flag[pIter->iwave] && !pIter->filter.accept_flagged)
    return false;

  return true;
}

/**
 * Advance iterator to next record (row) in current OI_VIS2 table.
 */
static bool oi_vis2_iter_next_record(oi_vis2_iter *pIter)
{
  if (pIter->link == NULL)
    return false;  /* no OI_VIS2 tables */
  if (pIter->irec < pIter->pTable->numrec - 1) {
    ++pIter->irec;
    pIter->iwave = 0;
    return true;
  } else {
    return false;
  }
}

/**
 * Does current OI_VIS2 record pass filter?
 */
static bool oi_vis2_iter_accept_record(oi_vis2_iter *pIter)
{
  double bas;
  oi_vis2_record *pRec = &pIter->pTable->record[pIter->irec];

  if (pIter->filter.target_id >= 0 &&
      pRec->target_id != pIter->filter.target_id)
    return false;
  if ((pRec->mjd < pIter->filter.mjd_range[0]) ||
      (pRec->mjd > pIter->filter.mjd_range[1]))
    return false;
  bas = pow(pRec->ucoord * pRec->ucoord + pRec->vcoord * pRec->vcoord, 0.5);
  if (bas < pIter->filter.bas_range[0] || bas > pIter->filter.bas_range[1])
    return false;

  return true;
}

/**
 * Advance iterator to next OI_VIS2 table in file.
 */
static bool oi_vis2_iter_next_table(oi_vis2_iter *pIter)
{
  if (pIter->link == NULL || pIter->link->next == NULL)
    return false;  /* no more OI_VIS2 tables */

  pIter->link = pIter->link->next;
  pIter->pTable = (pIter->link != NULL) ? (oi_vis2 *)pIter->link->data : NULL;
  ++pIter->extver;
  pIter->irec = 0;
  pIter->iwave = 0;
  return true;
}

/**
 * Does current OI_VIS2 table pass filter?
 */
static bool oi_vis2_iter_accept_table(oi_vis2_iter *pIter)
{
  return (ACCEPT_ARRNAME(pIter->pTable, &pIter->filter) &&
          ACCEPT_INSNAME(pIter->pTable, &pIter->filter) &&
          ACCEPT_CORRNAME(pIter->pTable, &pIter->filter));
  return true;
}

/**
 * Get next squared visibility datum that passes filter.
 *
 * @param pIter    Initialised iterator struct.
 * @param pExtver  Return location for EXTVER, or NULL.
 * @param ppTable  Return location for pointer to table struct, or NULL.
 * @param pIrec    Return location for record index, or NULL.
 * @param ppRec    Return location for pointer to record struct, or NULL.
 * @param pIwave   Return location for channel index, or NULL.
 * @return bool  true if succesful, false if end of data reached.
 */
bool oi_vis2_iter_next(oi_vis2_iter *pIter,
                       int *const pExtver, oi_vis2 **ppTable,
                       long *const pIrec, oi_vis2_record **ppRec,
                       int *const pIwave)
{
  bool ret = false;
  
  g_assert(pIter != NULL);
  g_assert(pIter->pData != NULL);

  if (!pIter->filter.accept_vis2) return false;

  /* Compile glob-style patterns for efficiency */
  if (pIter->filter.arrname_pttn == NULL)
    pIter->filter.arrname_pttn = g_pattern_spec_new(pIter->filter.arrname);
  if (pIter->filter.insname_pttn == NULL)
    pIter->filter.insname_pttn = g_pattern_spec_new(pIter->filter.insname);
  if (pIter->filter.corrname_pttn == NULL)
    pIter->filter.corrname_pttn = g_pattern_spec_new(pIter->filter.corrname);

  /* Advance to next data point */
  do {
    if (!(oi_vis2_iter_next_channel(pIter) ||
          oi_vis2_iter_next_record(pIter) ||
          oi_vis2_iter_next_table(pIter)))
      goto finally;
  } while (!(oi_vis2_iter_accept_table(pIter) &&
             oi_vis2_iter_accept_record(pIter) &&
             oi_vis2_iter_accept_channel(pIter)));

  /* Return new current data point */
  //:TODO: provide access to wavelength or u/lambda, v/lambda
  ret = true;
  if (pExtver != NULL)
    *pExtver = pIter->extver;
  if (ppTable != NULL)
    *ppTable = pIter->pTable;
  if (pIrec != NULL)
    *pIrec = pIter->irec;
  if (ppRec != NULL)
    *ppRec = &pIter->pTable->record[pIter->irec];
  if (pIwave != NULL)
    *pIwave = pIter->iwave;

finally:
  /* Free compiled patterns */
  g_pattern_spec_free(pIter->filter.arrname_pttn);
  g_pattern_spec_free(pIter->filter.insname_pttn);
  g_pattern_spec_free(pIter->filter.corrname_pttn);
  pIter->filter.arrname_pttn = NULL;
  pIter->filter.insname_pttn = NULL;
  pIter->filter.corrname_pttn = NULL;

  return ret;
}

