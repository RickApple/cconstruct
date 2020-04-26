# cconstruct

A tool to make IDE project files, using C as a 'description' language.

## Building your binary

### Visual Studio

When building using the Visual Studio command-line compiler cl.exe, you have to add the /FC switch so that CConstruct can reason about your file paths.
Ref https://developercommunity.visualstudio.com/content/problem/364466/-file-macro-expands-to-absolute-path-when-used-fro.html
Ref https://docs.microsoft.com/en-us/cpp/build/reference/fc-full-path-of-source-code-file-in-diagnostics?view=vs-2019

## Warnings

Warnings are set to a high level by default, and treated as errors by default. Some choices of warnings are set such that that fixing them reduces the amount of code (eg, unused parameters are ok, unused variables need to be solved by removing the variables).

Currently there is no way to prevent CConstruct from setting the warnings it wants to, but you could disable those warnings again with regular compiler flags, as they are applied after the chosen warning level is applied.

## TODO

- Support recompile as C++ file instead of C file
- XCode remove hardcoded debug and release configurations
- Rename platform to architecture. MacOS is a platform, as is Windows, iOS, tvOS, Linux etc. x86_32, x86_64, arm64 are architectures
- Allow adding any XCode setting you want as UI option, not as other flags
- Allow adding files that shouldn't be compiled (.h,.inl)
- Should the language dialect be exposed?
- Allow for a way to add files in a specific folder so user doesn't have to manually give complete path to every single file
- Create test for hierarchical folder structure
- Make tests for cconstruct build out of tree
- allows handle relative locations correctly
- simplify output folders taking into account that VS gives warning if multiple projects have same output/intermediate folder
- support pre build actions (post already works)
- have option to create project to debug cconstruct binary?
- Custom configuration Testing for example. Take into account that XCode generator adds some Debug/Release differences hardcoded.
- Allow projects to have different configurations/platforms, instead of automatically having all created ones.

## Done

Run Jenkins service under standard user account
https://issues.jenkins-ci.org/browse/JENKINS-49011

devenv.exe didn't do anything (no output), but devenv.com reported outdated license. Solved by changing to standard account

Requirement: cconstruct binary compiled from project root

- config file in root folder, or other sub-folder
  **FILE** if requirement is satisfied, this is a valid path to find the root of project.

? cconstruct binary can be located in root, next to config, or in build folder
? expected to be run from root folder, or build folder
