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

#include <stdio.h>


/**
 * Main function for command-line upgrade utility
 */
int main(int argc, char *argv[])
{
  const char *inFilename, *outFilename, *origin, *observer, *insmode;
  oi_fits oi;
  int status;

  /* Parse command-line */
  if (argc < 5) {
    printf("Usage:\n%s INFILE OUTFILE ORIGIN OBSERVER INSMODE\n", argv[0]);
    exit(2); /* standard unix behaviour */
  }
  inFilename = argv[1];
  outFilename = argv[2];
  origin = argv[3];
  observer = argv[4];
  insmode = argv[5];

  /* Read FITS file */
  status = 0;
  read_oi_fits(inFilename, &oi, &status);
  if (status) goto except;

  if (is_oi_fits_two(&oi))
  {
    printf("Input datafile is already latest OIFITS version\n");
    goto except;
  }
  if (oi.numArray == 0)
  {
    printf("Input datafile has no OI_ARRAY table - cannot convert\n");
    goto except;
  }

  /* Set additional header keywords from command-line arguments */
  g_strlcpy(oi.header.origin, origin, FLEN_VALUE);
  g_strlcpy(oi.header.observer, observer, FLEN_VALUE);
  g_strlcpy(oi.header.insmode, insmode, FLEN_VALUE);

  /* Display summary info */
  printf("=== INPUT DATA: ===\n");
  print_oi_fits_summary(&oi);

  /* Write out data (automatically uses latest format) */
  //:TODO: suppress version warnings?
  write_oi_fits(outFilename, oi, &status);
  if (status) goto except;

  free_oi_fits(&oi);
  exit(EXIT_SUCCESS);

except:
  exit(EXIT_FAILURE);
}
