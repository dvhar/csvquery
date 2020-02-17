#!/usr/bin/env python3
from subprocess import Popen

f1 = "'test/country.csv' nh"
f2 = "'test/cities.csv'"

# [description, expected return code, query]
tests = [
["Basic top 10",0,f"select top 10 from {f1}"],
["With int and arithmetic",0,
	"with c7 as int select top 10 int, c9, int+c9, int+c1, int+c7 "
	"c1+c2 as ss, c1+c9 as sf, c1+c7 as si, c7+c9 as if, "
	"int+-c5 as ii, int-c5 as ii2 c16, c16+'10 years', c16-'10 years', '10 years'*-2 "
	f"from {f1}"]
]

def runtest(test):
	print("===============================================================")
	print(test[0])
	print("---------------------------------------")
	print(test[2])
	print("---------------------------------------")
	ret = Popen(["./cql","-c",test[2]])
	ret.communicate()
	if ret.returncode != test[1]:
		print(f"Failed! Expected return code {test[1]} but got {ret.returncode}")
		quit()
	print("---------------------------------------")
	print(f"Success! return code: {ret.returncode}")


for test in tests:
	runtest(test)

print("All tests passed!")
