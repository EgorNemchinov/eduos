#!/bin/sh

. $TESTDIR/lib.sh

make -C shell

check_out $CDIR/1 shell/shell
