/*
    This file is part of clogging.

    clogging is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    clogging is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with clogging.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "logging.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
	INIT_LOGGING(argv[0], "", LOG_LEVEL_DEBUG);
	LOG_DEBUG("A basic debug log looks like this");
	return 0;
}
