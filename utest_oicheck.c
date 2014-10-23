/**
 * @file
 * @ingroup oicheck
 * Unit tests of OIFITS conformity checker.
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

#include "oicheck.h"
#include "oifile.h"

#define DIR "test/OIFITS1/"

typedef struct {
  char filename[FLEN_VALUE];
  check_func check;
  oi_breach_level expected;
} TestCase;

typedef struct {
  int numCases;
  const TestCase *cases;
} TestSet;


static const TestCase passCases[] = {
  {DIR "Mystery--AMBER--LowH.fits", check_unique_targets,   OI_BREACH_NONE},
  {DIR "Mystery--AMBER--LowH.fits", check_targets_present,  OI_BREACH_NONE},
  {DIR "Mystery--AMBER--LowH.fits", check_elements_present, OI_BREACH_NONE},
  {DIR "Mystery--AMBER--LowH.fits", check_flagging,         OI_BREACH_NONE},
  {DIR "Mystery--AMBER--LowH.fits", check_t3amp,            OI_BREACH_NONE},
  {DIR "Mystery--AMBER--LowH.fits", check_waveorder,        OI_BREACH_NONE}
};

static const TestCase failCases[] = {
  {DIR "bad_dup_target.fits",      check_unique_targets,   OI_BREACH_WARNING},
  {DIR "bad_missing_target.fits",  check_targets_present,  OI_BREACH_NOT_OIFITS},
  {DIR "bad_missing_element.fits", check_elements_present, OI_BREACH_NOT_OIFITS},
  {DIR "bad_neg_error.fits",       check_flagging,         OI_BREACH_NOT_OIFITS},
  {DIR "bad_big_t3amp.fits",       check_t3amp,            OI_BREACH_NOT_OIFITS},
  {DIR "bad_wave_reversed.fits",   check_waveorder,        OI_BREACH_WARNING}
};

static const TestSet passSet = {
  sizeof(passCases)/sizeof(passCases[0]),
  passCases
};

static const TestSet failSet = {
  sizeof(failCases)/sizeof(failCases[0]),
  failCases
};


static void test_check(gconstpointer userData)
{
  int i;
  oi_fits inData;
  int status;
  oi_check_result result;

  const TestSet *pSet = userData;
  
  for (i = 0; i < pSet->numCases; i++) {
    status = 0;
    read_oi_fits(pSet->cases[i].filename, &inData, &status);
    g_assert(!status);
    if ((*pSet->cases[i].check)(&inData, &result) != pSet->cases[i].expected) {
      g_error("Bad result for %s:\n  expected '%s'\n  got '%s'",
              pSet->cases[i].filename,
              oi_breach_level_desc[pSet->cases[i].expected],
              oi_breach_level_desc[result.level]);
    }
    free_check_result(&result);
    free_oi_fits(&inData);
  }
}


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_data_func("/oifitslib/oifilter/pass", &passSet, test_check);
  g_test_add_data_func("/oifitslib/oifilter/fail", &failSet, test_check);

  return g_test_run();
}
