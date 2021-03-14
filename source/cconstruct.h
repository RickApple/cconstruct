/*
 * Windows with Visual Studio
 * ==========================
 * 1a. Open a 'Developer Command Prompt' or
 * 1b. In an existing prompt call 'C:\Program Files (x86)\Microsoft Visual
 * Studio\2019\Community\Common7\\Tools\\VsDevCmd.bat' or similar
 * 2. Compile cconstruct exectuble with the following command
 *      'cl.exe /FC /Fo%TEMP% /Fecconstruct.exe your_config.cc'
 * 3. Run the resulting cconstruct.exe, which will regenerate itself from the config file
 *    and then construct the project files you specified in the config.
 *
 * After this you don't need the developer prompt anymore, the cconstruct.exe binary
 * stores the proper settings.
 *
 * MacOS with Clang
 * ==========================
 * 1. Open a Terminal
 * 2. Compile cconstruct exectuble with the following command
 *      'clang -x c your_config.cc -o cconstruct'
 * 3. Run the resulting cconstruct, which will regenerate itself from the config file
 *    and then construct the project files you specified in the config.
 */
#ifndef CC_CONSTRUCT_H
#define CC_CONSTRUCT_H

#pragma warning(disable : 4996)

#include <stdbool.h>

enum ErrorCodes {
  ERR_NO_ERROR      = 0,
  ERR_COMPILING     = 1,  // An error compiling the new CConstruct binary
  ERR_CONSTRUCTION  = 2,  // An error running the CConstruct binary to construct projects
  ERR_CONFIGURATION = 3,  // An error in the configuration. Check error output for more details
};

typedef enum {
  CCProjectTypeConsoleApplication = 0,
  CCProjectTypeWindowedApplication,
  CCProjectTypeStaticLibrary,
} EProjectType;
typedef enum {
  EArchitectureX86 = 0,
  EArchitectureX64,
  EArchitectureARM,
} EArchitecture;
typedef enum {
  EPlatformDesktop = 0, /* MacOS, Windows */
  EPlatformPhone,       /* iOS */
} EPlatform;
typedef enum {
  EStateWarningLevelDefault = 0,
  EStateWarningLevelHigh    = EStateWarningLevelDefault,
  EStateWarningLevelMedium,
  EStateWarningLevelLow,
  EStateWarningLevelAll,   // This really enables everything, not recommended for production use
  EStateWarningLevelNone,  // This really disables everything, not recommended for production use
} EStateWarningLevel;

// Opaque handles in public interface
typedef struct cc_project_impl_t* cc_project_t;
typedef void* cc_group_t;
typedef struct cc_architecture_impl_t* cc_architecture_t;
typedef struct cc_architecture_impl_t* cc_platform_t;
typedef struct cc_configuration_impl_t* cc_configuration_t;
typedef struct cc_state_impl_t* cc_state_t;

typedef struct cconstruct_t {
  cc_configuration_t (*createConfiguration)(const char* in_label);
  cc_architecture_t (*createArchitecture)(EArchitecture in_type);
  cc_platform_t (*createPlatform)(EPlatform in_type);
  cc_project_t (*createProject)(const char* in_project_name, EProjectType in_project_type,
                                const cc_group_t in_parent_group);

  /* Create a group/folder in the workspace, at any level (files and projects can go into groups).
   *
   * @param in_parent may be NULL, the parent will then be whatever is applicable at that level
   * (workspace/project)
   */
  cc_group_t (*createGroup)(const char* in_group_name, const cc_group_t in_parent_group);

  cc_state_t (*createState)();

  const struct {
    void (*reset)(cc_state_t in_out_state);
    void (*addIncludeFolder)(cc_state_t in_out_state, const char* in_include_folder);
    void (*addPreprocessorDefine)(cc_state_t in_out_state, const char* in_define);
    void (*addCompilerFlag)(cc_state_t in_out_state, const char* in_compiler_flag);
    void (*addLinkerFlag)(cc_state_t in_out_state, const char* in_linker_flag);

    /* Add dependency on external library or framework.
     */
    void (*linkExternalLibrary)(cc_state_t in_out_state, const char* in_external_library_path);

    void (*setWarningLevel)(cc_state_t in_out_state, EStateWarningLevel in_level);
    void (*disableWarningsAsErrors)(cc_state_t in_out_state);
  } state;

  const struct {
    /* Add files to a project. .c/.cpp files are automatically added to be compiled, everything
     * else is treated as a header file.
     */
    void (*addFiles)(cc_project_t in_project, unsigned num_files, const char* in_file_names[],
                     const cc_group_t in_parent_group);
    void (*addFilesFromFolder)(cc_project_t in_project, const char* folder, unsigned num_files,
                               const char* in_file_names[], const cc_group_t in_parent_group);

    void (*addFileWithCustomCommand)(cc_project_t in_project, const char* in_file_names,
                                     const cc_group_t in_parent_group, const char* command,
                                     const char* in_output_file_path);

    /* Add dependency between projects. This will only link the dependency and set the build order
     * in the IDE. It will *NOT* automatically add include-folders.
     */
    void (*addInputProject)(cc_project_t target_project, const cc_project_t on_project);

    /* Set state on a project.
     *
     * @param in_platform may be NULL if state is for all architectures
     * @param in_configuration may be NULL if state is for all configurations
     */
    void (*setFlags)(cc_project_t in_out_project, const cc_state_t in_state,
                     cc_architecture_t in_architecture, cc_configuration_t in_configuration);

    /* Add a command line instruction to execute after the build has finished successfully.
     */
    void (*addPostBuildAction)(cc_project_t in_out_project, const char* in_action_command);

    /* Output folder for project build results (default:"${platform}/${configuration}")
     */
    void (*setOutputFolder)(cc_project_t in_out_project, const char* in_output_folder);
  } project;

  const struct {
    /* Label of the workspace, used for filename of the workspace.
     */
    void (*setLabel)(const char* label);

    /* Add a configuration
     */
    void (*addConfiguration)(const cc_configuration_t in_configuration);
    void (*addArchitecture)(const cc_architecture_t in_architecture);
    void (*addPlatform)(const cc_platform_t in_platform);

  } workspace;

} cconstruct_t;

extern void (*cc_default_generator)(const char* workspace_folder);

/* Call this once at the start of your main config file. You can usually call it with __FILE__, and
 * simply forward argc and argv from the main function.
 */
cconstruct_t cc_init(const char* in_absolute_config_file_path, int argc, const char* const* argv);

/***********************************************************************************************************************
 *                                             Implementation starts here
 ***********************************************************************************************************************/

#include "cconstruct_main.inl"

// For ease of use set a default CConstruct generator for each OS
#if defined(_MSC_VER)
void (*cc_default_generator)(const char* workspace_folder) = vs2019_generateInFolder;
#else
void (*cc_default_generator)(const char* workspace_folder) = xcode_generateInFolder;
#endif

#endif  // CC_CONSTRUCT_H
