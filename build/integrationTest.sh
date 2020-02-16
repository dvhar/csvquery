#!/bin/bash

f1="'test/country.csv' nh"
f2="'test/cities.csv'"

tests=$(mktemp)
trap "rm $tests; exit" EXIT
cat >> $tests << EOF

["Basic top 10",0,"select top 10 from $f1"]
["With int and addition types",0,
	"with c7 as int select top 10 int, c9, int+c9, int+c1, int+c7
	c1+c2 as ss, c1+c9 as sf, c1+c7 as si, c7+c9 as if, c16+'11 years' as dd
	from $f1"]

EOF

runtest(){
	echo ===================================================================
	echo $@ | jq -r '.[0]'
	echo ------------------
	expect=$(echo $@ | jq -r '.[1]')
	query=$(echo $@ | jq -r '.[2]')
	echo $query
	echo ------------------
	./cql -c "$query"
	[ ! "$?" = "$expect" ] && echo Failed! && exit 1
}

fullq=""
while read q; do
	fullq="$fullq $q"
	if [ ! "$(echo $q | grep ']')" = "" ]; then
		runtest $fullq
		fullq=""
	fi
done < $tests

