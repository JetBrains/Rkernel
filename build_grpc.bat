cd vcpkg
call .\bootstrap-vcpkg.bat
.\vcpkg.exe install grpc:x64-windows-static
RD /S /Q ..\GRPC
MOVE /Y installed ..\GRPC

