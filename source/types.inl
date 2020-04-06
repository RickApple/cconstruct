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
  const char** files;
  const char** groups;
  TProject** dependantOn;

  std::vector<cc_flags> flags;
  TConfiguration** configs;
  TPlatform** platforms;
} TProject;

struct {
  const char* outputFolder;
  const char* workspaceLabel = "workspace";
  TProject** projects;
  const TConfiguration** configurations;
  const TPlatform** platforms;
} privateData;

void* cc_project_create_(const char* in_project_name, EProjectType in_project_type) {
  auto p             = new TProject;
  memset(p, 0, sizeof(TProject));
  p->type            = in_project_type;
  unsigned name_length = strlen(in_project_name);

  char* name_copy = (char*)cc_alloc_(name_length + 1);
  memcpy(name_copy, in_project_name, name_length);
  name_copy[name_length] = 0;
  p->name                = name_copy;
  array_push(privateData.projects, p);
  return p;
}

/**
 * Add multiple files to a project, from a NULL-terminated array
 */
void addFilesToProject(void* in_project, const char* in_group_name, unsigned num_files, const char* in_file_names[]) {
  for( unsigned i=0; i<num_files; ++i,++in_file_names) {
    array_push(((TProject*)in_project)->files, cc_string_clone(*in_file_names));
    array_push(((TProject*)in_project)->groups, in_group_name ? cc_string_clone(in_group_name) : "");
  }
}

void addInputProject(const void* target_project, const void* on_project) {
  array_push(((TProject*)target_project)->dependantOn, (TProject*)on_project);
}

void addConfiguration(const CCConfigurationHandle in_configuration) {
  array_push(privateData.configurations, (const TConfiguration*)in_configuration);
}
void addPlatform(const CCPlatformHandle in_platform) {
  array_push(privateData.platforms, (const TPlatform*)in_platform);
}

void setOutputFolder(const char* of) { privateData.outputFolder = of; }
void setWorkspaceLabel(const char* label) { privateData.workspaceLabel = label; }

void cc_state_reset(cc_flags* out_flags) { memset(out_flags, 0, sizeof(*out_flags)); }
void cc_state_addPreprocessorDefine(cc_flags* in_flags, const char* in_define_string) {
  array_push(in_flags->defines, cc_string_clone(in_define_string));
}

void cc_project_setFlagsLimited_(const void* in_out_project, const cc_flags* in_flags,
                                 CCPlatformHandle in_platform,
                                 CCConfigurationHandle in_configuration) {
  ((TProject*)in_out_project)->flags.push_back(*in_flags);
  array_push(((TProject*)in_out_project)->platforms, (TPlatform*)in_platform);
  array_push(((TProject*)in_out_project)->configs, (TConfiguration*)in_configuration);
}

void cc_project_setFlags_(const void* in_out_project, const cc_flags* in_flags) {
  cc_project_setFlagsLimited_(in_out_project, in_flags, NULL, NULL);
}

CCPlatformHandle cc_platform_create(EPlatformType in_type) {
  unsigned byte_count = sizeof(TPlatform);
  TPlatform* p      = (TPlatform*)cc_alloc_(byte_count);
  p->type           = in_type;
  return p;
}
CCConfigurationHandle cc_configuration_create(const char* in_label) {
  unsigned byte_count = strlen(in_label) + 1 + sizeof(TConfiguration);
  TConfiguration* c = (TConfiguration*)cc_alloc_(byte_count);
  strcpy((char*)c->label, in_label);
  return c;
}
