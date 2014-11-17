/**
 * @file
 * Command-line OIFITS conformity check utility.
 *
 * Copyright (C) 2007, 2014 John Young
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
  /** Checking functions to call */
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
  oi_fits oi;
  oi_check_result result;
  oi_breach_level worst;
  char filename[FLEN_FILENAME];
  int status, i;

  /* Parse command-line */
  if(argc != 2) {
    printf("Usage:\n%s FILE\n", argv[0]);
    exit(2); /* standard unix behaviour */
  }
  (void) g_strlcpy(filename, argv[1], FLEN_FILENAME);

  /* Read FITS file */
  status = 0;
  read_oi_fits(filename, &oi, &status);
  if(status) goto except;

  /* Display summary info */
  print_oi_fits_summary(&oi);

  /* Run checks */
  worst = OI_BREACH_NONE;
  i=0;
  while(checks[i] != NULL) {
    if((*checks[i++])(&oi, &result) != OI_BREACH_NONE) {
      print_check_result(&result);
      if(result.level > worst) worst = result.level;
    }
    free_check_result(&result);
  }

  if(worst == OI_BREACH_NONE)
    printf("All checks passed\n");

  free_oi_fits(&oi);
  exit(EXIT_SUCCESS);

 except:
  exit(EXIT_FAILURE);
}
