
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\\Tools\\VsDevCmd.bat"
echo on

set COMPILE_CONSTRUCT_COMMAND=cl.exe /FC /EHsc /Fo%TEMP% /Fecconstruct.exe /nologo /TC
set COMPILE_CONSTRUCT_CPP_COMMAND=cl.exe /FC /EHsc /Fo%TEMP% /Fecconstruct.exe /nologo /TP







pushd 18_custom_commands
rd /S /Q build
del src\test.txt
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
set TEST_TIME=%time%
echo %TEST_TIME%>src\test_source.txt
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
rem copy src\test_source.txt src\test.txt
rem Get output into variable
FOR /F "tokens=* USEBACKQ" %%F IN (`build\x64\Debug\custom_commands.exe`) DO (
SET CMD_OUTPUT=%%F
)
if NOT "%CMD_OUTPUT%" == "test %TEST_TIME%" exit 1
popd


exit 0


pushd 01_hello_world
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\hello_world.exe || exit /b
popd


pushd 02_include_folders
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\include_folders.exe || exit /b
popd


pushd 03_library_dependency
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\msvc\library_dependency.sln /Build "Debug|x64" || exit /b
build\msvc\x64\Debug\my_binary.exe || exit /b
popd


pushd 04_preprocessor
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\preprocessor.exe
rem Debug build has the expected value for the define, so should return 0
if %errorlevel% neq 0 exit /b %errorlevel%
devenv.com build\workspace.sln /Build "Release|x64" || exit /b
build\x64\Release\preprocessor.exe
rem Release build is expected to return 1, since the define has a different value for that build
if %errorlevel% neq 1 exit /b %errorlevel%
popd


pushd 05_compile_flags
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Release|x64"
if %errorlevel% neq 1 exit /b %errorlevel%
REM building should cause an error because flag has been added to set warnings as errors
popd


pushd 06_post_build_action
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64"
rem The build is expected to give an error, since the post build action doesn't succeed ...
if %errorlevel% equ 0 exit /b %errorlevel%
rem ... However, the executable has been built, so test it is there and works correctly
build\x64\Debug\post_build_action.exe || exit /b
popd


pushd 07_changed_config
rd /S /Q build
copy return_value1.inl return_value.inl
cl.exe /EHsc /FC /Fecconstruct.exe config.cc /nologo || exit /b
cconstruct.exe
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\changed_config.exe
rem The first config builds the program so that it returns 1, so check for that specifically
if %errorlevel% neq 1 exit /b %errorlevel%
copy return_value2.inl return_value.inl
cconstruct.exe
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\changed_config.exe
rem The second config builds the program so that it returns 2, so check for that specifically
if %errorlevel% neq 2 exit /b %errorlevel%
rem Now set back the first config, but don't rebuild
copy return_value1.inl return_value.inl
cconstruct.exe --generate-projects
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\changed_config.exe
if %errorlevel% neq 2 exit /b %errorlevel%
popd


pushd 08_project_structure
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\project_structure.sln /Build "Debug|x64" || exit /b
build\x64\Debug\my_binary.exe || exit /b
popd


pushd 09_warning_level
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
rem Without doing something about warnings, the following builds would fail.
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
devenv.com build\workspace.sln /Build "Release|x64" || exit /b
popd


pushd 10_mixing_c_and_cpp
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\mixing_c_and_cpp.exe || exit /b
popd


pushd 11_nested_folders
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% src/config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\nested_folders.exe || exit /b
popd


pushd 12_config_folders
rd /S /Q build
REM this test requires the build folder be there
mkdir build 
cl.exe /EHsc /Fo%TEMP% /FC /Febuild/cconstruct.exe /nologo /TC project/config.cc || exit /b
pushd build
cconstruct.exe --generate-projects || exit /b
devenv.com workspace.sln /Build "Debug|x64" || exit /b
x64\Debug\config_folders.exe || exit /b
popd
REM also check if it works when calling it from a different folder
del build\config_folder.vcxproj.*
build\cconstruct.exe || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
popd


pushd 13_cpp_config
rd /S /Q build
%COMPILE_CONSTRUCT_CPP_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
popd


pushd 14_c_config
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
popd


pushd 15_other_file_types
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
popd


pushd 16_windowed_application
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
build\x64\Debug\windowed_application.exe || exit /b
popd


pushd 17_link_flags
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
if not exist build\x64\Debug\link_flags_named.pdb (
  exit 1
)
popd



pushd ..\tools
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
devenv.com build\workspace.sln /Build "Debug|x64" || exit /b
rem Create output folder for combined file
mkdir ..\build
build\x64\Debug\cconstruct_release.exe ../source/cconstruct.h ../build/cconstruct.h || exit /b
popd

echo All Tests Finished