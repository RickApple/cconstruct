typedef struct cc_project_impl_t cc_project_impl_t;

typedef struct cc_configuration_impl_t {
  const char label[256];
} cc_configuration_impl_t;
typedef struct cc_architecture_impl_t {
  EArchitecture type;
} cc_architecture_impl_t;
typedef struct cc_platform_impl_t {
  EPlatform type;
} cc_platform_impl_t;

enum EFileType {
  FileTypeNone = 0,      /* added to the project, but not compiled in any way */
  FileTypeCompileable,   /* should be compiled by the C/C++ compiler */
  FileTypeCustomCommand, /* has a custom build command, will not be included in the linking step */
};

struct cc_file_t_ {
  enum EFileType file_type;
  const char* path;
  size_t parent_group_idx;
};

struct cc_file_custom_command_t_ {
  enum EFileType file_type;
  const char* path;
  size_t parent_group_idx;
  const char* output_file;
  const char* command;
};

typedef struct cc_group_impl_t {
  const char* name;
  size_t parent_group_idx;
} cc_group_impl_t;

typedef struct cc_state_impl_t {
  const char** defines;         /* stretch array */
  const char** include_folders; /* stretch array */
  const char** compile_options; /* stretch array */
  const char** link_options;    /* stretch array */
  const char** external_libs;   /* stretch array */

  /* By default warnings are turned up to the highest level below _all_ */
  EStateWarningLevel warningLevel;
  /* By default warnings are treated as errors */
  bool disableWarningsAsErrors;
  int cpp_version;
} cc_state_impl_t;

typedef struct cc_project_impl_t {
  EProjectType type;
  const char* name;
  struct cc_file_t_** file_data;                               /* stretch array */
  struct cc_file_custom_command_t_** file_data_custom_command; /* stretch array */
  cc_project_impl_t** dependantOn;                             /* stretch array */
  cc_project_impl_t** dependantOnNoLink;                       /* stretch array */

  cc_state_impl_t* state;                 /* stretch array */
  cc_configuration_impl_t** configs;      /* stretch array */
  cc_architecture_impl_t** architectures; /* stretch array */
  const char* preBuildAction;
  const char* postBuildAction;

  size_t parent_group_idx;
  const char* outputFolder;

} cc_project_impl_t;

struct {
  bool is_inited;
  const char* base_folder;
  const char* workspaceLabel;
  cc_project_impl_t** projects;                   /* stretch array */
  const cc_configuration_impl_t** configurations; /* stretch array */
  const cc_architecture_impl_t** architectures;   /* stretch array */
  const cc_platform_impl_t** platforms;           /* stretch array */
  cc_group_impl_t* groups;                        /* stretch array */
} cc_data_ = {0};

cc_group_t cc_group_create(const char* in_group_name, const cc_group_t in_parent_group) {
  cc_group_impl_t group;
  group.name             = cc_printf("%s", in_group_name);
  group.parent_group_idx = (size_t)in_parent_group;
  array_push(cc_data_.groups, group);
  return (cc_group_t)(size_t)(array_count(cc_data_.groups) - 1);
}

cc_state_t cc_state_create() {
  cc_state_impl_t* out = (cc_state_impl_t*)cc_alloc_(sizeof(cc_state_impl_t));
  memset(out, 0, sizeof(cc_state_impl_t));
  return (cc_state_t)out;
}

cc_project_t cc_project_create_(const char* in_project_name, EProjectType in_project_type,
                                const cc_group_t in_parent_group) {
  // TODO: having no workspace label crashes on XCode generator
  if (cc_data_.workspaceLabel == 0) {
    cc_data_.workspaceLabel = "workspace";
  }
  if (cc_data_.base_folder == 0) {
    cc_data_.base_folder = "";
  }

  cc_project_impl_t* p = (cc_project_impl_t*)malloc(sizeof(cc_project_impl_t));
  memset(p, 0, sizeof(cc_project_impl_t));
  p->outputFolder    = "${platform}/${configuration}";
  p->type            = in_project_type;
  size_t name_length = strlen(in_project_name);

  char* name_copy = (char*)cc_alloc_(name_length + 1);
  memcpy(name_copy, in_project_name, name_length);
  name_copy[name_length] = 0;
  p->name                = name_copy;
  array_push(cc_data_.projects, p);

  p->parent_group_idx = (size_t)in_parent_group;
  return p;
}

/**
 * Add multiple files to a project, from a NULL-terminated array
 *
 * @param in_parent_group may be NULL
 */
void addFilesToProject(cc_project_t in_out_project, unsigned num_files,
                       const char* in_file_names[], const cc_group_t in_parent_group) {
  assert(in_out_project);

  cc_project_impl_t* project = (cc_project_impl_t*)in_out_project;
  for (unsigned i = 0; i < num_files; ++i, ++in_file_names) {
    struct cc_file_t_* file_data = (struct cc_file_t_*)cc_alloc_(sizeof(struct cc_file_t_));
    file_data->file_type         = FileTypeCustomCommand;
    file_data->path              = cc_printf("%s", *in_file_names);
    file_data->parent_group_idx  = (size_t)in_parent_group;
    array_push(project->file_data, file_data);
  }
}

void addFilesFromFolderToProject(cc_project_t in_out_project, const char* folder,
                                 unsigned num_files, const char* in_file_names[],
                                 const cc_group_t in_parent_group) {
  assert(in_out_project);
  assert(folder);
  assert(num_files == 0 || (in_file_names != NULL));

  cc_project_impl_t* project = (cc_project_impl_t*)in_out_project;
  const char* relative_path  = make_path_relative(cc_data_.base_folder, folder);
  if (relative_path[strlen(relative_path) - 1] != '/') {
    relative_path = cc_printf("%s/", relative_path);
  }
  for (unsigned i = 0; i < num_files; ++i, ++in_file_names) {
    const char* file_path = cc_printf("%s%s", relative_path, make_uri(*in_file_names));

    struct cc_file_t_* file_data = (struct cc_file_t_*)cc_alloc_(sizeof(struct cc_file_t_));
    file_data->file_type         = FileTypeCustomCommand;
    file_data->path              = file_path;
    file_data->parent_group_idx  = (size_t)in_parent_group;
    array_push(project->file_data, file_data);
  }
}

void cc_project_addFileWithCustomCommand(cc_project_t in_out_project, const char* in_file_name,
                                         const cc_group_t in_parent_group,
                                         const char* in_custom_command,
                                         const char* in_output_file_name) {
  assert(in_out_project);

  cc_project_impl_t* project = (cc_project_impl_t*)in_out_project;

  struct cc_file_custom_command_t_* file_data =
      (struct cc_file_custom_command_t_*)cc_alloc_(sizeof(struct cc_file_custom_command_t_));
  file_data->file_type        = FileTypeCustomCommand;
  file_data->path             = cc_printf("%s", in_file_name);
  file_data->parent_group_idx = (size_t)in_parent_group;
  file_data->command          = cc_printf("%s", in_custom_command);
  file_data->output_file      = cc_printf("%s", in_output_file_name);
  array_push(project->file_data_custom_command, file_data);
}

void cc_project_addInputProject(cc_project_t target_project, const cc_project_t on_project) {
  assert(target_project);
  assert(on_project);

  array_push(((cc_project_impl_t*)target_project)->dependantOn, (cc_project_impl_t*)on_project);
}

void cc_project_addDependency(cc_project_t earlier_project, const cc_project_t later_project) {
  assert(earlier_project);
  assert(later_project);

  array_push(((cc_project_impl_t*)later_project)->dependantOnNoLink,
             (cc_project_impl_t*)earlier_project);
}

void addConfiguration(const cc_configuration_t in_configuration) {
  array_push(cc_data_.configurations, (const cc_configuration_impl_t*)in_configuration);
}
void addArchitecture(const cc_architecture_t in_architecture) {
  array_push(cc_data_.architectures, (const cc_architecture_impl_t*)in_architecture);
}
void addPlatform(const cc_platform_t in_platform) {
  array_push(cc_data_.platforms, (const cc_platform_impl_t*)in_platform);
}

void setWorkspaceLabel(const char* label) { cc_data_.workspaceLabel = label; }

void cc_state_reset(cc_state_t out_flags) {
  // TODO: free memory where needed
  memset((cc_project_impl_t*)out_flags, 0, sizeof(cc_state_impl_t));
}

void cc_state_setCppVersion(cc_state_t in_state, int version) {
  cc_state_impl_t* flags = ((cc_state_impl_t*)in_state);
  flags->cpp_version = version;
}

void cc_state_addIncludeFolder(cc_state_t in_state, const char* in_include_folder) {
  cc_state_impl_t* flags = ((cc_state_impl_t*)in_state);
  array_push(flags->include_folders, make_uri(in_include_folder));
}

void cc_state_addPreprocessorDefine(cc_state_t in_state, const char* in_define_string) {
  array_push(((cc_state_impl_t*)in_state)->defines, cc_printf("%s", in_define_string));
}

void cc_state_addCompilerFlag(cc_state_t in_state, const char* in_compiler_flag) {
  array_push(((cc_state_impl_t*)in_state)->compile_options, cc_printf("%s", in_compiler_flag));
}

void cc_state_linkExternalLibrary(cc_state_t in_state, const char* in_external_library_path) {
  array_push(((cc_state_impl_t*)in_state)->external_libs,
             cc_printf("%s", in_external_library_path));
}

void cc_state_addLinkerFlag(cc_state_t in_state, const char* in_linker_flag) {
  array_push(((cc_state_impl_t*)in_state)->link_options, cc_printf("%s", in_linker_flag));
}

void cc_state_setWarningLevel(cc_state_t in_state, EStateWarningLevel in_level) {
  ((cc_state_impl_t*)in_state)->warningLevel = in_level;
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
void cc_state_disableWarningsAsErrors(cc_state_t in_state) {
  ((cc_state_impl_t*)in_state)->disableWarningsAsErrors = true;
}

void cc_project_addPreBuildAction(cc_project_t in_out_project, const char* in_action_command) {
  ((cc_project_impl_t*)in_out_project)->preBuildAction = cc_printf("%s", in_action_command);
}

void cc_project_addPostBuildAction(cc_project_t in_out_project, const char* in_action_command) {
  ((cc_project_impl_t*)in_out_project)->postBuildAction = cc_printf("%s", in_action_command);
}

void cc_project_setOutputFolder(cc_project_t in_out_project, const char* of) {
  ((cc_project_impl_t*)in_out_project)->outputFolder = cc_printf("%s", of);
}

void cc_project_setFlags_(cc_project_t in_out_project, const cc_state_t in_state,
                          cc_architecture_t in_architecture, cc_configuration_t in_configuration) {
  // Clone the flags, so later changes aren't applied to this version
  cc_state_impl_t stored_flags = *(const cc_state_impl_t*)in_state;
  stored_flags.defines         = string_array_clone(((const cc_state_impl_t*)in_state)->defines);
  stored_flags.include_folders =
      string_array_clone(((const cc_state_impl_t*)in_state)->include_folders);
  stored_flags.compile_options =
      string_array_clone(((const cc_state_impl_t*)in_state)->compile_options);
  stored_flags.link_options = string_array_clone(((const cc_state_impl_t*)in_state)->link_options);

  array_push(((cc_project_impl_t*)in_out_project)->state, stored_flags);
  array_push(((cc_project_impl_t*)in_out_project)->architectures,
             (cc_architecture_impl_t*)in_architecture);
  array_push(((cc_project_impl_t*)in_out_project)->configs,
             (cc_configuration_impl_t*)in_configuration);
}

cc_architecture_t cc_architecture_create(EArchitecture in_type) {
  unsigned byte_count       = sizeof(cc_architecture_impl_t);
  cc_architecture_impl_t* p = (cc_architecture_impl_t*)cc_alloc_(byte_count);
  p->type                   = in_type;
  return p;
}
cc_platform_t cc_platform_create(EPlatform in_type) {
  unsigned byte_count   = sizeof(cc_platform_impl_t);
  cc_platform_impl_t* p = (cc_platform_impl_t*)cc_alloc_(byte_count);
  p->type               = in_type;
  return (cc_platform_t)p;
}
cc_configuration_t cc_configuration_create(const char* in_label) {
  cc_configuration_impl_t* c =
      (cc_configuration_impl_t*)cc_alloc_(sizeof(cc_configuration_impl_t));
  strncpy((char*)c->label, in_label, sizeof(c->label));
  ((char*)c->label)[sizeof(c->label) - 1] = 0;
  return c;
}
