/**
 * @file
 * @ingroup oifile
 * Unit tests of Gregorian date/MJD conversion.
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

#include "datemjd.h"
#include <glib.h>


static void test_convert(void)
{
  long year, month, day;

  g_assert_cmpint(date2mjd(1901,  1,  1), ==, 15385);
  g_assert_cmpint(date2mjd(2014, 11, 13), ==, 56974);
  g_assert_cmpint(date2mjd(2099, 12, 31), ==, 88068);

  mjd2date(15385, &year, &month, &day);
  g_assert_cmpint(year, ==, 1901);
  g_assert_cmpint(month, ==, 1);
  g_assert_cmpint(day, ==, 1);
  mjd2date(56974, &year, &month, &day);
  g_assert_cmpint(year, ==, 2014);
  g_assert_cmpint(month, ==, 11);
  g_assert_cmpint(day, ==, 13);
  mjd2date(88068, &year, &month, &day);
  g_assert_cmpint(year, ==, 2099);
  g_assert_cmpint(month, ==, 12);
  g_assert_cmpint(day, ==, 31);
}


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/oifitslib/datemjd/convert", test_convert);

  return g_test_run();
}
