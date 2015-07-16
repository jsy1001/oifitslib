/**
 * @file
 * @ingroup oiiter
 * Definition of OIFITS iterator interface.
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

/**
 * @defgroup oiiter  Iterator interface for OIFITS data
 *
 * This module implements an iterator interface for OIFITS data. This
 * interface allows an application to iterate over all of the data
 * points of a particular type (complex visibilities, squared
 * visibilities, or bispectra) within a file, without explicit
 * iteration over the tables that contain them.
 *
 * @{
 */

#ifndef OIITER_H
#define OIITER_H

#include "oifile.h"
#include "oifilter.h"  /* oi_filter_spec */

#include <stdbool.h>

/**
 * Opaque structure representing an iterator that can be used to
 * iterate over the squared visibility points in an OIFITS dataset.
 */
typedef struct {
  /** @privatesection */
  oi_fits *pData;
  oi_filter_spec filter;
  GList *link;
  int extver;
  long irec;
  int iwave;

} oi_vis2_iter;

void oi_vis2_iter_init(oi_vis2_iter *, oi_fits *, oi_filter_spec *const);
bool oi_vis2_iter_next(oi_vis2_iter *, int *const, long *const,
                       oi_vis2_record **, int *const);

#endif  /* #ifndef OIITER_H */

/** @} */
