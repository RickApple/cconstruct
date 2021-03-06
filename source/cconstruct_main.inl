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
    LOG_ERROR_AND_QUIT(
        "When using the Microsoft compiler cl.exe add the /FC flag to ensure __FILE__ emits "
        "an absolute path.\n");
#elif defined(__APPLE__)
    LOG_ERROR_AND_QUIT(
        "You can make the file you are compiling absolute by adding $PWD/ in front of it.\n");
#endif
  }

  if (cc_data_.is_inited) {
    LOG_ERROR_AND_QUIT("Error: calling cc_init() multiple times. Don't do this.\n");
  }
  cc_data_.is_inited = true;

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--verbose") == 0) {
      cc_is_verbose = true;
    }
  }

  // First group is no group
  cc_group_impl_t null_group = {0};
  array_push(cc_data_.groups, null_group);

  cc_autoRecompileFromConfig(in_absolute_config_file_path, argc, argv);

  cc_data_.base_folder = folder_path_only(in_absolute_config_file_path);

  // Keep this as a local, so that users are forced to call cc_init to get an instance the struct.
  cconstruct_t out = {
      &cc_configuration_create,
      &cc_architecture_create,
      &cc_platform_create,
      &cc_project_create_,
      &cc_group_create,
      &cc_state_create,
      {&cc_state_reset, &cc_state_addIncludeFolder, &cc_state_addPreprocessorDefine,
       &cc_state_addCompilerFlag, &cc_state_addLinkerFlag, &cc_state_linkExternalLibrary,
       &cc_state_setWarningLevel, &cc_state_disableWarningsAsErrors},
      {
          &addFilesToProject,
          &addFilesFromFolderToProject,
          &cc_project_addFileWithCustomCommand,
          &cc_project_addInputProject,
          &cc_project_setFlags_,
          &addPostBuildAction,
          &cc_project_setOutputFolder,
      },
      {&setWorkspaceLabel, &addConfiguration, &addArchitecture, &addPlatform}};

  return out;
}
