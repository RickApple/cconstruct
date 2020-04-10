#pragma warning(disable : 4996)
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { CCProjectTypeConsoleApplication = 0, CCProjectTypeStaticLibrary } EProjectType;
typedef enum { EPlatformTypeX86 = 0, EPlatformTypeX64, EPlatformTypeARM } EPlatformType;

// Project functions
void* createProject(const char* in_project_name, EProjectType in_project_type);
void addFileToProject(void* in_project, const char* in_file_name, const char* in_group_name);
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
} cc_flags;

// Opaque handles at this point
typedef struct TPlatform* CCPlatformHandle;
typedef struct TConfiguration* CCConfigurationHandle;

typedef struct CConstruct {
  CCConfigurationHandle (*createConfiguration)(const char* in_label);
  CCPlatformHandle (*createPlatform)(EPlatformType in_type);
  void* (*createProject)(const char* in_project_name, EProjectType in_project_type);

  const struct {
    void (*reset)(cc_flags* out_flags);
    void (*addPreprocessorDefine)(cc_flags* in_flags, const char* in_define);
  } state;

  const struct {
    void (*addFiles)(void* in_project, const char* in_group_name, unsigned num_files,
                     const char* in_file_names[]);
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

#include "tools.inl"
#include "types.inl"
#include "vs2019_project.inl"
#include "vs2019_workspace.inl"
#include "xcode11_project.inl"

const CConstruct cc = {cc_configuration_create,
                       cc_platform_create,
                       cc_project_create_,
                       {cc_state_reset, cc_state_addPreprocessorDefine},
                       {addFilesToProject, addInputProject, cc_project_setFlags_,
                        cc_project_setFlagsLimited_, addPostBuildAction},
                       {setWorkspaceLabel, setOutputFolder, addConfiguration, addPlatform}};

// For ease of use set a default CConstruct generator for each platform
#if defined(_MSC_VER)
void (*cc_default_generator)(const char* workspace_folder) = vs2019_generateInFolder;
#else
void (*cc_default_generator)(const char* workspace_folder) = xcode_generateInFolder;
#endif