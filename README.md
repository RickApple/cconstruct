# cconstruct

A tool to make IDE project files, using C as a description language.

## TODO

- VS2019 project doesn't seem to allow the setting of debug arguments in the IDE
- Allow setting generic flags like warning level, warnings as errors in non-general way so can properly set the flag in VS2019
- Allow substitution in output paths
- Allow for a way to add files in a specific folder so user doesn't have to manually give complete path to every single file
- Create test for hierarchical folder structure
- Make tests for cconstruct build out of tree
- set default output folder to build/${platform}
- allows handle relative locations correctly





## Done

Run Jenkins service under standard user account
https://issues.jenkins-ci.org/browse/JENKINS-49011

devenv.exe didn't do anything (no output), but devenv.com reported outdated license. Solved by changing to standard account