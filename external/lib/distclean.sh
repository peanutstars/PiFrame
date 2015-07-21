#!/bin/sh

SOFILES=`ls *a *.so*`

for f in $SOFILES pkgconfig
do
	rm -rf $1/$f
done
