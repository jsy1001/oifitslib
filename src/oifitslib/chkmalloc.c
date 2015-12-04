/**
 * @file
 * @ingroup oitable
 * Implementation of wrappers for malloc() and realloc().
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

#include "chkmalloc.h"
#include <stdlib.h>
#include <stdio.h>


void *_chkmalloc(size_t size, const char *file, int line, const char *func)
{
  if (size == 0)
  {
    fprintf(stderr, "ERROR:%s:%d:%s: Trapped memory allocation of zero bytes\n",
            file, line, func);
    abort();
  }
  void *ret = malloc(size);
  if (ret == NULL)
  {
    fprintf(stderr, "ERROR:%s:%d:%s: Memory allocation of %lu bytes failed\n",
            file, line, func, (unsigned long)size);
    abort();
  }
  return ret;
}

void *_chkrealloc(void *ptr, size_t size,
                  const char *file, int line, const char *func)
{
  /* Note that realloc() with a zero size could be used instead of
   * free(), but we don't allow that here */
  if (size == 0)
  {
    fprintf(stderr, "ERROR:%s:%d:%s: Trapped reallocation of memory at %p"
            " to zero bytes. Use free() instead\n", file, line, func, ptr);
    abort();
  }
  void *ret = realloc(ptr, size);
  if (ret == NULL)
  {
    fprintf(stderr, "ERROR:%s:%d:%s: Reallocation of memory at %p"
            " to %lu bytes failed\n",
            file, line, func, ptr, (unsigned long)size);
    abort();
  }
  return ret;
}
