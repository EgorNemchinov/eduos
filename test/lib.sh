
CDIR=$(dirname $0)

check_out() {
	local bin=$1
	local pat=$2

	$bin <$pat.in >$pat.out
	diff -q $pat.out $pat.out.gold
}

make_eduos() {
	make clean all
}

cd $TESTDIR/..
