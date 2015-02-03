/**
 * @file
 * @ingroup oimerge
 * Unit tests of OIFITSlib merge component.
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

#include "oimerge.h"
#include "oifile.h"
#include "oicheck.h"

#define DIR1 "test/OIFITS1/"
#define DIR2 "test/OIFITS2/"

typedef struct {
  const char *filename1;
  const char *filename2;
  const char *filename3;
  int numArray;       /**< Number of OI_ARRAY expected in output */
  int numWavelength;  /**< Number of OI_WAVELENGTH expected in output */
} TestCase;

typedef struct {
  int numCases;
  const TestCase *cases;
} TestSet;

typedef struct {
  int numVis;
  int numVis2;
  int numT3;
} DataCount;


static const TestCase v1Cases[] = {
  {DIR1 "Mystery--AMBER--LowH.fits", DIR1 "alp_aur--COAST_NICMOS.fits",
   NULL, 2, 2},
  {DIR1 "Mystery--AMBER--LowH.fits", DIR1 "alp_aur--COAST_NICMOS.fits",
   DIR1 "Bin_Ary--MIRC_H.fits", 3, 3},
  {DIR1 "Mystery--AMBER--LowH.fits", DIR1 "Mystery--AMBER--MedH.fits",
   NULL, 1, 2},
  {DIR1 "Alp_Vic--MIRC_H.fits", DIR1 "Alp_Vic--MIRC_K.fits", NULL, 1, 2},
  {DIR1 "Alp_Vic--MIRC_H.fits", DIR1 "Bin_Ary--MIRC_H.fits", NULL, 1, 1},
};

static const TestCase v2Cases[] = {
  {DIR2 "Mystery--AMBER--LowH.fits", DIR2 "alp_aur--COAST_NICMOS.fits",
   NULL, 2, 2},
  {DIR2 "Mystery--AMBER--LowH.fits", DIR2 "alp_aur--COAST_NICMOS.fits",
   DIR2 "Bin_Ary--MIRC_H.fits", 3, 3},
  {DIR2 "Mystery--AMBER--LowH.fits", DIR2 "Mystery--AMBER--MedH.fits",
   NULL, 1, 2},
  {DIR2 "Alp_Vic--MIRC_H.fits", DIR2 "Alp_Vic--MIRC_K.fits", NULL, 1, 2},
  {DIR2 "Alp_Vic--MIRC_H.fits", DIR2 "Bin_Ary--MIRC_H.fits", NULL, 1, 1},
  {"testdata.fits", DIR2 "bigtest2.fits", NULL, 3, 3}
};

static const TestSet v1Set = {
  sizeof(v1Cases)/sizeof(v1Cases[0]),
  v1Cases
};

static const TestSet v2Set = {
  sizeof(v2Cases)/sizeof(v2Cases[0]),
  v2Cases
};


static void check(oi_fits *pData)
{
  check_func checks[] = {
    check_tables,
    check_unique_targets,
    check_targets_present,
    check_elements_present,
    check_flagging,
    check_t3amp,
    check_waveorder,
    NULL
  };
  oi_check_result result;
  oi_breach_level worst;
  int i;

  /* Run checks */
  worst = OI_BREACH_NONE;
  i = 0;
  while (checks[i] != NULL) {
    if ((*checks[i++])(pData, &result) != OI_BREACH_NONE) {
      print_check_result(&result);
      g_warning("Check failed");
      if (result.level > worst) worst = result.level;
    }
    free_check_result(&result);
  }

  if (worst == OI_BREACH_NONE)
    g_debug("All checks passed");

  g_assert_cmpint(pData->numArray, ==, g_list_length(pData->arrayList));
  g_assert_cmpint(pData->numWavelength,
                  ==, g_list_length(pData->wavelengthList));
  g_assert_cmpint(pData->numVis, ==, g_list_length(pData->visList));
  g_assert_cmpint(pData->numVis2, ==, g_list_length(pData->vis2List));
  g_assert_cmpint(pData->numT3, ==, g_list_length(pData->t3List));

  g_assert_cmpint(pData->numArray, ==, g_hash_table_size(pData->arrayHash));
  g_assert_cmpint(pData->numWavelength,
                  ==, g_hash_table_size(pData->wavelengthHash));
}

static void zero_count(DataCount *pCount)
{
  pCount->numVis = 0;
  pCount->numVis2 = 0;
  pCount->numT3 = 0;
}

static void add_count(DataCount *pCount, const oi_fits *pData)
{
  GList *link;
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;

  /* Count VISAMP/VISPHI in OI_VIS tables */
  link = pData->visList;
  while(link != NULL) {
    pVis = link->data;
    pCount->numVis += pVis->numrec * pVis->nwave;
    link = link->next;
  }

  /* Count VIS2DATA in OI_VIS2 tables */
  link = pData->vis2List;
  while(link != NULL) {
    pVis2 = link->data;
    pCount->numVis2 += pVis2->numrec * pVis2->nwave;
    link = link->next;
  }

  /* Count T3AMP/T3PHI in OI_T3 tables */
  link = pData->t3List;
  while(link != NULL) {
    pT3 = link->data;
    pCount->numT3 += pT3->numrec * pT3->nwave;
    link = link->next;
  }
}

static void test_merge(gconstpointer userData)
{
  int i, numCorr;
  oi_fits outData, inData1, inData2, inData3;
  int status;
  DataCount inCount, outCount;

  const TestSet *pSet = userData;

  for (i = 0; i < pSet->numCases; i++) {
    g_message("\nMerging [%s\n         %s\n         %s]",
              pSet->cases[i].filename1, pSet->cases[i].filename2,
              pSet->cases[i].filename3);

    /* Read files to merge */
    status = 0;
    numCorr = 0;
    read_oi_fits(pSet->cases[i].filename1, &inData1, &status);
    g_assert(!status);
    check(&inData1);
    numCorr += inData1.numCorr;
    read_oi_fits(pSet->cases[i].filename2, &inData2, &status);
    g_assert(!status);
    check(&inData2);
    numCorr += inData2.numCorr;
    if(pSet->cases[i].filename3 != NULL) {
      read_oi_fits(pSet->cases[i].filename3, &inData3, &status);
      g_assert(!status);
      check(&inData3);
      numCorr += inData3.numCorr;
    }

    /* Merge datasets */
    if (pSet->cases[i].filename3 != NULL)
      merge_oi_fits(&outData, &inData1, &inData2, &inData3, NULL);
    else
      merge_oi_fits(&outData, &inData1, &inData2, NULL);
    check(&outData);

    /* Compare number of OI_ARRAY and OI_WAVELENGTH tables */
    g_assert_cmpint(outData.numArray, ==, pSet->cases[i].numArray);
    g_assert_cmpint(outData.numWavelength, ==, pSet->cases[i].numWavelength);

    /* Compare number of OI_CORR tables */
    g_assert_cmpint(outData.numCorr, ==, numCorr);

    /* Compare number of data */
    zero_count(&inCount);
    add_count(&inCount, &inData1);
    add_count(&inCount, &inData2);
    if (pSet->cases[i].filename3 != NULL)
      add_count(&inCount, &inData3);
    zero_count(&outCount);
    add_count(&outCount, &outData);
    g_assert_cmpint(outCount.numVis, ==, inCount.numVis);
    g_assert_cmpint(outCount.numVis2, ==, inCount.numVis2);
    g_assert_cmpint(outCount.numT3, ==, inCount.numT3);

    /* Free storage */
    free_oi_fits(&outData);
    free_oi_fits(&inData1);
    free_oi_fits(&inData2);
    if (pSet->cases[i].filename3 != NULL)
      free_oi_fits(&inData3);
  }
}


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_data_func("/oifitslib/oimerge/ver1", &v1Set, test_merge);
  g_test_add_data_func("/oifitslib/oimerge/ver2", &v2Set, test_merge);

  return g_test_run();
}
