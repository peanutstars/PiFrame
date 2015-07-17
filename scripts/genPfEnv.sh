#!/bin/bash

RULES_FILE=$BASEDIR/Rules.mk
SRC_FILE=./pf-env

INFO_RELEASE=1
INFO_VERSION="0.00"
INFO_MODEL="NONE"

fEcho() {
	[ z"$1" == z"C" ] && echo > $SRC_FILE
	echo "$2" >> $SRC_FILE
}

fGetInformation() {
	DEBUG=`cat $RULES_FILE | grep DEBUG | sed -e 's/^ *//g' -e 's/ *$//g' | egrep -v "^#" | awk -F "=" '{ print $2 }'`
	INFO_MODEL=`cat $RULES_FILE | grep MODEL | sed -e 's/^ *//g' -e 's/ *$//g' | egrep -v "^#" | awk -F "=" '{ print $2 }'`
	INFO_VERSION=`cat $RULES_FILE | grep VERSION | sed -e 's/^ *//g' -e 's/ *$//g' | egrep -v "^#" | awk -F "=" '{ print $2 }'`

	if [ ."$DEBUG" == ."1" ]; then
		INFO_RELEASE=0
	fi
}

fMakeFile() {
	fEcho C "export INFO_RELEASE=$INFO_RELEASE"
	fEcho N "export INFO_VERSION=$INFO_VERSION"
	fEcho N "export INFO_MODEL=$INFO_MODEL"
	fEcho N ""

	fEcho N "export LD_LIBRARY_PATH=/system/lib"
	fEcho N "export PATH=/system/bin:/bin:/sbin:/usr/bin:/usr/sbin"
	fEcho N ""
}

fGetInformation
fMakeFile

