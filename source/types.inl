typedef struct TProject TProject;

typedef struct TPlatform {
  EPlatformType type;
} TPlatform;
typedef struct TConfiguration {
  const char label[];
} TConfiguration;

typedef struct TProject {
  EProjectType type;
  const char* name;
  std::vector<std::string> files;
  std::vector<std::string> groups;
  std::vector<TProject*> dependantOn;

  std::vector<cc_flags> flags;
  std::vector<CCConfigurationHandle> configs;
  std::vector<CCPlatformHandle> platforms;
} TProject;

struct {
  const char* outputFolder;
  const char* workspaceLabel = "workspace";
  std::vector<TProject*> projects;
  std::vector<const TConfiguration*> configurations;
  std::vector<const TPlatform*> platforms;
} privateData;

void* cc_project_create_(const char* in_project_name, EProjectType in_project_type) {
  auto p             = new TProject;
  p->type            = in_project_type;
  size_t name_length = strlen(in_project_name);

  char* name_copy = (char*)cc_alloc_(name_length + 1);
  memcpy(name_copy, in_project_name, name_length);
  name_copy[name_length] = 0;
  p->name                = name_copy;
  privateData.projects.push_back(p);
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

void addConfiguration(const CCConfigurationHandle in_configuration) {
  privateData.configurations.push_back((const TConfiguration*)in_configuration);
}
void addPlatform(const CCPlatformHandle in_platform) {
  privateData.platforms.push_back((const TPlatform*)in_platform);
}

void setOutputFolder(const char* of) { privateData.outputFolder = of; }
void setWorkspaceLabel(const char* label) { privateData.workspaceLabel = label; }

void cc_state_reset(cc_flags* out_flags) { memset(out_flags, 0, sizeof(*out_flags)); }
void cc_state_addPreprocessorDefine(cc_flags* in_flags, const char* in_define_string) {
  in_flags->defines.push_back(in_define_string);
}

void cc_project_setFlagsLimited_(const void* in_out_project, const cc_flags* in_flags,
                                 CCPlatformHandle in_platform,
                                 CCConfigurationHandle in_configuration) {
  ((TProject*)in_out_project)->flags.push_back(*in_flags);
  ((TProject*)in_out_project)->platforms.push_back(in_platform);
  ((TProject*)in_out_project)->configs.push_back(in_configuration);
}

void cc_project_setFlags_(const void* in_out_project, const cc_flags* in_flags) {
  cc_project_setFlagsLimited_(in_out_project, in_flags, NULL, NULL);
}

CCPlatformHandle cc_platform_create(EPlatformType in_type) {
  size_t byte_count = sizeof(TPlatform);
  TPlatform* p      = (TPlatform*)cc_alloc_(byte_count);
  p->type           = in_type;
  return p;
}
CCConfigurationHandle cc_configuration_create(const char* in_label) {
  size_t byte_count = strlen(in_label) + 1 + sizeof(TConfiguration);
  TConfiguration* c = (TConfiguration*)cc_alloc_(byte_count);
  strcpy((char*)c->label, in_label);
  return c;
}
