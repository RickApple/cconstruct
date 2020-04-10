#!/bin/bash
set -e
set -x

# Compile a single instance with C++ to check compatiblity of cconstruct
pushd 01_hello_world
clang -x cpp config.cc -o cconstruct
popd


COMPILE_CCONSTRUCT_COMMAND='clang -x c'


pushd 01_hello_world
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme hello_world
#-sdk "*targetSDK*" -configuration *buildConfig* CODE_SIGN_IDENTITY="*NameOfCertificateIdentity*" PROVISIONING_PROFILE="*ProvisioningProfileName" OTHER_CODE_SIGN_FLAGS="--keychain *keyChainName*"
./build/x64/Debug/hello_world
popd


pushd 02_include_folders
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme include_folders
./build/x64/Debug/include_folders
popd


pushd 03_library_dependency
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/xcode/library_dependency.xcworkspace -scheme my_binary
./build/xcode/x64/Debug/my_binary
popd


pushd 04_preprocessor
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme preprocessor -configuration Debug
./build/x64/Debug/preprocessor
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme preprocessor -configuration Release
./build/x64/Release/preprocessor   
popd


set +e
pushd 05_compile_flags
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme compile_flags
# building should cause an error because flag has been added to set warnings as errors
if [ $? -eq 0 ]
then
  exit 1
fi
popd

pushd 06_post_build_action
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme post_build_action
# building should fail due to the unknown post build command
if [ $? -eq 0 ]
then
  exit 1
fi
# but the binary should still have been built and be runnable
./build/x64/Debug/post_build_action
popd

pushd ../tools
rm -rf build
$COMPILE_CCONSTRUCT_COMMAND config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme cconstruct_release
mkdir -p ../build
./build/x64/Debug/cconstruct_release ../source/cconstruct.h ../build/cconstruct.h
popd
