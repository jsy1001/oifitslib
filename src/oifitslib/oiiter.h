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
#include "oifilter.h" /* oi_filter_spec */

#include <stdbool.h>

/**
 * Opaque structure representing an iterator that can be used to
 * iterate over data points in an OIFITS dataset.
 */
typedef struct
{
  /** @privatesection */
  const oi_fits *pData;
  oi_filter_spec filter;
  GList *link;
  oi_wavelength *pWave;
  int extver;
  long irec;
  int iwave;

} _oi_iter;

/**
 * Opaque structure representing an iterator that can be used to
 * iterate over the complex visibility points in an OIFITS dataset.
 */
typedef _oi_iter oi_vis_iter;

/**
 * Opaque structure representing an iterator that can be used to
 * iterate over the squared visibility points in an OIFITS dataset.
 */
typedef _oi_iter oi_vis2_iter;

/**
 * Opaque structure representing an iterator that can be used to
 * iterate over the triple product points in an OIFITS dataset.
 */
typedef _oi_iter oi_t3_iter;

void oi_vis_iter_init(oi_vis_iter *, const oi_fits *, const oi_filter_spec *);
bool oi_vis_iter_next(oi_vis_iter *, int *const, oi_vis **, long *const,
                      oi_vis_record **, int *const);
void oi_vis_iter_get_uv(const oi_vis_iter *, double *const, double *const,
                        double *const);
void oi_vis2_iter_init(oi_vis2_iter *, const oi_fits *, const oi_filter_spec *);
bool oi_vis2_iter_next(oi_vis2_iter *, int *const, oi_vis2 **, long *const,
                       oi_vis2_record **, int *const);
void oi_vis2_iter_get_uv(const oi_vis2_iter *, double *const, double *const,
                         double *const);
void oi_t3_iter_init(oi_t3_iter *, const oi_fits *, const oi_filter_spec *);
bool oi_t3_iter_next(oi_t3_iter *, int *const, oi_t3 **, long *const,
                     oi_t3_record **, int *const);
void oi_t3_iter_get_uv(const oi_t3_iter *, double *const, double *const,
                       double *const, double *const, double *const);

#endif /* #ifndef OIITER_H */

/** @} */
