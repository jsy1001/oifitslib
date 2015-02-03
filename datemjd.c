/**
 * @file
 * @ingroup oifile
 * Implementation of Gregorian date/MJD conversions.
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

#include <assert.h>


/**
 * Convert Gregorian date to Modified Julian Day.
 *
 * Algorithm adapted from http://aa.usno.navy.mil/faq/docs/JD_Formula.php
 *
 * @param year   Year (1901-2099)
 * @param month  Month (1-12)
 * @param day    Day of month (1-31)
 *
 * @return MJD at the start of the specified date (i.e. at midnight)
 */
long date2mjd(long year, long month, long day)
{
  assert(year >= 1901 && year <= 2099);
  return (367 * year -
          (7 * (year + (month + 9) / 12)) / 4 + 
          (275 * month) / 9 + day + 1721013 - 2400000);
}

/**
 * Convert Modified Julian Day to Gregorian date.
 *
 * Adapted from Fliegel & van Flandern (1968), Communications of the
 * ACM 11, no. 10, p. 657
 *
 * @param mjd     Modified Julian Day to convert
 * @param pYear   Return location for year
 * @param pMonth  Return location for month (1-12)
 * @param pDay    Return location for day of month (1-31)
 */
void mjd2date(long mjd,
              long *const pYear, long *const pMonth, long *const pDay)
{
  long L, N, I, J;

  L = mjd + 2400001 + 68569;
  N = 4 * L / 146097;
  L = L - (146097 * N + 3) / 4;
  I = 4000 * (L + 1) / 1461001;
  L = L - 1461 * I / 4 + 31;
  J = 80 * L / 2447;
  *pDay = L - 2447 * J / 80;
  L = J / 11;
  *pMonth = J + 2 - (12 * L);
  *pYear = 100 * (N - 49) + I + L;
}
