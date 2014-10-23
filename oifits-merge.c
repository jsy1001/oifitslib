/**
 * @file
 * Command-line OIFITS merge utility.
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

#include "oimerge.h"


/**
 * Main function for command-line check utility.
 */
int main(int argc, char *argv[]) 
{
  GList *filenameList, *inOiList, *link;
  char *filename, *outFilename;
  oi_fits *pOi;
  oi_fits outOi;
  int status, i, num;

  /* Parse command-line */
  if(argc < 4) {
    printf("Usage:\n%s OUTFILE INFILE1 INFILE2...\n", argv[0]);
    exit(2); /* standard unix behaviour */
  }
  outFilename = argv[1];
  filenameList = NULL;
  num = argc - 2;
  for(i=0; i<num; i++) {
    filenameList = g_list_append(filenameList, argv[2+i]);
  }

  /* Read input files */
  status = 0;
  inOiList = NULL;
  link = filenameList;
  while(link != NULL) {
    filename = (char *) link->data;
    pOi = malloc(sizeof(oi_fits));
    read_oi_fits(filename, pOi, &status);
    if (status) goto except;
    inOiList = g_list_append(inOiList, pOi);
    link = link->next;
  }

  /* Do merge */
  merge_oi_fits_list(inOiList, &outOi);

  /* Display summary info for merged data */
  printf("=== MERGED DATA: ===\n");
  print_oi_fits_summary(&outOi);

  /* Write out merged data */
  write_oi_fits(outFilename, outOi, &status);
  if (status) goto except;

  /* Free storage */
  g_list_free(filenameList);
  link = inOiList;
  while(link != NULL) {
    pOi = (oi_fits *) link->data;
    free_oi_fits(pOi);
    free(pOi);
    link = link->next;
  }
  g_list_free(inOiList);
  free_oi_fits(&outOi);

  exit(EXIT_SUCCESS);

 except:
  exit(EXIT_FAILURE);
}
