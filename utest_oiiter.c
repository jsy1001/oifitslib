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

#define FILENAME "test/OIFITS2/bigtest2.fits"

typedef struct {
  oi_fits inData;

} TestFixture;

static void setup_fixture(TestFixture *fix, gconstpointer userData)
{
  int status;
  char msg[FLEN_ERRMSG];
  const char *filename = userData;

  g_debug("setup_fixture");
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


static void test_vis2(TestFixture *fix, gconstpointer userData)
{
  oi_vis2_iter iter;
  oi_vis2_record *pRec;
  int extver, iwave;
  long irec;

  g_debug("test_vis2");
  oi_vis2_iter_init(&iter, &fix->inData, NULL);
  while (oi_vis2_iter_next(&iter, &extver, &irec, &pRec, &iwave))
  {
    g_debug("OI_VIS2 EXTVER=%2d rec%03ld chan%02d V^2=%6.3lf",
            extver, irec, iwave, pRec->vis2data[iwave]);
  }
}


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add("/oifitslib/oiiter/vis2", TestFixture, FILENAME,
             setup_fixture, test_vis2, teardown_fixture);

  return g_test_run();
}

