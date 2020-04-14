# cconstruct

A tool to make IDE project files, using C as a description language.

## TODO

- Allow adding files that shouldn't be compiled (.h,.inl)
- Visual Studio warns that both /WX- (warnings not as errors) and /WX are defined. Need to properly integrate the setting.
- In Visual Studio you can group projects into folders. Can that be done in XCode?
- Allow setting generic flags like warning level, warnings as errors in non-general way so can properly set the flag in VS2019
- Allow for a way to add files in a specific folder so user doesn't have to manually give complete path to every single file
- Create test for hierarchical folder structure
- Make tests for cconstruct build out of tree
- allows handle relative locations correctly
- simplify output folders taking into account that VS gives warning if multiple projects have same output/intermediate folder
- support pre build actions (post already works)
- have option to create project to debug cconstruct binary?

## Done

Run Jenkins service under standard user account
https://issues.jenkins-ci.org/browse/JENKINS-49011

devenv.exe didn't do anything (no output), but devenv.com reported outdated license. Solved by changing to standard account
