cd build
cmake .. -G "MinGW Makefiles"
make clean
make testdummyscheme

pause
cd ..
.\build\testdummyscheme.exe
pause
