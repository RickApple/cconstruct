#if defined(_WIN32)
  // First
  #include <Windows.h>
  // Later
  #include <DbgHelp.h>
  #include <direct.h>
  #include <errno.h>
  #include <malloc.h>
  #include <tchar.h>
  #pragma comment(lib, "DbgHelp.lib")
#else
  #include <err.h>
  #include <errno.h>
  #include <signal.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#if defined(__APPLE__)
  #include <limits.h>
  #include <mach-o/dyld.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
const char* cconstruct_binary_name          = "cconstruct.exe";
const char* cconstruct_internal_binary_name = "cconstruct_internal.exe";
const char* cconstruct_old_binary_name      = "cconstruct.exe.old";
#else
const char* cconstruct_binary_name          = "cconstruct";
const char* cconstruct_internal_binary_name = "cconstruct_internal";
const char* cconstruct_old_binary_name      = "cconstruct.old";
#endif

// Tools
// clang-format off
#include "stack.inl"
#include "tools.inl"
#include "types.inl"
#include "data_tree.inl"

struct {
  const char* config_file_path;
  const char* active_config;
  const char* active_arch_label;
  EArchitecture active_arch;
  bool show_includes;
} _internal = {NULL, "Debug", 
#ifdef _M_IX86
  "x86", EArchitectureX86,
#elif _M_X64
  "x64", EArchitectureX64,
#endif
   false};

// Constructors
#include "process.inl"
#include "builder.inl"
#ifdef _WIN32
#include "vs2019_constructor.inl"
#else
#include "xcode11_constructor.inl"
#endif
#include "ninja_constructor.inl"
// clang-format on

#if defined(_WIN32)
LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
  fprintf(stderr, "Unhandled exception occurred with the following stack:\n");
  PrintStrackFromContext(pExceptionInfo->ContextRecord);

  exit(ERR_CONSTRUCTION);
}
#else
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

int cc_runNewBuild_(char const* const argv[], const int argc) {
  const char* new_construct_command =
      cc_printf("%s --generate-projects", cconstruct_internal_binary_name);
#if !defined(_WIN32)
  new_construct_command = cc_printf("./%s", new_construct_command);
#endif

  for (int i = 1; i < argc; ++i) {
    new_construct_command = cc_printf("%s %s", new_construct_command, argv[i]);
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

// Generates a project to build cconstruct itself. Use this when cconstruct crashes and you can't
// figure out why.
void generate_cc_project(cconstruct_t cc, const char* cc_config_path) {
  printf("Generating project for CConstruct config\n");
  cc_architecture_t arch = cc.architecture.create(EArchitectureX64);
  cc_platform_t platform = cc.platform.create(EPlatformDesktop);
  cc.workspace.addArchitecture(arch);
  cc.workspace.addPlatform(platform);

  cc_configuration_t configuration_debug = cc.configuration.create("Debug");
  cc.workspace.addConfiguration(configuration_debug);

  cc_project_t p = cc.project.create("cconstruct", CCProjectTypeConsoleApplication, NULL);

  const char* files[] = {strip_path(make_uri(cc_config_path))};
  cc.project.addFiles(p, countof(files), files, NULL);

  cc.workspace.setLabel("cconstruct");

  cc_default_generator("build");
}

static void cc_print_statistics_(void) {
  if (cc_total_bytes_allocated_) {
    printf("======================\nCConstruct used %u KiB of memory\n",
           (unsigned int)(cc_total_bytes_allocated_ / 1024));
  }
}

cconstruct_t cc_init(const char* in_absolute_config_file_path, int argc, const char* const* argv) {
  _internal.config_file_path = in_absolute_config_file_path;

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
    LOG_ERROR_AND_QUIT(
        ERR_CONFIGURATION,
        "You can make the file you are compiling absolute by adding $PWD/ in front of it.\n");
#endif
  }

  if (cc_data_.is_inited) {
    LOG_ERROR_AND_QUIT(ERR_CONFIGURATION,
                       "Error: calling cc_init() multiple times. Don't do this.\n");
  }
  cc_data_.is_inited = true;

  cc_default_generator =
#if defined(_MSC_VER)
      vs2019_generateInFolder;
#else
      xcode_generateInFolder;
#endif

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--verbose") == 0) {
      cc_is_verbose = true;
    }
    if (strcmp(argv[i], "--generate-projects") == 0) {
      cc_only_generate = true;
    }
    if (strcmp(argv[i], "--generate-cconstruct-project") == 0) {
      cc_only_generate       = true;
      cc_generate_cc_project = true;
    }

    // Choose generator
#if defined(_MSC_VER)
    if (strcmp(argv[i], "--generator=msvc") == 0) {
      cc_default_generator = vs2019_generateInFolder;
    }
    if (strcmp(argv[i], "--generator=ninja") == 0) {
      cc_default_generator    = ninja_generateInFolder;
      _internal.show_includes = true;
    }
#else
    if (strcmp(argv[i], "--generator=xcode") == 0) {
      cc_default_generator = xcode_generateInFolder;
    }
#endif
    if (strcmp(argv[i], "--generator=ninja") == 0) {
      cc_default_generator = ninja_generateInFolder;
    }

    {  // Check for explicit config
      const char* test_arg = "--config=";
      if (strncmp(argv[i], test_arg, strlen(test_arg)) == 0) {
        _internal.active_config = argv[i] + strlen(test_arg);
        LOG_VERBOSE("Config: %s\n", _internal.active_config);
      }
    }

    {  // Check for explicit architecture
      const char* test_arg = "--arch=";
      if (strncmp(argv[i], test_arg, strlen(test_arg)) == 0) {
        _internal.active_arch_label = argv[i] + strlen(test_arg);
        LOG_VERBOSE("Architecture: %s\n", _internal.active_arch_label);
      }
    }

    if (strcmp(argv[i], "--help") == 0) {
      printf("Options:\n");
      printf("  --verbose\n");
      printf("  --generate-projects: only generate projects, don't rebuild cconstruct binary.\n");
      printf(
          "  --generate-cconstruct-project: Don't rebuild cconstruct binary. Generate projects "
          "and generate a project to debug cconstruct itself.\n");
#if defined(_MSC_VER)
      printf("  --generator=[msvc/ninja]: generate MSVC/Ninja project.");
#else
      printf("  --generator=[xcode/ninja]: generate XCode/Ninja project.");
#endif
      exit(0);
    }
  }

  // If not only generating, then a new binary is built, that is run, and only then do we attempt
  // to clean up this existing version. This binary doesn't do any construction of projects.
  if (!cc_only_generate) {
    printf("Rebuilding CConstruct ...");
    cc_recompile_binary_(in_absolute_config_file_path);
    printf(" done\n");
    int result = cc_runNewBuild_(argv, argc);
    if (result == 0) {
      cc_activateNewBuild_();
    }
    exit(result);
  }

  if (atexit(cc_print_statistics_) != 0) {
    // Oh well, failed to install that, but don't care much as it doesn't affect operation of
    // CConstruct.
  }

  // First group is no group
  cc_group_impl_t null_group = {0};
  array_push(cc_data_.groups, null_group);

  cc_data_.base_folder = folder_path_only(in_absolute_config_file_path);

  // Keep this as a local, so that users are forced to call cc_init to get an instance the struct.
  cconstruct_t out = {
      {&cc_configuration_create},
      {&cc_architecture_create},
      {&cc_platform_create},
      {&cc_group_create},
      {&cc_state_create, &cc_state_reset, &cc_state_addIncludeFolder,
       &cc_state_addPreprocessorDefine, &cc_state_addCompilerFlag, &cc_state_addLinkerFlag,
       &cc_state_linkExternalLibrary, &cc_state_setWarningLevel,
       &cc_state_disableWarningsAsErrors},
      {
          &cc_project_create_,
          &addFilesToProject,
          &addFilesFromFolderToProject,
          &cc_project_addFileWithCustomCommand,
          &cc_project_addInputProject,  // Visual Studio terminology, add a reference
          &cc_project_addDependency,    // Visual Studio terminology, add a dependency
          &cc_project_setFlags_,
          &cc_project_addPreBuildAction,
          &cc_project_addPostBuildAction,
          &cc_project_setOutputFolder,
      },
      {&setWorkspaceLabel, &addConfiguration, &addArchitecture, &addPlatform}};

  if (cc_generate_cc_project) {
    generate_cc_project(out, in_absolute_config_file_path);
    exit(0);
  }
  return out;
}
