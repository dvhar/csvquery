#!/usr/bin/env python3

import subprocess
import tempfile
import yaml
import os

picky = False
picks = [24] #which tests to run when picky

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
        # print(err)
        quit()
    if test['code'] == 0:
        #print(out.decode('UTF-8'))
        with open('test/results/' + test['output'], 'rb') as res:
            expected = res.read()
            if os.name == 'nt':
                expected = expected.replace(b'\n',b'\r\n')
            #print(expected.decode('utf-8'))
            if out != expected:
                print(f"Failed. Expected from {test['output']}:")
                # Write expected and actual output to temp files
                expected_path = 'test/results/' + test['output']
                with tempfile.NamedTemporaryFile(delete=False, mode='wb') as outf:
                    outf.write(out)
                    outf.flush()
                    out_path = outf.name

                # Run icdiff (make sure it's installed and in your PATH)
                print(f"\ndiff between:\nExpected: {expected_path}\nActual: {out_path}\n")
                subprocess.run(['diff', expected_path, out_path])
                quit()
    print("---------------------------------------")
    print(f"Success! return code: {ret.returncode}")

with open('test/testdata.yml') as testfile:
    tests = yaml.load(testfile, Loader=yaml.FullLoader)
    i=0
    for test in tests:
        if picky:
            if i in picks:
                runtest(test)
        else:
            runtest(test)
        i += 1

print("All tests passed")
