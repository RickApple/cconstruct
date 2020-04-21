#ifndef CC_CONSTRUCT_H
#define CC_CONSTRUCT_H

#pragma warning(disable : 4996)

#include <stdbool.h>

typedef enum {
  CCProjectTypeConsoleApplication = 0,
  CCProjectTypeStaticLibrary,
} EProjectType;
typedef enum {
  EPlatformTypeX86 = 0,
  EPlatformTypeX64,
  EPlatformTypeARM,
} EPlatformType;
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
typedef struct cc_group_impl_t* cc_group_t;
typedef struct cc_platform_impl_t* cc_platform_t;
typedef struct cc_configuration_impl_t* cc_configuration_t;

typedef struct cc_flags {
  const char** defines;
  const char** include_folders;
  const char** compile_options;
  const char** link_options;
  // By default warnings are turned up to the highest level below _all_.
  EStateWarningLevel warningLevel;
  bool disableWarningsAsErrors;  // By default warnings are treated as errors.
} cc_flags;

typedef struct cconstruct_t {
  cc_configuration_t (*createConfiguration)(const char* in_label);
  cc_platform_t (*createPlatform)(EPlatformType in_type);
  cc_project_t (*createProject)(const char* in_project_name, EProjectType in_project_type,
                                const cc_group_t in_parent_group);

  /* Create a group/folder in the workspace, at any level (files and projects can go into groups).
   *
   * @param in_parent may be NULL, the parent will then be whatever is applicable at that level
   * (workspace/project)
   */
  cc_group_t (*createGroup)(const char* in_group_name, const cc_group_t in_parent_group);

  const struct {
    void (*reset)(cc_flags* out_flags);
    void (*addPreprocessorDefine)(cc_flags* in_flags, const char* in_define);
    void (*setWarningLevel)(cc_flags* in_flags, EStateWarningLevel in_level);
    void (*disableWarningsAsErrors)(cc_flags* in_flags);
  } state;

  const struct {
    /* Add files to a project. *.c/*.cpp files are automatically added to be compiled, everything
     * else is treated as a header file.
     */
    void (*addFiles)(cc_project_t in_project, unsigned num_files, const char* in_file_names[],
                     const cc_group_t in_parent_group);
    void (*addFilesFromFolder)(cc_project_t in_project, const char* folder, unsigned num_files,
                               const char* in_file_names[], const cc_group_t in_parent_group);

    /* Add dependency between projects. This will only link the dependency and set the build order
     * in the IDE. It will *NOT* automatically add include-folders.
     */
    void (*addInputProject)(cc_project_t target_project, const cc_project_t on_project);

    /* Set flags on a project.
     *
     * @param in_platform may be NULL if flags are for all platforms
     * @param in_configuration may be NULL if flags are for all configurations
     */
    void (*setFlags)(cc_project_t in_out_project, const cc_flags* in_flags,
                     cc_platform_t in_platform, cc_configuration_t in_configuration);

    /* Add a command line instruction to execute after the build has finished successfully.
     */
    void (*addPostBuildAction)(cc_project_t in_out_project, const char* in_action_command);
  } project;

  const struct {
    /* Label of the workspace, used for filename of the workspace.
     */
    void (*setLabel)(const char* label);

    /* Output folder for build results.
     */
    void (*setOutputFolder)(const char* in_output_folder);

    /* Add a configuration
     */
    void (*addConfiguration)(const cc_configuration_t in_configuration);
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tools
#include "tools.inl"
#include "types.inl"

// Generators
#include "vs2019_project.inl"
#include "vs2019_workspace.inl"
#include "xcode11_project.inl"

// Automatic updating
#include "process.inl"
#include "restart_api.inl"

// For automatic updates to the config
#include "builder.inl"

// For ease of use set a default CConstruct generator for each platform
#if defined(_MSC_VER)
void (*cc_default_generator)(const char* workspace_folder) = vs2019_generateInFolder;
#else
void (*cc_default_generator)(const char* workspace_folder) = xcode_generateInFolder;
#endif

#endif  // CC_CONSTRUCT_H