#if defined(_WIN32)
#include <Windows.h>

static char stdout_data[16 * 1024 * 1024] = {0};
static char stderr_data[16 * 1024 * 1024] = {0};

// This function finds the location of the VcDevCmd.bat file on your system. This is needed to set
// the environment when compiling a new version of CConstruct binary.
// It currently used vswhere executable which should be at a fixed location.
const char* cc_find_VcDevCmd_bat_() {
  char program_files_x86_path[MAX_PATH];
  GetEnvironmentVariable("ProgramFiles(x86)", program_files_x86_path, MAX_PATH);

  int exit_code       = 0;
  const char* command = cc_printf(
      "\"%s\\Microsoft Visual Studio\\Installer\\vswhere\" -latest -property "
      "installationPath",
      program_files_x86_path);
  int r = system_np(command, 100 * 1000, stdout_data, sizeof(stdout_data), stderr_data,
                    sizeof(stderr_data), &exit_code);
  if (r != 0) {
    // Not much to do at this point
    LOG_ERROR_AND_QUIT(
        "Couldn't rebuild cconstruct exe, please do so manually if you changed the "
        "configuration file.\n");
  }

  // Remove new line from stdout_data
  char* s = stdout_data;
  while (*s) {
    if (*s == '\n' || *s == '\r') {
      *s = 0;
      break;
    }
    s++;
  }

  return cc_printf("%s\\Common7\\Tools\\VsDevCmd.bat", stdout_data);
}

void cc_recompile_binary_(const char* cconstruct_config_file_path) {
  char temp_path[MAX_PATH];
  GetEnvironmentVariable("temp", temp_path, MAX_PATH);

  const char* VsDevCmd_bat                        = cc_find_VcDevCmd_bat_();
  const char* cconstruct_internal_build_file_path = "cconstruct_internal_build.exe";
  const char* recompile_command                   = cc_printf(
      "\"%s\" > nul && cl.exe -EHsc "
      "/Fo%s\\cconstruct.obj "
      "/Fe%s %s "
      "/nologo ",
      VsDevCmd_bat, temp_path, cconstruct_internal_build_file_path, cconstruct_config_file_path);

  LOG_VERBOSE("Compiling new version of CConstruct binary with the following command: '%s'\n",
              recompile_command);
  int exit_code = 0;
  int result    = system_np(recompile_command, 100 * 1000, stdout_data, sizeof(stdout_data),
                         stderr_data, sizeof(stderr_data), &exit_code);

  if (result != 0) {
    char* message;
    DWORD rfm = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                              result, 0, (LPSTR)&message, 1024, NULL);
    if (rfm != 0) {
      LOG_ERROR_AND_QUIT("Error recompiling with command '%s'\nError: %s", recompile_command,
                         message);
    } else {
      LOG_ERROR_AND_QUIT("Error recompiling with command '%s'\nError: %i", recompile_command,
                         result);
    }
  } else {
    if (exit_code == 0) {
      LOG_VERBOSE("Built new CConstruct binary at '%s'\n", cconstruct_internal_build_file_path);
    } else {
      // Succesfully ran the command, but there was an error, likely an issue compiling the config
      // file
      printf("%s\n", stdout_data);

      LOG_ERROR_AND_QUIT(
          "Error recompiling CConstruct config file. You can add '%s' to generate projects "
          "with the settings built into the existing CConstruct binary.\n",
          RA_CMDLINE_RESTART_PROCESS);
    }
  }
}

void cc_swapBuilds_() {
  char path[MAX_PATH];
  if (!GetModuleFileNameA(NULL, path, MAX_PATH)) exit(1);
  const char* old_path = cc_printf("%s.old", path);

  // Moving only works if the target doesn't exist yet
  if (!MoveFileA(path, old_path)) {
    LOG_ERROR_AND_QUIT("Error: Couldn't move binary from '%s' to '%s'\n", path, old_path);
  } else {
    LOG_VERBOSE("Moved binary from '%s' to '%s'\n", path, old_path);
  }

  const char* internal_build_path = "cconstruct_internal_build.exe";
  if (!MoveFileA(internal_build_path, path)) {
    LOG_ERROR_AND_QUIT("Error: Couldn't move binary from '%s' to '%s'\n", internal_build_path,
                       path);
  } else {
    LOG_VERBOSE("Moved binary from '%s' to '%s'\n", internal_build_path, path);
  }
}

void cc_deletePreviousBuild_() {
  char path[MAX_PATH];
  if (!GetModuleFileNameA(NULL, path, MAX_PATH)) exit(1);
  const char* old_path = cc_printf("%s.old", path);
  if (!DeleteFileA(old_path)) {
    DWORD last_error = GetLastError();
    if (last_error != ERROR_FILE_NOT_FOUND) {
      LOG_VERBOSE("Couldn't delete old binary '%s'\n", old_path);
    }
  } else {
    LOG_VERBOSE("Deleted binary at '%s'\n", old_path);
  }
}

void cc_autoRecompileFromConfig(const char* config_file_path, int argc, const char* argv[]) {
  cc_deletePreviousBuild_();
  if (RA_CheckForRestartProcessStart()) {
    RA_WaitForPreviousProcessFinish();
  } else {
    cc_recompile_binary_(config_file_path);
    cc_swapBuilds_();
    printf("Rebuilt CConstruct binary\n");
    RA_ActivateRestartProcess();
    exit(0);
  }
}
#elif defined(__APPLE__)
#include <stdio.h>
void cc_recompile_binary_(const char* cconstruct_config_file_path) {
  const char* cconstruct_internal_build_file_path = "cconstruct_internal_build";
  const char* compile_command = cc_printf("clang %s -o %s", cconstruct_config_file_path,
                                          cconstruct_internal_build_file_path);

  FILE* pipe = popen(compile_command, "r");

  if (!pipe) {
    LOG_ERROR_AND_QUIT("popen(%s) failed!", compile_command);
  }

  char buffer[128];
  const char* result = "";

  while (!feof(pipe)) {
    if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
      result = cc_printf("%s%s", result, buffer);
    }
  }

  int rc = pclose(pipe);

  if (rc == EXIT_SUCCESS) {  // == 0
    LOG_VERBOSE("Built new CConstruct binary at '%s'\n", cconstruct_internal_build_file_path);
  } else {
    // It looks like the errors have already been printed to the console, so no need to to do that
    // manually.
    LOG_ERROR_AND_QUIT(
        "Error recompiling CConstruct config file. You can add '%s' to generate projects "
        "with the settings built into the existing CConstruct binary.\n",
        RA_CMDLINE_RESTART_PROCESS);
  }
}

void cc_swapBuilds_() {
  const char* path = getprogname();

  const char* internal_build_path = "cconstruct_internal_build";
  if (rename(internal_build_path, path) != 0) {
    LOG_ERROR_AND_QUIT("Error: Couldn't move binary from '%s' to '%s'\n", internal_build_path,
                       path);
  } else {
    LOG_VERBOSE("Moved binary from '%s' to '%s'\n", internal_build_path, path);
  }
}

bool cc_should_generate_projects(int argc, const char* argv[]) {
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], RA_CMDLINE_RESTART_PROCESS) == 0) {
      return true;
    }
  }
  return false;
}

void cc_autoRecompileFromConfig(const char* config_file_path, int argc, const char* argv[]) {
  if (cc_should_generate_projects(argc, argv)) {
    // Do nothing
  } else {
    cc_recompile_binary_(config_file_path);
    cc_swapBuilds_();
    printf("Rebuilt CConstruct binary\n");
    system(cc_printf("%s --generate-projects", argv[0]));
    exit(0);
  }
}
#else
void cc_autoRecompileFromConfig(const char* config_file_path, int argc, const char* argv[]) {
  fprintf(stderr, "Auto recompile not implemented on this platform\n");
}
#endif