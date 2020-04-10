typedef struct TProject TProject;

typedef struct TPlatform {
  EPlatformType type;
} TPlatform;
typedef struct TConfiguration {
  int placeholder;  // Else get errors in C
  const char label[];
} TConfiguration;

typedef struct TProject {
  EProjectType type;
  const char* name;
  const char** files;
  const char** groups;
  TProject** dependantOn;

  cc_flags* flags;
  TConfiguration** configs;
  TPlatform** platforms;
  const char* postBuildAction;
} TProject;

struct {
  const char* outputFolder;
  const char* workspaceLabel;
  TProject** projects;
  const TConfiguration** configurations;
  const TPlatform** platforms;
} privateData;

void* cc_project_create_(const char* in_project_name, EProjectType in_project_type) {
  // TODO: having no workspace label crashes on XCode generator
  if (privateData.workspaceLabel == 0) {
    privateData.workspaceLabel = "workspace";
  }
  if (privateData.outputFolder == 0) {
    privateData.outputFolder = "${platform}/${configuration}";
  }

  TProject* p = (TProject*)malloc(sizeof(TProject));
  memset(p, 0, sizeof(TProject));
  p->type              = in_project_type;
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
void addFilesToProject(void* in_project, const char* in_group_name, unsigned num_files,
                       const char* in_file_names[]) {
  for (unsigned i = 0; i < num_files; ++i, ++in_file_names) {
    array_push(((TProject*)in_project)->files, cc_printf("%s", *in_file_names));
    array_push(((TProject*)in_project)->groups,
               in_group_name ? cc_printf("%s", in_group_name) : "");
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
  array_push(in_flags->defines, cc_printf("%s", in_define_string));
}

void addPostBuildAction(void* in_out_project, const char* in_action_command) {
  ((TProject*)in_out_project)->postBuildAction = cc_printf("%s", in_action_command);
}

const char** string_array_clone(const char** in) {
  char** out                   = {0};
  const array_header_t* header = array_header(in);
  if (header && header->count_ > 0) {
    array_header_t* out_header =
        (array_header_t*)cc_alloc_(sizeof(array_header_t) + sizeof(const char*) * header->count_);
    out_header->count_ = out_header->capacity_ = header->count_;
    out                                        = (char**)(out_header + 1);
    memcpy(out, in, sizeof(const char*) * header->count_);
  }
  return (const char**)out;
}

void cc_project_setFlagsLimited_(void* in_out_project, const cc_flags* in_flags,
                                 CCPlatformHandle in_platform,
                                 CCConfigurationHandle in_configuration) {
  // Clone the flags, so later changes aren't applied to this version
  cc_flags stored_flags        = {0};
  stored_flags.defines         = string_array_clone(in_flags->defines);
  stored_flags.include_folders = string_array_clone(in_flags->include_folders);
  stored_flags.compile_options = string_array_clone(in_flags->compile_options);
  stored_flags.link_options    = string_array_clone(in_flags->link_options);

  array_push(((TProject*)in_out_project)->flags, stored_flags);
  array_push(((TProject*)in_out_project)->platforms, (TPlatform*)in_platform);
  array_push(((TProject*)in_out_project)->configs, (TConfiguration*)in_configuration);
}

void cc_project_setFlags_(void* in_out_project, const cc_flags* in_flags) {
  cc_project_setFlagsLimited_(in_out_project, in_flags, NULL, NULL);
}

CCPlatformHandle cc_platform_create(EPlatformType in_type) {
  unsigned byte_count = sizeof(TPlatform);
  TPlatform* p        = (TPlatform*)cc_alloc_(byte_count);
  p->type             = in_type;
  return p;
}
CCConfigurationHandle cc_configuration_create(const char* in_label) {
  unsigned byte_count = strlen(in_label) + 1 + sizeof(TConfiguration);
  TConfiguration* c   = (TConfiguration*)cc_alloc_(byte_count);
  strcpy((char*)c->label, in_label);
  return c;
}
