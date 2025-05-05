echo on
@set COMPILE_DEBUG_CONSTRUCT_COMMAND=cl.exe /ZI /W4 /WX /DEBUG /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TC
@set COMPILE_CONSTRUCT_COMMAND=cl.exe /W4 /WX /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TC
@set COMPILE_CONSTRUCT_CPP_COMMAND=cl.exe /W4 /WX /FC /Fo%TEMP% /Fecconstruct.exe /nologo /TP
@set BUILD_COMMAND=ninja -C build

@rem Find location of Visual Studio
for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
    set VSPATH=%%i
)

@rem Now set a single environment for the rest of the tests
SETLOCAL
@rem -arch=x86 for 32-bit
@rem -arch=amd64 for 64-bit
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat"



pushd 01_hello_world
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe  --generator=ninja --generate-projects || exit /b
%BUILD_COMMAND% || exit /b
build\hello_world.exe || exit /b
popd


pushd 02_include_folders
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generator=ninja --generate-projects || exit /b
%BUILD_COMMAND% || exit /b
build\include_folders.exe || exit /b
popd


pushd 03_library_dependency
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generator=ninja --generate-projects || exit /b
%BUILD_COMMAND% || exit /b
build\my_binary.exe || exit /b
popd


pushd 04_preprocessor
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generator=ninja --generate-projects || exit /b
%BUILD_COMMAND% || exit /b
build\preprocessor.exe
rem Debug build has the expected value for the define, so should return 0
if %errorlevel% neq 0 exit /b 1
cconstruct.exe --generator=ninja --generate-projects --config=Release || exit /b
%BUILD_COMMAND% --verbose || exit /b
build\preprocessor.exe
rem Release build is expected to return 1, since the define has a different value for that build
if %errorlevel% neq 1 exit /b 1
popd

pushd 05_compile_flags
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generator=ninja --generate-projects || exit /b
%BUILD_COMMAND%
if %errorlevel% neq 1 exit /b %errorlevel%
REM building should cause an error because flag has been added to set warnings as errors
popd

pushd 06_post_build_action
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generator=ninja --generate-projects || exit /b
%BUILD_COMMAND%
rem The build is expected to give an error, since the post build action doesn't succeed ...
if %errorlevel% equ 0 exit /b %errorlevel%
rem ... However, the executable has been built, so test it is there and works correctly
build\post_build_action.exe || exit /b
popd


pushd 07_changed_config
if exist build rd /S /Q build
mkdir build
copy return_value1.inl return_value.inl
echo. >> return_value.inl
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
cconstruct.exe --generator=ninja --generate-projects
pushd build
ninja || exit /b
changed_config.exe
rem The first config builds the program so that it returns 1, so check for that specifically
if %errorlevel% neq 1 exit /b %errorlevel%
copy /Y ..\return_value2.inl ..\return_value.inl
echo. >> ..\return_value.inl
ninja || exit /b
changed_config.exe
rem The second config builds the program so that it returns 2, so check for that specifically
if %errorlevel% neq 2 exit /b %errorlevel%
rem Now set back the first config, Ninja should automatically rebuild cconstruct
copy /Y ..\return_value1.inl ..\return_value.inl
echo. >> ..\return_value.inl
ninja || exit /b
changed_config.exe
if %errorlevel% neq 1 exit /b %errorlevel%
popd
popd


pushd 09_warning_level
if exist build rd /S /Q build
%COMPILE_CONSTRUCT_COMMAND% config.cc || exit /b
rem Without doing something about warnings, the following builds would fail.
cconstruct.exe --generator=ninja --generate-projects || exit /b
%BUILD_COMMAND% || exit /b
cconstruct.exe --generator=ninja --generate-projects --config=Release|| exit /b
%BUILD_COMMAND% || exit /b
popd