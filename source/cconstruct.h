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
void addConfiguration(const char* in_configuration_name);

typedef struct cc_flags {
  std::vector<std::string> defines;
} cc_flags;

typedef void* CCPlatformHandle;

typedef struct CConstruct {
  const struct {
    CCPlatformHandle (*create)(const char* in_label, EPlatformType in_type);
  } platform;

  const struct {
    void (*reset)(cc_flags* out_flags);
    void (*addPreprocessorDefine)(cc_flags* in_flags, const char* in_define);
  } state;

  const struct {
    void* (*create)(const char* in_project_name, EProjectType in_project_type);
    void (*addFiles)(void* in_project, const char* in_group_name, const char* in_file_names[]);
    void (*addInputProject)(const void* target_project, const void* on_project);
    void (*setFlags)(const void* in_project, const cc_flags* in_flags);
  } project;

  const struct {
    void (*setLabel)(const char* label);
    void (*setOutputFolder)(const char* in_output_folder);
    void (*addProject)(const void* in_project);
    void (*addConfiguration)(const char* in_configuration_name);
    void (*addPlatform)(const CCPlatformHandle in_platform);
  } workspace;

  void (*generateInFolder)(const char* workspace_folder);

} CConstruct;

extern CConstruct builder;

#include "tools.inl"
#include "types.inl"
#include "vs2019_project.inl"
#include "vs2019_workspace.inl"
#include "xcode11_project.inl"

// For ease of use set a default CConstruct instance for each platform
#if defined(_MSC_VER)
auto cc_default = cc_vs2019_builder;
#else
auto cc_default = cc_xcode_builder;
#endif