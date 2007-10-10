/* $Id$ */

/**
 * @file oicheck.h
 * @ingroup oicheck
 *
 * Definitions for OIFITS conformity checker.
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

/**
 * @defgroup oicheck  OIFITS Conformity Checker
 *
 * This module provides a set of checks that a OIFITS dataset conforms
 * to the OIFITS standard. The list of checks is not exhaustive, but
 * goes beyond the level of conformity needed for read_oi_fits() to
 * succeed.
 *
 * Separate functions are provided to check different aspects of the
 * dataset, so that an application can perform only those checks that
 * are relevant to the tasks it performs. The available checking
 * functions are:
 * - check_unique_targets()
 * - check_targets_present()
 * - check_elements_present()
 * - check_flagging()
 * - check_t3amp()
 * - check_waveorder()
 *
 * Each checking function is passed a pointer to a oi_fits struct
 * containing the data to check (except check_unique_targets()), and a
 * pointer to an unintialised oi_check_result struct. The latter will
 * contain the detailed results of the check (including a truncated
 * list of the places in the file where problems were detected) when
 * the function returns.
 *
 * The checking function returns a oi_breach_level giving the severity
 * of the worst problem found. The caller may use this value to decide
 * whether to report the problems to the user, which it should do by
 * calling format_check_result() or print_check_result().
 *
 * Before reusing a oi_check_result for another check, you should pass
 * its address to free_check_result() to avoid memory leaks.
 *
 * @{
 */

#ifndef OICHECK_H
#define OICHECK_H

#include "oifile.h"

/*
 * Data structures
 */


#define MAX_REPORT 10  /**< Maximum times to report same class of breach */

/** Severity of a check failure */
typedef enum {
  OI_BREACH_NONE,       /**< No problem */
  OI_BREACH_WARNING,    /**< Valid OIFITS, but may cause problems */
  OI_BREACH_NOT_OIFITS, /**< Does not conform to the OIFITS standard */
  OI_BREACH_NOT_FITS,   /**< Does not conform to the FITS standard */
} oi_breach_level;

/** Result of checking for a particular class of standard breach */
typedef struct {
  oi_breach_level level;      /**< Severity of breach */
  char *description;    /**< Description of breach */
  int numBreach;              /**< Number of occurrences found */
  char *location[MAX_REPORT]; /**< Strings identifying locations of breaches */
  GStringChunk *chunk;        /**< Storage for strings */
} oi_check_result;

/** Standard interface to checking function. */
typedef oi_breach_level (*check_func)(oi_fits *, oi_check_result *);


/*
 * Function prototypes
 */
void free_check_result(oi_check_result *);
char *format_check_result(oi_check_result *);
void print_check_result(oi_check_result *);
oi_breach_level check_unique_targets(oi_fits *, oi_check_result *);
oi_breach_level check_targets_present(oi_fits *, oi_check_result *);
oi_breach_level check_elements_present(oi_fits *, oi_check_result *);
oi_breach_level check_flagging(oi_fits *, oi_check_result *);
oi_breach_level check_t3amp(oi_fits *, oi_check_result *);
oi_breach_level check_waveorder(oi_fits *, oi_check_result *);

#endif /* #ifndef OICHECK_H */

/** @} */
