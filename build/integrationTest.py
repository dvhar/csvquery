#!/usr/bin/env python3

#TODO: check actual results instead of just return code after csv formatting is finalized

from subprocess import Popen
import json

picky = False
picks= [3,4] #which tests to run when picky

f1 = "'test/country.csv' nh"
f2 = "'test/cities.csv'"

def runtest(test):
	q = test['query'] % {'f1':f1, 'f2':f2}
	print("===============================================================")
	print(test['title'])
	print("---------------------------------------")
	print(q)
	print("---------------------------------------")
	ret = Popen(["./cql","-c", q])
	ret.communicate()
	if ret.returncode != test['code']:
		print(f"Failed! Expected return code {test[1]} but got {ret.returncode}")
		quit()
	print("---------------------------------------")
	print(f"Success! return code: {ret.returncode}")

with open('testdata.json') as testfile:
	tests = json.load(testfile)
	for test in tests:
		if picky:
			if i in picks:
				runtest(test)
		else:
			runtest(test)

print("All tests passed!")
