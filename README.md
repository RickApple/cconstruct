# cconstruct

A tool to make IDE project files, using C as a description language.

## TODO
- XCode remove hardcoded debug and release configurations
- Rename platform to architexture. MacOS is a platform, as is Windows, iOS, tvOS, Linux etc. x86_32, x86_64, arm64 are architectures
- Allow adding any XCode setting you want as UI option, not as other flags
- Allow adding files that shouldn't be compiled (.h,.inl)
- Should we expose the language dialect
- Allow for a way to add files in a specific folder so user doesn't have to manually give complete path to every single file
- Create test for hierarchical folder structure
- Make tests for cconstruct build out of tree
- allows handle relative locations correctly
- simplify output folders taking into account that VS gives warning if multiple projects have same output/intermediate folder
- support pre build actions (post already works)
- have option to create project to debug cconstruct binary?
- Custom configuration Testing for example. Take into account that XCode generator adds some Debug/Release differences hardcoded.

## Done

Run Jenkins service under standard user account
https://issues.jenkins-ci.org/browse/JENKINS-49011

devenv.exe didn't do anything (no output), but devenv.com reported outdated license. Solved by changing to standard account
