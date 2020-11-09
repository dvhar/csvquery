#!/usr/bin/env bash

mkdir -p csvquery.app/Contents/MacOS/
mkdir -p csvquery.app/Contents/libs

cp ./cql ./csvquery.app/Contents/MacOS/

dylibbundler -od -b -x ./csvquery.app/Contents/MacOS/cql -d ./csvquery.app/Contents/libs
