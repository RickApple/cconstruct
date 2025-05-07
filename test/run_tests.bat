echo on
@set COMPILE_DEBUG_CONSTRUCT_COMMAND=cl.exe /ZI /W4 /WX /DEBUG /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TC
@set COMPILE_CONSTRUCT_COMMAND=cl.exe /W4 /WX /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TC
@set COMPILE_CONSTRUCT_CPP_COMMAND=cl.exe /W4 /WX /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TP

@set BUILD_DEBUG_COMMAND=msbuild /p:Configuration=Debug /p:Platform=x64 /v:minimal
@set BUILD_RELEASE_COMMAND=msbuild /p:Configuration=Release /p:Platform=x64 /v:minimal

@rem Find location of Visual Studio
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
    set VSPATH=%%i
)

@rem Do this test case first, as it needs to change the build environment around. Doing that with SETLOCAL/ENDLOCAL.
@SETLOCAL
pushd 22_cconstruct_architecture
@rd /S /Q build
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\cconstruct_architecture.exe
if %errorlevel% neq 64 exit /b %errorlevel%
@popd
@ENDLOCAL
@SETLOCAL
@pushd 22_cconstruct_architecture
@rd /S /Q build
call "%VSPATH%\VC\Auxiliary\Build\vcvars32.bat"
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x86\cconstruct_architecture.exe
if %errorlevel% neq 86 exit /b %errorlevel%
popd
@ENDLOCAL




@rem Now set a single environment for the rest of the tests
SETLOCAL
@rem -arch=x86 for 32-bit
@rem -arch=amd64 for 64-bit
call "%VSPATH%\Common7\Tools\VsDevCmd.bat"


pushd 01_hello_world
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\hello_world.exe || exit /b
popd


pushd 02_include_folders
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\include_folders.exe || exit /b
popd


pushd 03_library_dependency
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\library_dependency.sln || exit /b
build\x64\Debug\my_binary.exe || exit /b
popd


pushd 03a_library_dependency_explicit
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
msbuild build\library_dependency_explicit.sln /t:my_library /p:Configuration=Debug;Platform=x64 || exit /b
msbuild build\library_dependency_explicit.sln /t:my_binary /p:Configuration=Debug;Platform=x64 || exit /b
msbuild build\library_dependency_explicit.sln /t:my_library /p:Configuration=Debug;Platform=x86 || exit /b
msbuild build\library_dependency_explicit.sln /t:my_binary /p:Configuration=Debug;Platform=x86 || exit /b
msbuild build\library_dependency_explicit.sln /t:my_library /p:Configuration=Release;Platform=x64 || exit /b
msbuild build\library_dependency_explicit.sln /t:my_binary /p:Configuration=Release;Platform=x64 || exit /b
msbuild build\library_dependency_explicit.sln /t:my_library /p:Configuration=Release;Platform=x86 || exit /b
msbuild build\library_dependency_explicit.sln /t:my_binary /p:Configuration=Release;Platform=x86 || exit /b
rem Debug lib returns 1
build\x64\Debug\bin\my_binary.exe
if %errorlevel% neq 1 exit /b %errorlevel%
build\Win32\Debug\bin\my_binary.exe
if %errorlevel% neq 1 exit /b %errorlevel%
rem Release lib returns 2
build\x64\Release\bin\my_binary.exe
if %errorlevel% neq 2 exit /b %errorlevel%
build\Win32\Release\bin\my_binary.exe
if %errorlevel% neq 2 exit /b %errorlevel%
popd


pushd 04_preprocessor
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\preprocessor.exe
rem Debug build has the expected value for the define, so should return 0
if %errorlevel% neq 0 exit /b 1
%BUILD_RELEASE_COMMAND% build\workspace.sln || exit /b
build\x64\Release\preprocessor.exe
rem Release build is expected to return 1, since the define has a different value for that build
if %errorlevel% neq 1 exit /b 1
popd


pushd 05_compile_flags
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_RELEASE_COMMAND% build\workspace.sln
if %errorlevel% neq 1 exit /b %errorlevel%
REM building should cause an error because flag has been added to set warnings as errors
popd


pushd 06_post_build_action
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln
rem The build is expected to give an error, since the post build action doesn't succeed ...
if %errorlevel% equ 0 exit /b %errorlevel%
rem ... However, the executable has been built, so test it is there and works correctly
build\x64\Debug\post_build_action.exe || exit /b
popd


pushd 07_changed_config
rd /S /Q build
copy return_value1.inl return_value.inl
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\changed_config.exe
rem The first config builds the program so that it returns 1, so check for that specifically
if %errorlevel% neq 1 exit /b %errorlevel%
copy return_value2.inl return_value.inl
cconstruct.exe
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\changed_config.exe
rem The second config builds the program so that it returns 2, so check for that specifically
if %errorlevel% neq 2 exit /b %errorlevel%
rem Now set back the first config, but don't rebuild
copy return_value1.inl return_value.inl
cconstruct.exe --generate-projects
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\changed_config.exe
if %errorlevel% neq 2 exit /b %errorlevel%
popd


pushd 08_project_structure
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\project_structure.sln || exit /b
build\x64\Debug\my_binary.exe || exit /b
popd


pushd 09_warning_level
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
rem Without doing something about warnings, the following builds would fail.
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
%BUILD_RELEASE_COMMAND% build\workspace.sln || exit /b
popd


pushd 10_mixing_c_and_cpp
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\mixing_c_and_cpp.exe || exit /b
popd


pushd 11_nested_folders
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% src/config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\nested_folders.exe || exit /b
popd


pushd 12_config_folders
rd /S /Q build
REM this test requires the build folder be there
mkdir build 
cl.exe /EHsc /Fo%TEMP% /FC /Febuild/cconstruct.exe /nologo /TC project/config.cc || exit /b
pushd build
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% workspace.sln || exit /b
x64\Debug\config_folders.exe || exit /b
popd
REM also check if it works when calling it from a different folder
del build\config_folder.vcxproj.*
build\cconstruct.exe || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
popd


pushd 13_cpp_config
rd /S /Q build
%COMPILE_CONSTRUCT_CPP_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
popd


pushd 14_c_config
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
popd


pushd 15_other_file_types
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
popd


pushd 16_windowed_application
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\windowed_application.exe || exit /b
popd


pushd 17_link_flags
rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
if not exist build\x64\Debug\link_flags_named.pdb (
  exit 1
)
popd


pushd 18_custom_commands
rd /S /Q build
del src\test.txt
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generate-projects || exit /b
set TEST_TIME=%time%
echo %TEST_TIME%>src\test_source.txt
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
rem copy src\test_source.txt src\test.txt
rem Get output into variable
FOR /F "tokens=* USEBACKQ" %%F IN (`build\x64\Debug\custom_commands.exe`) DO (
SET CMD_OUTPUT=%%F
)
if NOT "%CMD_OUTPUT%" == "test %TEST_TIME%" exit 1
popd


pushd 21_errors
rd /S /Q build
rem Create CConstruct binary which works fine
copy error_none.inl error.inl
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
rem Cause the recreation of CConstruct binary to fail
copy error_compile.inl error.inl
cconstruct.exe
if %errorlevel% neq 1 exit /b %errorlevel%
rem Cause the recreation of CConstruct binary to succeed, but then crash during construction
copy error_construction.inl error.inl
cconstruct.exe
if %errorlevel% neq 2 exit /b %errorlevel%
rem No error at all, everything goes OK from here on out
copy error_none.inl error.inl
cconstruct.exe || exit /b
%BUILD_DEBUG_COMMAND% build\workspace.sln || exit /b
build\x64\Debug\errors.exe || exit /b
popd


echo All Tests Finished