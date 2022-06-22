cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
make clean
make

cd ..
.\build\testdummyscheme.exe
