/**
 * @file
 * Example program - uses oitable API to write and read a OIFITS file.
 *
 * Copyright (C) 2007, 2015-2017 John Young
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "exchange.h"

static bool scan_opt_string(FILE *fp, const char *format, char *dest)
{
  if (fscanf(fp, format, dest) < 1) {
    dest[0] = '\0';
    printf("Optional %s not specified - set to \"\"\n", format);
    return FALSE;
  }
  return TRUE;
}

static bool scan_opt_int(FILE *fp, const char *format, int *dest, int nullval)
{
  if (fscanf(fp, format, dest) < 1) {
    *dest = nullval;
    printf("Optional %s not specified - set to %d\n", format, nullval);
    return FALSE;
  }
  return TRUE;
}

static bool scan_opt_double(FILE *fp, const char *format, double *dest,
                            double nullval)
{
  if (fscanf(fp, format, dest) < 1) {
    *dest = nullval;
    printf("Optional %s not specified - set to %lf\n", format, nullval);
    return FALSE;
  }
  return TRUE;
}

/**
 * Read data from a user-specified text file and write out in OIFITS format.
 */
void demo_write(void)
{
  oi_header header;
  oi_array array;
  oi_target targets;
  oi_wavelength wave;
  oi_corr corr;
  oi_inspol inspol;
  oi_vis vis;
  oi_vis2 vis2;
  oi_t3 t3;
  oi_flux flux;
  char filename[FLEN_FILENAME];
  FILE *fp;
  int i, irec, iwave, itarg, status, nrows;
  long numrec;
  float real, imag;
  fitsfile *fptr;

  /* get input filename */
  printf("Enter input TEXT filename: ");
  fgets(filename, FLEN_FILENAME, stdin);
  filename[strlen(filename) - 1] = '\0'; /* zap newline */

  fp = fopen(filename, "r");

  /* Read info for OI_ARRAY table */
  fscanf(fp, "OI_ARRAY arrname %70s ", array.arrname);
  fscanf(fp, "frame %70s ", array.frame);
  fscanf(fp, "arrayx %lf arrayy %lf arrayz %lf ", &array.arrayx,
         &array.arrayy, &array.arrayz);
  fscanf(fp, "nelement %d ", &nrows);
  alloc_oi_array(&array, nrows);
  for (i = 0; i < array.nelement; i++) {
    fscanf(fp, "tel_name %16s sta_name %16s ", array.elem[i].tel_name,
           array.elem[i].sta_name);
    fscanf(fp, "staxyz %lf %lf %lf diameter %f ", &array.elem[i].staxyz[0],
           &array.elem[i].staxyz[1], &array.elem[i].staxyz[2],
           &array.elem[i].diameter);
    fscanf(fp, "fov %lf fovtype %s ",
           &array.elem[i].fov, array.elem[i].fovtype);
    array.elem[i].sta_index = i + 1;
  }
  array.revision = OI_REVN_V2_ARRAY;

  /* Read info for OI_TARGET table */
  fscanf(fp, "OI_TARGET ntarget %d ", &nrows);
  alloc_oi_target(&targets, nrows);
  targets.usecategory = FALSE;
  for (itarg = 0; itarg < targets.ntarget; itarg++) {
    fscanf(fp, "target_id %d ", &targets.targ[itarg].target_id);
    fscanf(fp, "target %16s ", targets.targ[itarg].target);
    fscanf(fp, "raep0 %lf ", &targets.targ[itarg].raep0);
    fscanf(fp, "decep0 %lf ", &targets.targ[itarg].decep0);
    fscanf(fp, "equinox %f ", &targets.targ[itarg].equinox);
    fscanf(fp, "ra_err %lf ", &targets.targ[itarg].ra_err);
    fscanf(fp, "dec_err %lf ", &targets.targ[itarg].dec_err);
    fscanf(fp, "sysvel %lf ", &targets.targ[itarg].sysvel);
    fscanf(fp, "veltyp %8s ", targets.targ[itarg].veltyp);
    fscanf(fp, "veldef %8s ", targets.targ[itarg].veldef);
    fscanf(fp, "pmra %lf ", &targets.targ[itarg].pmra);
    fscanf(fp, "pmdec %lf ", &targets.targ[itarg].pmdec);
    fscanf(fp, "pmra_err %lf ", &targets.targ[itarg].pmra_err);
    fscanf(fp, "pmdec_err %lf ", &targets.targ[itarg].pmdec_err);
    fscanf(fp, "parallax %f ", &targets.targ[itarg].parallax);
    fscanf(fp, "para_err %f ", &targets.targ[itarg].para_err);
    fscanf(fp, "spectyp %16s ", targets.targ[itarg].spectyp);
    if (scan_opt_string(fp, "category %3s", targets.targ[itarg].category))
      targets.usecategory = TRUE;
    fscanf(fp, " ");
  }
  targets.revision = OI_REVN_V2_TARGET;

  /* Read info for OI_WAVELENGTH table */
  fscanf(fp, "OI_WAVELENGTH insname %70s ", wave.insname);
  fscanf(fp, "nwave %d ", &nrows);
  alloc_oi_wavelength(&wave, nrows);
  fscanf(fp, "eff_wave ");
  for (i = 0; i < wave.nwave; i++) {
    fscanf(fp, "%f ", &wave.eff_wave[i]);
  }
  fscanf(fp, "eff_band ");
  for (i = 0; i < wave.nwave; i++) {
    fscanf(fp, "%f ", &wave.eff_band[i]);
  }
  wave.revision = OI_REVN_V2_WAVELENGTH;

  /* Read info for OI_CORR table */
  fscanf(fp, "OI_CORR corrname %70s ", corr.corrname);
  fscanf(fp, "ndata %d ", &corr.ndata);
  fscanf(fp, "ncorr %d ", &nrows);
  alloc_oi_corr(&corr, nrows);
  fscanf(fp, "iindx ");
  for (i = 0; i < corr.ncorr; i++) {
    fscanf(fp, "%d ", &corr.iindx[i]);
  }
  fscanf(fp, "jindx ");
  for (i = 0; i < corr.ncorr; i++) {
    fscanf(fp, "%d ", &corr.jindx[i]);
  }
  fscanf(fp, "corr ");
  for (i = 0; i < corr.ncorr; i++) {
    fscanf(fp, "%lf ", &corr.corr[i]);
  }
  corr.revision = OI_REVN_V2_CORR;

  /* Read info for OI_INSPOL table */
  fscanf(fp, "OI_INSPOL date-obs %70s ", inspol.date_obs);
  fscanf(fp, "npol %d ", &inspol.npol);
  fscanf(fp, "arrname %70s ", inspol.arrname);
  fscanf(fp, "orient %70s ", inspol.orient);
  fscanf(fp, "model %70s ", inspol.model);
  fscanf(fp, "numrec %ld ", &numrec);
  alloc_oi_inspol(&inspol, numrec, wave.nwave);
  printf("Reading %ld inspol records...\n", inspol.numrec);
  /* loop over records */
  for (irec = 0; irec < inspol.numrec; irec++) {
    fscanf(fp, "target_id %d insname %70s mjd_obs %lf ",
           &inspol.record[irec].target_id, inspol.record[irec].insname,
           &inspol.record[irec].mjd_obs);
    fscanf(fp, "mjd_end %lf jxx ", &inspol.record[irec].mjd_end);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%f %fi ", &real, &imag);
      inspol.record[irec].jxx[iwave] = real + imag * I;
    }
    fscanf(fp, "jyy ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%f %fi ", &real, &imag);
      inspol.record[irec].jyy[iwave] = real + imag * I;
    }
    fscanf(fp, "jxy ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%f %fi ", &real, &imag);
      inspol.record[irec].jxy[iwave] = real + imag * I;
    }
    fscanf(fp, "jyx ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%f %fi ", &real, &imag);
      inspol.record[irec].jyx[iwave] = real + imag * I;
    }
    fscanf(fp, "sta_index %d ", &inspol.record[irec].sta_index);
  }
  inspol.revision = OI_REVN_V2_INSPOL;

  /* Read info for OI_VIS table */
  fscanf(fp, "OI_VIS date-obs %70s ", vis.date_obs);
  fscanf(fp, "arrname %70s insname %70s ", vis.arrname, vis.insname);
  fscanf(fp, "corrname %70s ", vis.corrname);
  // TODO: can't scan "correlated flux" due to space
  scan_opt_string(fp, "amptyp %70s", vis.amptyp);
  scan_opt_string(fp, " phityp %70s", vis.phityp);
  scan_opt_int(fp, " amporder %d", &vis.amporder, -1);
  scan_opt_int(fp, " phiorder %d", &vis.phiorder, -1);
  fscanf(fp, " numrec %ld ", &numrec);
  alloc_oi_vis(&vis, numrec, wave.nwave);
  printf("Reading %ld vis records...\n", vis.numrec);
  /* loop over records */
  for (irec = 0; irec < vis.numrec; irec++) {
    fscanf(fp, "target_id %d time %lf mjd %lf ", &vis.record[irec].target_id,
           &vis.record[irec].time, &vis.record[irec].mjd);
    fscanf(fp, "int_time %lf visamp ", &vis.record[irec].int_time);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &vis.record[irec].visamp[iwave]);
    }
    fscanf(fp, "visamperr ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &vis.record[irec].visamperr[iwave]);
    }
    fscanf(fp, "corrindx_visamp %d ", &vis.record[irec].corrindx_visamp);
    fscanf(fp, "visphi ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &vis.record[irec].visphi[iwave]);
    }
    fscanf(fp, "visphierr ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &vis.record[irec].visphierr[iwave]);
    }
    fscanf(fp, "corrindx_visphi %d ", &vis.record[irec].corrindx_visphi);
    fscanf(fp, "ucoord %lf vcoord %lf ", &vis.record[irec].ucoord,
           &vis.record[irec].vcoord);
    fscanf(fp, "sta_index %d %d ", &vis.record[irec].sta_index[0],
           &vis.record[irec].sta_index[1]);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      vis.record[irec].flag[iwave] = FALSE;
    }
  }
  vis.revision = OI_REVN_V2_VIS;
  vis.usevisrefmap = FALSE;
  vis.usecomplex = FALSE;  /* hence complexunit not used */
  vis.complexunit[0] = '\0';
  if (strcmp(vis.amptyp, "correlated flux") == 0)
    strcpy(vis.ampunit, "Jy");
  else
    vis.ampunit[0] = '\0';

  /* Read info for OI_VIS2 table */
  fscanf(fp, "OI_VIS2 date-obs %70s ", vis2.date_obs);
  fscanf(fp, "arrname %70s insname %70s ", vis2.arrname, vis2.insname);
  fscanf(fp, "corrname %70s ", vis2.corrname);
  fscanf(fp, "numrec %ld ", &numrec);
  alloc_oi_vis2(&vis2, numrec, wave.nwave);
  printf("Reading %ld vis2 records...\n", vis2.numrec);
  /* loop over records */
  for (irec = 0; irec < vis2.numrec; irec++) {
    fscanf(fp, "target_id %d time %lf mjd %lf ", &vis2.record[irec].target_id,
           &vis2.record[irec].time, &vis2.record[irec].mjd);
    fscanf(fp, "int_time %lf vis2data ", &vis2.record[irec].int_time);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &vis2.record[irec].vis2data[iwave]);
    }
    fscanf(fp, "vis2err ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &vis2.record[irec].vis2err[iwave]);
    }
    fscanf(fp, "corrindx_vis2data %d ", &vis2.record[irec].corrindx_vis2data);
    fscanf(fp, "ucoord %lf vcoord %lf ", &vis2.record[irec].ucoord,
           &vis2.record[irec].vcoord);
    fscanf(fp, "sta_index %d %d ", &vis2.record[irec].sta_index[0],
           &vis2.record[irec].sta_index[1]);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      vis2.record[irec].flag[iwave] = FALSE;
    }
  }
  vis2.revision = OI_REVN_V2_VIS2;

  /* Read info for OI_T3 table */
  fscanf(fp, "OI_T3 date-obs %70s ", t3.date_obs);
  fscanf(fp, "arrname %70s insname %70s ", t3.arrname, t3.insname);
  fscanf(fp, "corrname %70s ", t3.corrname);
  fscanf(fp, "numrec %ld ", &numrec);
  alloc_oi_t3(&t3, numrec, wave.nwave);
  printf("Reading %ld t3 records...\n", t3.numrec);
  /* loop over records */
  for (irec = 0; irec < t3.numrec; irec++) {
    fscanf(fp, "target_id %d time %lf mjd %lf ", &t3.record[irec].target_id,
           &t3.record[irec].time, &t3.record[irec].mjd);
    fscanf(fp, "int_time %lf t3amp ", &t3.record[irec].int_time);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &t3.record[irec].t3amp[iwave]);
    }
    fscanf(fp, "t3amperr ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &t3.record[irec].t3amperr[iwave]);
    }
    fscanf(fp, "corrindx_t3amp %d ", &t3.record[irec].corrindx_t3amp);
    fscanf(fp, "t3phi ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &t3.record[irec].t3phi[iwave]);
    }
    fscanf(fp, "t3phierr ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &t3.record[irec].t3phierr[iwave]);
    }
    fscanf(fp, "corrindx_t3phi %d ", &t3.record[irec].corrindx_t3phi);
    fscanf(fp, "u1coord %lf v1coord %lf ", &t3.record[irec].u1coord,
           &t3.record[irec].v1coord);
    fscanf(fp, "u2coord %lf v2coord %lf ", &t3.record[irec].u2coord,
           &t3.record[irec].v2coord);
    fscanf(fp, "sta_index %d %d %d ", &t3.record[irec].sta_index[0],
           &t3.record[irec].sta_index[1],
           &t3.record[irec].sta_index[2]);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      t3.record[irec].flag[iwave] = FALSE;
    }
  }
  t3.revision = OI_REVN_V2_T3;

  /* Read info for OI_FLUX table */
  fscanf(fp, "OI_FLUX date-obs %70s ", flux.date_obs);
  scan_opt_string(fp, "arrname %70s ", flux.arrname);
  fscanf(fp, "insname %70s corrname %70s ", flux.insname, flux.corrname);
  scan_opt_double(fp, "fov %lf ", &flux.fov, 0.0);
  scan_opt_string(fp, "fovtype %6s ", flux.fovtype);
  fscanf(fp, "calstat %c ", &flux.calstat);
  fscanf(fp, "numrec %ld ", &numrec);
  alloc_oi_flux(&flux, numrec, wave.nwave);
  printf("Reading %ld flux records...\n", flux.numrec);
  /* loop over records */
  for (irec = 0; irec < flux.numrec; irec++) {
    fscanf(fp, "target_id %d mjd %lf ", &flux.record[irec].target_id,
           &flux.record[irec].mjd);
    fscanf(fp, "int_time %lf fluxdata ", &flux.record[irec].int_time);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &flux.record[irec].fluxdata[iwave]);
    }
    fscanf(fp, "fluxerr ");
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      fscanf(fp, "%lf ", &flux.record[irec].fluxerr[iwave]);
    }
    fscanf(fp, "corrindx_fluxdata %d ",
           &flux.record[irec].corrindx_fluxdata);
    scan_opt_int(fp, "sta_index %d ", &flux.record[irec].sta_index, -1);
    for (iwave = 0; iwave < wave.nwave; iwave++) {
      flux.record[irec].flag[iwave] = FALSE;
    }
  }
  flux.revision = OI_REVN_V2_FLUX;
  strcpy(flux.fluxunit, "Jy");

  fclose(fp);

  /* Set header keywords */
  strcpy(header.origin, "ESO");
  strncpy(header.date_obs, vis.date_obs, FLEN_VALUE);
  strncpy(header.telescop, array.arrname, FLEN_VALUE);
  strncpy(header.instrume, wave.insname, FLEN_VALUE);
  strncpy(header.observer, "Astronomer", FLEN_VALUE);
  strcpy(header.insmode, "Low_JHK");
  strncpy(header.object, targets.targ[0].target, FLEN_VALUE);
  header.referenc[0] = '\0';
  header.author[0] = '\0';
  header.prog_id[0] = '\0';
  header.procsoft[0] = '\0';
  header.obstech[0] = '\0';

  /* Write out FITS file */
  printf("Enter output OIFITS filename: ");
  fgets(filename, FLEN_FILENAME, stdin);
  filename[strlen(filename) - 1] = '\0'; /* zap newline */
  printf("Writing FITS file %s...\n", filename);
  status = 0;
  fits_create_file(&fptr, filename, &status);
  if (status) {
    fits_report_error(stderr, status);
    exit(EXIT_FAILURE);
  }
  write_oi_header(fptr, header, &status);
  write_oi_target(fptr, targets, &status);
  write_oi_vis(fptr, vis, 1, &status);
  write_oi_vis2(fptr, vis2, 1, &status);
  write_oi_t3(fptr, t3, 1, &status);
  write_oi_flux(fptr, flux, 1, &status);
  write_oi_array(fptr, array, 1, &status);
  write_oi_wavelength(fptr, wave, 1, &status);
  write_oi_corr(fptr, corr, 1, &status);
  write_oi_inspol(fptr, inspol, 1, &status);

  if (status) {
    /* Error occurred - delete partially-created file and exit */
    fits_delete_file(fptr, &status);
    exit(EXIT_FAILURE);
  } else {
    fits_close_file(fptr, &status);
  }

  /* Free storage */
  free_oi_target(&targets);
  free_oi_vis(&vis);
  free_oi_vis2(&vis2);
  free_oi_t3(&t3);
  free_oi_flux(&flux);
  free_oi_array(&array);
  free_oi_wavelength(&wave);
  free_oi_corr(&corr);
  free_oi_inspol(&inspol);
}


/**
 * Read OIFITS file.
 *
 * This code will read a general OIFITS file containing multiple
 * tables of each type (except OI_TARGET). However, its not that
 * useful since the same oi_vis/vis2/t3/flux object is used to
 * receive the data from all tables of the corresponding type!  See
 * read_oi_fits() in oifile.c for a more useful routine (which
 * requires GLib).
 *
 */
void demo_read(void)
{
  oi_header header;
  oi_array array;
  oi_target targets;
  oi_wavelength wave;
  oi_corr corr;
  oi_inspol inspol;
  oi_vis vis;
  oi_vis2 vis2;
  oi_t3 t3;
  oi_flux flux;
  char filename[FLEN_FILENAME];
  int status, hdutype;
  fitsfile *fptr, *fptr2;

  printf("Enter input OIFITS filename: ");
  fgets(filename, FLEN_FILENAME, stdin);
  filename[strlen(filename) - 1] = '\0'; /* zap newline */
  printf("Reading FITS file %s...\n", filename);
  status = 0;
  fits_open_file(&fptr, filename, READONLY, &status);
  if (status) {
    fits_report_error(stderr, status);
    goto except;
  }
  read_oi_header(fptr, &header, &status);
  read_oi_target(fptr, &targets, &status);
  if (status) goto except;
  /* open 2nd connection to read referenced OI_ARRAY and OI_WAVELENGTH tables
     without losing place in file */
  fits_open_file(&fptr2, filename, READONLY, &status);

  /* Read all OI_VIS tables & corresponding
     OI_ARRAY/OI_CORR/OI_WAVELENGTH tables */
  while (TRUE) {
    if (read_next_oi_vis(fptr, &vis, &status)) break;  /* no more OI_VIS */
    printf("Read OI_VIS      with  ARRNAME=%s INSNAME=%s CORRNAME=%s\n",
           vis.arrname, vis.insname, vis.corrname);
    if (strlen(vis.arrname) > 0) {
      /* if ARRNAME specified, read corresponding OI_ARRAY
         Note we may have read it previously */
      read_oi_array(fptr2, vis.arrname, &array, &status);
    }
    if (strlen(vis.corrname) > 0) {
      /* if CORRNAME specified, read corresponding OI_CORR
         Note we may have read it previously */
      read_oi_corr(fptr2, vis.corrname, &corr, &status);
    }
    read_oi_wavelength(fptr2, vis.insname, &wave, &status);
    if (!status) {
      /* Free storage ready to reuse structs for next table */
      free_oi_wavelength(&wave);
      if (strlen(vis.arrname) > 0) free_oi_array(&array);
      if (strlen(vis.corrname) > 0) free_oi_corr(&corr);
      free_oi_vis(&vis);
    }
  }
  if (status != END_OF_FILE) goto except;
  status = 0;

  /* Read all OI_VIS2 tables & corresponding
     OI_ARRAY/OI_CORR/OI_WAVELENGTH tables */
  fits_movabs_hdu(fptr, 1, &hdutype, &status); /* back to start */
  while (TRUE) {
    if (read_next_oi_vis2(fptr, &vis2, &status)) break;  /* no more OI_VIS2 */
    printf("Read OI_VIS2     with  ARRNAME=%s INSNAME=%s CORRNAME=%s\n",
           vis2.arrname, vis2.insname, vis.corrname);
    if (strlen(vis2.arrname) > 0) {
      /* if ARRNAME specified, read corresponding OI_ARRAY */
      read_oi_array(fptr2, vis2.arrname, &array, &status);
    }
    if (strlen(vis2.corrname) > 0) {
      /* if CORRNAME specified, read corresponding OI_CORR
         Note we may have read it previously */
      read_oi_corr(fptr2, vis2.corrname, &corr, &status);
    }
    read_oi_wavelength(fptr2, vis2.insname, &wave, &status);
    if (!status) {
      /* Free storage ready to reuse structs for next table */
      free_oi_wavelength(&wave);
      if (strlen(vis2.arrname) > 0) free_oi_array(&array);
      if (strlen(vis2.corrname) > 0) free_oi_corr(&corr);
      free_oi_vis2(&vis2);
    }
  }
  if (status != END_OF_FILE) goto except;
  status = 0;

  /* Read all OI_T3 tables & corresponding
     OI_ARRAY/OI_CORR/OI_WAVELENGTH tables */
  fits_movabs_hdu(fptr, 1, &hdutype, &status); /* back to start */
  while (TRUE) {
    if (read_next_oi_t3(fptr, &t3, &status)) break;  /* no more OI_T3 */
    printf("Read OI_T3       with  ARRNAME=%s INSNAME=%s CORRNAME=%s\n",
           t3.arrname, t3.insname, t3.corrname);
    if (strlen(t3.arrname) > 0) {
      /* if ARRNAME specified, read corresponding OI_ARRAY */
      read_oi_array(fptr2, t3.arrname, &array, &status);
    }
    if (strlen(t3.corrname) > 0) {
      /* if CORRNAME specified, read corresponding OI_CORR
         Note we may have read it previously */
      read_oi_corr(fptr2, t3.corrname, &corr, &status);
    }
    read_oi_wavelength(fptr2, t3.insname, &wave, &status);
    if (!status) {
      /* Free storage ready to reuse structs for next table */
      free_oi_wavelength(&wave);
      if (strlen(t3.arrname) > 0) free_oi_array(&array);
      if (strlen(t3.corrname) > 0) free_oi_corr(&corr);
      free_oi_t3(&t3);
    }
  }
  if (status != END_OF_FILE) goto except;
  status = 0;

  /* Read all OI_FLUX tables & corresponding
     OI_ARRAY/OI_CORR/OI_WAVELENGTH tables */
  fits_movabs_hdu(fptr, 1, &hdutype, &status); /* back to start */
  while (TRUE) {
    if (read_next_oi_flux(fptr, &flux, &status)) break;
    printf("Read OI_FLUX     with  ARRNAME=%s INSNAME=%s CORRNAME=%s\n",
           flux.arrname, flux.insname, flux.corrname);
    if (strlen(flux.arrname) > 0) {
      /* if ARRNAME specified, read corresponding OI_ARRAY */
      read_oi_array(fptr2, flux.arrname, &array, &status);
    }
    if (strlen(flux.corrname) > 0) {
      /* if CORRNAME specified, read corresponding OI_CORR
         Note we may have read it previously */
      read_oi_corr(fptr2, flux.corrname, &corr, &status);
    }
    read_oi_wavelength(fptr2, flux.insname, &wave, &status);
    if (!status) {
      /* Free storage ready to reuse structs for next table */
      free_oi_wavelength(&wave);
      if (strlen(flux.arrname) > 0) free_oi_array(&array);
      if (strlen(flux.corrname) > 0) free_oi_corr(&corr);
      free_oi_flux(&flux);
    }
  }
  if (status != END_OF_FILE) goto except;
  status = 0;

  /* Read all OI_INSPOL tables and corresponding OI_ARRAY tables */
  fits_movabs_hdu(fptr, 1, &hdutype, &status); /* back to start */
  while (TRUE) {
    if (read_next_oi_inspol(fptr, &inspol, &status))
      break;  /* no more OI_INSPOL */
    printf("Read OI_INSPOL   with  ARRNAME=%s\n", inspol.arrname);
    read_oi_array(fptr2, inspol.arrname, &array, &status);
    if (!status) {
      /* Free storage ready to reuse structs for next table */
      free_oi_array(&array);
      free_oi_inspol(&inspol);
    }
  }
  if (status != END_OF_FILE) goto except;
  status = 0;

  fits_close_file(fptr, &status);
  fits_close_file(fptr2, &status);
  free_oi_target(&targets);
  return;

except:
  exit(EXIT_FAILURE);
}


/**
 * Main function for demonstration program
 */
int main(int argc, char *argv[])
{
  demo_write();
  demo_read();
  exit(EXIT_SUCCESS);
}
