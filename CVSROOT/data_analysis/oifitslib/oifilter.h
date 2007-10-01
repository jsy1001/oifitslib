/* $Id$ */

/**
 * @file oifilter.h
 * @ingroup oifilter
 *
 * Definitions for OIFITS filter.
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
 * @defgroup oifilter  Filter for OIFITS data
 *
 * This module implements a filter for OIFITS data, intended for use
 * in selecting a subset of the data in a OIFITS file for analysis.
 *
 * The criteria by which data are accepted or rejected are specified
 * by an oi_filter_spec struct. This is passed to oi_filter() along
 * with a pointer to a oi_fits struct containing the input data. The
 * function returns a pointer to a new oi_fits struct containing the
 * filtered (output) data.
 *
 * @{
 */

#ifndef OIFILTER_H
#define OIFILTER_H

#include "oifile.h"


/** Filter specification for OIFITS data */
typedef struct {
  char arrname[FLEN_VALUE]; /**< If not "", accept this ARRNAME only */
  char insname[FLEN_VALUE]; /**< If not "", accept this INSNAME only */
  int target_id;       /**< If >= 0, accept this target only */
  float mjd_range[2];  /**< Minimum and maximum MJD to accept */
  float wave_range[2]; /**< Minimum and maximum central wavelength to accept */
  int accept_vis;      /**< If non-zero, accept OI_VIS data */
  int accept_vis2;     /**< If non-zero, accept OI_VIS2 data */
  int accept_t3amp;    /**< If non-zero, accept OI_T3 amplitude data */
  int accept_t3phi;    /**< If non-zero, accept OI_T3 phase data */
} oi_filter_spec;


/*
 * Function prototypes
 */
oi_fits *oi_filter(const oi_fits *, oi_filter_spec);

#endif /* #ifndef OIFILTER_H */

/** @} */
