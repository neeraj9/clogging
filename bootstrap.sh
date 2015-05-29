#!/bin/bash
#
# copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
#
# License: LGPLv3 or later

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
