/**
 * @file
 * @ingroup oifile
 * Unit tests of OIFITS file-level API.
 *
 * Copyright (C) 2014 John Young
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

#define FILENAME "utest_oifile.fits"


static void test_init(void)
{
  oi_fits data;
  int status;
  
  init_oi_fits(&data);
  status = 0;
  g_assert(write_oi_fits(FILENAME, data, &status) == 0);
  unlink(FILENAME);
}


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/oifitslib/oifile/init", test_init);

  return g_test_run();
}
