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

#define FILENAME_OUT "utest_oifile.fits"
#define FILENAME_V1 "test/OIFITS1/alp_aur--COAST_NICMOS.fits"
#define FILENAME_V2 "test/OIFITS2/alp_aur--COAST_NICMOS.fits"
#define FILENAME_ATOMIC "test/OIFITS2/alp_aur--COAST_NICMOS.fits"
#define FILENAME_MULTI "test/OIFITS2/bigtest2.fits"

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


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/oifitslib/oifile/init", test_init);
  g_test_add_func("/oifitslib/oifile/version", test_version);
  g_test_add_func("/oifitslib/oifile/atomic", test_atomic);
  g_test_add_func("/oifitslib/oifile/count", test_count);

  return g_test_run();
}
