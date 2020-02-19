
pushd preprocessor

rd /S /Q build

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild build\workspace.sln
if %errorlevel% neq 0 exit /b %errorlevel%

build\Debug\preprocessor.exe
if %errorlevel% neq 0 exit /b %errorlevel%

popd


pushd hello_world

rd /S /Q build

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild build\workspace.sln
if %errorlevel% neq 0 exit /b %errorlevel%

build\Debug\hello_world.exe
if %errorlevel% neq 0 exit /b %errorlevel%

popd


pushd library_dependency

rd /S /Q build

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
cl.exe -EHsc -Fe: cconstruct.exe config.cc
if %errorlevel% neq 0 exit /b %errorlevel%

cconstruct.exe
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild build\msvc\workspace.sln
if %errorlevel% neq 0 exit /b %errorlevel%

build\msvc\Debug\my_binary.exe
if %errorlevel% neq 0 exit /b %errorlevel%

popd


