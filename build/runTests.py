#!/usr/bin/env python3

import subprocess
import yaml
import os

picky = False
picks = [3,4] #which tests to run when picky

def runtest(test):
    q = test['query']
    print("===============================================================")
    print(test['title'])
    print("---------------------------------------")
    print(q)
    ret = subprocess.Popen(["./cql","-c", q], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    out, err = ret.communicate()
    if ret.returncode != test['code']:
        print(f"Failed! Expected return code {test['code']} but got {ret.returncode}")
        print(err)
        quit()
    if test['code'] == 0:
        print("---------------------------------------")
        #print(out.decode('UTF-8'))
        with open('test/results/' + test['output'], 'rb') as res:
            expected = res.read()
            if os.name == 'nt':
                expected = expected.replace(b'\n',b'\r\n')
            #print(expected.decode('utf-8'))
            if out != expected:
                print(f"Failed. Expected from {test['output']}:")
                print(expected.decode('utf-8'))
                print('Got instead:')
                print(out.decode('utf-8'))
                quit()
    print("---------------------------------------")
    print(f"Success! return code: {ret.returncode}")

with open('test/testdata.yml') as testfile:
    tests = yaml.load(testfile, Loader=yaml.FullLoader)
    for test in tests:
        if picky:
            if i in picks:
                runtest(test)
        else:
            runtest(test)

print("All tests passed")
