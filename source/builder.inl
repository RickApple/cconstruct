#if defined(_WIN32)
  #include <Windows.h>

static char stdout_data[16 * 1024 * 1024] = {0};
static char stderr_data[16 * 1024 * 1024] = {0};

// It currently used vswhere executable which should be at a fixed location.
const char* cc_find_VcDev_install_folder_() {
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

  return stdout_data;
}

// This function finds the location of the VcDevCmd.bat file on your system. This is needed to set
// the environment when compiling a new version of CConstruct binary.
const char* cc_find_VcDevCmd_bat_() {
  const char* install_path = cc_find_VcDev_install_folder_();
  #if _M_X64
  return cc_printf("%s\\VC\\Auxiliary\\Build\\vcvars64.bat", install_path);
  #else
  return cc_printf("%s\\VC\\Auxiliary\\Build\\vcvars32.bat", install_path);
  #endif
}

void cc_recompile_binary_(const char* cconstruct_config_file_path) {
  char temp_path[MAX_PATH];
  GetEnvironmentVariable("temp", temp_path, MAX_PATH);

  const char* path_abs_cconstruct = folder_path_only(cc_path_executable());
  const char* path_abs_internal_cconstruct =
      cc_printf("%s%s", path_abs_cconstruct, cconstruct_internal_binary_name);
  const char* VsDevCmd_bat      = cc_find_VcDevCmd_bat_();
  const char* recompile_command = cc_printf(
      "\"%s\" > nul && pushd %s && cl.exe "
      // Enable exception handling so can help users fix issues in their config files.
      "-EHsc "
      // Always add debug symbols so can give stack trace on exceptions.
      // Would prefer to embed them into binary, but that isn't possible. The binary will reference
      // the PDB file in the %TEMP% folder where it was built.
      "/ZI "
  #ifndef NDEBUG
      "/DEBUG "
  #endif
      "/Fe%s %s "
      "/nologo "
      "%s"  // space for /showIncludes or not
      "/INCREMENTAL:NO "
  #ifdef __cplusplus
      // Set compiler option so that the file is compiled as C or C++, depending on the compiler
      // settings used to manually compile the first CConstruct binary.
      "/TP "
  #else
      "/TC "
  #endif
      "&& popd",
      VsDevCmd_bat, temp_path, path_abs_internal_cconstruct, cconstruct_config_file_path,
      _internal.show_includes ? "/showIncludes " : "");

  LOG_VERBOSE("Compiling new version of CConstruct binary with the following command:\n'%s'\n\n",
              recompile_command);
  int exit_code = 0;
  int result    = system_np(recompile_command, 100 * 1000, stdout_data, sizeof(stdout_data),
                            stderr_data, sizeof(stderr_data), &exit_code);
  if (result != 0) {
    char* message;
    DWORD rfm = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                              result, 0, (LPSTR)&message, 1024, NULL);
    if (rfm != 0) {
      LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error recompiling with command '%s'\nError: %s",
                         recompile_command, message);
    } else {
      LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error recompiling with command '%s'\nError: %i",
                         recompile_command, result);
    }
  } else {
    if (exit_code == 0) {
      LOG_VERBOSE("Built new CConstruct binary at '%s'\n", cconstruct_internal_binary_name);
      // TODO: if Ninja,
      if (_internal.show_includes) {
        printf(stdout_data);
      }
    } else {
      // Succesfully ran the command, but there was an error, likely an issue compiling the config
      // file
      printf("stdout: %s\n", stdout_data);
      printf("stderr: %s\n", stderr_data);

      LOG_ERROR_AND_QUIT(
          ERR_COMPILING,
          "Error (%i) recompiling CConstruct config file. You can add '--generate-projects' to "
          "generate projects with the settings built into the existing CConstruct binary.\n",
          exit_code);
    }
  }

  const char* from_path = path_abs_internal_cconstruct;
  const char* to_path   = cc_printf("%s", cconstruct_internal_binary_name);

  if (!MoveFileEx(from_path, to_path, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
    char buf[256];
    int err = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (sizeof(buf) / sizeof(wchar_t)),
                   NULL);
    LOG_ERROR_AND_QUIT(ERR_COMPILING,
                       "Couldn't move internal binary from '%s' to '%s'\nError %i: '%s'\n",
                       from_path, to_path, err, buf);
  }
}

// On Windows cannot delete a binary while it is being run. It is possible to move the binary
// though (as long as it's on the same drive), so move the existing binary to a different name and
// then rename the new one to the original name.
void cc_activateNewBuild_() {
  char temp_folder_path[MAX_PATH];
  GetEnvironmentVariable("temp", temp_folder_path, MAX_PATH);

  char existing_binary_path[MAX_PATH];
  char existing_binary_name[MAX_PATH];
  if (!GetModuleFileNameA(NULL, existing_binary_path, MAX_PATH)) {
    exit(1);
  }

  strcpy(existing_binary_name, strrchr(existing_binary_path, '\\') + 1);

  if (!MoveFileEx(existing_binary_path, cconstruct_old_binary_name, MOVEFILE_REPLACE_EXISTING)) {
    LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error: Couldn't move old binary from '%s' to '%s'\n",
                       existing_binary_path, cconstruct_old_binary_name);
  } else {
    LOG_VERBOSE("Moved old binary from '%s' to '%s'\n", existing_binary_path,
                cconstruct_old_binary_name);
  }

  if (!MoveFileEx(cconstruct_internal_binary_name, existing_binary_path,
                  MOVEFILE_REPLACE_EXISTING)) {
    LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error: Couldn't move new binary from '%s' to '%s'\n",
                       cconstruct_internal_binary_name, existing_binary_path);
  } else {
    LOG_VERBOSE("Moved new binary from '%s' to '%s'\n", cconstruct_internal_binary_name,
                existing_binary_path);
  }

  // This will delete the old file after a reboot. Not great, but cleans up a little bit.
  (void)MoveFileEx(cconstruct_old_binary_name, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
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
      cc_printf("clang++ -x c++ -Wno-deprecated-declarations %s -std=c++%s -o %s",
                cconstruct_config_file_path, language_revision, cconstruct_internal_binary_name);

  #else
  const char* recompile_command =
      cc_printf("clang -x c -Wno-deprecated-declarations %s -o %s", cconstruct_config_file_path,
                cconstruct_internal_binary_name);
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
    LOG_VERBOSE("Built new CConstruct binary at '%s'\n", cconstruct_internal_binary_name);
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

  if (rename(cconstruct_internal_binary_name, path) != 0) {
    LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error: Couldn't move binary from '%s' to '%s'\n",
                       cconstruct_internal_binary_name, path);
  } else {
    LOG_VERBOSE("Moved binary from '%s' to '%s'\n", cconstruct_internal_binary_name, path);
  }
}
#endif
