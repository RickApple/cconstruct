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

| CConstruct | MSVS | Clang |
| --- | --- | --- |
| `EStateWarningLevelHigh` | /W4 | -Wall -Wextra |
| `EStateWarningLevelMedium` | /W3 | -Wall |
| `EStateWarningLevelLow` | /W2 | -Wall |
| `EStateWarningLevelAll` | /Wall | -Weverything |
| `EStateWarningLevelNone` | /W0 | -w |


## TODO
- Using VS2022 gives errors most of the time when building the replacement cconstruct_internal.exe. Not sure why.
- For custom commands it would make things more readable if ${input_file} and ${output_file} etc were supported [MacOS]
- Remove some of the conversions between filename, and clarify what path is what path. It's a bit of a mess, especially for VS2019.
- Compile for Itanium, 32bit and 64bit to check for errors.
- reference post build actions relative to where? check and document
- have option to create project to debug cconstruct binary?
- add description to cconstruct.h on how to build the initial version of the binary on MacOS
- Need to add support for setting Info.plist for MacOS and iOS
- Make selection of compilable files more flexible
- Architecture and platform should be set at project level instead of top-level
- Custom commands on files (eg to compile GLSL)
  [ref https://www.gamedev.net/blogs/entry/2266894-fully-featured-custom-build-targets-in-visual-c/]
- Allow config to set linker flags
- Allow config to set environment and arguments for debug run?
- XCode remove hardcoded debug and release configurations
- Allow adding any XCode setting you want as UI option, not as other flags
- Allow adding files that shouldn't be compiled (.h,.inl)
- Should the language dialect be exposed?
- Create test for hierarchical folder structure
- Make tests for cconstruct build out of tree (so ../ from location of main cconstruct file)
- allows handle relative locations correctly
- simplify output folders taking into account that VS gives warning if multiple projects have same output/intermediate folder
- support pre build actions (post already works)
- Custom configuration Testing for example. Take into account that XCode generator adds some Debug/Release differences hardcoded.
- Better name for CCProjectTypeWindowedApplication as that is also the type used for iOS applications
  Maybe just CCProjectTypeApplication
- Use existing functions for path manipulation:
  https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/makepath-wmakepath?view=vs-2019
  https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fullpath-wfullpath?view=vs-2019
- Should users handle difference between .a and .lib static library extension when adding dependency on external lib?

## Done

Run Jenkins service under standard user account
https://issues.jenkins-ci.org/browse/JENKINS-49011

devenv.exe didn't do anything (no output), but devenv.com reported outdated license. Solved by changing to standard account

Requirement: cconstruct binary compiled from project root

- config file in root folder, or other sub-folder
  **FILE** if requirement is satisfied, this is a valid path to find the root of project.

? cconstruct binary can be located in root, next to config, or in build folder
? expected to be run from root folder, or build folder
