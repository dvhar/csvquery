#!/usr/bin/env bash

dir=csvwin

mkdir -p $dir

while read LINE; do
	f=$(echo $LINE | cut -d' ' -f3 | grep mingw)
	[ -f $f ] && cp $f $dir
done < <(ldd ./cql.exe)

cp ./cql.exe $dir

zip -r ${dir}.zip $dir
