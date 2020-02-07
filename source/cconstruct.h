#pragma warning(disable : 4996)
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

enum EProjectType
{
  CCProjectTypeConsoleApplication = 0,
  CCProjectTypeStaticLibrary
};
enum EPlatformType
{
  EPlatformTypeX86 = 0,
  EPlatformTypeX64,
  EPlatformTypeARM
};

// Project functions
void *createProject(const char *in_project_name, EProjectType in_project_type);
void addFileToProject(void *in_project, const char *in_file_name, const char *in_group_name);

// Workspace functions
void setOutputFolder(const char *of);
void create();
void addProject(const void *in_project);
void addConfiguration(const char *in_configuration_name);
void addPlatform(const char *in_platform_name, EPlatformType in_type);

typedef struct TBuilder_
{

  const struct
  {

    void *(*create)(const char *in_project_name, EProjectType in_project_type);
    void (*addFile)(void *in_project, const char *in_file_name, const char *in_group_name);
  } project;

  const struct
  {
    void (*setLabel)(const char *label);
    void (*setOutputFolder)(const char *in_output_folder);
    void (*addProject)(const void *in_project);
    void (*addConfiguration)(const char *in_configuration_name);
    void (*addPlatform)(const char *in_platform_name, EPlatformType in_type);
  } workspace;

  void (*generate)();

} TBuilder;

extern TBuilder builder;

void writeToFile(FILE *f, const char *s) { fwrite(s, 1, strlen(s), f); }

#include "tools.inl"
#include "types.inl"
#include "xcode11_project.inl"
#include "vs2017_project.inl"
#include "vs2017_workspace.inl"
