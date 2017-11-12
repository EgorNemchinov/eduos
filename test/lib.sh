
CDIR=$(dirname $0)

check_out() {
	local pat=$1
	shift 1

	"$@" <$pat.in >$pat.out
	diff -q $pat.out $pat.out.gold
}

make_eduos() {
	make clean all
}

cd $TESTDIR/..
