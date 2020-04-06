#!/bin/bash
set -e
set -x

pushd hello_world
rm -rf build
clang++ -std=c++11 config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme hello_world
#-sdk "*targetSDK*" -configuration *buildConfig* CODE_SIGN_IDENTITY="*NameOfCertificateIdentity*" PROVISIONING_PROFILE="*ProvisioningProfileName" OTHER_CODE_SIGN_FLAGS="--keychain *keyChainName*"
./build/test/hello_world
popd

pushd library_dependency
rm -rf build
clang++ -std=c++11 config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/xcode/library_dependency.xcworkspace -scheme my_binary
./build/xcode/bin/x64/my_binary
popd

pushd preprocessor
rm -rf build
clang++ -std=c++11 config.cc -o cconstruct
./cconstruct
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme preprocessor -configuration Debug
./build/test/preprocessor
xcodebuild -quiet -workspace build/workspace.xcworkspace -scheme preprocessor -configuration Release
./build/test/preprocessor
popd


