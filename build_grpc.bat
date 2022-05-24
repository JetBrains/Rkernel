cd vcpkg
call .\bootstrap-vcpkg.bat || exit /b %ERRORLEVEL%
.\vcpkg.exe install grpc[codegen,core]:x64-windows-static || exit /b %ERRORLEVEL%
.\vcpkg.exe install tiny-process-library:x64-windows-static || exit /b %ERRORLEVEL%
RD /S /Q ..\GRPC
MOVE /Y installed ..\GRPC || exit /b 3

