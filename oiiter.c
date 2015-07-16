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
  pIter->extver = 1;
  pIter->irec = 0;
  pIter->iwave = 0;
}

bool oi_vis2_iter_next(oi_vis2_iter *pIter, int *const pExtver,
                       long *const pIrec, oi_vis2_record **ppRec,
                       int *const pIwave)
{
  oi_vis2 *pTab;

  g_assert(pIter != NULL);

  if (pIter->link == NULL)
    return false; /* no OI_VIS2 tables */

  /* Advance to next data point :TODO: account for filter */
  pTab = (oi_vis2 *)pIter->link->data;
  if (pIter->iwave < pTab->nwave - 1) {
    ++pIter->iwave;
  } else if (pIter->irec < pTab->numrec - 1) {
    ++pIter->irec;
    pIter->iwave = 0;
  } else if (pIter->link->next != NULL) {
    pIter->link = pIter->link->next;
    ++pIter->extver;
    pTab = (oi_vis2 *)pIter->link->data;
    pIter->irec = 0;
    pIter->iwave = 0;
  } else {
    return false;
  }

  /* Return next data point */
  if (pExtver != NULL)
    *pExtver = pIter->extver;
  if (pIrec != NULL)
    *pIrec = pIter->irec;
  if (ppRec != NULL)
    *ppRec = &pTab->record[pIter->irec];
  if (pIwave != NULL)
    *pIwave = pIter->iwave;
  return true;
}

