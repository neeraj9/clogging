#!/bin/bash
#
# copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
#
#  This file is part of clogging.
#
#   clogging is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published by
#   the Free Software Foundation, either version 2.1 of the License, or
#   (at your option) any later version.
# 
#   clogging is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU Lesser General Public License
#   along with clogging.  If not, see <http://www.gnu.org/licenses/>.
#


aclocal
libtoolize --force
autoconf -i
autoheader
automake --add-missing
#mkdir build
#cd build
#../configure --prefix=`pwd`/build
#make

# run after updating configure.ac
#autoconf
#cd build
#../configure --prefix=`pwd`/../../install --no-create --no-recursion
#make
