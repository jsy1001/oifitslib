/**
 * @file
 * @ingroup oifilter
 * Definitions for OIFITS filter.
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
 * @defgroup oifilter  Filter for OIFITS data
 *
 * This module implements a filter for OIFITS data, intended for use
 * in selecting a subset of the data in a OIFITS file for analysis.
 *
 * The criteria by which data are accepted or rejected are specified
 * by an oi_filter_spec struct. A pointer to this struct is passed to
 * apply_oi_filter() along with pointers to two oi_fits structs, one
 * containing the input data. The second oi_fits should be
 * uninitialised on calling apply_oi_filter(); when the function
 * returns this will contain the filtered (output) data.
 *
 * In most cases, empty tables are not included in the filtered output.
 *
 * The module also implements a parser for command-line options that
 * may be used to specify the filter. To support these options, an
 * application must use the GLib commandline option parser described
 * at
 * http://library.gnome.org/devel/glib/stable/glib-Commandline-option-parser.html
 * Typically support for filtering command-line options would be
 * implemented as follows:
 * - Call g_option_context_new() to create an empty set of option definitions
 * - Call get_oi_filter_option_group() to retrieve the options supported by
 *   oifitslib
 * - Add these options to the definition set using g_option_context_add_group()
 * - Add other options supported by the application to the definition set
 * - Pass argc and argv to g_option_context_parse()
 * - Apply the specified filter to a dataset by calling apply_user_oi_filter()
 *
 * Functions to return and to display string representations of a
 * filter are also provided: format_oi_filter() and print_oi_filter()
 *
 * Applications should not normally need to call the lower-level
 * functions that filter subsets of the OIFITS tables (such as
 * filter_oi_target() and filter_all_oi_vis2())
 *
 * @{
 */

#ifndef OIFILTER_H
#define OIFILTER_H

#include "oifile.h"


/** Filter specification for OIFITS data */
typedef struct {
  /** @publicsection */
  char arrname[FLEN_VALUE];  /**< Accept ARRNAMEs matching this pattern */
  char insname[FLEN_VALUE];  /**< Accept INSNAMEs matching this pattern */
  char corrname[FLEN_VALUE]; /**< Accept CORRNAMEs matching this pattern */
  int target_id;       /**< If >= 0, accept this TARGET_ID only */
  double mjd_range[2];  /**< Minimum and maximum MJD to accept */
  float wave_range[2];  /**< Minimum and maximum central wavelength /m */
  double bas_range[2]; /**< Minimum and maximum projected baseline /m */
  float snr_range[2];  /**< Minimum and maximum SNR to accept */
  int accept_vis;      /**< If non-zero, accept OI_VIS data */
  int accept_vis2;     /**< If non-zero, accept OI_VIS2 data */
  int accept_t3amp;    /**< If non-zero, accept OI_T3 amplitude data */
  int accept_t3phi;    /**< If non-zero, accept OI_T3 phase data */
  int accept_spectrum; /**< If non-zero, accept OI_SPECTRUM data */
  int accept_flagged;  /**< If non-zero, accept records with all data flagged */

  /** @privatesection */
  GPatternSpec *arrname_pttn;  /**< Compiled pattern to match ARRNAME against */
  GPatternSpec *insname_pttn;  /**< Compiled pattern to match INSNAME against */
  GPatternSpec *corrname_pttn; /**< Compiled pattern to match CORRNAME against */
} oi_filter_spec;


/*
 * Function prototypes
 */
void init_oi_filter(oi_filter_spec *);
const char *format_oi_filter(oi_filter_spec *);
void print_oi_filter(oi_filter_spec *);
void apply_oi_filter(const oi_fits *, oi_filter_spec *, oi_fits *);
GOptionGroup *get_oi_filter_option_group(void);
oi_filter_spec *get_user_oi_filter(void);
void apply_user_oi_filter(const oi_fits *, oi_fits *);
void filter_oi_header(const oi_header *, const oi_filter_spec *, oi_header *);
void filter_oi_target(const oi_target *, const oi_filter_spec *, oi_target *);
void filter_all_oi_array(const oi_fits *, const oi_filter_spec *, oi_fits *);
GHashTable *filter_all_oi_wavelength(const oi_fits *, const oi_filter_spec *,
				     oi_fits *);
void filter_oi_wavelength(const oi_wavelength *, const float[2],
			  oi_wavelength *, char *);
void filter_all_oi_corr(const oi_fits *, const oi_filter_spec *, oi_fits *);
void filter_all_oi_polar(const oi_fits *, const oi_filter_spec *, GHashTable *,
                         oi_fits *);
void filter_oi_polar(const oi_polar *, const oi_filter_spec *, GHashTable *,
                     oi_polar *);
void filter_all_oi_vis(const oi_fits *, const oi_filter_spec *, GHashTable *,
		       oi_fits *);
void filter_oi_vis(const oi_vis *, const oi_filter_spec *, const char *,
		   oi_vis *);
void filter_all_oi_vis2(const oi_fits *, const oi_filter_spec *, GHashTable *,
			oi_fits *);
void filter_oi_vis2(const oi_vis2 *, const oi_filter_spec *, const char *,
		 oi_vis2 *);
void filter_all_oi_t3(const oi_fits *, const oi_filter_spec *, GHashTable *,
		      oi_fits *);
void filter_oi_t3(const oi_t3 *, const oi_filter_spec *, const char *,
		  oi_t3 *);
void filter_all_oi_spectrum(const oi_fits *, const oi_filter_spec *,
                            GHashTable *, oi_fits *);
void filter_oi_spectrum(const oi_spectrum *, const oi_filter_spec *,
                        const char *, oi_spectrum *);

#endif /* #ifndef OIFILTER_H */

/** @} */
