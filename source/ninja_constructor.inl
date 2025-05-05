char const* const project_type_suffix[] = {".exe", ".exe", ".lib", ".dll"};

char const* compiler_path =
    "C:\\PROGRA~1\\MICROS~3\\2022\\COMMUN~1\\VC\\Tools\\Llvm\\x64\\bin\\clang-cl.exe";
char const* lib_linker_path =
    "C:\\PROGRA~1\\MICROS~3\\2022\\COMMUN~1\\VC\\Tools\\Llvm\\x64\\bin\\lld-link.exe";
char const* linker_path =
    "C:\\PROGRA~1\\MICROS~3\\2022\\COMMUN~1\\VC\\Tools\\Llvm\\x64\\bin\\lld-link.exe";

const char* ninja_generateUUID() {
  static size_t count = 0;
  ++count;

  char* buffer = (char*)cc_alloc_(37);
  sprintf(buffer, "00000000-0000-0000-0000-%012zi", count);
  return buffer;
};

const char* ninja_findUUIDForProject(const char** uuids, const cc_project_impl_t* project) {
  unsigned i = 0;
  while (cc_data_.projects[i] != project) {
    ++i;
  }

  return uuids[i];
}

const char* ninja_projectArch2String_(EArchitecture arch) {
  switch (arch) {
    case EArchitectureX86:
      return "Win32";
    case EArchitectureX64:
      return "x64";
    case EArchitectureARM:
      return "ARM";
  }
  return "";
}

void ninja_replaceForwardSlashWithBackwardSlashInPlace(char* in_out) {
  if (in_out == 0) return;

  char* in_out_start = in_out;

  while (*in_out) {
    if (*in_out == '/') {
      // On Windows commands may use / for flags (eg xcopy ... /s). Don't want to change those.
      if (in_out == in_out_start || *(in_out - 1) != ' ') {
        *in_out = '\\';
      }
    }
    in_out++;
  }
}

typedef struct ninja_compiler_setting {
  const char* key;
  const char* value;
} ninja_compiler_setting;

void ninja_createFilters(const cc_project_impl_t* in_project, const char* in_output_folder) {
  const char* projectfilters_file_path = cc_printf("%s.vcxproj.filters", in_project->name);

  struct data_tree_t dt = data_tree_api.create();

  unsigned int project = data_tree_api.create_object(&dt, 0, "Project");
  data_tree_api.set_object_parameter(&dt, project, "ToolsVersion", "4.0");
  data_tree_api.set_object_parameter(&dt, project, "xmlns",
                                     "http://schemas.microsoft.com/developer/msbuild/2003");

  // Create list of groups needed.
  const cc_project_impl_t* p = (cc_project_impl_t*)in_project;
  bool* groups_needed        = (bool*)cc_alloc_(array_count(cc_data_.groups) * sizeof(bool));
  memset(groups_needed, 0, array_count(cc_data_.groups) * sizeof(bool));
  for (size_t fi = 0; fi < array_count(p->file_data); fi++) {
    size_t gi = p->file_data[fi]->parent_group_idx;
    while (gi) {
      groups_needed[gi] = true;
      gi                = cc_data_.groups[gi].parent_group_idx;
    }
  }
  for (size_t fi = 0; fi < array_count(p->file_data_custom_command); fi++) {
    size_t gi = p->file_data_custom_command[fi]->parent_group_idx;
    while (gi) {
      groups_needed[gi] = true;
      gi                = cc_data_.groups[gi].parent_group_idx;
    }
  }

  // Create names for the needed groups. Nested groups append their name in the filter file
  //    Group A
  //    Group A\Nested Group
  //    Group B
  const char** unique_group_names = {0};
  for (unsigned gi = 0; gi < array_count(cc_data_.groups); gi++) {
    const char* name = "";
    if (groups_needed[gi]) {
      const cc_group_impl_t* g = &cc_data_.groups[gi];
      name                     = g->name[0] ? g->name : "<group>";
      while (g->parent_group_idx) {
        g    = &cc_data_.groups[g->parent_group_idx];
        name = cc_printf("%s\\%s", g->name[0] ? g->name : "<group>", name);
      }
    }
    array_push(unique_group_names, name);
  }

  unsigned int itemgroup = data_tree_api.create_object(&dt, project, "ItemGroup");
  for (unsigned gi = 0; gi < array_count(cc_data_.groups); gi++) {
    if (groups_needed[gi]) {
      const char* group_name = unique_group_names[gi];
      unsigned int filter    = data_tree_api.create_object(&dt, itemgroup, "Filter");
      data_tree_api.set_object_parameter(&dt, filter, "Include", group_name);
      unsigned int uid = data_tree_api.create_object(&dt, filter, "UniqueIdentifier");
      data_tree_api.set_object_value(&dt, uid, ninja_generateUUID());
    }
  }

  for (unsigned fi = 0; fi < array_count(p->file_data); ++fi) {
    const struct cc_file_t_* file = p->file_data[fi];

    itemgroup                      = data_tree_api.create_object(&dt, project, "ItemGroup");
    const char* f                  = file->path;
    const size_t gi                = file->parent_group_idx;
    const char* group_name         = NULL;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
    group_name = unique_group_names[gi];

    unsigned int g = 0;

    if (is_header_file(f)) {
      g = data_tree_api.create_object(&dt, itemgroup, "ClInclude");
    } else if (is_source_file(f)) {
      g = data_tree_api.create_object(&dt, itemgroup, "ClCompile");
    } else {
      g = data_tree_api.create_object(&dt, itemgroup, "None");
    }
    data_tree_api.set_object_parameter(&dt, g, "Include", relative_file_path);
    unsigned int fil = data_tree_api.create_object(&dt, g, "Filter");
    data_tree_api.set_object_value(&dt, fil, group_name);
  }

  for (unsigned fi = 0; fi < array_count(p->file_data_custom_command); ++fi) {
    const struct cc_file_custom_command_t_* file = p->file_data_custom_command[fi];

    const char* f                  = file->path;
    const size_t gi                = file->parent_group_idx;
    const char* group_name         = NULL;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
    group_name = unique_group_names[gi];

    itemgroup       = data_tree_api.create_object(&dt, project, "ItemGroup");
    unsigned int cb = data_tree_api.create_object(&dt, itemgroup, "CustomBuild");
    data_tree_api.set_object_parameter(&dt, cb, "Include", relative_file_path);
    unsigned int fil = data_tree_api.create_object(&dt, cb, "Filter");
    data_tree_api.set_object_value(&dt, fil, group_name);
  }

  FILE* filter_file = fopen(projectfilters_file_path, "wb");
  fprintf(filter_file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  // ninja export_tree_as_xml(filter_file, &dt, 0, 0);
  fclose(filter_file);
}

typedef void (*func)(struct data_tree_t* dt, unsigned int compile_group,
                     const char* remaining_flag_value);

typedef struct ninja_compiler_flag {
  const char* flag;
  const func action;
} ninja_compiler_flag;

void optionEmpty(struct data_tree_t* dt, unsigned int compile_group,
                 const char* remaining_flag_value) {
  (void)dt;
  (void)compile_group;
  (void)remaining_flag_value;
}

const ninja_compiler_flag ninja_known_compiler_flags[] = {
    {"/Zi", &optionEmpty},  {"/ZI", &optionEmpty}, {"/MT", &optionEmpty},
    {"/MTd", &optionEmpty}, {"/MD", &optionEmpty}, {"/MDd", &optionEmpty}};
const ninja_compiler_flag ninja_known_linker_flags[] = {{"/PDB:", &optionEmpty}};

void ninja_search_platform_toolset_version(const char* directory, char* platform_toolset_version,
                                           const size_t length) {
  WIN32_FIND_DATA findFileData;
  HANDLE hFind              = INVALID_HANDLE_VALUE;
  char subdirPath[MAX_PATH] = {0};
  char searchPath[MAX_PATH] = {0};

  // Prepare the search pattern
  snprintf(searchPath, sizeof(searchPath), "%s\\*", directory);

  // Start search
  hFind = FindFirstFile(searchPath, &findFileData);
  if (hFind == INVALID_HANDLE_VALUE) {
    printf("FindFirstFile failed (%lu)\n", GetLastError());
    return;
  }

  do {
    // Skip "." and ".." directories
    if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
      continue;

    // Check if it is a directory
    if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      // printf("Checking folder %s\\%s\n", directory, findFileData.cFileName);
      //  Check if the directory name matches pattern "VC/v???"
      if (strstr(directory, "PlatformToolsets") != NULL && strlen(findFileData.cFileName) == 4 &&
          findFileData.cFileName[0] == 'v') {
        int res = strcmp(platform_toolset_version, findFileData.cFileName);
        if (res < 0) {
          strncpy(platform_toolset_version, findFileData.cFileName, length - 1);
        }
      }

      // Prepare the path for the next search
      snprintf(subdirPath, sizeof(subdirPath), "%s\\%s", directory, findFileData.cFileName);
      ninja_search_platform_toolset_version(subdirPath, platform_toolset_version,
                                            length);  // Recursively search this directory
    }
  } while (FindNextFile(hFind, &findFileData) != 0);

  FindClose(hFind);
}

void ninja_createProjectFile(FILE* ninja_file, const cc_project_impl_t* p, const char* project_id,
                             const char** project_ids, const char* in_output_folder,
                             const char* build_to_base_path) {
  (void)project_ids;
  (void)project_id;
  (void)in_output_folder;

  const char* substitution_keys[]   = {"configuration", "platform"};
  const char* substitution_values[] = {_internal.active_config, "$(Platform)"};

#if 0
  const char* project_file_path = cc_printf("%s.vcxproj", p->name);

  struct data_tree_t dt = data_tree_api.create();
  unsigned int project  = data_tree_api.create_object(&dt, 0, "Project");
  data_tree_api.set_object_parameter(&dt, project, "DefaultTargets", "Build");
  data_tree_api.set_object_parameter(&dt, project, "ToolsVersion", "15.0");
  data_tree_api.set_object_parameter(&dt, project, "xmlns",
                                     "http://schemas.microsoft.com/developer/msbuild/2003");

  {
    unsigned int itemgroup = data_tree_api.create_object(&dt, project, "ItemGroup");
    data_tree_api.set_object_parameter(&dt, itemgroup, "Label", "ProjectConfigurations");
    for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
      const char* c = cc_data_.configurations[ci]->label;
      for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
        const char* platform_label = ninja_projectArch2String_(cc_data_.architectures[pi]->type);

        unsigned int pc = data_tree_api.create_object(&dt, itemgroup, "ProjectConfiguration");
        data_tree_api.set_object_parameter(&dt, pc, "Include",
                                           cc_printf("%s|%s", c, platform_label));
        unsigned int cobj = data_tree_api.create_object(&dt, pc, "Configuration");
        data_tree_api.set_object_value(&dt, cobj, c);
        unsigned int platformobj = data_tree_api.create_object(&dt, pc, "Platform");
        data_tree_api.set_object_value(&dt, platformobj, platform_label);
      }
    }
  }

  {
    unsigned int pg = data_tree_api.create_object(&dt, project, "PropertyGroup");
    data_tree_api.set_object_parameter(&dt, pg, "Label", "Globals");
    data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pg, "VCProjectVersion"),
                                   "15.0");
    data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pg, "ProjectGuid"),
                                   cc_printf("{%s}", project_id));
    data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pg, "Keyword"),
                                   "Win32Proj");
    data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pg, "RootNamespace"),
                                   "builder");
    data_tree_api.set_object_value(
        &dt, data_tree_api.create_object(&dt, pg, "WindowsTargetPlatformVersion"), "10.0");

    data_tree_api.set_object_parameter(&dt, data_tree_api.create_object(&dt, project, "Import"),
                                       "Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
  }

  char platform_toolset_version[5] = {0};
  ninja_search_platform_toolset_version(cc_find_VcDev_install_folder_(), platform_toolset_version,
                                        countof(platform_toolset_version));

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const char* platform_label = ninja_projectArch2String_(cc_data_.architectures[pi]->type);
      unsigned int pg            = data_tree_api.create_object(&dt, project, "PropertyGroup");
      data_tree_api.set_object_parameter(
          &dt, pg, "Condition",
          cc_printf("'$(Configuration)|$(Platform)' == '%s|%s'", c, platform_label));
      data_tree_api.set_object_parameter(&dt, pg, "Label", "Configuration");

      unsigned int ct      = data_tree_api.create_object(&dt, pg, "ConfigurationType");
      const char* ct_value = NULL;
      switch (p->type) {
        case CCProjectTypeConsoleApplication:  // Intentional fallthrough
        case CCProjectTypeWindowedApplication:
          ct_value = "Application";
          break;
        case CCProjectTypeStaticLibrary:
          ct_value = "StaticLibrary";
          break;
        case CCProjectTypeDynamicLibrary:
          ct_value = "DynamicLibrary";
          break;
      }
      data_tree_api.set_object_value(&dt, ct, ct_value);

      data_tree_api.set_object_value(
          &dt, data_tree_api.create_object(&dt, pg, "UseDebugLibraries"), "true");

      data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pg, "PlatformToolset"),
                                     platform_toolset_version);
      data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pg, "CharacterSet"),
                                     "MultiByte");
    }
  }

  data_tree_api.set_object_parameter(&dt, data_tree_api.create_object(&dt, project, "Import"),
                                     "Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
  data_tree_api.set_object_parameter(&dt, data_tree_api.create_object(&dt, project, "ImportGroup"),
                                     "Label", "ExtensionSettings");
  data_tree_api.set_object_parameter(&dt, data_tree_api.create_object(&dt, project, "ImportGroup"),
                                     "Label", "Shared");

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const char* platform_label = ninja_projectArch2String_(cc_data_.architectures[pi]->type);
      unsigned int importgroup   = data_tree_api.create_object(&dt, project, "ImportGroup");
      data_tree_api.set_object_parameter(&dt, importgroup, "Label", "PropertySheets");
      data_tree_api.set_object_parameter(
          &dt, importgroup, "Condition",
          cc_printf("'$(Configuration)|$(Platform)' == '%s|%s'", c, platform_label));
      unsigned int import_obj = data_tree_api.create_object(&dt, importgroup, "Import");
      data_tree_api.set_object_parameter(&dt, import_obj, "Project",
                                         "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
      data_tree_api.set_object_parameter(
          &dt, import_obj, "Condition",
          "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
      data_tree_api.set_object_parameter(&dt, import_obj, "Label", "LocalAppDataPlatform");
    }
  }

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const char* platform_label = ninja_projectArch2String_(cc_data_.architectures[pi]->type);
      const char* resolved_output_folder = cc_substitute(
          p->outputFolder, substitution_keys, substitution_values, countof(substitution_keys));
      ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)resolved_output_folder);

      unsigned int pg = data_tree_api.create_object(&dt, project, "PropertyGroup");
      data_tree_api.set_object_parameter(&dt, pg, "Label", "UserMacros");
      pg = data_tree_api.create_object(&dt, project, "PropertyGroup");
      data_tree_api.set_object_parameter(
          &dt, pg, "Condition",
          cc_printf("'$(Configuration)|$(Platform)'=='%s|%s'", c, platform_label));

      // TODO: fix detection of type of config
      bool is_debug_build = (strcmp(c, "Debug") == 0);
      data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pg, "LinkIncremental"),
                                     is_debug_build ? "true" : "false");

      // Because users can add post-build commands, make sure these custom build commands have
      // executed before then by doing it after BuildCompile instead of after Build.
      data_tree_api.set_object_value(
          &dt, data_tree_api.create_object(&dt, pg, "CustomBuildAfterTargets"), "BuildCompile");
      data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pg, "OutDir"),
                                     cc_printf("$(SolutionDir)\\%s\\", resolved_output_folder));

      // VS2019 warns if multiple projects have the same intermediate directory, so avoid that
      // here
      data_tree_api.set_object_value(
          &dt, data_tree_api.create_object(&dt, pg, "IntDir"),
          cc_printf("$(SolutionDir)\\%s\\Intermediate\\$(ProjectName)\\", resolved_output_folder));
    }
  }

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const cc_configuration_impl_t* config = cc_data_.configurations[ci];
    const char* configuration_label       = config->label;
    const bool is_debug_build             = (strcmp(configuration_label, "Debug") == 0);
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const cc_architecture_impl_t* arch = cc_data_.architectures[pi];

      unsigned int idg = data_tree_api.create_object(&dt, project, "ItemDefinitionGroup");
      const char* platform_label = ninja_projectArch2String_(arch->type);
      data_tree_api.set_object_parameter(&dt, idg, "Condition",
                                         cc_printf("'$(Configuration)|$(Platform)'=='%s|%s'",
                                                   configuration_label, platform_label));
      unsigned int compile_obj = data_tree_api.create_object(&dt, idg, "ClCompile");
      unsigned int link_obj    = data_tree_api.create_object(&dt, idg, "Link");

      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "PrecompiledHeader"),
          "NotUsing");
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "SDLCheck"), "true");
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "ConformanceMode"), "true");

      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, link_obj, "SubSystem"),
          ((p->type == CCProjectTypeWindowedApplication) ? "Windows" : "Console"));
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, link_obj, "GenerateDebugInformation"),
          "true");

      if (is_debug_build) {
        data_tree_api.set_object_value(
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "RuntimeLibrary"),
            "MultiThreadedDebugDLL");
      } else {
        data_tree_api.set_object_value(
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "RuntimeLibrary"),
            "MultiThreadedDLL");
      }

      if (is_debug_build) {
        data_tree_api.set_object_value(
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "Optimization"), "Disabled");
      } else {
        data_tree_api.set_object_value(
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "Optimization"), "MaxSpeed");
        data_tree_api.set_object_value(
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "BasicRuntimeChecks"),
            "Default");
      }
#endif

  const char* preprocessor_defines = "";
  switch (p->type) {
    case CCProjectTypeConsoleApplication:
      preprocessor_defines = cc_printf("%s /D \"%s\"", preprocessor_defines, "_CONSOLE");
      break;
    case CCProjectTypeWindowedApplication:
      preprocessor_defines = cc_printf("%s /D \"%s\"", preprocessor_defines, "_WINDOWS");
      break;
    case CCProjectTypeStaticLibrary:
      preprocessor_defines = cc_printf("%s /D \"%s\"", preprocessor_defines, "_LIB");
      break;
    case CCProjectTypeDynamicLibrary: {
      preprocessor_defines =
          cc_printf("%s /D \"%s%s\"", preprocessor_defines, str_strip_spaces(p->name), "_EXPORTS");
    } break;
    default:
      LOG_ERROR_AND_QUIT(ERR_CONFIGURATION, "Unknown project type for project '%s'\n", p->name);
  }

  const char* additional_compiler_flags   = "";
  const char* additional_link_flags       = "";
  const char** additional_include_folders = NULL;
  const char* includes_command            = "";
  // const char** link_additional_directories = NULL;
  // const char* link_additional_dependencies = "";

  // EStateWarningLevel combined_warning_level = EStateWarningLevelDefault;
  // bool shouldDisableWarningsAsError         = false;
  for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
    const cc_state_impl_t* flags = &(p->state[ipc]);

    // TODO ordering and combination so that more specific flags can override general ones
    if ((p->configs[ipc] != NULL) &&
        (strcmp(p->configs[ipc]->label, _internal.active_config) != 0))
      continue;
#if 0
        if ((p->architectures[ipc] != arch) && (p->architectures[ipc] != NULL)) continue;

        shouldDisableWarningsAsError = flags->disableWarningsAsErrors;
        combined_warning_level       = flags->warningLevel;
#endif
    for (unsigned pdi = 0; pdi < array_count(flags->defines); ++pdi) {
      preprocessor_defines = cc_printf("%s /D \"%s\"", preprocessor_defines, flags->defines[pdi]);
    }
    for (unsigned cfi = 0; cfi < array_count(flags->compile_options); ++cfi) {
      const char* current_flag  = flags->compile_options[cfi];
      additional_compiler_flags = cc_printf("%s %s", additional_compiler_flags, current_flag);
    }
    for (unsigned lfi = 0; lfi < array_count(flags->link_options); ++lfi) {
      const char* current_flag = flags->link_options[lfi];
      additional_link_flags    = cc_printf("%s %s", additional_link_flags, current_flag);
    }

    for (unsigned ifi = 0; ifi < array_count(flags->include_folders); ++ifi) {
      array_push(additional_include_folders, flags->include_folders[ifi]);
    }
#if 0
        for (unsigned di = 0; di < array_count(flags->external_libs); di++) {
          const char* lib_path_from_base = flags->external_libs[di];
          const bool is_absolute_path =
              (lib_path_from_base[0] == '/') || (lib_path_from_base[1] == ':');
          const bool starts_with_env_variable = (lib_path_from_base[0] == '$');
          if (!is_absolute_path && !starts_with_env_variable) {
            lib_path_from_base = cc_printf("%s%s", build_to_base_path, lib_path_from_base);
          }

          const char* relative_lib_path = make_path_relative(in_output_folder, lib_path_from_base);
          const char* lib_name          = strip_path(relative_lib_path);
          const char* lib_folder        = make_uri(folder_path_only(relative_lib_path));
          const char* resolved_lib_folder = cc_substitute(
              lib_folder, substitution_keys, substitution_values, countof(substitution_keys));

          ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)resolved_lib_folder);
          link_additional_dependencies =
              cc_printf("%s;%s", link_additional_dependencies, lib_name);
          // Check if this path is already in there
          bool did_find = false;
          for (size_t ilf = 0; ilf < array_count(link_additional_directories); ilf++) {
            if (strcmp(link_additional_directories[ilf], resolved_lib_folder) == 0)
              did_find = true;
          }
          if (!did_find) {
            array_push(link_additional_directories, resolved_lib_folder);
          }
        }
      }

      {
        const char* warning_strings[] = {"Level4", "Level3", "Level2", "EnableAllWarnings",
                                         "TurnOffAllWarnings"};
        assert(EStateWarningLevelHigh == 0);
        assert(EStateWarningLevelAll == 3);
        assert(EStateWarningLevelNone == 4);
        data_tree_api.set_object_value(
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "WarningLevel"),
            warning_strings[combined_warning_level]);
      }

      // Disable unreferenced parameter warning
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "DisableSpecificWarnings"),
          "4100");

      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "TreatWarningAsError"),
          shouldDisableWarningsAsError ? "false" : "true");

      if (is_debug_build) {
        preprocessor_defines = cc_printf("_DEBUG;%s", preprocessor_defines);
      } else {
        preprocessor_defines = cc_printf("NDEBUG;%s", preprocessor_defines);
      }
      const bool is_win32 = (cc_data_.architectures[pi]->type == EArchitectureX86);
      if (is_win32) {
        preprocessor_defines = cc_printf("WIN32;%s", preprocessor_defines);
      }

      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "PreprocessorDefinitions"),
          preprocessor_defines);
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "AdditionalOptions"),
          additional_compiler_flags);
#endif

    if (array_count(additional_include_folders) != 0) {
      // Put later folders before earlier ones
      // RATODO: why put later ones before earlier ones?
      const char* combined_include_folders = "";
      for (size_t i = 0; i < array_count(additional_include_folders); i++) {
        ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)additional_include_folders[i]);
        combined_include_folders =
            cc_printf("%s /I\"%s\"", combined_include_folders, additional_include_folders[i]);
      }
      includes_command = combined_include_folders;
    }

#if 0
    data_tree_api.set_object_value(
        &dt, data_tree_api.get_or_create_object(&dt, link_obj, "AdditionalOptions"),
        additional_link_flags);

    link_additional_dependencies =
        cc_printf("%s;%%(AdditionalDependencies)", link_additional_dependencies);

    if (array_count(link_additional_directories) != 0) {
      // Put later folders in front of earlier folders
      const char* combined_link_folders = link_additional_directories[0];
      for (size_t i = 1; i < array_count(link_additional_directories); i++) {
        combined_link_folders =
            cc_printf("%s;%s", link_additional_directories[i], combined_link_folders);
      }
      ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)additional_include_folders);
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, link_obj, "AdditionalLibraryDirectories"),
          combined_link_folders);
    }
    data_tree_api.set_object_value(
        &dt, data_tree_api.get_or_create_object(&dt, link_obj, "AdditionalDependencies"),
        link_additional_dependencies);

    const bool have_pre_build_action = (p->preBuildAction != 0);
    if (have_pre_build_action) {
      const char* windowsPreBuildAction = cc_printf("%s", p->preBuildAction);
      windowsPreBuildAction             = cc_substitute(windowsPreBuildAction, substitution_keys,
                                                        substitution_values, countof(substitution_keys));
      ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)windowsPreBuildAction);
      unsigned int pbe = data_tree_api.create_object(&dt, idg, "PreBuildEvent");
      data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pbe, "Command"),
                                     windowsPreBuildAction);
    }
    const bool have_post_build_action = (p->postBuildAction != 0);
    if (have_post_build_action) {
      const char* windowsPostBuildAction = cc_printf("%s", p->postBuildAction);
      windowsPostBuildAction             = cc_substitute(windowsPostBuildAction, substitution_keys,
                                                         substitution_values, countof(substitution_keys));
      ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)windowsPostBuildAction);
      unsigned int pbe = data_tree_api.create_object(&dt, idg, "PostBuildEvent");
      data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pbe, "Command"),
                                     windowsPostBuildAction);
    }
  }
#endif
  }

  const bool have_post_build_action = (p->postBuildAction != 0);
  if (have_post_build_action) {
    const char* post_build_action = cc_printf("%s", p->postBuildAction);
    post_build_action = cc_substitute(post_build_action, substitution_keys, substitution_values,
                                      countof(substitution_keys));
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)post_build_action);

    fprintf(ninja_file, "rule postbuild_%i_%s\n", 0, p->name);
    fprintf(ninja_file, "  command = %s\n", post_build_action);
    fprintf(ninja_file, "  description = Running post-build action\n");
  }

#if 0
  for (unsigned fi = 0; fi < array_count(p->file_data); ++fi) {
    const char* f                  = p->file_data[fi]->path;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
    unsigned int itemgroup = data_tree_api.create_object(&dt, project, "ItemGroup");
    unsigned int obj       = 0;
    if (is_header_file(f)) {
      obj = data_tree_api.create_object(&dt, itemgroup, "ClInclude");
    } else if (is_source_file(f)) {
      obj             = data_tree_api.create_object(&dt, itemgroup, "ClCompile");
      unsigned int ca = data_tree_api.create_object(&dt, obj, "CompileAs");
      data_tree_api.set_object_value(
          &dt, ca, ((strstr(f, ".cpp") != NULL) ? "CompileAsCpp" : "CompileAsC"));
    } else {
      obj = data_tree_api.create_object(&dt, itemgroup, "None");
    }
    data_tree_api.set_object_parameter(&dt, obj, "Include", relative_file_path);
  }

  for (unsigned fi = 0; fi < array_count(p->file_data_custom_command); ++fi) {
    const struct cc_file_custom_command_t_* file = p->file_data_custom_command[fi];
    const char* relative_in_file_path = make_path_relative(in_output_folder, file->path);
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_in_file_path);
    const char* relative_out_file_path = make_path_relative(in_output_folder, file->output_file);
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_out_file_path);

    /*
    Apparently VS2019 Custom Build Tool does not reset paths between multiple invocations of Custom
    Build Tool. That's why there needs to be an absolute path here, instead of only the simpler
    relative build_to_base_path.
    */
    const char* in_file_path_from_base = make_path_relative(
        cc_data_.base_folder, make_uri(cc_printf("%s/%s", in_output_folder, file->path)));
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)in_file_path_from_base);
    const char* out_file_path_from_base = make_path_relative(
        cc_data_.base_folder, make_uri(cc_printf("%s/%s", in_output_folder, file->output_file)));
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)out_file_path_from_base);
    const char* input_output_substitution_keys[]   = {"input", "output"};
    const char* input_output_substitution_values[] = {in_file_path_from_base,
                                                      out_file_path_from_base};

    const char* in_file_path   = cc_substitute(relative_in_file_path, substitution_keys,
                                               substitution_values, countof(substitution_keys));
    const char* custom_command = cc_substitute(file->command, substitution_keys,
                                               substitution_values, countof(substitution_keys));
    custom_command =
        cc_substitute(custom_command, input_output_substitution_keys,
                      input_output_substitution_values, countof(input_output_substitution_keys));
    const char* out_file_path = cc_substitute(relative_out_file_path, substitution_keys,
                                              substitution_values, countof(substitution_keys));
    unsigned int itemgroup    = data_tree_api.create_object(&dt, project, "ItemGroup");
    unsigned int cb           = data_tree_api.create_object(&dt, itemgroup, "CustomBuild");
    data_tree_api.set_object_parameter(&dt, cb, "Include", in_file_path);
    unsigned int command_obj = data_tree_api.create_object(&dt, cb, "Command");
    data_tree_api.set_object_value(
        &dt, command_obj,
        cc_printf("cd $(ProjectDir)%s &amp;&amp; %s", build_to_base_path, custom_command));
    unsigned int outputs_obj = data_tree_api.create_object(&dt, cb, "Outputs");
    data_tree_api.set_object_value(&dt, outputs_obj, out_file_path);
  }
#endif

  fprintf(ninja_file, "\nrule compile_%s\n", p->name);
  fprintf(ninja_file, "  command = %s%s%s%s /nologo -c $in /Fo$out\n", compiler_path,
          preprocessor_defines, includes_command, additional_compiler_flags);
  fprintf(ninja_file, "  description = Compiling $in\n");

  fprintf(ninja_file, "\nrule link_%s\n", p->name);
  if (p->type == CCProjectTypeStaticLibrary) {
    fprintf(ninja_file, "  command = %s%s $in /nologo /OUT:$out\n", lib_linker_path,
            additional_link_flags);
    fprintf(ninja_file, "  description = Linking static library %s\n", p->name);
  } else if (p->type == CCProjectTypeDynamicLibrary) {
    fprintf(ninja_file, "  command = %s%s $in /DLL /nologo /OUT:$dyn_lib /IMPLIB:$imp_lib\n",
            linker_path, additional_link_flags);
    fprintf(ninja_file, "  description = Linking dynamic library %s\n", p->name);
  } else {
    fprintf(ninja_file, "  command = %s%s $in /nologo /OUT:$out\n", linker_path,
            additional_link_flags);
    fprintf(ninja_file, "  description = Linking binary %s\n", p->name);
  }

  const char* deps = "";
  for (unsigned fi = 0; fi < array_count(p->file_data); ++fi) {
    const char* f = p->file_data[fi]->path;
    // const char* relative_file_path = make_path_relative(in_output_folder, f);
    if (is_header_file(f)) {
    } else if (is_source_file(f)) {
      const char* obj_file_path = cc_printf("%s.obj", strip_extension(f));
      const char* src_file_path = cc_printf("%s%s", build_to_base_path, p->file_data[fi]->path);
      fprintf(ninja_file, "\nbuild %s: compile_%s %s\n", obj_file_path, p->name, src_file_path);
      deps = cc_printf("%s %s", deps, obj_file_path);
    } else {
    }
  }

  // Add project dependencies
  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const cc_project_impl_t* dp = p->dependantOn[i];
    switch (dp->type) {
      case CCProjectTypeStaticLibrary:
      case CCProjectTypeDynamicLibrary:
        deps = cc_printf("%s %s%s", deps, dp->name, ".lib");
        break;
      default:
        deps = cc_printf("%s %s%s", deps, dp->name, project_type_suffix[dp->type]);
        break;
    }
  }

  if (p->type == CCProjectTypeDynamicLibrary) {
    fprintf(ninja_file, "\nbuild %s%s %s%s: link_%s%s", p->name, project_type_suffix[p->type],
            p->name, ".lib", p->name, deps);
    fprintf(ninja_file, "\n  dyn_lib = %s%s", p->name, project_type_suffix[p->type]);
    fprintf(ninja_file, "\n  imp_lib = %s%s", p->name, ".lib");
  } else {
    fprintf(ninja_file, "\nbuild %s%s: link_%s%s", p->name, project_type_suffix[p->type], p->name,
            deps);
  }
  fprintf(ninja_file, "\n");

  if (have_post_build_action) {
    fprintf(ninja_file, "\nbuild post_build_%s: postbuild_0_%s %s%s\n", p->name, p->name, p->name,
            project_type_suffix[p->type]);
  }
}

void ninja_generateInFolder(const char* in_workspace_path) {
  const char* VsDevCmd_bat = cc_find_VcDevCmd_bat_();

  // Find compiler location
  {
    const char* find_compiler_command = cc_printf("%s >nul && where cl.exe", VsDevCmd_bat);
    int exit_code                     = 0;
    int result = system_np(find_compiler_command, 100 * 1000, stdout_data, sizeof(stdout_data),
                           stderr_data, sizeof(stderr_data), &exit_code);
    if (result != 0) {
      char* message;
      DWORD rfm = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                                result, 0, (LPSTR)&message, 1024, NULL);
      if (rfm != 0) {
        LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error finding compiler with command '%s'\nError: %s",
                           find_compiler_command, message);
      } else {
        LOG_ERROR_AND_QUIT(ERR_COMPILING, "Error finding compiler with command '%s'\nError: %i",
                           find_compiler_command, result);
      }
    } else {
      if (exit_code == 0) {
        const char* bin_folder = folder_path_only(stdout_data);
        compiler_path          = cc_printf("%scl.exe", bin_folder);
        lib_linker_path        = cc_printf("%slib.exe", bin_folder);
        linker_path            = cc_printf("%slink.exe", bin_folder);
      } else {
        // Succesfully ran the command, but there was an error, likely an issue compiling the
        // config file
        printf("stdout: %s\n", stdout_data);
        printf("stderr: %s\n", stderr_data);

        LOG_ERROR_AND_QUIT(
            ERR_COMPILING,
            "Error (%i) finding compiler. You can add '--generate-projects' to "
            "generate projects with the settings built into the existing CConstruct binary.\n",
            exit_code);
      }
    }
  }

  LOG_VERBOSE("Compiler: '%s'\n", compiler_path);
  LOG_VERBOSE("Linker: '%s'\n", linker_path);
  LOG_VERBOSE("Lib-linker: '%s'\n", lib_linker_path);

  in_workspace_path = make_uri(in_workspace_path);
  if (in_workspace_path[strlen(in_workspace_path) - 1] != '/')
    in_workspace_path = cc_printf("%s/", in_workspace_path);

  char* output_folder = make_uri(cc_printf("%s%s", cc_data_.base_folder, in_workspace_path));

  char* build_to_base_path = make_path_relative(output_folder, cc_data_.base_folder);

  for (unsigned project_idx = 0; project_idx < array_count(cc_data_.projects); project_idx++) {
    // Adjust all the files to be relative to the build output folder
    cc_project_impl_t* project = cc_data_.projects[project_idx];
    for (unsigned file_idx = 0; file_idx < array_count(project->file_data); file_idx++) {
    }
    for (unsigned file_idx = 0; file_idx < array_count(project->file_data_custom_command);
         file_idx++) {
      project->file_data_custom_command[file_idx]->path =
          cc_printf("%s%s", build_to_base_path, project->file_data_custom_command[file_idx]->path);
      project->file_data_custom_command[file_idx]->output_file = cc_printf(
          "%s%s", build_to_base_path, project->file_data_custom_command[file_idx]->output_file);
    }
    for (unsigned state_idx = 0; state_idx < array_count(project->state); state_idx++) {
      const cc_state_impl_t* state = project->state + state_idx;
      // Also all the include paths
      for (unsigned includes_idx = 0; includes_idx < array_count(state->include_folders);
           includes_idx++) {
        const char* include_path            = state->include_folders[includes_idx];
        const bool is_absolute_path         = (include_path[0] == '/') || (include_path[1] == ':');
        const bool starts_with_env_variable = (include_path[0] == '$');
        if (!is_absolute_path && !starts_with_env_variable) {
          state->include_folders[includes_idx] =
              cc_printf("%s%s", build_to_base_path, include_path);
        }
      }
    }
  }

  printf("Generating ninja.build in '%s'...\n", output_folder);

  int result = make_folder(output_folder);
  if (result != 0) {
    fprintf(stderr, "Error %i creating path '%s'\n", result, output_folder);
  }
  (void)chdir(output_folder);

  const char** project_ids =
      (const char**)cc_alloc_(sizeof(const char*) * array_count(cc_data_.projects));

  const char* ninja_file_path = cc_printf("%s/build.ninja", output_folder);
  FILE* ninja_file            = fopen(ninja_file_path, "wt");
  fprintf(ninja_file, "ninja_required_version = 1.5\n\n");
  fprintf(ninja_file, "msvc_deps_prefix = Note: including file:\n\n");

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    project_ids[i] = ninja_generateUUID();
  }

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const cc_project_impl_t* p = cc_data_.projects[i];
    const char* project_id     = project_ids[i];
    ninja_createProjectFile(ninja_file, p, project_id, project_ids, output_folder,
                            build_to_base_path);
  }

  {  // Add dependency on config file
    fprintf(ninja_file, "\nrule build_cconstruct\n");
    fprintf(ninja_file,
            "  command = cl.exe /ZI /W4 /WX /DEBUG /FC /Focconstruct.obj /Fe../cconstruct.exe "
            "/showIncludes /nologo /TC ../config.cc\n");
    fprintf(ninja_file, "  description = Building CConstruct ...\n");
    fprintf(ninja_file, "  deps = msvc\n");

    fprintf(ninja_file,
            "\nbuild ../cconstruct.exe cconstruct.obj: build_cconstruct ../config.cc\n");

    fprintf(ninja_file, "\nrule RERUN_CCONSTRUCT\n");
    fprintf(ninja_file, "  command = ../cconstruct.exe --generator=ninja --generate-projects\n");
    fprintf(ninja_file, "  description = Re-running CConstruct ...\n");
    fprintf(ninja_file, "  generator = 1\n");

    fprintf(ninja_file, "\nbuild build.ninja: RERUN_CCONSTRUCT ../cconstruct.exe\n");
    fprintf(ninja_file, "  pool = console\n");
  }

  fclose(ninja_file);
}
