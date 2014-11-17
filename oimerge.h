/**
 * @file
 * @ingroup oimerge
 * Definitions for merge component of OIFITSlib.
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

/**
 * @defgroup oimerge  OIFITS Merger
 *
 * This module implements merging of a list of OIFITS datasets into a
 * single dataset.
 *
 * To simplify the implementation, OI_ARRAY tables are not copied into
 * the output dataset (these are not required by the OIFITS
 * standard). Target records with the same target name are merged
 * (without checking that the coordinates etc. are identical), as are
 * duplicate OI_WAVELENGTH tables.
 *
 * A merged dataset should be obtained by calling merge_oi_fits()
 * (which takes a variable number of arguments) or
 * merge_oi_fits_list() (which takes a linked list of datasets to
 * merge). Applications should not normally need to call the
 * lower-level functions that merge subsets of the OIFITS tables (such
 * as merge_oi_target() and merge_all_oi_vis2()).
 *
 * @{
 */

#ifndef OIMERGE_H
#define OIMERGE_H

#include "oifile.h"

/*
 * Function prototypes
 */
void merge_oi_header(const GList *, oi_fits *);
GHashTable *merge_oi_target(const GList *, oi_fits *);
GList *merge_all_oi_array(const GList *, oi_fits *);
GList *merge_all_oi_wavelength(const GList *, oi_fits *);
GList *merge_all_oi_corr(const GList *, oi_fits *);
void merge_all_oi_vis(const GList *, GHashTable *,
                      const GList *, const GList *, const GList *, oi_fits *);
void merge_all_oi_vis2(const GList *, GHashTable *,
                       const GList *, const GList *, const GList *, oi_fits *);
void merge_all_oi_t3(const GList *, GHashTable *,
                     const GList *, const GList *, const GList *, oi_fits *);
void merge_all_oi_spectrum(const GList *, GHashTable *, const GList *,
                           const GList *, const GList *, oi_fits *);
void merge_oi_fits_list(const GList *, oi_fits *);
void merge_oi_fits(oi_fits *, oi_fits *, oi_fits *,...);

#endif /* #ifndef OIMERGE_H */

/** @} */
