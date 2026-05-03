# CSV query engine for big files

## Build
#### Linux and Mac
```
cd build
cmake ..
make
```
#### Windows
Install msys2.
In msys2 shell, install dependencies:
```
pacman -Syu git mingw-w64-x86_64-{cmake,toolchain,python-pip}
```
Then build it:
```
cd build
cmake .. -G 'MinGW Makefiles'
mingw32-make.exe
```
## Test
First install `pyyaml` python library if you don't already have it
```
python3 -m pip install pyyaml
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
./cql "select * from stuff.csv"
```
Start http server to run queries from web browser interface or to view query language documentation:
```
./cql
```
Here are two 11GB files being joined on a computer with 8GB of ram, though it would have worked fine with 2GB:
<img src="https://github.com/user-attachments/assets/9cda8b45-8b6e-4a53-a186-9a9b355ec7fa"/>
