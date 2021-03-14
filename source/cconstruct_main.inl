#if defined(_WIN32)
LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
  fprintf(stderr, "Unhandled exception occurred\n");
  exit(ERR_CONSTRUCTION);
}
#else
#include <signal.h>

#include <err.h>
void posix_signal_handler(int sig, siginfo_t* siginfo, void* context) {
  (void)sig;
  (void)siginfo;
  (void)context;
  // posix_print_stack_trace();
  exit(ERR_CONSTRUCTION);
}

static uint8_t alternate_stack[SIGSTKSZ];
void set_signal_handler() {
  /* setup alternate stack */
  {
    stack_t ss = {};
    /* malloc is usually used here, I'm not 100% sure my static allocation
       is valid but it seems to work just fine. */
    ss.ss_sp    = (void*)alternate_stack;
    ss.ss_size  = SIGSTKSZ;
    ss.ss_flags = 0;

    if (sigaltstack(&ss, NULL) != 0) {
      err(1, "sigaltstack");
    }
  }

  /* register our signal handlers */
  {
    struct sigaction sig_action = {};
    sig_action.sa_sigaction     = posix_signal_handler;
    sigemptyset(&sig_action.sa_mask);

#ifdef __APPLE__
    /* for some reason we backtrace() doesn't work on osx
       when we use an alternate stack */
    sig_action.sa_flags = SA_SIGINFO;
#else
    sig_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
#endif

    if (sigaction(SIGSEGV, &sig_action, NULL) != 0) {
      err(1, "sigaction");
    }
    if (sigaction(SIGFPE, &sig_action, NULL) != 0) {
      err(1, "sigaction");
    }
    if (sigaction(SIGINT, &sig_action, NULL) != 0) {
      err(1, "sigaction");
    }
    if (sigaction(SIGILL, &sig_action, NULL) != 0) {
      err(1, "sigaction");
    }
    if (sigaction(SIGTERM, &sig_action, NULL) != 0) {
      err(1, "sigaction");
    }
    if (sigaction(SIGABRT, &sig_action, NULL) != 0) {
      err(1, "sigaction");
    }
  }
}
#endif

int cc_runNewBuild_() {
  const char* new_construct_command =
      cc_printf("%s --generate-projects", cconstruct_internal_binary_name);
#if !defined(_WIN32)
  new_construct_command = cc_printf("./%s", new_construct_command);
#endif

  if (cc_is_verbose) {
    new_construct_command = cc_printf("%s --verbose", new_construct_command);
  }
  LOG_VERBOSE("Executing new binary: '%s'\n", new_construct_command);
  int result = system(new_construct_command);
#if !defined(_WIN32)
  // The spec for system() doesn't require it to return the error from the executed command.
  // On MacOS it doesn't, so query the actual error here.
  result = WEXITSTATUS(result);
#endif
  return result;
}

cconstruct_t cc_init(const char* in_absolute_config_file_path, int argc, const char* const* argv) {
#if defined(_WIN32)
  (void)DeleteFile(cconstruct_old_binary_name);
  SetUnhandledExceptionFilter(ExceptionHandler);
#else
  set_signal_handler();
#endif

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
        ERR_CONFIGURATION,
        "When using the Microsoft compiler cl.exe add the /FC flag to ensure __FILE__ emits "
        "an absolute path.\n");
#elif defined(__APPLE__)
    LOG_ERROR_AND_QUIT(ERR_CONFIGURATION,
        "You can make the file you are compiling absolute by adding $PWD/ in front of it.\n");
#endif
  }

  if (cc_data_.is_inited) {
    LOG_ERROR_AND_QUIT(ERR_CONFIGURATION,"Error: calling cc_init() multiple times. Don't do this.\n");
  }
  cc_data_.is_inited = true;

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--verbose") == 0) {
      cc_is_verbose = true;
    }
    if (strcmp(argv[i], "--generate-projects") == 0) {
      cc_only_generate = true;
    }
  }

  // First group is no group
  cc_group_impl_t null_group = {0};
  array_push(cc_data_.groups, null_group);

  // If not only generating, then a new binary is built, that is run, and only then do we attempt
  // to clean up this existing version. This binary doesn't do any construction of projects.
  if (!cc_only_generate) {
    cc_recompile_binary_(in_absolute_config_file_path);
    int result = cc_runNewBuild_();
    if (result == 0) {
      cc_activateNewBuild_();
    }
    exit(result);
  }

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
