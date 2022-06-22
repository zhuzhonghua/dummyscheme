cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
make clean
make
pause

cd ..

.\build\dummyscheme.exe
pause