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

#define RAD2DEG (180.0 / 3.14159)

/*
 * Private functions
 */

/**
 * Does current OI_VIS datum pass filter?
 */
static bool oi_vis_iter_accept_channel(oi_vis_iter *pIter)
{
  double uvrad;
  float snrAmp, snrPhi;
  oi_vis *pTable = (oi_vis *)pIter->link->data;
  oi_vis_record *pRec = &pTable->record[pIter->irec];

  if (pIter->pWave->eff_wave[pIter->iwave] < pIter->filter.wave_range[0] ||
      pIter->pWave->eff_wave[pIter->iwave] > pIter->filter.wave_range[1])
    return false;
  snrAmp = pRec->visamp[pIter->iwave] / pRec->visamperr[pIter->iwave];
  if (snrAmp < pIter->filter.snr_range[0] ||
      snrAmp > pIter->filter.snr_range[1])
    return false;
  snrPhi = RAD2DEG / pRec->visphierr[pIter->iwave];
  if (snrPhi < pIter->filter.snr_range[0] ||
      snrPhi > pIter->filter.snr_range[1])
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
 * Does current OI_VIS2 datum pass filter?
 */
static bool oi_vis2_iter_accept_channel(oi_vis2_iter *pIter)
{
  double uvrad;
  float snr;
  oi_vis2 *pTable = (oi_vis2 *)pIter->link->data;
  oi_vis2_record *pRec = &pTable->record[pIter->irec];

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
 * Does current OI_T3 datum pass filter?
 */
static bool oi_t3_iter_accept_channel(oi_t3_iter *pIter)
{
  double u1, v1, u2, v2, abRad, bcRad, acRad;
  float snrAmp, snrPhi;
  oi_t3 *pTable = (oi_t3 *)pIter->link->data;
  oi_t3_record *pRec = &pTable->record[pIter->irec];

  if (pIter->pWave->eff_wave[pIter->iwave] < pIter->filter.wave_range[0] ||
      pIter->pWave->eff_wave[pIter->iwave] > pIter->filter.wave_range[1])
    return false;
  if (pIter->filter.accept_t3amp) {
    snrAmp = pRec->t3amp[pIter->iwave] / pRec->t3amperr[pIter->iwave];
    if (snrAmp < pIter->filter.snr_range[0] ||
        snrAmp > pIter->filter.snr_range[1])
      return false;
  }
  if (pIter->filter.accept_t3phi) {
    snrPhi = RAD2DEG / pRec->t3phierr[pIter->iwave];
    if (snrPhi < pIter->filter.snr_range[0] ||
        snrPhi > pIter->filter.snr_range[1])
      return false;
  }
  u1 = pRec->u1coord;
  v1 = pRec->v1coord;
  u2 = pRec->u2coord;
  v2 = pRec->v2coord;
  abRad = pow(u1 * u1 + v1 * v1, 0.5) / pIter->pWave->eff_wave[pIter->iwave];
  bcRad = pow(u2 * u2 + v2 * v2, 0.5) / pIter->pWave->eff_wave[pIter->iwave];
  acRad = (pow((u1 + u2) * (u1 + u2) + (v1 + v2) * (v1 + v2), 0.5) /
           pIter->pWave->eff_wave[pIter->iwave]);
  if (abRad < pIter->filter.uvrad_range[0] ||
      abRad > pIter->filter.uvrad_range[1] ||
      bcRad < pIter->filter.uvrad_range[0] ||
      bcRad > pIter->filter.uvrad_range[1] ||
      acRad < pIter->filter.uvrad_range[0] ||
      acRad > pIter->filter.uvrad_range[1])
    return false;
  if (pRec->flag[pIter->iwave] && !pIter->filter.accept_flagged)
    return false;

  return true;
}

/**
 * Does current OI_VIS record pass filter?
 */
static bool oi_vis_iter_accept_record(oi_vis_iter *pIter)
{
  double bas;
  oi_vis *pTable = (oi_vis *)pIter->link->data;
  oi_vis_record *pRec = &pTable->record[pIter->irec];

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
 * Does current OI_VIS2 record pass filter?
 */
static bool oi_vis2_iter_accept_record(oi_vis2_iter *pIter)
{
  double bas;
  oi_vis2 *pTable = (oi_vis2 *)pIter->link->data;
  oi_vis2_record *pRec = &pTable->record[pIter->irec];

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
 * Does current OI_T3 record pass filter?
 */
static bool oi_t3_iter_accept_record(oi_t3_iter *pIter)
{
  double u1, v1, u2, v2, bas;
  oi_t3 *pTable = (oi_t3 *)pIter->link->data;
  oi_t3_record *pRec = &pTable->record[pIter->irec];

  if (pIter->filter.target_id >= 0 &&
      pRec->target_id != pIter->filter.target_id)
    return false;
  if ((pRec->mjd < pIter->filter.mjd_range[0]) ||
      (pRec->mjd > pIter->filter.mjd_range[1]))
    return false;
  u1 = pRec->u1coord;
  v1 = pRec->v1coord;
  u2 = pRec->u2coord;
  v2 = pRec->v2coord;
  bas = pow(u1 * u1 + v1 * v1, 0.5);
  if (bas < pIter->filter.bas_range[0] || bas > pIter->filter.bas_range[1])
    return false;
  bas = pow(u2 * u2 + v2 * v2, 0.5);
  if (bas < pIter->filter.bas_range[0] || bas > pIter->filter.bas_range[1])
    return false;
  bas = pow((u1 + u2) * (u1 + u2) + (v1 + v2) * (v1 + v2), 0.5);
  if (bas < pIter->filter.bas_range[0] || bas > pIter->filter.bas_range[1])
    return false;

  return true;
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
 * Does current OI_VIS table pass filter?
 */
static bool oi_vis_iter_accept_table(oi_vis_iter *pIter)
{
  oi_vis *pTable = (oi_vis *)pIter->link->data;
  return (ACCEPT_ARRNAME(pTable, &pIter->filter) &&
          ACCEPT_INSNAME(pTable, &pIter->filter) &&
          ACCEPT_CORRNAME(pTable, &pIter->filter));
}

/**
 * Does current OI_VIS2 table pass filter?
 */
static bool oi_vis2_iter_accept_table(oi_vis2_iter *pIter)
{
  oi_vis2 *pTable = (oi_vis2 *)pIter->link->data;
  return (ACCEPT_ARRNAME(pTable, &pIter->filter) &&
          ACCEPT_INSNAME(pTable, &pIter->filter) &&
          ACCEPT_CORRNAME(pTable, &pIter->filter));
}

/**
 * Does current OI_T3 table pass filter?
 */
static bool oi_t3_iter_accept_table(oi_t3_iter *pIter)
{
  oi_t3 *pTable = (oi_t3 *)pIter->link->data;
  return (ACCEPT_ARRNAME(pTable, &pIter->filter) &&
          ACCEPT_INSNAME(pTable, &pIter->filter) &&
          ACCEPT_CORRNAME(pTable, &pIter->filter));
}


/*
 * Public functions
 */

/**
 * Initialise complex visibility iterator.
 *
 * @param pIter    Iterator struct to initialise.
 * @param pData    OIFITS dataset to iterate over.
 * @param pFilter  Filter to apply, or NULL.
 */
void oi_vis_iter_init(oi_vis_iter *pIter, const oi_fits *pData,
                      const oi_filter_spec *pFilter)
{
  g_assert(pIter != NULL);
  g_assert(pData != NULL);

  pIter->pData = pData;
  if (pFilter != NULL)
    pIter->filter = *pFilter;
  else
    init_oi_filter(&pIter->filter);
  pIter->link = pData->visList;
  if (pIter->link != NULL) {
    oi_vis *pTable = (oi_vis *)pIter->link->data;
    pIter->pWave = oi_fits_lookup_wavelength(pIter->pData, pTable->insname);
  } else {
    pIter->pWave = NULL;
  }
  pIter->extver = 1;
  pIter->irec = 0;
  pIter->iwave = -1;
}

/**
 * Initialise squared visibility iterator.
 *
 * @param pIter    Iterator struct to initialise.
 * @param pData    OIFITS dataset to iterate over.
 * @param pFilter  Filter to apply, or NULL.
 */
void oi_vis2_iter_init(oi_vis2_iter *pIter, const oi_fits *pData,
                       const oi_filter_spec *pFilter)
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
    oi_vis2 *pTable = (oi_vis2 *)pIter->link->data;
    pIter->pWave = oi_fits_lookup_wavelength(pIter->pData, pTable->insname);
  } else {
    pIter->pWave = NULL;
  }
  pIter->extver = 1;
  pIter->irec = 0;
  pIter->iwave = -1;
}

/**
 * Initialise triple product iterator.
 *
 * @param pIter    Iterator struct to initialise.
 * @param pData    OIFITS dataset to iterate over.
 * @param pFilter  Filter to apply, or NULL.
 */
void oi_t3_iter_init(oi_t3_iter *pIter, const oi_fits *pData,
                     const oi_filter_spec *pFilter)
{
  g_assert(pIter != NULL);
  g_assert(pData != NULL);

  pIter->pData = pData;
  if (pFilter != NULL)
    pIter->filter = *pFilter;
  else
    init_oi_filter(&pIter->filter);
  pIter->link = pData->t3List;
  if (pIter->link != NULL) {
    oi_t3 *pTable = (oi_t3 *)pIter->link->data;
    pIter->pWave = oi_fits_lookup_wavelength(pIter->pData, pTable->insname);
  } else {
    pIter->pWave = NULL;
  }
  pIter->extver = 1;
  pIter->irec = 0;
  pIter->iwave = -1;
}

#define NEXT_CHANNEL(pIter, tabType)                                    \
  ( (pIter)->link != NULL &&                                            \
    (pIter)->iwave < ((tabType *)((pIter)->link->data))->nwave - 1 &&   \
    (++(pIter)->iwave, true) )

#define NEXT_RECORD(pIter, tabType)                                     \
  ( (pIter)->link != NULL &&                                            \
    (pIter)->irec < ((tabType *)((pIter)->link->data))->numrec - 1 &&   \
    (++(pIter)->irec, (pIter)->iwave = 0, true) )

#define NEXT_TABLE(pIter, tabType)                                      \
  ( (pIter)->link != NULL && (pIter)->link->next != NULL &&             \
    ((pIter)->link = (pIter)->link->next,                               \
     ((pIter)->pWave =                                                  \
      oi_fits_lookup_wavelength((pIter)->pData,                         \
                                ((tabType *)((pIter)->link->data))->insname)), \
     ++(pIter)->extver,                                                 \
     (pIter)->irec = 0,                                                 \
     (pIter)->iwave = 0,                                                \
     true) )

/**
 * Get next complex visibility datum that passes filter.
 *
 * @param pIter    Initialised iterator struct.
 * @param pExtver  Return location for EXTVER, or NULL.
 * @param ppTable  Return location for pointer to table struct, or NULL.
 * @param pIrec    Return location for record index, or NULL.
 * @param ppRec    Return location for pointer to record struct, or NULL.
 * @param pIwave   Return location for channel index, or NULL.
 * @return bool  true if succesful, false if end of data reached.
 */
bool oi_vis_iter_next(oi_vis_iter *pIter,
                     int *const pExtver, oi_vis **ppTable,
                     long *const pIrec, oi_vis_record **ppRec,
                     int *const pIwave)
{
  bool ret = false;
  
  g_assert(pIter != NULL);
  g_assert(pIter->pData != NULL);

  if (!pIter->filter.accept_vis) return false;

  /* Compile glob-style patterns for efficiency */
  if (pIter->filter.arrname_pttn == NULL)
    pIter->filter.arrname_pttn = g_pattern_spec_new(pIter->filter.arrname);
  if (pIter->filter.insname_pttn == NULL)
    pIter->filter.insname_pttn = g_pattern_spec_new(pIter->filter.insname);
  if (pIter->filter.corrname_pttn == NULL)
    pIter->filter.corrname_pttn = g_pattern_spec_new(pIter->filter.corrname);

  /* Advance to next data point */
  do {
    if (!(NEXT_CHANNEL(pIter, oi_vis) ||
          NEXT_RECORD(pIter, oi_vis) ||
          NEXT_TABLE(pIter, oi_vis)))
      goto finally;
  } while (!(oi_vis_iter_accept_table(pIter) &&
             oi_vis_iter_accept_record(pIter) &&
             oi_vis_iter_accept_channel(pIter)));

  /* Return new current data point */
  //:TODO: provide access to wavelength or u/lambda, v/lambda
  ret = true;
  oi_vis *pTable = (oi_vis *)pIter->link->data;
  if (pExtver != NULL)
    *pExtver = pIter->extver;
  if (ppTable != NULL)
    *ppTable = pTable;
  if (pIrec != NULL)
    *pIrec = pIter->irec;
  if (ppRec != NULL)
    *ppRec = &pTable->record[pIter->irec];
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
    if (!(NEXT_CHANNEL(pIter, oi_vis2) ||
          NEXT_RECORD(pIter, oi_vis2) ||
          NEXT_TABLE(pIter, oi_vis2)))
      goto finally;
  } while (!(oi_vis2_iter_accept_table(pIter) &&
             oi_vis2_iter_accept_record(pIter) &&
             oi_vis2_iter_accept_channel(pIter)));

  /* Return new current data point */
  //:TODO: provide access to wavelength or u/lambda, v/lambda
  ret = true;
  oi_vis2 *pTable = (oi_vis2 *)pIter->link->data;
  if (pExtver != NULL)
    *pExtver = pIter->extver;
  if (ppTable != NULL)
    *ppTable = pTable;
  if (pIrec != NULL)
    *pIrec = pIter->irec;
  if (ppRec != NULL)
    *ppRec = &pTable->record[pIter->irec];
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

/**
 * Get next triple product datum that passes filter.
 *
 * @param pIter    Initialised iterator struct.
 * @param pExtver  Return location for EXTVER, or NULL.
 * @param ppTable  Return location for pointer to table struct, or NULL.
 * @param pIrec    Return location for record index, or NULL.
 * @param ppRec    Return location for pointer to record struct, or NULL.
 * @param pIwave   Return location for channel index, or NULL.
 * @return bool  true if succesful, false if end of data reached.
 */
bool oi_t3_iter_next(oi_t3_iter *pIter,
                     int *const pExtver, oi_t3 **ppTable,
                     long *const pIrec, oi_t3_record **ppRec,
                     int *const pIwave)
{
  bool ret = false;
  
  g_assert(pIter != NULL);
  g_assert(pIter->pData != NULL);

  if (!pIter->filter.accept_t3amp && !pIter->filter.accept_t3phi) return false;

  /* Compile glob-style patterns for efficiency */
  if (pIter->filter.arrname_pttn == NULL)
    pIter->filter.arrname_pttn = g_pattern_spec_new(pIter->filter.arrname);
  if (pIter->filter.insname_pttn == NULL)
    pIter->filter.insname_pttn = g_pattern_spec_new(pIter->filter.insname);
  if (pIter->filter.corrname_pttn == NULL)
    pIter->filter.corrname_pttn = g_pattern_spec_new(pIter->filter.corrname);

  /* Advance to next data point */
  do {
    if (!(NEXT_CHANNEL(pIter, oi_t3) ||
          NEXT_RECORD(pIter, oi_t3) ||
          NEXT_TABLE(pIter, oi_t3)))
      goto finally;
  } while (!(oi_t3_iter_accept_table(pIter) &&
             oi_t3_iter_accept_record(pIter) &&
             oi_t3_iter_accept_channel(pIter)));

  /* Return new current data point */
  //:TODO: provide access to wavelength or u/lambda, v/lambda
  ret = true;
  oi_t3 *pTable = (oi_t3 *)pIter->link->data;
  if (pExtver != NULL)
    *pExtver = pIter->extver;
  if (ppTable != NULL)
    *ppTable = pTable;
  if (pIrec != NULL)
    *pIrec = pIter->irec;
  if (ppRec != NULL)
    *ppRec = &pTable->record[pIter->irec];
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

/**
 * Get uv coordinates, in wavelengths, for current complex visibility.
 *
 * @param pIter  Initialised iterator struct.
 * @param u      Return location for u coordinate /wavelengths.
 * @param v      Return location for v coordinate /wavelengths.
 */
void oi_vis_iter_get_uv(const oi_vis_iter *pIter,
                        double *const pU, double *const pV)
{
  g_assert(pIter != NULL);

  oi_vis *pTable = (oi_vis *)pIter->link->data;
  oi_vis_record *pRec = &pTable->record[pIter->irec];
  double eff_wave = pIter->pWave->eff_wave[pIter->iwave];
  if (pU != NULL)
    *pU = pRec->ucoord / eff_wave;
  if (pV != NULL)
    *pV = pRec->vcoord / eff_wave;
}

/**
 * Get uv coordinates, in wavelengths, for current squared visibility.
 *
 * @param pIter  Initialised iterator struct.
 * @param u      Return location for u coordinate /wavelengths.
 * @param v      Return location for v coordinate /wavelengths.
 */
void oi_vis2_iter_get_uv(const oi_vis2_iter *pIter,
                         double *const pU, double *const pV)
{
  g_assert(pIter != NULL);

  oi_vis2 *pTable = (oi_vis2 *)pIter->link->data;
  oi_vis2_record *pRec = &pTable->record[pIter->irec];
  double eff_wave = pIter->pWave->eff_wave[pIter->iwave];
  if (pU != NULL)
    *pU = pRec->ucoord / eff_wave;
  if (pV != NULL)
    *pV = pRec->vcoord / eff_wave;
}

/**
 * Get uv coordinates, in wavelengths, for current triple product.
 *
 * @param pIter  Initialised iterator struct.
 * @param u1     Return location for baseline AB u coordinate /wavelengths.
 * @param v1     Return location for baseline AB v coordinate /wavelengths.
 * @param u2     Return location for baseline BC u coordinate /wavelengths.
 * @param v2     Return location for baseline BC v coordinate /wavelengths.
 */
void oi_t3_iter_get_uv(const oi_t3_iter *pIter,
                       double *const pU1, double *const pV1,
                       double *const pU2, double *const pV2)
{
  g_assert(pIter != NULL);

  oi_t3 *pTable = (oi_t3 *)pIter->link->data;
  oi_t3_record *pRec = &pTable->record[pIter->irec];
  double eff_wave = pIter->pWave->eff_wave[pIter->iwave];
  if (pU1 != NULL)
    *pU1 = pRec->u1coord / eff_wave;
  if (pV1 != NULL)
    *pV1 = pRec->v1coord / eff_wave;
  if (pU2 != NULL)
    *pU2 = pRec->u2coord / eff_wave;
  if (pV2 != NULL)
    *pV2 = pRec->v2coord / eff_wave;
}
