# CSV query engine for big files

## Build
First install `boost` libraries.
```
cd build
cmake ..
make
```
---
To build on windows using msys2 with mingw64, do this instead of `cmake ..` and `make`:
```
cmake .. -G "MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND"
mingw32-make.exe
```
Install msys2 dependencies for windows:
```
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-boost mingw-w64-x86_64-toolchain mingw-w64-x86_64-python-pip
python3 -m pip install pyyaml
```
## Test
First install `pyyaml` python library
```
cd build
./runTests.py
```
## Run
Show usage info:
```
./cql help
```
Run queries from command line:
```
./cql "select from stuff.csv"
```
Start http server to run queries from web browser interface or to view query language documentation:
```
./cql
```
