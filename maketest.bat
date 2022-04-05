cd build
cmake .. -G "MinGW Makefiles"
make clean
make

pause
cd ..
.\build\testdummyscheme.exe
pause
