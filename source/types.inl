typedef struct TProject TProject;

typedef struct TProject {
  EProjectType type;
  std::string name;
  std::vector<std::string> files;
  std::vector<std::string> groups;
  std::vector<TProject*> dependantOn;
  cc_flags flags;
} TProject;

struct {
  const char* outputFolder;
  const char* workspaceLabel;
  std::vector<TProject*> projects;
  std::vector<std::string> configurations;
  std::vector<std::string> platform_names;
  std::vector<EPlatformType> platforms;
} privateData;

void* createProject(const char* in_project_name, EProjectType in_project_type) {
  auto p  = new TProject;
  p->type = in_project_type;
  p->name = in_project_name;
  return p;
}

/**
 * Add multiple files to a project, from a NULL-terminated array
 */
void addFilesToProject(void* in_project, const char* in_group_name, const char* in_file_names[]) {
  while (*in_file_names) {
    ((TProject*)in_project)->files.push_back(*in_file_names);
    ((TProject*)in_project)->groups.push_back(in_group_name ? in_group_name : "");
    ++in_file_names;
  }
}

void addInputProject(const void* target_project, const void* on_project) {
  ((TProject*)target_project)->dependantOn.push_back((TProject*)on_project);
}

void addConfiguration(const char* in_configuration_name) {
  privateData.configurations.push_back(in_configuration_name);
}
void addPlatform(const char* in_platform_name, EPlatformType in_type) {
  privateData.platform_names.push_back(in_platform_name);
  privateData.platforms.push_back(in_type);
}

void setOutputFolder(const char* of) { privateData.outputFolder = of; }
void setWorkspaceLabel(const char* label) { privateData.workspaceLabel = label; }

void addProject(const void* in_project) { privateData.projects.push_back((TProject*)in_project); }

void cc_state_reset(cc_flags* out_flags) { memset(out_flags, 0, sizeof(*out_flags)); }
void cc_state_addPreprocessorDefine(cc_flags* in_flags, const char* in_define_string) {
  in_flags->defines.push_back(in_define_string);
}

void cc_project_setFlags(const void* in_project, const cc_flags* in_flags) {
  ((TProject*)in_project)->flags = *in_flags;
}