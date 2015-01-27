/**
 * @file
 * Command-line utility to upgrade OIFITS from v1 to v2.
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


/**
 * Main function for command-line upgrade utility.
 */
int main(int argc, char *argv[]) 
{
  GError *error;
  GOptionContext *context;
  char inFilename[FLEN_FILENAME], outFilename[FLEN_FILENAME];
  oi_fits data;
  int status;

  /* Parse command-line */
  error = NULL;
  context = g_option_context_new("INFILE OUTFILE - "
                                 "upgrade OIFITS to latest version");
  //g_option_context_set_main_group(context, get_oi_filter_option_group());
  g_option_context_parse(context, &argc, &argv, &error);
  if(error != NULL) {
    printf("Error parsing command-line options: %s\n", error->message);
    g_error_free(error);
    exit(2); /* standard unix behaviour */
  }
  if(argc != 3) {
    printf("Wrong number of command-line arguments\n"
	   "Enter '%s --help' for usage information\n", argv[0]);
    exit(2);
  }
  g_strlcpy(inFilename, argv[1], FLEN_FILENAME);
  g_strlcpy(outFilename, argv[2], FLEN_FILENAME);

  /* Read FITS file */
  status = 0;
  read_oi_fits(inFilename, &data, &status);
  if(status) goto except;

  /* :TODO: Set additional header keywords */
  g_strlcpy(data.header.origin, "ESO", FLEN_VALUE);
  g_strlcpy(data.header.insmode, "SCAN", FLEN_VALUE);

  /* Display summary info */
  printf("=== INPUT DATA: ===\n");
  print_oi_fits_summary(&data);

  //:TODO: check isn't already latest version

  /* Write out data (automatically uses latest format) */
  //:TODO: suppress version warnings?
  write_oi_fits(outFilename, data, &status);
  if(status) goto except;

  free_oi_fits(&data);
  g_option_context_free(context);
  exit(EXIT_SUCCESS);

 except:
  exit(EXIT_FAILURE);
}
