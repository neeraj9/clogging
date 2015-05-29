/*
 * Copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
 *
 *  This file is part of clogging.
 *
 *  clogging is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  clogging is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with clogging.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOGGING_H
#define LOGGING_H

/* include appropriate logging, but in the future
 * more logging mechanisms will be added here, which
 * will be conditionally available via #ifdef.
 *
 * Alternatively the user can directly include the
 * appropriate logging instance (in this case basic_logging)
 * directly.
 *
 * In short this header is just the placeholder for a single
 * logging implementation which will be selected (in future)
 * based on flags.
 */
#include "basic_logging.h"

#endif /*LOGGING_H */
