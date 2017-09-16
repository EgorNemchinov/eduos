#!/bin/sh

. $TESTDIR/lib.sh

make -C shell

check_out shell/shell $CDIR/1
