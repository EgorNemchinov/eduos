#!/bin/sh

. $TESTDIR/lib.sh

make

check_out ./image $CDIR/1
