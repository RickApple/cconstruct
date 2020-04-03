#pragma warning(disable : 4996)
#include <stdbool.h>
#include <algorithm>
#include <set>
#include <string>
#include <vector>

enum EProjectType { CCProjectTypeConsoleApplication = 0, CCProjectTypeStaticLibrary };
enum EPlatformType { EPlatformTypeX86 = 0, EPlatformTypeX64, EPlatformTypeARM };

// Project functions
void* createProject(const char* in_project_name, EProjectType in_project_type);
void addFileToProject(void* in_project, const char* in_file_name, const char* in_group_name);
void addInputProject(const void* target_project, const void* on_project);

// Workspace functions
void setOutputFolder(const char* of);
void create();
void addProject(const void* in_project);

typedef struct cc_flags {
  std::vector<std::string> defines;
  std::vector<std::string> include_folders;
  std::vector<std::string> compile_options;
  std::vector<std::string> link_options;
} cc_flags;

// Opaque handles at this point
typedef struct TPlatform* CCPlatformHandle;
typedef struct TConfiguration* CCConfigurationHandle;

typedef struct CConstruct {
  const struct { CCPlatformHandle (*create)(EPlatformType in_type); } platform;
  const struct { CCConfigurationHandle (*create)(const char* in_label); } configuration;

  const struct {
    void (*reset)(cc_flags* out_flags);
    void (*addPreprocessorDefine)(cc_flags* in_flags, const char* in_define);
  } state;

  const struct {
    void* (*create)(const char* in_project_name, EProjectType in_project_type);
    void (*addFiles)(void* in_project, const char* in_group_name, unsigned num_files, const char* in_file_names[]);
    void (*addInputProject)(const void* target_project, const void* on_project);
    void (*setFlags)(const void* in_project, const cc_flags* in_flags);
    void (*setFlagsLimited)(const void* in_out_project, const cc_flags* in_flags,
                            CCPlatformHandle in_platform, CCConfigurationHandle in_configuration);
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

CConstruct cc = {{cc_platform_create},
                 {cc_configuration_create},
                 {cc_state_reset, cc_state_addPreprocessorDefine},
                 {cc_project_create_, addFilesToProject, addInputProject, cc_project_setFlags_,
                  cc_project_setFlagsLimited_},
                 {
                     setWorkspaceLabel,
                     setOutputFolder,
                     addConfiguration,
                     addPlatform,
                 }};

// For ease of use set a default CConstruct instance for each platform
#if defined(_MSC_VER)
void (*cc_default_generator)(const char* workspace_folder) = vs2019_generateInFolder;
#else
void (*cc_default_generator)(const char* workspace_folder) = xcode_generateInFolder;
#endif