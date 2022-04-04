cd build
cmake .. -G "MinGW Makefiles"
make clean
make
pause

cd ..

.\build\dummyscheme.exe
pause