#!/usr/bin/env python3

import subprocess
import json

picky = False
picks= [3,4] #which tests to run when picky

def runtest(test):
    q = test['query']
    print("===============================================================")
    print(test['title'])
    print("---------------------------------------")
    print(q)
    ret = subprocess.Popen(["./cql","-c", q], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    out, err = ret.communicate()
    print("---------------------------------------")
    print(out.decode('UTF-8'))
    if ret.returncode != test['code']:
        print(f"Failed! Expected return code {test[1]} but got {ret.returncode}")
        print(err)
        quit()
    with open('test/results/' + test['output'], 'rb') as res:
        expected = res.read()
        if out != expected:
            print(f"Failed. Expected from {test['output']}:")
            print('==>'+expected.decode('UTF-8')+'<==')
            print('Got instead:')
            print('==>'+out.decode('UTF-8')+'<==')
            quit()
    print("---------------------------------------")
    print(f"Success! return code: {ret.returncode}")

with open('test/testdata.json') as testfile:
    tests = json.load(testfile)
    for test in tests:
        if picky:
            if i in picks:
                runtest(test)
        else:
            runtest(test)

print("All tests passed")
