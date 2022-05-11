cd build
cmake .. -G "MinGW Makefiles"
make clean
make

cd ..
.\build\testdummyscheme.exe
