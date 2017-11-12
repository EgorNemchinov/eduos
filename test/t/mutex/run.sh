#!/bin/sh

. $TESTDIR/lib.sh

make_eduos

./image <<< "mutex_test"
