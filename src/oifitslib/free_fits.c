/**
 * @file
 * @ingroup oitable
 * Implementation of functions to free storage allocated by routines
 * in read_fits.c
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
 * Free dynamically-allocated storage within oi_array struct
 *
 * @param pArray  pointer to array data struct, see exchange.h
 */
void free_oi_array(oi_array *pArray)
{
  free(pArray->elem);
}

/**
 * Free dynamically-allocated storage within oi_target struct
 *
 * @param pTargets  pointer to targets data struct, see exchange.h
 */
void free_oi_target(oi_target *pTargets)
{
  free(pTargets->targ);
}

/**
 * Free dynamically-allocated storage within oi_wavelength struct
 *
 * @param pWave  pointer to wavelength data struct, see exchange.h
 */
void free_oi_wavelength(oi_wavelength *pWave)
{
  free(pWave->eff_wave);
  free(pWave->eff_band);
}

/**
 * Free dynamically-allocated storage within oi_corr struct
 *
 * @param pCorr  pointer to corr data struct, see exchange.h
 */
void free_oi_corr(oi_corr *pCorr)
{
  free(pCorr->iindx);
  free(pCorr->jindx);
  free(pCorr->corr);
}

/**
 * Free dynamically-allocated storage within oi_inspol struct
 *
 * @param pInspol  pointer to inspol data struct, see exchange.h
 */
void free_oi_inspol(oi_inspol *pInspol)
{
  int i;

  for (i = 0; i < pInspol->numrec; i++) {
    free(pInspol->record[i].lxx);
    free(pInspol->record[i].lyy);
    free(pInspol->record[i].lxy);
    free(pInspol->record[i].lyx);
  }
  free(pInspol->record);
}

/**
 * Free dynamically-allocated storage within oi_vis struct
 *
 * @param pVis  pointer to data struct, see exchange.h
 */
void free_oi_vis(oi_vis *pVis)
{
  int i;

  for (i = 0; i < pVis->numrec; i++) {
    free(pVis->record[i].visamp);
    free(pVis->record[i].visamperr);
    free(pVis->record[i].visphi);
    free(pVis->record[i].visphierr);
    free(pVis->record[i].flag);

    if (pVis->usevisrefmap)
      free(pVis->record[i].visrefmap);

    if (pVis->usecomplex) {
      free(pVis->record[i].rvis);
      free(pVis->record[i].rviserr);
      free(pVis->record[i].ivis);
      free(pVis->record[i].iviserr);
    }
  }
  free(pVis->record);
}

/**
 * Free dynamically-allocated storage within oi_vis2 struct
 *
 * @param pVis2  pointer to data struct, see exchange.h
 */
void free_oi_vis2(oi_vis2 *pVis2)
{
  int i;

  for (i = 0; i < pVis2->numrec; i++) {
    free(pVis2->record[i].vis2data);
    free(pVis2->record[i].vis2err);
    free(pVis2->record[i].flag);
  }
  free(pVis2->record);
}

/**
 * Free dynamically-allocated storage within oi_t3 struct
 *
 * @param pT3  pointer to data struct, see exchange.h
 */
void free_oi_t3(oi_t3 *pT3)
{
  int i;

  for (i = 0; i < pT3->numrec; i++) {
    free(pT3->record[i].t3amp);
    free(pT3->record[i].t3amperr);
    free(pT3->record[i].t3phi);
    free(pT3->record[i].t3phierr);
    free(pT3->record[i].flag);
  }
  free(pT3->record);
}

/**
 * Free dynamically-allocated storage within oi_flux struct
 *
 * @param pFlux  pointer to data struct, see exchange.h
 */
void free_oi_flux(oi_flux *pFlux)
{
  int i;

  for (i = 0; i < pFlux->numrec; i++) {
    free(pFlux->record[i].fluxdata);
    free(pFlux->record[i].fluxerr);
    free(pFlux->record[i].flag);
  }
  free(pFlux->record);
}
