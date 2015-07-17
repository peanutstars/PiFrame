#!/bin/bash


export LD_LIBRARY_PATH=`pwd`/../../target/x86/system/lib:$LD_LIBRARY_PATH

for IDX in 1 2 3 4
do
	./vmshm_test $IDX &
done
