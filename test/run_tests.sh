#!/bin/bash
set -e
set -x

COMPILE_CCONSTRUCT_COMMAND='clang -x c'
COMPILE_DEBUG_CCONSTRUCT_COMMAND='clang -x c -g'
COMPILE_CCONSTRUCT_CPP_COMMAND='clang++ -x c++ -std=c++11'




pushd 01_hello_world
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme hello_world
#-sdk "*targetSDK*" -configuration *buildConfig* CODE_SIGN_IDENTITY="*NameOfCertificateIdentity*" PROVISIONING_PROFILE="*ProvisioningProfileName" OTHER_CODE_SIGN_FLAGS="--keychain *keyChainName*"
./build/x64/Debug/hello_world
popd


pushd 02_include_folders
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme include_folders
./build/x64/Debug/include_folders
popd


pushd 03_library_dependency
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/xcode/library_dependency.xcworkspace -scheme my_binary
./build/xcode/x64/Debug/my_binary
popd


pushd 03a_library_dependency_explicit
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct  --generate-projects
xcodebuild -quiet -workspace build/xcode/library_dependency_explicit.xcworkspace -scheme my_library
xcodebuild -quiet -workspace build/xcode/library_dependency_explicit.xcworkspace -scheme my_binary
xcodebuild -quiet -workspace build/xcode/library_dependency_explicit.xcworkspace -scheme my_library -configuration Release
xcodebuild -quiet -workspace build/xcode/library_dependency_explicit.xcworkspace -scheme my_binary -configuration Release
set +e
# Debug lib returns 1
build/xcode/x64/Debug/bin/my_binary
[ $? -ne 1 ] && exit 1
# Release lib returns 2
build/xcode/x64/Release/bin/my_binary
[ $? -ne 2 ] && exit 2
set -e
popd


set +e
pushd 04_preprocessor
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme preprocessor -configuration Debug
./build/x64/Debug/preprocessor
if [ $? -ne 0 ]
then
  exit 1
fi
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme preprocessor -configuration Release
./build/x64/Release/preprocessor   
if [ $? -eq 0 ]
then
  exit 1
fi
popd
set -e


set +e
pushd 05_compile_flags
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme compile_flags
# building should cause an error because flag has been added to set warnings as errors
if [ $? -eq 0 ]
then
  exit 1
fi
popd
set -e


pushd 06_post_build_action
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
set +e
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme post_build_action
# building should fail due to the unknown post build command
if [ $? -eq 0 ]
then
  exit 1
fi
set -e
# but the binary should still have been built and be runnable
./build/x64/Debug/post_build_action
popd


set +e
pushd 07_changed_config
rm -rf build
cp return_value1.inl return_value.inl
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme changed_config
./build/x64/Debug/changed_config
# The first config builds the program so that it returns 1, so check for that specifically
if [ $? -ne 1 ]
then
  exit 1
fi
cp return_value2.inl return_value.inl
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme changed_config
./build/x64/Debug/changed_config
# The second config builds the program so that it returns 2, so check for that specifically
if [ $? -ne 2 ]
then
  exit 1
fi
popd
set -e


pushd 08_project_structure
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/project_structure.xcworkspace -scheme my_binary
./build/x64/Debug/my_binary
popd


pushd 09_warning_level
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
# Without doing something about warnings, the following builds would fail.
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme warning_level -configuration Debug
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme warning_level -configuration Release
popd


pushd 10_mixing_c_and_cpp
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme mixing_c_and_cpp
./build/x64/Debug/mixing_c_and_cpp
popd


pushd 11_nested_folders
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/src/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme nested_folders
./build/x64/Debug/nested_folders
popd


pushd 12_config_folders
rm -rf build
# This test requires the build folder to exist
mkdir build
$COMPILE_CCONSTRUCT_COMMAND $PWD/project/config.cc -o build/cconstruct
pushd build
./cconstruct
xcodebuild -quiet -workspace workspace.xcworkspace -scheme config_folders
./x64/Debug/config_folders
popd
# Also check if it works when calling it from the parent folder
rm -rf build/*.xc*
./build/cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme config_folders
popd


pushd 13_cpp_config
rm -rf build
$COMPILE_CCONSTRUCT_CPP_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme cpp_config
popd


pushd 14_c_config
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme c_config
popd


pushd 15_other_file_types
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme other_file_types
popd


pushd 18_custom_commands
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
TEST_TIME=`date`
echo $TEST_TIME > src/test_source.txt
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme custom_commands
CMD_OUTPUT=$(./build/x64/Debug/custom_commands)
[ "$CMD_OUTPUT" != "test $TEST_TIME" ] && exit 1
popd


pushd 19_macos_framework
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct --generate-projects
xcodebuild -quiet -workspace build/xcode/macos_framework.xcworkspace -scheme my_binary
./build/xcode/x64/Debug/my_binary
popd


pushd 21_errors
rm -rf build
set +e
# Create CConstruct binary which works fine
cp error_none.inl error.inl
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
# Cause the recreation of CConstruct binary to fail
cp error_compile.inl error.inl
./cconstruct
[ $? -ne 1 ] && exit 1
# Cause the recreation of CConstruct binary to succeed, but then crash during construction
cp error_construction.inl error.inl
./cconstruct
[ $? -ne 2 ] && exit 1
# No error at all, everything goes OK from here on out
cp error_none.inl error.inl
./cconstruct
set -e
popd






pushd ../tools
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND $PWD/config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme cconstruct_release
mkdir -p ../build
./build/x64/Debug/cconstruct_release ../source/cconstruct.h ../build/cconstruct.h
popd
