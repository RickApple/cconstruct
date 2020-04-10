
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
echo on

set COMPILE_CONSTRUCT_COMMAND=cl.exe -EHsc -Fe: cconstruct.exe /TC


pushd 01_hello_world
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\hello_world\Output\hello_world.exe || exit /b
popd


pushd 02_include_folders
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\include_folders\Output\include_folders.exe || exit /b
popd


pushd 03_library_dependency
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
devenv.com build\msvc\library_dependency.sln /Build "Debug|x64" || exit /b
build\msvc\x64\Debug\my_binary\Output\my_binary.exe || exit /b
popd


pushd 04_preprocessor
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\preprocessor\Output\preprocessor.exe || exit /b
devenv.com build\workspace.sln /Build "Release|x64" || exit /b
build\x64\Release\preprocessor\Output\preprocessor.exe
rem the release build is expected to give an error, since the preprocessor has a different value for that build
if %errorlevel% neq 1 exit /b %errorlevel%
popd


pushd 05_compile_flags
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
devenv.com build\workspace.sln /Build "Release|x64"
if %errorlevel% neq 1 exit /b %errorlevel%
REM building should cause an error because flag has been added to set warnings as errors
popd


pushd 06_post_build_action
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
devenv.com build\workspace.sln /Build "Debug|x64"
if %errorlevel% equ 0 exit /b %errorlevel%
rem The build is expected to give an error, since the post build action doesn't succeed. However, the executable has been built as tested on the next line
build\x64\Debug\post_build_action\Output\post_build_action.exe || exit /b
popd



pushd ..\tools
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
rem Create output folder for combined file
mkdir ..\build
build\x64\Debug\cconstruct_release\Output\cconstruct_release.exe ../source/cconstruct.h ../build/cconstruct_release.h || exit /b
popd

echo All Tests Finished