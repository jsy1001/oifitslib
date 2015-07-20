/**
 * @file
 * @ingroup oifilter
 * Unit tests of OIFITS filter.
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
#include "oifile.h"

#include <math.h>

#define FILENAME "test/OIFITS2/bigtest2.fits"

typedef struct {
  oi_fits inData;

} TestFixture;

static void setup_fixture(TestFixture *fix, gconstpointer userData)
{
  int status;
  char msg[FLEN_ERRMSG];
  const char *filename = userData;

  status = 0;
  read_oi_fits(filename, &fix->inData, &status);
  g_assert(!status);
  if (fits_read_errmsg(msg))
    g_error("Uncleared CFITSIO error message: %s", msg);
}

static void teardown_fixture(TestFixture *fix, gconstpointer userData)
{
  free_oi_fits(&fix->inData);
}


static void test_vis2_default(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2_record *pRec;
  int extver, iwave;
  int lastextver, lastwave;
  long irec;
  long lastrec, nvis2, nvis2in;

  init_oi_filter(&filt);
  filt.accept_flagged = false; /* count_oi_fits_data() counts unflagged data */
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  lastextver = -1;
  lastrec = -1;
  lastwave = -1;
  nvis2 = 0;
  while (oi_vis2_iter_next(&iter, &extver, NULL, &irec, &pRec, &iwave)) {
    ++nvis2;
    if (extver == lastextver) {
      g_assert_cmpint(irec, >=, lastrec);
      if (irec == lastrec)
        g_assert_cmpint(iwave, >=, lastwave);
    }
    lastextver = extver;
    lastrec = irec;
    lastwave = iwave;
  }
  count_oi_fits_data(&fix->inData, NULL, &nvis2in, NULL);
  g_assert_cmpint(nvis2, ==, nvis2in);
}

static void test_vis2_arrname(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2 *pTable;
  
  init_oi_filter(&filt);
  g_strlcpy(filt.arrname, "C?ARA*", FLEN_VALUE);
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL)) {
    g_assert_cmpstr(pTable->arrname, ==, "CHARA_2004Jan");
  }
}

static void test_vis2_insname(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2 *pTable;
  
  init_oi_filter(&filt);
  g_strlcpy(filt.insname, "*I?NIC*", FLEN_VALUE);
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL)) {
    g_assert_cmpstr(pTable->insname, ==, "IOTA_IONIC_PICNIC");
  }
}

static void test_vis2_corrname(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2 *pTable;
  
  init_oi_filter(&filt);
  g_strlcpy(filt.corrname, "*TE?T", FLEN_VALUE);
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL)) {
    g_assert_cmpstr(pTable->corrname, ==, "TEST");
  }
}
static void test_vis2_target(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2_record *pRec;
  
  init_oi_filter(&filt);
  filt.target_id = 1;
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL)) {
    g_assert_cmpint(pRec->target_id, ==, 1);
  }
}

static void test_vis2_wave(TestFixture *fix, gconstpointer userData)
{
  const float range[2] = {1500.0e-9, 1700.0e-9};
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2 *pTable;
  oi_wavelength *pWave;
  int iwave;
  
  init_oi_filter(&filt);
  filt.wave_range[0] = range[0];
  filt.wave_range[1] = range[1];
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, &pTable, NULL, NULL, &iwave)) {
    pWave = oi_fits_lookup_wavelength(&fix->inData, pTable->insname);
    g_assert_cmpfloat(pWave->eff_wave[iwave], >=, range[0]);
    g_assert_cmpfloat(pWave->eff_wave[iwave], <=, range[1]);
  }
}

static void test_vis2_mjd(TestFixture *fix, gconstpointer userData)
{
  /* note bigtest2.fits has nonsense MJD values */
  const float range[2] = {0.0, 0.0075};
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2_record *pRec;
  
  init_oi_filter(&filt);
  filt.mjd_range[0] = range[0];
  filt.mjd_range[1] = range[1];
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL)) {
    g_assert_cmpfloat(pRec->mjd, >=, range[0]);
    g_assert_cmpfloat(pRec->mjd, <=, range[1]);
  }
}

static void test_vis2_bas(TestFixture *fix, gconstpointer userData)
{
  const double range[2] = {0.0, 3.0};
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2_record *pRec;
  double bas;
  
  init_oi_filter(&filt);
  filt.bas_range[0] = range[0];
  filt.bas_range[1] = range[1];
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL)) {
    bas = pow(pRec->ucoord * pRec->ucoord + pRec->vcoord * pRec->vcoord, 0.5);
    g_assert_cmpfloat(bas, >=, range[0]);
    g_assert_cmpfloat(bas, <=, range[1]);
  }
}

static void test_vis2_uvrad(TestFixture *fix, gconstpointer userData)
{
  const double range[2] = {0.0, 1e8};
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2 *pTable;
  oi_vis2_record *pRec;
  oi_wavelength *pWave;
  int iwave;
  double uvrad;
  
  init_oi_filter(&filt);
  filt.uvrad_range[0] = range[0];
  filt.uvrad_range[1] = range[1];
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, &pTable, NULL, &pRec, &iwave)) {
    pWave = oi_fits_lookup_wavelength(&fix->inData, pTable->insname);
    uvrad = (pow(pRec->ucoord * pRec->ucoord +
                 pRec->vcoord * pRec->vcoord, 0.5) / pWave->eff_wave[iwave]);
    g_assert_cmpfloat(uvrad, >=, range[0]);
    g_assert_cmpfloat(uvrad, <=, range[1]);
  }
}

static void test_vis2_snr(TestFixture *fix, gconstpointer userData)
{
  const double range[2] = {20.0, 100.0};
  oi_filter_spec filt;
  oi_vis2_iter iter;
  oi_vis2_record *pRec;
  int iwave;
  double snr;
  
  init_oi_filter(&filt);
  filt.snr_range[0] = range[0];
  filt.snr_range[1] = range[1];
  oi_vis2_iter_init(&iter, &fix->inData, &filt);
  while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, &pRec, &iwave)) {
    snr = pRec->vis2data[iwave] / pRec->vis2err[iwave];
    g_assert_cmpfloat(snr, >=, range[0]);
    g_assert_cmpfloat(snr, <=, range[1]);
  }
}


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add("/oifitslib/oiiter/vis2/default", TestFixture, FILENAME,
             setup_fixture, test_vis2_default, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/arrname", TestFixture, FILENAME,
             setup_fixture, test_vis2_arrname, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/insname", TestFixture, FILENAME,
             setup_fixture, test_vis2_insname, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/corrname", TestFixture, FILENAME,
             setup_fixture, test_vis2_corrname, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/target", TestFixture, FILENAME,
             setup_fixture, test_vis2_target, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/wave", TestFixture, FILENAME,
             setup_fixture, test_vis2_wave, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/mjd", TestFixture, FILENAME,
             setup_fixture, test_vis2_mjd, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/bas", TestFixture, FILENAME,
             setup_fixture, test_vis2_bas, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/uvrad", TestFixture, FILENAME,
             setup_fixture, test_vis2_uvrad, teardown_fixture);
  g_test_add("/oifitslib/oiiter/vis2/snr", TestFixture, FILENAME,
             setup_fixture, test_vis2_snr, teardown_fixture);

  return g_test_run();
}

