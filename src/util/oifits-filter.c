/**
 * @file
 * Command-line OIFITS filter utility.
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
#include "glib/gstdio.h"  /* g_remove() */

static gboolean clobber = FALSE;

static GOptionEntry entries[] =
{
  { "clobber", 'o', 0, G_OPTION_ARG_NONE, &clobber, "Overwrite output file",
    NULL },
  { NULL }
};

/**
 * Main function for command-line filter utility
 */
int main(int argc, char *argv[])
{
  GError *error;
  GOptionContext *context;
  char inFilename[FLEN_FILENAME], outFilename[FLEN_FILENAME];
  oi_fits inData, outData;
  int status;

  /* Parse command-line */
  error = NULL;
  context =
    g_option_context_new("INFILE OUTFILE - write filtered dataset to new file");
  g_option_context_add_main_entries(context, entries, NULL);
  g_option_context_add_group(context, get_oi_filter_option_group());
  g_option_context_parse(context, &argc, &argv, &error);
  if (error != NULL) {
    printf("Error parsing command-line options: %s\n", error->message);
    g_error_free(error);
    exit(2); /* standard unix behaviour */
  }
  if (argc != 3) {
    printf("Wrong number of command-line arguments\n"
           "Enter '%s --help' for usage information\n", argv[0]);
    exit(2);
  }
  g_strlcpy(inFilename, argv[1], FLEN_FILENAME);
  g_strlcpy(outFilename, argv[2], FLEN_FILENAME);

  /* Read FITS file */
  status = 0;
  read_oi_fits(inFilename, &inData, &status);
  if (status) goto except;

  /* Display summary info */
  printf("=== INPUT DATA: ===\n");
  print_oi_fits_summary(&inData);
  printf("=== Applying filter: ===\n");
  print_oi_filter(get_user_oi_filter());

  /* Apply filter */
  apply_user_oi_filter(&inData, &outData);
  printf("--> OUTPUT DATA: ===\n");
  print_oi_fits_summary(&outData);

  /* Check for existing output file */
  if (g_file_test(outFilename, G_FILE_TEST_EXISTS))
  {
    if (!clobber) {
      printf("Output file '%s' exists and '--clobber' not specified "
             "-> Exiting...\n", outFilename);
      exit(1);
    } else if (g_remove(outFilename)) {
      printf("Failed to remove existing output file '%s'\n", outFilename);
      exit(1);
    }
  }

  /* Write out filtered data */
  write_oi_fits(outFilename, outData, &status);
  if (status) goto except;

  free_oi_fits(&inData);
  free_oi_fits(&outData);
  g_option_context_free(context);
  exit(EXIT_SUCCESS);

except:
  exit(EXIT_FAILURE);
}
