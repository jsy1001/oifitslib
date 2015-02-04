/**
 * @file
 * @ingroup oitable
 * Implementation of functions to allocate storage within table structs
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

#include "exchange.h"


/**
 * Allocate storage within oi_array struct.
 *
 * Sets the oi_array::nelement attribute of @a pArray.
 *
 * @param pArray    pointer to array data struct, see exchange.h
 * @param nelement  number of array elements (table rows) to allocate
 */
void alloc_oi_array(oi_array *pArray, int nelement)
{
  pArray->elem = malloc(nelement*sizeof(element));
  pArray->nelement = nelement;
}

/**
 * Allocate storage within oi_target struct.
 *
 * Sets the oi_target::ntarget attribute of @a pTargets.
 *
 * @param pTargets  pointer to targets data struct, see exchange.h
 * @param ntarget   number of targets (table rows) to allocate
 */
void alloc_oi_target(oi_target *pTargets, int ntarget)
{
  pTargets->targ = malloc(ntarget*sizeof(target));
  pTargets->ntarget = ntarget;
}

/**
 * Allocate storage within oi_wavelength struct.
 *
 * Sets the oi_wavelength::nwave attribute of @a pWave.
 *
 * @param pWave  pointer to wavelength data struct, see exchange.h
 * @param nwave  number of wavelength channels (table rows) to allocate
 */
void alloc_oi_wavelength(oi_wavelength *pWave, int nwave)
{
  pWave->eff_wave = malloc(nwave*sizeof(pWave->eff_band[0]));
  pWave->eff_band = malloc(nwave*sizeof(pWave->eff_band[0]));
  pWave->nwave = nwave;
}

/**
 * Allocate storage within oi_corr struct.
 *
 * Sets the oi_corr::ncorr attribute of @a pCorr.
 *
 * @param pCorr  pointer to corr data struct, see exchange.h
 * @param ncorr  number of correlated data (table rows) to allocate
 */
void alloc_oi_corr(oi_corr *pCorr, int ncorr)
{
  pCorr->iindx = malloc(ncorr*sizeof(pCorr->iindx[0]));
  pCorr->jindx = malloc(ncorr*sizeof(pCorr->jindx[0]));
  pCorr->corr = malloc(ncorr*sizeof(pCorr->corr[0]));
  pCorr->ncorr = ncorr;
}

/**
 * Allocate storage within oi_polar struct.
 *
 * Sets the oi_polar::numrec and oi_polar::nwave attributes of @a pPolar.
 *
 * @param pPolar  pointer to polar data struct, see exchange.h
 * @param numrec  number of records (table rows) to allocate
 * @param nwave   number of wavelength channels to allocate
 */
void alloc_oi_polar(oi_polar *pPolar, long numrec, int nwave)
{
  int i;
  oi_polar_record *pRec;
  
  pPolar->record = malloc(numrec*sizeof(oi_polar_record));
  for(i=0; i<numrec; i++) {
    pRec = &pPolar->record[i];
    pRec->lxx = malloc(nwave*sizeof(pRec->lxx[0]));
    pRec->lyy = malloc(nwave*sizeof(pRec->lyy[0]));
    pRec->lxy = malloc(nwave*sizeof(pRec->lxy[0]));
    pRec->lyx = malloc(nwave*sizeof(pRec->lyx[0]));
  }
  pPolar->numrec = numrec;
  pPolar->nwave = nwave;
}

/**
 * Allocate storage within oi_vis struct.
 *
 * Sets the oi_vis::numrec and oi_vis::nwave attributes of @a pVis.
 *
 * @param pVis    pointer to data struct, see exchange.h
 * @param numrec  number of records (table rows) to allocate
 * @param nwave   number of wavelength channels to allocate
 */
void alloc_oi_vis(oi_vis *pVis, long numrec, int nwave)
{
  int i;
  oi_vis_record *pRec;
  
  pVis->record = malloc(numrec*sizeof(oi_vis_record));
  for(i=0; i<numrec; i++) {
    pRec = &pVis->record[i];
    pRec->visamp = malloc(nwave*sizeof(pRec->visamp[0]));
    pRec->visamperr = malloc(nwave*sizeof(pRec->visamperr[0]));
    pRec->visphi = malloc(nwave*sizeof(pRec->visphi[0]));
    pRec->visphierr = malloc(nwave*sizeof(pRec->visphierr[0]));
    pRec->flag = malloc(nwave*sizeof(pRec->flag[0]));
    pRec->visrefmap = NULL;
    pRec->rvis = NULL;
    pRec->rviserr = NULL;
    pRec->ivis = NULL;
    pRec->iviserr = NULL;
  }
  pVis->numrec = numrec;
  pVis->nwave = nwave;
  pVis->usevisrefmap = FALSE;
  pVis->usecomplex = FALSE;
}

/**
 * Allocate storage within oi_vis2 struct.
 *
 * Sets the oi_vis2::numrec and oi_vis2::nwave attributes of @a pVis2.
 *
 * @param pVis2   pointer to data struct, see exchange.h
 * @param numrec  number of records (table rows) to allocate
 * @param nwave   number of wavelength channels to allocate
 */
void alloc_oi_vis2(oi_vis2 *pVis2, long numrec, int nwave)
{
  int i;
  oi_vis2_record *pRec;
  
  pVis2->record = malloc(numrec*sizeof(oi_vis2_record));
  for(i=0; i<numrec; i++) {
    pRec = &pVis2->record[i];
    pRec->vis2data = malloc(nwave*sizeof(pRec->vis2data[0]));
    pRec->vis2err = malloc(nwave*sizeof(pRec->vis2err[0]));
    pRec->flag = malloc(nwave*sizeof(pRec->flag[0]));
  }
  pVis2->numrec = numrec;
  pVis2->nwave = nwave;
}

/**
 * Allocate storage within oi_t3 struct.
 *
 * Sets the oi_t3::numrec and oi_t3::nwave attributes of @a pT3.
 *
 * @param pT3     pointer to data struct, see exchange.h
 * @param numrec  number of records (table rows) to allocate
 * @param nwave   number of wavelength channels to allocate
 */
void alloc_oi_t3(oi_t3 *pT3, long numrec, int nwave)
{
  int i;
  oi_t3_record *pRec;
  
  pT3->record = malloc(numrec*sizeof(oi_t3_record));
  for(i=0; i<numrec; i++) {
    pRec = &pT3->record[i];
    pRec->t3amp = malloc(nwave*sizeof(pRec->t3amp[0]));
    pRec->t3amperr = malloc(nwave*sizeof(pRec->t3amperr[0]));
    pRec->t3phi = malloc(nwave*sizeof(pRec->t3phi[0]));
    pRec->t3phierr = malloc(nwave*sizeof(pRec->t3phierr[0]));
    pRec->flag = malloc(nwave*sizeof(pRec->flag[0]));
  }
  pT3->numrec = numrec;
  pT3->nwave = nwave;
}

/**
 * Allocate storage within oi_spectrum struct.
 *
 * Sets the oi_spectrum::numrec and oi_spectrum::nwave attributes of
 * @a pSpectrum.
 *
 * @param pSpectrum  pointer to data struct, see exchange.h
 * @param numrec     number of records (table rows) to allocate
 * @param nwave      number of wavelength channels to allocate
 */
void alloc_oi_spectrum(oi_spectrum *pSpectrum, long numrec, int nwave)
{
  int i;
  oi_spectrum_record *pRec;
  
  pSpectrum->record = malloc(numrec*sizeof(oi_spectrum_record));
  for(i=0; i<numrec; i++) {
    pRec = &pSpectrum->record[i];
    pRec->fluxdata = malloc(nwave*sizeof(pRec->fluxdata[0]));
    pRec->fluxerr = malloc(nwave*sizeof(pRec->fluxerr[0]));
  }
  pSpectrum->numrec = numrec;
  pSpectrum->nwave = nwave;
}