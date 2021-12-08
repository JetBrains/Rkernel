set R_HOME=C:\intellij\rplugin\rwrapper\R-4.1.0
set R_INCLUDE=C:\intellij\rplugin\rwrapper\R-4.1.0\include
set PATH=C:\intellij\rplugin\rwrapper\R-4.1.0\bin;C:\intellij\rplugin\rwrapper\R-4.1.0\bin\x64;%PATH%

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\vsdevcmd.bat" -arch=amd64

cd rwrapper
call clone_dependency.bat

mkdir gens\protos
XCOPY ..\protos\* Rkernel-proto /s /i

call Rscript.exe dll2lib.R

rmdir /S /Q build
mkdir build
cd build


..\cmake-distro\bin\cmake.exe .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" -G Ninja
"C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"