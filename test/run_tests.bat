
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
echo on


pushd hello_world

rd /S /Q build

cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

devenv.exe build\workspace.sln /Build "Debug|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
build\x64\Debug\hello_world.exe
if %errorlevel% neq 0 exit /b %errorlevel%

popd



pushd include_folders

rd /S /Q build

cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

devenv.exe build\workspace.sln /Build "Debug|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
build\x64\Debug\include_folders.exe
if %errorlevel% neq 0 exit /b %errorlevel%

popd



pushd library_dependency

rd /S /Q build

cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

devenv.exe build\msvc\library_dependency.sln /Build "Debug|x64"
if %errorlevel% neq 0 exit /b %errorlevel%

build\msvc\x64\Debug\my_binary.exe
if %errorlevel% neq 0 exit /b %errorlevel%

popd



pushd preprocessor

rd /S /Q build

cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

devenv.exe build\workspace.sln /Build "Debug|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
build\x64\Debug\preprocessor.exe
if %errorlevel% neq 0 exit /b %errorlevel%

devenv.exe build\workspace.sln /Build "Release|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
build\x64\Release\preprocessor.exe
rem the release build is expected to give an error, since the preprocessor has a different value for that build
if %errorlevel% neq 1 exit /b %errorlevel%

popd



pushd compile_flags

rd /S /Q build

cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

devenv.exe build\workspace.sln /Build "Release|x64"
if %errorlevel% neq 1 exit /b %errorlevel%
REM building should cause an error because flag has been added to set warnings as errors

popd





pushd ..\tools

rd /S /Q build

cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

devenv.exe build\workspace.sln /Build Debug
if %errorlevel% neq 0 exit /b %errorlevel%

rem Create output folder for combined file
mkdir ..\build
build\Debug\cconstruct_release.exe ../source/cconstruct.h ../build/cconstruct_release.h
if %errorlevel% neq 0 exit /b %errorlevel%

popd

echo All Tests Finished