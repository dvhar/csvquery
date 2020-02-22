#!/usr/bin/env python3
from subprocess import Popen

picky = False
picks= [3,4] #which tests to run when picky

f1 = "'test/country.csv' nh"
f2 = "'test/cities.csv'"

# [description, expected return code, query]
tests = [
["Basic top 10",0,f"select top 10 from {f1}"],
["With int and arithmetic",0, "with c7 as int select top 10 int, c9, int+c9, int+c1, int+c7 " "c1+c2 as ss, c1+c9 as sf, c1+c7 as si, c7+c9 as if, "
	f"int+-c5 as ii, int-c5 as ii2 c16, c16+'10 years', c16-'10 years', '10 years'*-2 from {f1}"],
["distinct string, case",0,f"select distinct c3 c15 case when c15 like '_F' or c15 like '%O' then 1 when c15 like '_i' then 2 else 0 end from {f1}"],
["distinct var",0,f"with c5 as int select top 10 distinct int, int from {f1}"],
["distinct hidden",0,f"with c5 as int select top 10 distinct hidden int, int from {f1}"],
["basic where",0,f"select top 10 c5 c7 c16 from {f1} where c5%2=0 and c7<200000 and c16 between '1/1/2020' and '1/1/2040'"],
["where in",0,f"select top 10 c1 from {f1} where c1 in (FRA, FRO, FSM, GAB, GBR, GEO, GHA, GIB, GIN)"],
["where not in",0,f"select top 10 c1 from {f1} where c1 not in (AFG, AGO, AIA, ALB, 'AND', ANT, ARE, ARG, ARM, ASM)"],
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


for i,test in enumerate(tests):
	if picky:
		if i in picks:
			runtest(test)
	else:
		runtest(test)

print("All tests passed!")
