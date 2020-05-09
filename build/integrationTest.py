#!/usr/bin/env python3

import subprocess
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
    ret = subprocess.Popen(["./cql","-c", q], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    out, err = ret.communicate()
    out = "".join(map(chr,out))
    print(out)
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
