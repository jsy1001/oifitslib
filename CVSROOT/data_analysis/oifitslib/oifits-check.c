/* $Id$ */

/**
 * @file oifits-check.c
 *
 * Command-line OIFITS conformity check utility.
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

#include "oicheck.h"


/**
 * Main function for command-line check utility.
 */
int main(int argc, char *argv[]) 
{
  oi_fits oi;
  oi_check_result result;
  char filename[FLEN_FILENAME];
  int status;

  /* Parse command-line */
  if(argc != 2) {
    printf("Usage:\n%s [FILE]\n", argv[0]);
    exit(2); /* standard unix behaviour */
  }
  (void) g_strlcpy(filename, argv[1], FLEN_FILENAME);

  /* Read FITS file */
  status = 0;
  read_oi_fits(filename, &oi, &status);
  if(status) goto except;

  /* Display summary info */
  oi_fits_print_summary(&oi);

  /* Run checks */
  if(check_unique_targets(&oi.targets, &result) != OI_BREACH_NONE) {
    print_check_result(&result);
  }
  if(check_targets_present(&oi, &result) != OI_BREACH_NONE) {
    print_check_result(&result);
  }
  if(check_elements_present(&oi, &result) != OI_BREACH_NONE) {
    print_check_result(&result);
  }
  if(check_flagging(&oi, &result) != OI_BREACH_NONE) {
    print_check_result(&result);
  }
  if(check_t3amp(&oi, &result) != OI_BREACH_NONE) {
    print_check_result(&result);
  }

  if(result.level == OI_BREACH_NONE)
    printf("All checks passed\n");

  free_check_result(&result);

  exit(EXIT_SUCCESS);

 except:
  exit(EXIT_FAILURE);
}
