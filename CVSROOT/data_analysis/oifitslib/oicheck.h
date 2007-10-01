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
 * to the OIFITS standard. The list of checks is not exhaustive.
 *
 * @{
 */

#ifndef OICHECK_H
#define OICHECK_H

#include "oifile.h"

/*
 * Data structures
 */


#define MAX_REPORT 10  /**< Max. times to report same class of breach */

/** Severity of a check failure */
typedef enum {
  OI_BREACH_NONE,       /**< No problem */
  OI_BREACH_WARNING,    /**< Valid OIFTS, but may cause problems */
  OI_BREACH_NOT_OIFITS, /**< Does not conform to the OIFITS standard */
  OI_BREACH_NOT_FITS,   /**< Does not conform to the FITS standard */
} oi_breach_level;

/** Result of checking for a particular class of standard breach */
typedef struct {
  oi_breach_level level;      /**< Severity of breach */
  const char *description;    /**< Description of breach */
  int numBreach;              /**< Number of occurrences found */
  char *location[MAX_REPORT]; /**< Strings identifying locations of breaches */
} oi_check_result;


/*
 * Function prototypes
 */
void free_check_result(oi_check_result *);
char *format_check_result(oi_check_result *);
void print_check_result(oi_check_result *);
oi_breach_level check_unique_targets(oi_target *, oi_check_result *);
oi_breach_level check_targets_present(oi_fits *, oi_check_result *);
oi_breach_level check_elements_present(oi_fits *, oi_check_result *);
oi_breach_level check_flagging(oi_fits *, oi_check_result *);
oi_breach_level check_t3amp(oi_fits *, oi_check_result *);

#endif /* #ifndef OICHECK_H */

/** @} */
