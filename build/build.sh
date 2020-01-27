#cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/dave/gits/vcpkg/scripts/buildsystems/vcpkg.cmake
make -j4 2>&1 | egrep -v "(preserved in literal|b2c)"
