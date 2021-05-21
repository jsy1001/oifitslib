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

#define FILENAME "OIFITS2/bigtest2.fits"
#define RAD2DEG (180.0 / 3.14159)

typedef struct
{
  oi_fits inData;

} TestFixture;

static void setup_fixture(TestFixture *fix, gconstpointer userData)
{
  int status;
  char msg[FLEN_ERRMSG];
  const char *filename = userData;

  status = 0;
  read_oi_fits(filename, &fix->inData, &status);
  g_assert_false(status);
  if (fits_read_errmsg(msg))
    g_error("Uncleared CFITSIO error message: %s", msg);
}

static void teardown_fixture(TestFixture *fix, gconstpointer userData)
{
  free_oi_fits(&fix->inData);
}

static void test_default_vis(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_vis_iter iter;
  oi_vis_record *pRec;
  int extver, iwave;
  int lastextver, lastwave;
  long irec;
  long lastrec, ndata, ntotal;

  oi_vis_iter_init(&iter, &fix->inData, pFilter);
  oi_vis_iter_next(&iter, NULL, NULL, NULL, NULL, NULL);
  oi_vis_iter_init(&iter, &fix->inData, pFilter); /* test re-init */
  lastextver = -1;
  lastrec = -1;
  lastwave = -1;
  ndata = 0;
  while (oi_vis_iter_next(&iter, &extver, NULL, &irec, &pRec, &iwave))
  {
    ++ndata;
    if (extver == lastextver)
    {
      g_assert_cmpint(irec, >=, lastrec);
      if (irec == lastrec) g_assert_cmpint(iwave, >=, lastwave);
    }
    lastextver = extver;
    lastrec = irec;
    lastwave = iwave;
  }
  count_oi_fits_data(&fix->inData, &ntotal, NULL, NULL);
  g_assert_cmpint(ndata, ==, ntotal);
}

static void test_default_vis2(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_vis2_iter iter;
  oi_vis2_record *pRec;
  int extver, iwave;
  int lastextver, lastwave;
  long irec;
  long lastrec, ndata, ntotal;

  oi_vis2_iter_init(&iter, &fix->inData, pFilter);
  oi_vis2_iter_next(&iter, NULL, NULL, NULL, NULL, NULL);
  oi_vis2_iter_init(&iter, &fix->inData, pFilter); /* test re-init */
  lastextver = -1;
  lastrec = -1;
  lastwave = -1;
  ndata = 0;
  while (oi_vis2_iter_next(&iter, &extver, NULL, &irec, &pRec, &iwave))
  {
    ++ndata;
    if (extver == lastextver)
    {
      g_assert_cmpint(irec, >=, lastrec);
      if (irec == lastrec) g_assert_cmpint(iwave, >=, lastwave);
    }
    lastextver = extver;
    lastrec = irec;
    lastwave = iwave;
  }
  count_oi_fits_data(&fix->inData, NULL, &ntotal, NULL);
  g_assert_cmpint(ndata, ==, ntotal);
}

static void test_default_t3(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_t3_iter iter;
  oi_t3_record *pRec;
  int extver, iwave;
  int lastextver, lastwave;
  long irec;
  long lastrec, ndata, ntotal;

  oi_t3_iter_init(&iter, &fix->inData, pFilter);
  oi_t3_iter_next(&iter, NULL, NULL, NULL, NULL, NULL);
  oi_t3_iter_init(&iter, &fix->inData, pFilter); /* test re-init */
  lastextver = -1;
  lastrec = -1;
  lastwave = -1;
  ndata = 0;
  while (oi_t3_iter_next(&iter, &extver, NULL, &irec, &pRec, &iwave))
  {
    ++ndata;
    if (extver == lastextver)
    {
      g_assert_cmpint(irec, >=, lastrec);
      if (irec == lastrec) g_assert_cmpint(iwave, >=, lastwave);
    }
    lastextver = extver;
    lastrec = irec;
    lastwave = iwave;
  }
  count_oi_fits_data(&fix->inData, NULL, NULL, &ntotal);
  g_assert_cmpint(ndata, ==, ntotal);
}

static void test_default(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;

  init_oi_filter(&filt);
  filt.accept_flagged = false; /* count_oi_fits_data() counts unflagged data */

  test_default_vis(fix, &filt);
  test_default_vis2(fix, &filt);
  test_default_t3(fix, &filt);
}

static void test_arrname(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;

  init_oi_filter(&filt);
  g_strlcpy(filt.arrname, "C?ARA*", FLEN_VALUE);

  {
    oi_vis_iter iter;
    oi_vis *pTable;
    oi_vis_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->arrname, ==, "CHARA_2004Jan");
    }
  }
  {
    oi_vis2_iter iter;
    oi_vis2 *pTable;
    oi_vis2_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis2_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->arrname, ==, "CHARA_2004Jan");
    }
  }
  {
    oi_t3_iter iter;
    oi_t3 *pTable;
    oi_t3_iter_init(&iter, &fix->inData, &filt);
    while (oi_t3_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->arrname, ==, "CHARA_2004Jan");
    }
  }
}

static void test_insname(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;

  init_oi_filter(&filt);
  g_strlcpy(filt.insname, "*I?NIC*", FLEN_VALUE);

  {
    oi_vis_iter iter;
    oi_vis *pTable;
    oi_vis_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->insname, ==, "IOTA_IONIC_PICNIC");
    }
  }
  {
    oi_vis2_iter iter;
    oi_vis2 *pTable;
    oi_vis2_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis2_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->insname, ==, "IOTA_IONIC_PICNIC");
    }
  }
  {
    oi_t3_iter iter;
    oi_t3 *pTable;
    oi_t3_iter_init(&iter, &fix->inData, &filt);
    while (oi_t3_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->insname, ==, "IOTA_IONIC_PICNIC");
    }
  }
}

static void test_corrname(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;

  init_oi_filter(&filt);
  g_strlcpy(filt.corrname, "*TE?T", FLEN_VALUE);

  {
    oi_vis_iter iter;
    oi_vis *pTable;
    oi_vis_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->corrname, ==, "TEST");
    }
  }
  {
    oi_vis2_iter iter;
    oi_vis2 *pTable;
    oi_vis2_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis2_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->corrname, ==, "TEST");
    }
  }
  {
    oi_t3_iter iter;
    oi_t3 *pTable;
    oi_t3_iter_init(&iter, &fix->inData, &filt);
    while (oi_t3_iter_next(&iter, NULL, &pTable, NULL, NULL, NULL))
    {
      g_assert_cmpstr(pTable->corrname, ==, "TEST");
    }
  }
}

static void test_target(TestFixture *fix, gconstpointer userData)
{
  oi_filter_spec filt;

  init_oi_filter(&filt);
  filt.target_id = 1;

  {
    oi_vis_iter iter;
    oi_vis_record *pRec;
    oi_vis_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
    {
      g_assert_cmpint(pRec->target_id, ==, 1);
    }
  }
  {
    oi_vis2_iter iter;
    oi_vis2_record *pRec;
    oi_vis2_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
    {
      g_assert_cmpint(pRec->target_id, ==, 1);
    }
  }
  {
    oi_t3_iter iter;
    oi_t3_record *pRec;
    oi_t3_iter_init(&iter, &fix->inData, &filt);
    while (oi_t3_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
    {
      g_assert_cmpint(pRec->target_id, ==, 1);
    }
  }
}

static void test_wave(TestFixture *fix, gconstpointer userData)
{
  const float range[2] = {1500.0e-9, 1700.0e-9};
  oi_filter_spec filt;

  init_oi_filter(&filt);
  filt.wave_range[0] = range[0];
  filt.wave_range[1] = range[1];

  {
    oi_vis_iter iter;
    oi_vis_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis_iter_next(&iter, NULL, NULL, NULL, NULL, NULL))
    {
      double effWave;
      oi_vis_iter_get_uv(&iter, &effWave, NULL, NULL);
      g_assert_cmpfloat(effWave, >=, range[0]);
      g_assert_cmpfloat(effWave, <=, range[1]);
    }
  }
  {
    oi_vis2_iter iter;
    oi_vis2_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, NULL, NULL))
    {
      double effWave;
      oi_vis2_iter_get_uv(&iter, &effWave, NULL, NULL);
      g_assert_cmpfloat(effWave, >=, range[0]);
      g_assert_cmpfloat(effWave, <=, range[1]);
    }
  }
  {
    oi_t3_iter iter;
    oi_t3_iter_init(&iter, &fix->inData, &filt);
    while (oi_t3_iter_next(&iter, NULL, NULL, NULL, NULL, NULL))
    {
      double effWave;
      oi_t3_iter_get_uv(&iter, &effWave, NULL, NULL, NULL, NULL);
      g_assert_cmpfloat(effWave, >=, range[0]);
      g_assert_cmpfloat(effWave, <=, range[1]);
    }
  }
}

static void test_mjd(TestFixture *fix, gconstpointer userData)
{
  /* note bigtest2.fits has nonsense MJD values */
  const float range[2] = {0.0, 0.0075};
  oi_filter_spec filt;

  init_oi_filter(&filt);
  filt.mjd_range[0] = range[0];
  filt.mjd_range[1] = range[1];

  {
    oi_vis_iter iter;
    oi_vis_record *pRec;
    oi_vis_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
    {
      g_assert_cmpfloat(pRec->mjd, >=, range[0]);
      g_assert_cmpfloat(pRec->mjd, <=, range[1]);
    }
  }
  {
    oi_vis2_iter iter;
    oi_vis2_record *pRec;
    oi_vis2_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
    {
      g_assert_cmpfloat(pRec->mjd, >=, range[0]);
      g_assert_cmpfloat(pRec->mjd, <=, range[1]);
    }
  }
  {
    oi_t3_iter iter;
    oi_t3_record *pRec;
    oi_t3_iter_init(&iter, &fix->inData, &filt);
    while (oi_t3_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
    {
      g_assert_cmpfloat(pRec->mjd, >=, range[0]);
      g_assert_cmpfloat(pRec->mjd, <=, range[1]);
    }
  }
}

static void test_bas_vis(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_vis_iter iter;
  oi_vis_record *pRec;
  double bas;

  oi_vis_iter_init(&iter, &fix->inData, pFilter);
  while (oi_vis_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
  {
    bas = pow(pRec->ucoord * pRec->ucoord + pRec->vcoord * pRec->vcoord, 0.5);
    g_assert_cmpfloat(bas, >=, pFilter->bas_range[0]);
    g_assert_cmpfloat(bas, <=, pFilter->bas_range[1]);
  }
}

static void test_bas_vis2(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_vis2_iter iter;
  oi_vis2_record *pRec;
  double bas;

  oi_vis2_iter_init(&iter, &fix->inData, pFilter);
  while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
  {
    bas = pow(pRec->ucoord * pRec->ucoord + pRec->vcoord * pRec->vcoord, 0.5);
    g_assert_cmpfloat(bas, >=, pFilter->bas_range[0]);
    g_assert_cmpfloat(bas, <=, pFilter->bas_range[1]);
  }
}

static void test_bas_t3(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_t3_iter iter;
  oi_t3_record *pRec;
  double u1, v1, u2, v2, bas;

  oi_t3_iter_init(&iter, &fix->inData, pFilter);
  while (oi_t3_iter_next(&iter, NULL, NULL, NULL, &pRec, NULL))
  {
    u1 = pRec->u1coord;
    v1 = pRec->v1coord;
    u2 = pRec->u2coord;
    v2 = pRec->v2coord;
    bas = pow(u1 * u1 + v1 * v1, 0.5);
    g_assert_cmpfloat(bas, >=, pFilter->bas_range[0]);
    g_assert_cmpfloat(bas, <=, pFilter->bas_range[1]);
    bas = pow(u2 * u2 + v2 * v2, 0.5);
    g_assert_cmpfloat(bas, >=, pFilter->bas_range[0]);
    g_assert_cmpfloat(bas, <=, pFilter->bas_range[1]);
    bas = pow((u1 + u2) * (u1 + u2) + (v1 + v2) * (v1 + v2), 0.5);
    g_assert_cmpfloat(bas, >=, pFilter->bas_range[0]);
    g_assert_cmpfloat(bas, <=, pFilter->bas_range[1]);
  }
}

static void test_bas(TestFixture *fix, gconstpointer userData)
{
  const double range[2] = {0.0, 3.0};
  oi_filter_spec filt;

  init_oi_filter(&filt);
  filt.bas_range[0] = range[0];
  filt.bas_range[1] = range[1];

  test_bas_vis(fix, &filt);
  test_bas_vis2(fix, &filt);
  test_bas_t3(fix, &filt);
}

static void test_uvrad_vis(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_vis_iter iter;
  double u, v, uvrad;

  oi_vis_iter_init(&iter, &fix->inData, pFilter);
  while (oi_vis_iter_next(&iter, NULL, NULL, NULL, NULL, NULL))
  {
    oi_vis_iter_get_uv(&iter, NULL, &u, &v);
    uvrad = pow(u * u + v * v, 0.5);
    g_assert_cmpfloat(uvrad, >=, pFilter->uvrad_range[0]);
    g_assert_cmpfloat(uvrad, <=, pFilter->uvrad_range[1]);
  }
}

static void test_uvrad_vis2(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_vis2_iter iter;
  double u, v, uvrad;

  oi_vis2_iter_init(&iter, &fix->inData, pFilter);
  while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, NULL, NULL))
  {
    oi_vis2_iter_get_uv(&iter, NULL, &u, &v);
    uvrad = pow(u * u + v * v, 0.5);
    g_assert_cmpfloat(uvrad, >=, pFilter->uvrad_range[0]);
    g_assert_cmpfloat(uvrad, <=, pFilter->uvrad_range[1]);
  }
}

static void test_uvrad_t3(TestFixture *fix, const oi_filter_spec *pFilter)
{
  oi_t3_iter iter;
  double u1, v1, u2, v2, uvrad;

  oi_t3_iter_init(&iter, &fix->inData, pFilter);
  while (oi_t3_iter_next(&iter, NULL, NULL, NULL, NULL, NULL))
  {
    oi_t3_iter_get_uv(&iter, NULL, &u1, &v1, &u2, &v2);
    uvrad = pow(u1 * u1 + v1 * v1, 0.5);
    g_assert_cmpfloat(uvrad, >=, pFilter->uvrad_range[0]);
    g_assert_cmpfloat(uvrad, <=, pFilter->uvrad_range[1]);
    uvrad = pow(u2 * u2 + v2 * v2, 0.5);
    g_assert_cmpfloat(uvrad, >=, pFilter->uvrad_range[0]);
    g_assert_cmpfloat(uvrad, <=, pFilter->uvrad_range[1]);
    uvrad = pow((u1 + u2) * (u1 + u2) + (v1 + v2) * (v1 + v2), 0.5);
    g_assert_cmpfloat(uvrad, >=, pFilter->uvrad_range[0]);
    g_assert_cmpfloat(uvrad, <=, pFilter->uvrad_range[1]);
  }
}

static void test_uvrad(TestFixture *fix, gconstpointer userData)
{
  const double range[2] = {0.0, 1e8};
  oi_filter_spec filt;

  init_oi_filter(&filt);
  filt.uvrad_range[0] = range[0];
  filt.uvrad_range[1] = range[1];

  test_uvrad_vis(fix, &filt);
  test_uvrad_vis2(fix, &filt);
  test_uvrad_t3(fix, &filt);
}

static void test_snr(TestFixture *fix, gconstpointer userData)
{
  const double range[2] = {20.0, 100.0};
  oi_filter_spec filt;
  int iwave;
  double snrAmp, snrPhi;

  init_oi_filter(&filt);
  filt.snr_range[0] = range[0];
  filt.snr_range[1] = range[1];

  {
    oi_vis_iter iter;
    oi_vis_record *pRec;
    oi_vis_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis_iter_next(&iter, NULL, NULL, NULL, &pRec, &iwave))
    {
      snrAmp = pRec->visamp[iwave] / pRec->visamperr[iwave];
      g_assert_cmpfloat(snrAmp, >=, range[0]);
      g_assert_cmpfloat(snrAmp, <=, range[1]);
      snrPhi = RAD2DEG / pRec->visphierr[iwave];
      g_assert_cmpfloat(snrPhi, >=, range[0]);
      g_assert_cmpfloat(snrPhi, <=, range[1]);
    }
  }
  {
    oi_vis2_iter iter;
    oi_vis2_record *pRec;
    oi_vis2_iter_init(&iter, &fix->inData, &filt);
    while (oi_vis2_iter_next(&iter, NULL, NULL, NULL, &pRec, &iwave))
    {
      snrAmp = pRec->vis2data[iwave] / pRec->vis2err[iwave];
      g_assert_cmpfloat(snrAmp, >=, range[0]);
      g_assert_cmpfloat(snrAmp, <=, range[1]);
    }
  }
  {
    oi_t3_iter iter;
    oi_t3_record *pRec;
    oi_t3_iter_init(&iter, &fix->inData, &filt);
    while (oi_t3_iter_next(&iter, NULL, NULL, NULL, &pRec, &iwave))
    {
      snrAmp = pRec->t3amp[iwave] / pRec->t3amperr[iwave];
      g_assert_cmpfloat(snrAmp, >=, range[0]);
      g_assert_cmpfloat(snrAmp, <=, range[1]);
      snrPhi = RAD2DEG / pRec->t3phierr[iwave];
      g_assert_cmpfloat(snrPhi, >=, range[0]);
      g_assert_cmpfloat(snrPhi, <=, range[1]);
    }
  }
}

int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add("/oifitslib/oiiter/default", TestFixture, FILENAME, setup_fixture,
             test_default, teardown_fixture);
  g_test_add("/oifitslib/oiiter/arrname", TestFixture, FILENAME, setup_fixture,
             test_arrname, teardown_fixture);
  g_test_add("/oifitslib/oiiter/insname", TestFixture, FILENAME, setup_fixture,
             test_insname, teardown_fixture);
  g_test_add("/oifitslib/oiiter/corrname", TestFixture, FILENAME, setup_fixture,
             test_corrname, teardown_fixture);
  g_test_add("/oifitslib/oiiter/target", TestFixture, FILENAME, setup_fixture,
             test_target, teardown_fixture);
  g_test_add("/oifitslib/oiiter/wave", TestFixture, FILENAME, setup_fixture,
             test_wave, teardown_fixture);
  g_test_add("/oifitslib/oiiter/mjd", TestFixture, FILENAME, setup_fixture,
             test_mjd, teardown_fixture);
  g_test_add("/oifitslib/oiiter/bas", TestFixture, FILENAME, setup_fixture,
             test_bas, teardown_fixture);
  g_test_add("/oifitslib/oiiter/uvrad", TestFixture, FILENAME, setup_fixture,
             test_uvrad, teardown_fixture);
  g_test_add("/oifitslib/oiiter/snr", TestFixture, FILENAME, setup_fixture,
             test_snr, teardown_fixture);

  return g_test_run();
}
