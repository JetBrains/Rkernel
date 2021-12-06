cd rwrapper
call clone_dependency.bat
mkdir gens\protos
XCOPY ..\protos\* Rkernel-proto /s /i
call "C:\Program Files\R\R-4.1.0\bin\x64\Rscript.exe" dll2lib.R
rmdir /S /Q build
mkdir build
cd build


..\cmake-distro\bin\cmake.exe .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -G Ninja
"C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"