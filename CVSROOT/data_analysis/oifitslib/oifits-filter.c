/* $Id$ */

/**
 * @file oifits-filter.c
 *
 * Command-line OIFITS filter utility.
 *
 * Copyright (C) 2007 John Young
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


/**
 * Main function for command-line filter utility.
 */
int main(int argc, char *argv[]) 
{
  oi_fits oi;
  char filename[FLEN_FILENAME];
  int status;

  /* Parse command-line */
  if(argc != 2) {
    printf("Usage:\n%s [OPTION]... [FILE]\n", argv[0]);
    exit(2); /* standard unix behaviour */
  }
  (void) g_strlcpy(filename, argv[1], FLEN_FILENAME);

  /* Read FITS file */
  status = 0;
  read_oi_fits(filename, &oi, &status);
  if(status) goto except;

  /* Display summary info */
  oi_fits_print_summary(&oi);

  /* Apply filter */
  /* :TODO: */

  exit(EXIT_SUCCESS);

 except:
  exit(EXIT_FAILURE);
}
