#!/bin/bash

CLI=`which cli`
[ -z "$CLI" ] && CLI=../cli

fRunTest() {
	# service cmd  - <cmd> [<fg|bg> <rvPath>] : execute command in svc.service.
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' 
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3'
}

fCheckPoint() {
	echo
	echo -e "It makes sure that FIFO buffer size is 5."
	echo -e "It is running and compled within 15 seconds at least."
	echo
}

fRunTest
fCheckPoint
