#!/bin/bash
#
# copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
#
#  This file is part of clogging.
#
#  See LICENSE file for licensing information.
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
