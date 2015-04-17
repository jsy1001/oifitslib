/**
 * @file
 * @ingroup oitable
 * Definition of wrappers for malloc() and realloc() and common assertions.
 *
 * Used to terminate the program if memory allocation fails or if an
 * unexpected NULL pointer value is encountered.
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

#ifndef CHK_MALLOC_H
#define CHK_MALLOC_H

#include <stdlib.h>
#include <stdio.h>  /* fprintf() */
#include <string.h>  /* strerror() */


/**
 * Macro to terminate the program if the value of ptr is NULL.
 */
#define assert_not_null(ptr)                                             \
  do {                                                                   \
    if((ptr) == NULL) {                                                  \
      fprintf(stderr, "ERROR:%s:%d:%s: Unexpected NULL pointer value\n", \
              __FILE__, __LINE__, __func__);                             \
      abort();                                                           \
    }                                                                    \
  } while(0)

/**
 * Macro to terminate program with useful error message if ret is non-zero.
 *
 * The error message includes strerror(errno). This macro should only
 * be used after calls that are expected to succeed, for example
 * pthread_mutex_init().
 */
#define assert_no_error(ret, errno)                                     \
  do {                                                                  \
    if(ret) {                                                           \
      fprintf(stderr,                                                   \
              "ERROR:%s:%d:%s: System call failed unexpectedly: %s\n",  \
              __FILE__, __LINE__, __func__, strerror(errno));           \
      abort();                                                          \
    }                                                                   \
  } while(0)
    

/**
 * Replacement for malloc() that either succeeds (returning a non-NULL
 * pointer) or terminates the program with a useful error message.
 */
#define chkmalloc(size) ( _chkmalloc(size, __FILE__, __LINE__, __func__) )

/**
 * Replacement for realloc() that either succeeds (returning a
 * non-NULL pointer) or terminates the program with a useful error
 * message.
 */
#define chkrealloc(ptr, size) ( _chkrealloc(ptr, size,                  \
                                            __FILE__, __LINE__, __func__) )

void *_chkmalloc(size_t size, const char *file, int line, const char *func);
void *_chkrealloc(void *ptr, size_t size,
                  const char *file, int line, const char *func);

#endif  /* #ifndef CHK_MALLOC_H */
