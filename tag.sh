#!/bin/sh
[ $# -lt 1 ] && echo need tag && exit 1
grep "string version" utils.cc | grep $1 > /dev/null
[ ! $? = 0 ] && echo update tag in source && exit 1
git diff-index --quiet HEAD
[ ! $? = 0 ] && echo repo is not clean && exit 1

git tag $1
