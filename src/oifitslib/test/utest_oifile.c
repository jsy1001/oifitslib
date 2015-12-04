/**
 * @file
 * @ingroup oifile
 * Unit tests of OIFITS file-level API.
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

#include "oifile.h"
#include <unistd.h>  /* unlink() */
#include <math.h>
#include <string.h>

#define FILENAME_OUT "utest_oifile.fits"
#define FILENAME_V1 "OIFITS1/alp_aur--COAST_NICMOS.fits"
#define FILENAME_V2 "OIFITS2/alp_aur--COAST_NICMOS.fits"
#define FILENAME_ATOMIC "OIFITS2/alp_aur--COAST_NICMOS.fits"
#define FILENAME_MULTI "OIFITS2/bigtest2.fits"

#define MULTI_NUM_VIS 40
#define MULTI_NUM_VIS2 40
#define MULTI_NUM_T3 40


static void test_init(void)
{
  oi_fits data;
  int status;

  init_oi_fits(&data);
  status = 0;
  g_assert(write_oi_fits(FILENAME_OUT, data, &status) == 0);
  unlink(FILENAME_OUT);
  free_oi_fits(&data);
}

static void test_version(void)
{
  oi_fits data;
  int status;
  char msg[FLEN_ERRMSG];

  status = 0;
  read_oi_fits(FILENAME_V1, &data, &status);
  g_assert(!status);
  g_assert(is_oi_fits_one(&data));
  g_assert(!is_oi_fits_two(&data));
  free_oi_fits(&data);

  read_oi_fits(FILENAME_V2, &data, &status);
  g_assert(!status);
  g_assert(!is_oi_fits_one(&data));
  g_assert(is_oi_fits_two(&data));
  free_oi_fits(&data);

  if (fits_read_errmsg(msg))
    g_error("Uncleared CFITSIO error message: %s", msg);
}

static void test_atomic(void)
{
  oi_fits data;
  int status;
  char msg[FLEN_ERRMSG];

  init_oi_fits(&data);
  g_assert(!is_atomic(&data, 0.5));
  free_oi_fits(&data);

  status = 0;
  read_oi_fits(FILENAME_ATOMIC, &data, &status);
  g_assert(!status);
  g_assert(is_atomic(&data, 0.5));
  free_oi_fits(&data);

  read_oi_fits(FILENAME_MULTI, &data, &status);
  g_assert(!status);
  g_assert(!is_atomic(&data, 0.5));
  free_oi_fits(&data);

  if (fits_read_errmsg(msg))
    g_error("Uncleared CFITSIO error message: %s", msg);
}

static void test_count(void)
{
  oi_fits data;
  int status;
  char msg[FLEN_ERRMSG];
  long numVis, numVis2, numT3;

  status = 0;
  read_oi_fits(FILENAME_MULTI, &data, &status);

  if (fits_read_errmsg(msg))
    g_error("Uncleared CFITSIO error message: %s", msg);

  count_oi_fits_data(&data, &numVis, &numVis2, &numT3);

  g_assert_cmpint(numVis, ==, MULTI_NUM_VIS);
  g_assert_cmpint(numVis2, ==, MULTI_NUM_VIS2);
  g_assert_cmpint(numT3, ==, MULTI_NUM_T3);
  free_oi_fits(&data);
}

static void test_lookup(void)
{
  oi_fits data;
  int status;
  char msg[FLEN_ERRMSG];
  oi_vis2 *pVis2;
  oi_array *pArray;
  element *pElem;
  oi_wavelength *pWave;
  oi_corr *pCorr;
  target *pTarg1, *pTarg2;

  status = 0;
  read_oi_fits(FILENAME_MULTI, &data, &status);

  if (fits_read_errmsg(msg))
    g_error("Uncleared CFITSIO error message: %s", msg);

  pVis2 = (oi_vis2 *)data.vis2List->data;

  if (strlen(pVis2->arrname) > 0) {
    pArray = oi_fits_lookup_array(&data, pVis2->arrname);
    g_assert(pArray != NULL);
    g_assert_cmpstr(pArray->arrname, ==, pVis2->arrname);
    pElem = oi_fits_lookup_element(&data, pVis2->arrname, 1);
    g_assert(pElem != NULL);
    g_assert_cmpint(pElem->sta_index, ==, 1);
  }

  pWave = oi_fits_lookup_wavelength(&data, pVis2->insname);
  g_assert(pWave != NULL);
  g_assert_cmpstr(pWave->insname, ==, pVis2->insname);
  
  if (strlen(pVis2->corrname) > 0) {
    pCorr = oi_fits_lookup_corr(&data, pVis2->corrname);
    g_assert(pCorr != NULL);
    g_assert_cmpstr(pCorr->corrname, ==, pVis2->corrname);
  }

  pTarg1 = oi_fits_lookup_target(&data, 1);
  g_assert_cmpint(pTarg1->target_id, ==, 1);
  pTarg2 = oi_fits_lookup_target_by_name(&data, pTarg1->target);
  g_assert_cmpstr(pTarg2->target, ==, pTarg1->target);

  free_oi_fits(&data);
}


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/oifitslib/oifile/init", test_init);
  g_test_add_func("/oifitslib/oifile/version", test_version);
  g_test_add_func("/oifitslib/oifile/atomic", test_atomic);
  g_test_add_func("/oifitslib/oifile/count", test_count);
  g_test_add_func("/oifitslib/oifile/lookup", test_lookup);

  return g_test_run();
}
