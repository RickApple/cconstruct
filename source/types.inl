typedef struct cc_project_impl_t cc_project_impl_t;

typedef struct cc_platform_impl_t {
  EPlatformType type;
} cc_platform_impl_t;
typedef struct cc_configuration_impl_t {
  int placeholder;  // Else get errors in C
  const char label[];
} cc_configuration_impl_t;

typedef struct cc_group_impl_t {
  const char* name;
  const struct cc_group_impl_t* parent_group;
} cc_group_impl_t;

typedef struct cc_project_impl_t {
  EProjectType type;
  const char* name;
  const char** files;              /* stretch array */
  const cc_group_impl_t** groups;  /* stretch array */
  cc_project_impl_t** dependantOn; /* stretch array */

  cc_flags* flags;                   /* stretch array */
  cc_configuration_impl_t** configs; /* stretch array */
  cc_platform_impl_t** platforms;    /* stretch array */
  const char* postBuildAction;

  const cc_group_impl_t* parent_group;
} cc_project_impl_t;

struct {
  const char* outputFolder;
  const char* workspaceLabel;
  cc_project_impl_t** projects;                   /* stretch array */
  const cc_configuration_impl_t** configurations; /* stretch array */
  const cc_platform_impl_t** platforms;           /* stretch array */
  const cc_group_impl_t* groups;                  /* stretch array */
} cc_data_;

cc_group_t cc_createGroup(const char* in_group_name, const cc_group_t in_parent_group) {
  cc_group_impl_t* group = (cc_group_impl_t*)cc_alloc_(sizeof(cc_group_impl_t));
  group->name            = cc_printf("%s", in_group_name);
  group->parent_group    = (const cc_group_impl_t*)in_parent_group;
  return (cc_group_t)group;
}

cc_project_t cc_project_create_(const char* in_project_name, EProjectType in_project_type,
                                const cc_group_t in_parent_group) {
  // TODO: having no workspace label crashes on XCode generator
  if (cc_data_.workspaceLabel == 0) {
    cc_data_.workspaceLabel = "workspace";
  }
  if (cc_data_.outputFolder == 0) {
    cc_data_.outputFolder = "${platform}/${configuration}";
  }

  cc_project_impl_t* p = (cc_project_impl_t*)malloc(sizeof(cc_project_impl_t));
  memset(p, 0, sizeof(cc_project_impl_t));
  p->type              = in_project_type;
  unsigned name_length = strlen(in_project_name);

  char* name_copy = (char*)cc_alloc_(name_length + 1);
  memcpy(name_copy, in_project_name, name_length);
  name_copy[name_length] = 0;
  p->name                = name_copy;
  array_push(cc_data_.projects, p);

  p->parent_group = (const cc_group_impl_t*)in_parent_group;
  return p;
}

/**
 * Add multiple files to a project, from a NULL-terminated array
 *
 * @param in_parent_group may be NULL
 */
void addFilesToProject(cc_project_t in_out_project, unsigned num_files,
                       const char* in_file_names[], const cc_group_t in_parent_group) {
  for (unsigned i = 0; i < num_files; ++i, ++in_file_names) {
    array_push(((cc_project_impl_t*)in_out_project)->files, cc_printf("%s", *in_file_names));
    array_push(((cc_project_impl_t*)in_out_project)->groups,
               (const cc_group_impl_t*)in_parent_group);
  }
}

void addInputProject(cc_project_t target_project, const cc_project_t on_project) {
  array_push(((cc_project_impl_t*)target_project)->dependantOn, (cc_project_impl_t*)on_project);
}

void addConfiguration(const cc_configuration_t in_configuration) {
  array_push(cc_data_.configurations, (const cc_configuration_impl_t*)in_configuration);
}
void addPlatform(const cc_platform_t in_platform) {
  array_push(cc_data_.platforms, (const cc_platform_impl_t*)in_platform);
}

void setOutputFolder(const char* of) { cc_data_.outputFolder = of; }
void setWorkspaceLabel(const char* label) { cc_data_.workspaceLabel = label; }

void cc_state_reset(cc_flags* out_flags) { memset(out_flags, 0, sizeof(*out_flags)); }
void cc_state_addPreprocessorDefine(cc_flags* in_flags, const char* in_define_string) {
  array_push(in_flags->defines, cc_printf("%s", in_define_string));
}
void cc_state_setWarningLevel(cc_flags* in_flags, EStateWarningLevel in_level) {
  in_flags->warningLevel = in_level;
  /*
  clang:
  -W -Wall
  -W -Wall -Wextra -pedantic
  -W -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization
-Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast
-Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel
-Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses
-fdiagnostics-show-option

-Weverything

  -Werror warnings as errors

  /W2
  /W3
  /W4

  /Wall

  /WX
  */
}
void cc_state_disableWarningsAsErrors(cc_flags* in_flags) {
  in_flags->disableWarningsAsErrors = true;
}

void addPostBuildAction(cc_project_t in_out_project, const char* in_action_command) {
  ((cc_project_impl_t*)in_out_project)->postBuildAction = cc_printf("%s", in_action_command);
}

void cc_project_setFlags_(cc_project_t in_out_project, const cc_flags* in_flags,
                          cc_platform_t in_platform, cc_configuration_t in_configuration) {
  // Clone the flags, so later changes aren't applied to this version
  cc_flags stored_flags        = *in_flags;
  stored_flags.defines         = string_array_clone(in_flags->defines);
  stored_flags.include_folders = string_array_clone(in_flags->include_folders);
  stored_flags.compile_options = string_array_clone(in_flags->compile_options);
  stored_flags.link_options    = string_array_clone(in_flags->link_options);

  array_push(((cc_project_impl_t*)in_out_project)->flags, stored_flags);
  array_push(((cc_project_impl_t*)in_out_project)->platforms, (cc_platform_impl_t*)in_platform);
  array_push(((cc_project_impl_t*)in_out_project)->configs,
             (cc_configuration_impl_t*)in_configuration);
}

cc_platform_t cc_platform_create(EPlatformType in_type) {
  unsigned byte_count   = sizeof(cc_platform_impl_t);
  cc_platform_impl_t* p = (cc_platform_impl_t*)cc_alloc_(byte_count);
  p->type               = in_type;
  return p;
}
cc_configuration_t cc_configuration_create(const char* in_label) {
  unsigned byte_count        = strlen(in_label) + 1 + sizeof(cc_configuration_impl_t);
  cc_configuration_impl_t* c = (cc_configuration_impl_t*)cc_alloc_(byte_count);
  strcpy((char*)c->label, in_label);
  return c;
}
