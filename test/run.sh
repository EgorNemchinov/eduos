#!/bin/bash

export TESTDIR=$(dirname $0)

verbopt=""
verbdir="&>/dev/null"
erropt="-e"

while getopts vE opt; do
	case $opt in
		v) verbopt="-x"
		   verbdir=""
		   ;;
		E) erropt=""
		   ;;
		?) printf "Usage: %s: [−v] [ test... ]\n" $0
		   exit 2
		   ;;
	esac
done

shift  $(($OPTIND - 1))

pat="$@"

if [ -z $pat ]; then
	pat="*"
fi

ecode=0
for t in $TESTDIR/t/$pat; do
	if [ $verbopt ]; then
		/bin/sh $verbopt $erropt $t/run.sh
	else
		/bin/sh $verbopt $erropt $t/run.sh &>/dev/null
	fi

	if [ 0 -eq $? ]; then
		echo "OK: $t"
	else
		echo "FAIL: $t"
		ecode=1
	fi
done
exit $ecode
