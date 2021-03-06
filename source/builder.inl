#if defined(_WIN32)
#include <Windows.h>

static char stdout_data[16 * 1024 * 1024] = {0};
static char stderr_data[16 * 1024 * 1024] = {0};

const char* cconstruct_internal_build_file_name = "cconstruct_internal_build";

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
    LOG_ERROR_AND_QUIT(ERR_COMPILING,
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

  const char* VsDevCmd_bat      = cc_find_VcDevCmd_bat_();
  const char* recompile_command = cc_printf(
      "\"%s\" > nul && cl.exe -EHsc "
#ifndef NDEBUG
      "/ZI /DEBUG"
#endif
      "/Fo%s\\cconstruct.obj "
      "/Fe%s.exe %s "
      "/nologo "
#ifdef __cplusplus
      "/TP "
#else
      "/TC "
#endif
      ,
      VsDevCmd_bat, temp_path, cconstruct_internal_build_file_name, cconstruct_config_file_path);

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
      LOG_ERROR_AND_QUIT(ERR_COMPILING,"Error recompiling with command '%s'\nError: %s", recompile_command,
                         message);
    } else {
      LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error recompiling with command '%s'\nError: %i",
                         recompile_command, result);
    }
  } else {
    if (exit_code == 0) {
      LOG_VERBOSE("Built new CConstruct binary at '%s.exe'\n",
                  cconstruct_internal_build_file_name);
    } else {
      // Succesfully ran the command, but there was an error, likely an issue compiling the config
      // file
      printf("%s\n", stdout_data);

      LOG_ERROR_AND_QUIT(
          ERR_COMPILING,
          "Error (%i) recompiling CConstruct config file. You can add '--generate-projects' to "
          "generate projects "
          "with the settings built into the existing CConstruct binary.\n",
          exit_code);
    }
  }
}

void cc_swapBuilds_() {
  char path[MAX_PATH];
  if (!GetModuleFileNameA(NULL, path, MAX_PATH)) exit(1);
  const char* old_path = cc_printf("%s.old", path);

  // Moving only works if the target doesn't exist yet
  if (!MoveFileA(path, old_path)) {
    LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error: Couldn't move binary from '%s' to '%s'\n", path,
                       old_path);
  } else {
    LOG_VERBOSE("Moved binary from '%s' to '%s'\n", path, old_path);
  }

  const char* internal_build_path = cc_printf("%s.exe", cconstruct_internal_build_file_name);
  if (!MoveFileA(internal_build_path, path)) {
    LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error: Couldn't move binary from '%s' to '%s'\n",
                       internal_build_path, path);
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

void cc_activateNewBuild_() {
  cc_deletePreviousBuild_();
  cc_swapBuilds_();
}

#elif defined(__APPLE__)
#include <stdio.h>
void cc_recompile_binary_(const char* cconstruct_config_file_path) {
#if __cplusplus
  const char* language_revision =
#if (__cplusplus > 201700)
      "17";
#elif (__cplusplus > 201400)
      "14";
#elif (__cplusplus > 201100)
      "11";
#else
      "98";
#endif
  const char* recompile_command =
      cc_printf("clang++ %s -x c++ -std=c++%s -o %s", cconstruct_config_file_path,
                language_revision, cconstruct_internal_build_file_name);

#else
  const char* recompile_command = cc_printf("clang %s -x c -o %s", cconstruct_config_file_path,
                                            cconstruct_internal_build_file_name);
#endif
  LOG_VERBOSE("Compiling new version of CConstruct binary with the following command: '%s'\n",
              recompile_command);

  FILE* pipe = popen(recompile_command, "r");

  if (!pipe) {
    LOG_ERROR_AND_QUIT(ERR_COMPILING, "popen(%s) failed!", recompile_command);
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
    LOG_VERBOSE("Built new CConstruct binary at '%s'\n", cconstruct_internal_build_file_name);
  } else {
    // It looks like the errors have already been printed to the console, so no need to to do that
    // manually.
    LOG_ERROR_AND_QUIT(
        ERR_COMPILING,
        "Error (%i) recompiling CConstruct config file. You can add '--generate-projects' to "
        "generate projects "
        "with the settings built into the existing CConstruct binary.\n",
        rc);
  }
}

void cc_activateNewBuild_() {
  const char* path = getprogname();

  if (rename(cconstruct_internal_build_file_name, path) != 0) {
    LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error: Couldn't move binary from '%s' to '%s'\n",
                       cconstruct_internal_build_file_name, path);
  } else {
    LOG_VERBOSE("Moved binary from '%s' to '%s'\n", cconstruct_internal_build_file_name, path);
  }
}

bool cc_should_generate_projects(int argc, const char* const* argv) {
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], RA_CMDLINE_RESTART_PROCESS) == 0) {
      return true;
    }
  }
  return false;
}
#endif
