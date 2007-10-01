/* $Id$ */

/**
 * @file oifilter.c
 * @ingroup oifilter
 *
 * Implementation of OIFITS filter.
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

#include "oifilter.h"


/*
 * Public functions
 */

/**
 * Filter OIFITS data
 *
 * @param input   input dataset to apply filter to.
 * @param filter  filter specification.
 *
 * @return Pointer to new oi_fits struct containing filtered data
 */
oi_fits *oi_filter(const oi_fits *input, oi_filter_spec filter)
{
  /* :TODO: */
}
