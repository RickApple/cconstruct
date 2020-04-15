#pragma warning(disable : 4996)
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Opaque handles at this point
typedef struct cc_group_impl_t* cc_group_t;
typedef struct TPlatform* CCPlatformHandle;
typedef struct TConfiguration* CCConfigurationHandle;

cc_group_t cc_createGroup(const char* in_group_name, const cc_group_t in_parent);

// Project functions
void* createProject(const char* in_project_name, EProjectType in_project_type,
                    const cc_group_t in_parent);
void addInputProject(void* target_project, const void* on_project);
void addPostBuildAction(void* target_project, const char* command);

// Workspace functions
void setOutputFolder(const char* of);
void create();
void addProject(const void* in_project);

typedef struct cc_flags {
  const char** defines;
  const char** include_folders;
  const char** compile_options;
  const char** link_options;
  EStateWarningLevel
      warningLevel;  // By default warnings are turned up to the highest level below all.
  bool disableWarningsAsErrors;  // By default warnings are treated as errors.
} cc_flags;

typedef struct CConstruct {
  /* If you want CConstruct to automatically pick up changes in your config files, then you need
   * to call this function at the top of your main function, passing in the filename of your config
   * file (you can use the __FILE__ macro).
   * It will then rebuild the binary whenever you run it, replace the current binary, and then
   * generate the projects. Since it's effectively only compiling a single file this is fast enough
   * to do on each run.
   */
  void (*autoRecompileFromConfig)(const char* cconstruct_config_file_path, int argc,
                                  const char* argv[]);

  CCConfigurationHandle (*createConfiguration)(const char* in_label);
  CCPlatformHandle (*createPlatform)(EPlatformType in_type);
  void* (*createProject)(const char* in_project_name, EProjectType in_project_type,
                         const cc_group_t in_parent_group);

  /* Create a group/folder in the project, at any level.
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
    void (*addFiles)(void* in_project, unsigned num_files, const char* in_file_names[],
                     const cc_group_t in_parent_group);
    void (*addInputProject)(void* target_project, const void* on_project);
    void (*setFlags)(void* in_project, const cc_flags* in_flags);
    void (*setFlagsLimited)(void* in_out_project, const cc_flags* in_flags,
                            CCPlatformHandle in_platform, CCConfigurationHandle in_configuration);
    void (*addPostBuildAction)(void* in_out_project, const char* in_action_command);
  } project;

  const struct {
    void (*setLabel)(const char* label);
    void (*setOutputFolder)(const char* in_output_folder);
    void (*addConfiguration)(const CCConfigurationHandle in_configuration);
    void (*addPlatform)(const CCPlatformHandle in_platform);

  } workspace;

} CConstruct;

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

const CConstruct cc = {cc_autoRecompileFromConfig,
                       cc_configuration_create,
                       cc_platform_create,
                       cc_project_create_,
                       cc_createGroup,
                       {cc_state_reset, cc_state_addPreprocessorDefine, cc_state_setWarningLevel,
                        cc_state_disableWarningsAsErrors},
                       {addFilesToProject, addInputProject, cc_project_setFlags_,
                        cc_project_setFlagsLimited_, addPostBuildAction},
                       {setWorkspaceLabel, setOutputFolder, addConfiguration, addPlatform}};

// For ease of use set a default CConstruct generator for each platform
#if defined(_MSC_VER)
void (*cc_default_generator)(const char* workspace_folder) = vs2019_generateInFolder;
#else
void (*cc_default_generator)(const char* workspace_folder) = xcode_generateInFolder;
#endif