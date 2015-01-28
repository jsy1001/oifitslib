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

#include "oifilter.h"
#include "oifile.h"
#include "oicheck.h"
#include <math.h>
#include <unistd.h>  /* unlink() */

#define FILENAME "test/OIFITS2/bigtest2.fits"
#define RAD2DEG (180.0/3.14159)

typedef struct {
  oi_fits inData;
  oi_fits outData;
  oi_filter_spec filter;
} TestFixture;


static gboolean ignoreRemoved(const char *logDomain, GLogLevelFlags logLevel,
                              const char *message, gpointer userData)
{
  return (!g_str_has_suffix(message, "removed from filter output"));
}

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
  g_assert_cmpint(pData->numCorr, ==, g_list_length(pData->corrList));
  g_assert_cmpint(pData->numVis, ==, g_list_length(pData->visList));
  g_assert_cmpint(pData->numVis2, ==, g_list_length(pData->vis2List));
  g_assert_cmpint(pData->numT3, ==, g_list_length(pData->t3List));
  g_assert_cmpint(pData->numSpectrum, ==, g_list_length(pData->spectrumList));

  g_assert_cmpint(pData->numArray, ==, g_hash_table_size(pData->arrayHash));
  g_assert_cmpint(pData->numWavelength,
                  ==, g_hash_table_size(pData->wavelengthHash));
  g_assert_cmpint(pData->numCorr, ==, g_hash_table_size(pData->corrHash));
}

static void setup_fixture(TestFixture *fix, gconstpointer userData)
{
  int status;
  const char *filename = userData;

  status = 0;
  read_oi_fits(filename, &fix->inData, &status);
  g_assert(!status);
  check(&fix->inData);
  init_oi_filter(&fix->filter);
  /* outData is initialised in test function */
}

static void teardown_fixture(TestFixture *fix, gconstpointer userData)
{
  free_oi_fits(&fix->inData);
  free_oi_fits(&fix->outData);
}

static void test_parse(TestFixture *fix, gconstpointer userData)
{
  char *args[] = {
    "utest_oifilter",
    "--insname=IOTA_IONIC_PICNIC",
    "--accept-vis2=0",
  };
  const int nargs = sizeof(args)/sizeof(args[0]);
  int argc;
  char **argv;
  GError *error;
  GOptionContext *context;
  
  context = g_option_context_new("");
  g_option_context_add_group(context, get_oi_filter_option_group());
  argc = nargs;
  argv = (char **) args;
  error = NULL;
  /* Note: g_option_context_parse() modifies argc and argv to remove
   * the options that were parsed successfully. The removed option
   * strings are not freed by GLib, because that shouldn't be done
   * with arguments to main() */
  g_option_context_parse(context, &argc, &argv, &error);
  if (error != NULL)
    g_error("Error parsing test options: %s", error->message);
  g_assert_cmpint(argc, ==, 1);
  g_option_context_free(context);

  g_test_log_set_fatal_handler(ignoreRemoved, NULL);
  apply_user_oi_filter(&fix->inData, &fix->outData);
  check(&fix->outData);
  g_assert_cmpint(fix->outData.numWavelength, ==, 1);
  g_assert_cmpint(fix->outData.numVis, ==, 1);
  g_assert_cmpint(fix->outData.numVis2, ==, 0);
  g_assert_cmpint(fix->outData.numT3, ==, 1);
}

static void test_default(TestFixture *fix, gconstpointer userData)
{
  int status;
  char *basename, *outFilename;
  const char *filename = userData;

  (void) format_oi_filter(&fix->filter);
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert_cmpint(fix->outData.numArray, ==, fix->inData.numArray);
  g_assert_cmpint(fix->outData.numWavelength, ==, fix->inData.numWavelength);
  g_assert_cmpint(fix->outData.numCorr, ==, fix->inData.numCorr);
  g_assert_cmpint(fix->outData.numVis, ==, fix->inData.numVis);
  g_assert_cmpint(fix->outData.numVis2, ==, fix->inData.numVis2);
  g_assert_cmpint(fix->outData.numT3, ==, fix->inData.numT3);
  g_assert_cmpint(fix->outData.numSpectrum, ==, fix->inData.numSpectrum);

  status = 0;
  basename = g_path_get_basename(filename);
  outFilename = g_strdup_printf("utest_%s", basename);
  g_free(basename);
  g_assert(write_oi_fits(outFilename, fix->outData, &status) == 0);
  unlink(outFilename);
  g_free(outFilename);
}

static void test_arrname(TestFixture *fix, gconstpointer userData)
{
  g_strlcpy(fix->filter.arrname, "C?ARA*", FLEN_VALUE);
  g_test_log_set_fatal_handler(ignoreRemoved, NULL);
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert(oi_fits_lookup_array(&fix->outData, "CHARA_2004Jan") != NULL);
  g_assert(oi_fits_lookup_array(&fix->outData, "IOTA_2002Dec17") == NULL);
}

static void test_insname(TestFixture *fix, gconstpointer userData)
{
  g_strlcpy(fix->filter.insname, "*I?NIC*", FLEN_VALUE);
  g_test_log_set_fatal_handler(ignoreRemoved, NULL);
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert(oi_fits_lookup_wavelength(&fix->outData, "IOTA_IONIC_PICNIC")
           != NULL);
  g_assert(oi_fits_lookup_wavelength(&fix->outData, "CHARA_MIRC") == NULL);
}

static void test_corrname(TestFixture *fix, gconstpointer userData)
{
  g_strlcpy(fix->filter.corrname, "TE?T", FLEN_VALUE);
  g_test_log_set_fatal_handler(ignoreRemoved, NULL);
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert(oi_fits_lookup_corr(&fix->outData, "TEST") != NULL);
}

static void test_target(TestFixture *fix, gconstpointer userData)
{
  fix->filter.target_id = 1;
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert(oi_fits_lookup_target(&fix->outData, 1) != NULL);
  g_assert(oi_fits_lookup_target(&fix->outData, 2) == NULL);
}

static void test_wave(TestFixture *fix, gconstpointer userData)
{
  const float range[2] = {1500.0e-9, 1700.0e-9};
  GList *link;
  oi_wavelength *pWave;
  int i;
    
  fix->filter.wave_range[0] = range[0];
  fix->filter.wave_range[1] = range[1];
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);

  link = fix->outData.wavelengthList;
  while (link != NULL)
  {
    pWave = (oi_wavelength *) link->data;
    for (i = 0; i < pWave->nwave; i++) {
      g_assert_cmpfloat(pWave->eff_wave[i], >=, range[0]);
      g_assert_cmpfloat(pWave->eff_wave[i], <=, range[1]);
    }
    link = link->next;
  }
}

#define ASSERT_MJD_IN_RANGE(tabList, tabType, range)            \
{                                                               \
  tabType *tab;                                                 \
  int i;                                                        \
  GList *link = (tabList);                                      \
  while (link != NULL)                                          \
  {                                                             \
    tab = (tabType *) link->data;                               \
    for (i = 0; i < tab->numrec; i++) {                         \
      g_assert_cmpfloat(tab->record[i].mjd, >=, (range)[0]);    \
      g_assert_cmpfloat(tab->record[i].mjd, <=, (range)[1]);    \
    }                                                           \
    link = link->next;                                          \
  }                                                             \
}

static void test_mjd(TestFixture *fix, gconstpointer userData)
{
  /* note bigtest2.fits has nonsense MJD values */
  const float range[2] = {0.0, 0.0075};
    
  fix->filter.mjd_range[0] = range[0];
  fix->filter.mjd_range[1] = range[1];
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);

  ASSERT_MJD_IN_RANGE(fix->outData.visList, oi_vis, range);
  ASSERT_MJD_IN_RANGE(fix->outData.vis2List, oi_vis2, range);
  ASSERT_MJD_IN_RANGE(fix->outData.t3List, oi_t3, range);
  ASSERT_MJD_IN_RANGE(fix->outData.spectrumList, oi_spectrum, range);
}

static void test_prune(TestFixture *fix, gconstpointer userData)
{
  /* note bigtest2.fits has nonsense MJD values */
  const float range[2] = {0.001, 0.0075};
    
  fix->filter.mjd_range[0] = range[0];
  fix->filter.mjd_range[1] = range[1];
  g_test_log_set_fatal_handler(ignoreRemoved, NULL);
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);

  ASSERT_MJD_IN_RANGE(fix->outData.visList, oi_vis, range);
  ASSERT_MJD_IN_RANGE(fix->outData.vis2List, oi_vis2, range);
  ASSERT_MJD_IN_RANGE(fix->outData.t3List, oi_t3, range);
  ASSERT_MJD_IN_RANGE(fix->outData.spectrumList, oi_spectrum, range);
}


#define ASSERT_BAS_IN_RANGE(tabList, tabType, range)                \
{                                                                   \
  tabType *tab;                                                     \
  int i;                                                            \
  double u1, v1, bas;                                                \
  GList *link = (tabList);                                          \
  while (link != NULL)                                              \
  {                                                                 \
    tab = (tabType *) link->data;                                   \
    for (i = 0; i < tab->numrec; i++) {                             \
      u1 = tab->record[i].ucoord;                                   \
      v1 = tab->record[i].vcoord;                                   \
      bas = pow(u1*u1 + v1*v1, 0.5);                                \
      g_assert_cmpfloat(bas, >=, (range)[0]);                       \
      g_assert_cmpfloat(bas, <=, (range)[1]);                       \
    }                                                               \
    link = link->next;                                              \
  }                                                                 \
}

static void test_bas(TestFixture *fix, gconstpointer userData)
{
  const double range[2] = {0.0, 3.0};
  oi_t3 *pT3;
  GList *link;
  int i;
  double u1, v1, u2, v2, bas;
    
  fix->filter.bas_range[0] = range[0];
  fix->filter.bas_range[1] = range[1];
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);

  ASSERT_BAS_IN_RANGE(fix->outData.visList, oi_vis, range);
  ASSERT_BAS_IN_RANGE(fix->outData.vis2List, oi_vis2, range);

  link = fix->outData.t3List;
  while (link != NULL)
  {
    pT3 = (oi_t3 *) link->data;
    for (i = 0; i < pT3->numrec; i++) {
      u1 = pT3->record[i].u1coord;
      v1 = pT3->record[i].v1coord;
      u2 = pT3->record[i].u2coord;
      v2 = pT3->record[i].v2coord;
      bas = pow(u1*u1 + v1*v1, 0.5);
      g_assert_cmpfloat(bas, >=, range[0]);
      g_assert_cmpfloat(bas, <=, range[1]);
      bas = pow(u2*u2 + v2*v2, 0.5);
      g_assert_cmpfloat(bas, >=, range[0]);
      g_assert_cmpfloat(bas, <=, range[1]);
      bas = pow((u1+u2)*(u1+u2) + (v1+v2)*(v1+v2), 0.5);
      g_assert_cmpfloat(bas, >=, range[0]);
      g_assert_cmpfloat(bas, <=, range[1]);
    }
    link = link->next;
  }
}

static void test_snr(TestFixture *fix, gconstpointer userData)
{
  const float range[2] = {20.0, 100.0};
  oi_vis *pVis;
  oi_vis2 *pVis2;
  oi_t3 *pT3;
  oi_spectrum *pSpectrum;
  GList *link;
  int i, j;
  float snr, snrAmp, snrPhi;
    
  fix->filter.snr_range[0] = range[0];
  fix->filter.snr_range[1] = range[1];
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);

  link = fix->outData.visList;
  while (link != NULL)
  {
    pVis = (oi_vis *) link->data;
    for (i = 0; i < pVis->numrec; i++) {
      for (j = 0; j < pVis->nwave; j++) {
        if (!pVis->record[i].flag[j])
        {
          snrAmp = pVis->record[i].visamp[j]/pVis->record[i].visamperr[j];
          snrPhi = RAD2DEG/pVis->record[i].visphierr[j];
          g_assert_cmpfloat(snrAmp, >=, range[0]);
          g_assert_cmpfloat(snrAmp, <=, range[1]);
          g_assert_cmpfloat(snrPhi, >=, range[0]);
          g_assert_cmpfloat(snrPhi, <=, range[1]);
        }
      }
    }
    link = link->next;
  }
  link = fix->outData.vis2List;
  while (link != NULL)
  {
    pVis2 = (oi_vis2 *) link->data;
    for (i = 0; i < pVis2->numrec; i++) {
      for (j = 0; j < pVis->nwave; j++) {
        if (!pVis2->record[i].flag[j])
        {
          snr = pVis2->record[i].vis2data[j]/pVis2->record[i].vis2err[j];
          g_assert_cmpfloat(snr, >=, range[0]);
          g_assert_cmpfloat(snr, <=, range[1]);
        }
      }
    }
    link = link->next;
  }
  link = fix->outData.t3List;
  while (link != NULL)
  {
    pT3 = (oi_t3 *) link->data;
    for (i = 0; i < pT3->numrec; i++) {
      for (j = 0; j < pVis->nwave; j++) {
        if (!pT3->record[i].flag[j])
        {
          snrAmp = pT3->record[i].t3amp[j]/pT3->record[i].t3amperr[j];
          snrPhi = RAD2DEG/pT3->record[i].t3phierr[j];
          g_assert_cmpfloat(snrAmp, >=, range[0]);
          g_assert_cmpfloat(snrAmp, <=, range[1]);
          g_assert_cmpfloat(snrPhi, >=, range[0]);
          g_assert_cmpfloat(snrPhi, <=, range[1]);
        }
      }
    }
    link = link->next;
  }
  link = fix->outData.spectrumList;
  while (link != NULL)
  {
    pSpectrum = (oi_spectrum *) link->data;
    for (i = 0; i < pSpectrum->numrec; i++) {
      for (j = 0; j < pSpectrum->nwave; j++) {
        snr = pSpectrum->record[i].fluxdata[j]/pSpectrum->record[i].fluxerr[j];
        g_assert_cmpfloat(snr, >=, range[0]);
        g_assert_cmpfloat(snr, <=, range[1]);
      }
    }
    link = link->next;
  }
}


static void test_vis(TestFixture *fix, gconstpointer userData)
{
  fix->filter.accept_vis = FALSE;
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert_cmpint(fix->inData.numVis, >, 0);
  g_assert_cmpint(fix->outData.numVis, ==, 0);
}

static void test_vis2(TestFixture *fix, gconstpointer userData)
{
  fix->filter.accept_vis2 = FALSE;
  g_test_log_set_fatal_handler(ignoreRemoved, NULL);
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert_cmpint(fix->inData.numVis2, >, 0);
  g_assert_cmpint(fix->outData.numVis2, ==, 0);
}

static void test_t3(TestFixture *fix, gconstpointer userData)
{
  fix->filter.accept_t3amp = FALSE;
  fix->filter.accept_t3phi = FALSE;
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert_cmpint(fix->inData.numT3, >, 0);
  g_assert_cmpint(fix->outData.numT3, ==, 0);

  free_oi_fits(&fix->outData);
  fix->filter.accept_t3amp = TRUE;
  fix->filter.accept_t3phi = FALSE;
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert_cmpint(fix->outData.numT3, ==, fix->inData.numT3);

  free_oi_fits(&fix->outData);
  fix->filter.accept_t3amp = FALSE;
  fix->filter.accept_t3phi = TRUE;
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert_cmpint(fix->outData.numT3, ==, fix->inData.numT3);
}


static void test_spectrum(TestFixture *fix, gconstpointer userData)
{
  fix->filter.accept_spectrum = FALSE;
  apply_oi_filter(&fix->inData, &fix->filter, &fix->outData);
  check(&fix->outData);
  g_assert_cmpint(fix->inData.numSpectrum, >, 0);
  g_assert_cmpint(fix->outData.numSpectrum, ==, 0);
}


int main(int argc, char *argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add("/oifitslib/oifilter/parse", TestFixture, FILENAME,
             setup_fixture, test_parse, teardown_fixture);
  g_test_add("/oifitslib/oifilter/default", TestFixture, FILENAME,
             setup_fixture, test_default, teardown_fixture);
  g_test_add("/oifitslib/oifilter/arrname", TestFixture, FILENAME,
             setup_fixture, test_arrname, teardown_fixture);
  g_test_add("/oifitslib/oifilter/insname", TestFixture, FILENAME,
             setup_fixture, test_insname, teardown_fixture);
  g_test_add("/oifitslib/oifilter/corrname", TestFixture, FILENAME,
             setup_fixture, test_corrname, teardown_fixture);
  g_test_add("/oifitslib/oifilter/target", TestFixture, FILENAME,
             setup_fixture, test_target, teardown_fixture);

  g_test_add("/oifitslib/oifilter/wave", TestFixture, FILENAME,
             setup_fixture, test_wave, teardown_fixture);
  g_test_add("/oifitslib/oifilter/mjd", TestFixture, FILENAME,
             setup_fixture, test_mjd, teardown_fixture);
  g_test_add("/oifitslib/oifilter/prune", TestFixture, FILENAME,
             setup_fixture, test_prune, teardown_fixture);
  g_test_add("/oifitslib/oifilter/bas", TestFixture, FILENAME,
             setup_fixture, test_bas, teardown_fixture);
  g_test_add("/oifitslib/oifilter/snr", TestFixture, FILENAME,
             setup_fixture, test_snr, teardown_fixture);

  g_test_add("/oifitslib/oifilter/vis", TestFixture, FILENAME,
             setup_fixture, test_vis, teardown_fixture);
  g_test_add("/oifitslib/oifilter/vis2", TestFixture, FILENAME,
             setup_fixture, test_vis2, teardown_fixture);
  g_test_add("/oifitslib/oifilter/t3", TestFixture, FILENAME,
             setup_fixture, test_t3, teardown_fixture);
  g_test_add("/oifitslib/oifilter/spectrum", TestFixture, FILENAME,
             setup_fixture, test_spectrum, teardown_fixture);

  return g_test_run();
}
