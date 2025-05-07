echo on
@set COMPILE_DEBUG_CONSTRUCT_COMMAND=cl.exe /ZI /W4 /WX /DEBUG /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TC
@set COMPILE_CONSTRUCT_COMMAND=cl.exe /W4 /WX /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TC
@set COMPILE_CONSTRUCT_CPP_COMMAND=cl.exe /W4 /WX /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TP
@set CMD_CONSTRUCT_WORKSPACE=cconstruct.exe --generator=ninja --generate-projects
@set BUILD_COMMAND=ninja -C build

@rem Find location of Visual Studio
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
    set VSPATH=%%i
)


@rem Do this test case first, as it needs to change the build environment around. Doing that with SETLOCAL/ENDLOCAL.
@SETLOCAL
pushd 22_cconstruct_architecture
@if exist build rd /S /Q build
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
build\x64\cconstruct_architecture.exe
if %errorlevel% neq 64 exit /b %errorlevel%
@popd
@ENDLOCAL
@SETLOCAL
@pushd 22_cconstruct_architecture
@if exist build rd /S /Q build
call "%VSPATH%\VC\Auxiliary\Build\vcvars32.bat"
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
build\x86\cconstruct_architecture.exe
if %errorlevel% neq 86 exit /b %errorlevel%
popd
@ENDLOCAL


@rem Now set a single environment for the rest of the tests
SETLOCAL
@rem -arch=x86 for 32-bit
@rem -arch=amd64 for 64-bit
rem call "%VSPATH%\Common7\Tools\VsDevCmd.bat"
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"




pushd 01_hello_world
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
build\x64\Debug\hello_world.exe || exit /b
popd


pushd 02_include_folders
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
build\x64\Debug\include_folders.exe || exit /b
popd


pushd 03_library_dependency
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
build\x64\Debug\my_binary.exe || exit /b
popd


pushd 03a_library_dependency_explicit
@if exist build rd /S /Q build
%COMPILE_DEBUG_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% --config=Debug || exit /b
%BUILD_COMMAND% x64\Debug\lib\my_library.lib || exit /b
%BUILD_COMMAND% || exit /b
build\x64\Debug\bin\my_binary.exe
if %errorlevel% neq 1 exit /b 1
%CMD_CONSTRUCT_WORKSPACE% --config=Release || exit /b
%BUILD_COMMAND% x64\Release\lib\my_library.lib || exit /b
%BUILD_COMMAND% || exit /b
build\x64\Release\bin\my_binary.exe
if %errorlevel% neq 2 exit /b 2
popd


pushd 04_preprocessor
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
build\x64\Debug\preprocessor.exe
rem Debug build has the expected value for the define, so should return 0
if %errorlevel% neq 0 exit /b 1
%CMD_CONSTRUCT_WORKSPACE% --config=Release || exit /b
%BUILD_COMMAND% --verbose || exit /b
build\x64\Release\preprocessor.exe
rem Release build is expected to return 1, since the define has a different value for that build
if %errorlevel% neq 1 exit /b 1
popd

pushd 05_compile_flags
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND%
if %errorlevel% neq 1 exit /b %errorlevel%
REM building should cause an error because flag has been added to set warnings as errors
popd

pushd 06_post_build_action
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND%
rem The build is expected to give an error, since the post build action doesn't succeed ...
if %errorlevel% equ 0 exit /b %errorlevel%
rem ... However, the executable has been built, so test it is there and works correctly
build\x64\Debug\post_build_action.exe || exit /b
popd


pushd 07_changed_config
if exist build rd /S /Q build
mkdir build
copy return_value1.inl return_value.inl
echo. >> return_value.inl
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE%
pushd build
ninja || exit /b
x64\Debug\changed_config.exe
rem The first config builds the program so that it returns 1, so check for that specifically
if %errorlevel% neq 1 exit /b %errorlevel%
copy /Y ..\return_value2.inl ..\return_value.inl
echo. >> ..\return_value.inl
ninja || exit /b
x64\Debug\changed_config.exe
rem The second config builds the program so that it returns 2, so check for that specifically
if %errorlevel% neq 2 exit /b %errorlevel%
rem Now set back the first config, Ninja should automatically rebuild cconstruct
copy /Y ..\return_value1.inl ..\return_value.inl
echo. >> ..\return_value.inl
ninja || exit /b
x64\Debug\changed_config.exe
if %errorlevel% neq 1 exit /b %errorlevel%
popd
popd


pushd 09_warning_level
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
rem Without doing something about warnings, the following builds would fail.
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
%CMD_CONSTRUCT_WORKSPACE% --config=Release || exit /b
%BUILD_COMMAND% || exit /b
popd


pushd 10_mixing_c_and_cpp
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND%  || exit /b
build\x64\Debug\mixing_c_and_cpp.exe || exit /b
popd


pushd 11_nested_folders
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% src/config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
build\x64\Debug\nested_folders.exe || exit /b
popd


pushd 12_config_folders
if exist build rd /S /Q build
REM this test requires the build folder be there
mkdir build 
cl.exe /EHsc /Fo%TEMP% /FC /Febuild/cconstruct.exe /nologo /TC /ZI /DEBUG project/config.cc || exit /b
pushd build
%CMD_CONSTRUCT_WORKSPACE% || exit /b
ninja || exit /b
x64\Debug\config_folders.exe || exit /b
popd
REM also check if it works when calling it from a different folder
del build\ninja.build
build\cconstruct.exe --generator=ninja || exit /b
%BUILD_COMMAND% || exit /b
popd


pushd 13_cpp_config
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_CPP_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND% || exit /b
popd


pushd 17_link_flags
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
%BUILD_COMMAND%  || exit /b
if not exist build\x64\Debug\link_flags_named.pdb (
  exit 1
)
popd


pushd 18_custom_commands
if exist build rd /S /Q build
del src\test.txt
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
%CMD_CONSTRUCT_WORKSPACE% || exit /b
set TEST_TIME=%time%
echo %TEST_TIME%>src\test_source.txt
%BUILD_COMMAND% || exit /b
rem copy src\test_source.txt src\test.txt
rem Get output into variable
FOR /F "tokens=* USEBACKQ" %%F IN (`build\x64\Debug\custom_commands.exe`) DO (
SET CMD_OUTPUT=%%F
)
if NOT "%CMD_OUTPUT%" == "test %TEST_TIME%" exit 1
popd


pushd 21_errors
if exist build rd /S /Q build
rem Create CConstruct binary which works fine
copy error_none.inl error.inl
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
rem Cause the recreation of CConstruct binary to fail
copy error_compile.inl error.inl
cconstruct.exe --generator=ninja
if %errorlevel% neq 1 exit /b %errorlevel%
rem Cause the recreation of CConstruct binary to succeed, but then crash during construction
copy error_construction.inl error.inl
cconstruct.exe --generator=ninja
if %errorlevel% neq 2 exit /b %errorlevel%
rem No error at all, everything goes OK from here on out
copy error_none.inl error.inl
cconstruct.exe --generator=ninja || exit /b
%BUILD_COMMAND% || exit /b
build\x64\Debug\errors.exe || exit /b
popd

echo All tests passed!


pushd ..\tools
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generator=ninja || exit /b
%BUILD_COMMAND% || exit /b
rem Create output folder for combined file
mkdir ..\build
build\x64\Debug\cconstruct_release.exe ../source/cconstruct.h ../build/cconstruct.h || exit /b
popd


echo Built release version