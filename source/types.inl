typedef struct cc_project_impl_t cc_project_impl_t;

typedef struct cc_platform_impl_t {
  EPlatformType type;
} cc_platform_impl_t;
typedef struct cc_configuration_impl_t {
  int placeholder;  // Else get errors in C
  const char label[];
} cc_configuration_impl_t;

enum EFileType {
  FileTypeNone = 0,      /* added to the project, but not compiled in any way */
  FileTypeCompileable,   /* should be compiled by the C/C++ compiler */
  FileTypeCustomCommand, /* has a custom build command, will not be included in the linking step */
};

struct cc_file_t_ {
  enum EFileType file_type;
  const char* path;
  const struct cc_group_impl_t* parent_group;
};

struct cc_file_custom_command_t_ {
  enum EFileType file_type;
  const char* path;
  const struct cc_group_impl_t* parent_group;
  const char* output_file;
  const char* command;
};

typedef struct cc_group_impl_t {
  const char* name;
  const struct cc_group_impl_t* parent_group;
} cc_group_impl_t;

typedef struct cc_state_impl_t {
  const char** defines;
  const char** include_folders;
  const char** compile_options;
  const char** link_options;
  // By default warnings are turned up to the highest level below _all_.
  EStateWarningLevel warningLevel;
  bool disableWarningsAsErrors;  // By default warnings are treated as errors.
} cc_state_impl_t;

typedef struct cc_project_impl_t {
  EProjectType type;
  const char* name;
  struct cc_file_t_** file_data;                               /* stretch array */
  struct cc_file_custom_command_t_** file_data_custom_command; /* stretch array */
  cc_project_impl_t** dependantOn;                             /* stretch array */

  cc_state_impl_t* state;            /* stretch array */
  cc_configuration_impl_t** configs; /* stretch array */
  cc_platform_impl_t** platforms;    /* stretch array */
  const char* postBuildAction;

  const cc_group_impl_t* parent_group;
} cc_project_impl_t;

struct {
  const char* base_folder;
  const char* outputFolder;
  const char* workspaceLabel;
  cc_project_impl_t** projects;                   /* stretch array */
  const cc_configuration_impl_t** configurations; /* stretch array */
  const cc_platform_impl_t** platforms;           /* stretch array */
  const cc_group_impl_t* groups;                  /* stretch array */
} cc_data_;

cc_group_t cc_group_create(const char* in_group_name, const cc_group_t in_parent_group) {
  cc_group_impl_t* group = (cc_group_impl_t*)cc_alloc_(sizeof(cc_group_impl_t));
  group->name            = cc_printf("%s", in_group_name);
  group->parent_group    = (const cc_group_impl_t*)in_parent_group;
  return (cc_group_t)group;
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
  if (cc_data_.outputFolder == 0) {
    cc_data_.outputFolder = "${platform}/${configuration}";
  }
  if (cc_data_.base_folder == 0) {
    cc_data_.base_folder = "";
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
  cc_project_impl_t* project = (cc_project_impl_t*)in_out_project;
  for (unsigned i = 0; i < num_files; ++i, ++in_file_names) {
    struct cc_file_t_* file_data = (struct cc_file_t_*)cc_alloc_(sizeof(struct cc_file_t_));
    file_data->file_type         = FileTypeCustomCommand;
    file_data->path              = cc_printf("%s", *in_file_names);
    file_data->parent_group      = (const cc_group_impl_t*)in_parent_group;
    array_push(project->file_data, file_data);
  }
}

void addFilesFromFolderToProject(cc_project_t in_out_project, const char* folder,
                                 unsigned num_files, const char* in_file_names[],
                                 const cc_group_t in_parent_group) {
  cc_project_impl_t* project = (cc_project_impl_t*)in_out_project;
  char* relative_path        = make_path_relative(cc_data_.base_folder, folder);
  for (unsigned i = 0; i < num_files; ++i, ++in_file_names) {
    const char* file_path = cc_printf("%s%s", relative_path, make_uri(*in_file_names));

    struct cc_file_t_* file_data = (struct cc_file_t_*)cc_alloc_(sizeof(struct cc_file_t_));
    file_data->file_type         = FileTypeCustomCommand;
    file_data->path              = file_path;
    file_data->parent_group      = (const cc_group_impl_t*)in_parent_group;
    array_push(project->file_data, file_data);
  }
}

void cc_project_addFileWithCustomCommand(cc_project_t in_out_project, const char* in_file_name,
                                         const cc_group_t in_parent_group,
                                         const char* in_custom_command,
                                         const char* in_output_file_name) {
  cc_project_impl_t* project = (cc_project_impl_t*)in_out_project;

  struct cc_file_custom_command_t_* file_data =
      (struct cc_file_custom_command_t_*)cc_alloc_(sizeof(struct cc_file_custom_command_t_));
  file_data->file_type    = FileTypeCustomCommand;
  file_data->path         = cc_printf("%s", in_file_name);
  file_data->parent_group = (const cc_group_impl_t*)in_parent_group;
  file_data->command      = cc_printf("%s", in_custom_command);
  file_data->output_file  = cc_printf("%s", in_output_file_name);
  array_push(project->file_data_custom_command, file_data);
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

void cc_state_reset(cc_state_t out_flags) {
  // TODO: free memory where needed
  memset((cc_project_impl_t*)out_flags, 0, sizeof(cc_state_impl_t));
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

void addPostBuildAction(cc_project_t in_out_project, const char* in_action_command) {
  ((cc_project_impl_t*)in_out_project)->postBuildAction = cc_printf("%s", in_action_command);
}

void cc_project_setFlags_(cc_project_t in_out_project, const cc_state_t in_state,
                          cc_platform_t in_platform, cc_configuration_t in_configuration) {
  // Clone the flags, so later changes aren't applied to this version
  cc_state_impl_t stored_flags = *(const cc_state_impl_t*)in_state;
  stored_flags.defines         = string_array_clone(((const cc_state_impl_t*)in_state)->defines);
  stored_flags.include_folders =
      string_array_clone(((const cc_state_impl_t*)in_state)->include_folders);
  stored_flags.compile_options =
      string_array_clone(((const cc_state_impl_t*)in_state)->compile_options);
  stored_flags.link_options = string_array_clone(((const cc_state_impl_t*)in_state)->link_options);

  array_push(((cc_project_impl_t*)in_out_project)->state, stored_flags);
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

void cc_autoRecompileFromConfig(const char* config_file_path, int argc, const char* const* argv);

cconstruct_t cc_init(const char* in_absolute_config_file_path, int argc, const char* const* argv) {
  bool is_path_absolute =
      (in_absolute_config_file_path[0] == '/') || (in_absolute_config_file_path[1] == ':');
  if (!is_path_absolute) {
    fprintf(stderr,
            "Error: config path passed to cc_init('%s', ...) is not an absolute path. If you are "
            "using "
            "__FILE__ check your compiler settings.\n",
            in_absolute_config_file_path);
#if defined(_MSC_VER)
    fprintf(stderr,
            "When using the Microsoft compiler cl.exe add the /FC flag to ensure __FILE__ emits "
            "an absolute path.\n");
#elif defined(__APPLE__)
    fprintf(stderr,
            "You can make the file you are compiling absolute by adding $PWD/ in front of it.\n");
#endif

    exit(1);
  }

  static bool is_inited = false;
  if (is_inited) {
    fprintf(stderr, "Error: calling cc_init() multiple times. Don't do this.\n");
    exit(1);
  }
  is_inited = true;

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--verbose") == 0) {
      cc_is_verbose = true;
    }
  }

  cc_autoRecompileFromConfig(in_absolute_config_file_path, argc, argv);

  cc_data_.base_folder = folder_path_only(in_absolute_config_file_path);

  // Keep this as a local, so that users are forced to call cc_init to get an instance the struct.
  cconstruct_t out = {
      &cc_configuration_create,
      &cc_platform_create,
      &cc_project_create_,
      &cc_group_create,
      &cc_state_create,
      {&cc_state_reset, &cc_state_addIncludeFolder, &cc_state_addPreprocessorDefine,
       &cc_state_addCompilerFlag, &cc_state_addLinkerFlag, &cc_state_setWarningLevel,
       &cc_state_disableWarningsAsErrors},
      {&addFilesToProject, &addFilesFromFolderToProject, &cc_project_addFileWithCustomCommand,
       &addInputProject, &cc_project_setFlags_, &addPostBuildAction},
      {&setWorkspaceLabel, &setOutputFolder, &addConfiguration, &addPlatform}};

  return out;
}
