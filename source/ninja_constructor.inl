
#if defined(_WIN32)
  #define OBJ_EXT ".obj"
  #define LIB_PREFIX ""
char const* const project_type_prefix[] = {"", "", "", ""};
char const* const project_type_suffix[] = {".exe", ".exe", ".lib", ".dll"};
char const* compiler_path               = "cl.exe";
char const* lib_linker_path             = "lib.exe";
char const* linker_path                 = "link.exe";
#elif defined(__APPLE__)
  #define OBJ_EXT ".o"
char const* const project_type_prefix[] = {"", "", "lib", "lib"};
char const* const project_type_suffix[] = {"", "", ".a", ".dylib"};
char const* compiler_path               = "clang";
char const* cpp_compiler_path           = "clang++";
char const* lib_linker_path             = "ar";
char const* linker_path                 = "link.exe";
#else
  #define OBJ_EXT ".o"
char const* const project_type_prefix[] = {"", "", "lib", "lib"};
char const* const project_type_suffix[] = {"", "", ".a", ".so"};
char const* compiler_path               = "clang";
char const* cpp_compiler_path           = "clang++";
char const* lib_linker_path             = "ar";
char const* linker_path                 = "link";
#endif

const char* ninja_generateUUID() {
  static size_t count = 0;
  ++count;

  char* buffer = (char*)cc_alloc_(37);
  sprintf(buffer, "00000000-0000-0000-0000-%012zi", count);
  return buffer;
}

const char* ninja_findUUIDForProject(const char** uuids, const cc_project_impl_t* project) {
  unsigned i = 0;
  while (cc_data_.projects[i] != project) {
    ++i;
  }

  return uuids[i];
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

void ninja_createProjectFile(FILE* ninja_file, const cc_project_impl_t* p,
                             const char* in_output_folder, const char* build_to_base_path) {
  (void)in_output_folder;

  const char* platform_label = _internal.active_arch_label;

  const char* substitution_keys[] = {
      "configuration",
      "platform",
      "workspace_folder",
  };
  const char* substitution_values[] = {
      _internal.active_config,
      platform_label,
      in_output_folder,
  };

  const char** external_frameworks = {0};
#if defined(__APPLE__)
  for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
    const bool is_global_state = ((p->configs[ipc] == NULL) && (p->architectures[ipc] == NULL));

    const cc_state_impl_t* s = &(p->state[ipc]);
    for (unsigned fi = 0; fi < array_count(s->external_libs);) {
      const char* lib_path = s->external_libs[fi];
      if (strstr(lib_path, ".framework") == NULL) {
        fi++;
      } else {
        if (!is_global_state) {
          LOG_ERROR_AND_QUIT(
              ERR_CONFIGURATION,
              "Apple framework added to state with configuration or architecture. This is not yet "
              "supported");
        }
        array_push(external_frameworks, lib_path);
        array_remove_at_index(s->external_libs, fi);
      }
    }
  }
// No frameworks left in the state now.
#else
  (void)external_frameworks;
#endif

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
#endif

  const char* resolved_output_folder = cc_substitute(
      p->outputFolder, substitution_keys, substitution_values, countof(substitution_keys));
#if defined(_WIN32)
  ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)resolved_output_folder);
#endif

#if 0
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

  const char** preprocessor_defines = NULL;
#if defined(_WIN32)
  switch (p->type) {
    case CCProjectTypeConsoleApplication:
      array_push(preprocessor_defines, "_CONSOLE");
      break;
    case CCProjectTypeWindowedApplication:
      array_push(preprocessor_defines, "_WINDOWS");
      break;
    case CCProjectTypeStaticLibrary:
      array_push(preprocessor_defines, "_LIB");
      break;
    case CCProjectTypeDynamicLibrary: {
      array_push(preprocessor_defines, cc_printf("%s%s", str_strip_spaces(p->name), "_EXPORTS"));
    } break;
    default:
      LOG_ERROR_AND_QUIT(ERR_CONFIGURATION, "Unknown project type for project '%s'\n", p->name);
  }
#endif

  const char* additional_compiler_flags    = "";
  const char* additional_link_flags        = "";
  const char** additional_include_folders  = NULL;
  const char* includes_command             = "";
  const char** link_additional_directories = NULL;
  const char* link_additional_dependencies = "";

  int cpp_version = 0;

  EStateWarningLevel combined_warning_level = EStateWarningLevelDefault;
  bool shouldDisableWarningsAsError         = false;
  for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
    const cc_state_impl_t* flags = &(p->state[ipc]);

    // TODO ordering and combination so that more specific flags can override general ones
    if ((p->configs[ipc] != NULL) &&
        (strcmp(p->configs[ipc]->label, _internal.active_config) != 0))
      continue;
#if 0
        if ((p->architectures[ipc] != arch) && (p->architectures[ipc] != NULL)) continue;
#endif

    // Choose the highest specific C++ version.
    cpp_version = (flags->cpp_version > cpp_version) ? flags->cpp_version : cpp_version;

    shouldDisableWarningsAsError = flags->disableWarningsAsErrors;
    combined_warning_level       = flags->warningLevel;

    for (unsigned pdi = 0; pdi < array_count(flags->defines); ++pdi) {
      array_push(preprocessor_defines, flags->defines[pdi]);
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
      array_push(additional_include_folders, make_uri(flags->include_folders[ifi]));
    }

    for (unsigned di = 0; di < array_count(flags->external_libs); di++) {
      const char* lib_path_from_base = flags->external_libs[di];
      const bool is_absolute_path =
          (lib_path_from_base[0] == '/') || (lib_path_from_base[1] == ':');
      const bool starts_with_env_variable = (lib_path_from_base[0] == '$');
      if (!is_absolute_path && !starts_with_env_variable) {
        lib_path_from_base = cc_printf("%s%s", build_to_base_path, lib_path_from_base);
      }

      const char* relative_lib_path   = make_path_relative(in_output_folder, lib_path_from_base);
      const char* lib_name            = strip_path(relative_lib_path);
      const char* lib_folder          = make_uri(folder_path_only(relative_lib_path));
      const char* resolved_lib_folder = cc_substitute(
          lib_folder, substitution_keys, substitution_values, countof(substitution_keys));

#if defined(_WIN32)
      ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)resolved_lib_folder);
      link_additional_dependencies =
          cc_printf("%s \"%s\"", link_additional_dependencies, lib_name);
#else
      link_additional_dependencies = cc_printf("%s -l%s", link_additional_dependencies, lib_name);
#endif
      // Check if this path is already in there
      bool did_find = false;
      for (size_t ilf = 0; ilf < array_count(link_additional_directories); ilf++) {
        if (strcmp(link_additional_directories[ilf], resolved_lib_folder) == 0) did_find = true;
      }
      if (!did_find) {
        array_push(link_additional_directories, resolved_lib_folder);
      }
    }

#if 0
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
#if defined(_WIN32)
        ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)additional_include_folders[i]);
        combined_include_folders =
            cc_printf("%s /I\"%s\"", combined_include_folders, additional_include_folders[i]);
#else
        combined_include_folders =
            cc_printf("%s -I\"%s\"", combined_include_folders, additional_include_folders[i]);
#endif
      }
      includes_command = combined_include_folders;
    }

#if 0
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

  const char* combined_link_folders = "";
  if (array_count(link_additional_directories) != 0) {
    // Put later folders in front of earlier folders
    for (size_t i = 0; i < array_count(link_additional_directories); i++) {
      int fl = (int)strlen(link_additional_directories[i]);
      if (link_additional_directories[i][fl - 1] == '\\') {
        fl -= 1;
      }
#if defined(_WIN32)
      combined_link_folders = cc_printf("%s /LIBPATH:\"%.*s\"", combined_link_folders, fl,
                                        link_additional_directories[i]);
#else
      combined_link_folders =
          cc_printf("%s -L%.*s", combined_link_folders, fl, link_additional_directories[i]);
#endif
    }
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)additional_include_folders);
  }

  {
#if defined(_WIN32)
    const char* warning_strings[] = {"/W4", "/W3", "/W2", "/Wall", "/W0"};
#else
    const char* warning_strings[] = {"-Wall -Wextra", "-Wall", "-Wall", "-Weverything", "-w"};
#endif
    assert(EStateWarningLevelHigh == 0);
    assert(EStateWarningLevelAll == 3);
    assert(EStateWarningLevelNone == 4);
    additional_compiler_flags =
        cc_printf("%s %s", additional_compiler_flags, warning_strings[combined_warning_level]);
  }

  if (!shouldDisableWarningsAsError) {
#if defined(_WIN32)
    additional_compiler_flags = cc_printf("%s /WX", additional_compiler_flags);
#else
    additional_compiler_flags = cc_printf("%s -Werror", additional_compiler_flags);
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
#endif

  for (unsigned fi = 0; fi < array_count(p->file_data_custom_command); ++fi) {
    const struct cc_file_custom_command_t_* file = p->file_data_custom_command[fi];
    const char* relative_in_file_path  = make_path_relative(in_output_folder, file->path);
    const char* relative_out_file_path = make_path_relative(in_output_folder, file->output_file);
#if defined(_WIN32)
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_in_file_path);
    ninja_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_out_file_path);
#endif
    const char* input_output_substitution_keys[]   = {"input", "output"};
    const char* input_output_substitution_values[] = {relative_in_file_path,
                                                      relative_out_file_path};

    const char* in_file_path   = cc_substitute(relative_in_file_path, substitution_keys,
                                               substitution_values, countof(substitution_keys));
    const char* custom_command = cc_substitute(file->command, substitution_keys,
                                               substitution_values, countof(substitution_keys));
    custom_command =
        cc_substitute(custom_command, input_output_substitution_keys,
                      input_output_substitution_values, countof(input_output_substitution_keys));
    const char* out_file_path = cc_substitute(relative_out_file_path, substitution_keys,
                                              substitution_values, countof(substitution_keys));

    fprintf(ninja_file, "\nrule post_build_rule_%s_%i\n", p->name, fi);
#if defined(_WIN32)
    fprintf(ninja_file, "  command = cmd /c %s\n", custom_command);
#else
    fprintf(ninja_file, "  command = %s\n", custom_command);
#endif
    fprintf(ninja_file, "\nbuild %s: post_build_rule_%s_%i %s/%s%s%s %s\n", out_file_path, p->name,
            fi, resolved_output_folder, project_type_prefix[p->type], p->name,
            project_type_suffix[p->type], in_file_path);
  }

  // Add project dependencies
  const char* deps         = "";
  const char* project_deps = "";
  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const cc_project_impl_t* dp = p->dependantOn[i];
    project_deps                = cc_printf("%s %s", project_deps, dp->name);
  }

#if defined(_WIN32)
  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const cc_project_impl_t* dp = p->dependantOn[i];
    switch (dp->type) {
      case CCProjectTypeStaticLibrary:
      case CCProjectTypeDynamicLibrary:
        additional_link_flags = cc_printf("%s %s/%s%s", additional_link_flags,
                                          resolved_output_folder, dp->name, ".lib");
        break;
      default:
        additional_link_flags =
            cc_printf("%s %s/%s%s%s", additional_link_flags, resolved_output_folder,
                      project_type_prefix[dp->type], dp->name, project_type_suffix[dp->type]);
        break;
    }
  }
#else
  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const cc_project_impl_t* dp = p->dependantOn[i];

    // Need to do the same here as for external_libs
    if (dp->type == CCProjectTypeDynamicLibrary) {
  #if defined(__APPLE__)
      additional_link_flags = cc_printf("%s -Wl,-rpath,@executable_path", additional_link_flags);
  #else
      additional_link_flags = cc_printf("%s -Wl,-rpath,'$$ORIGIN'", additional_link_flags);
  #endif
      additional_link_flags =
          cc_printf("%s -L%s -l%s", additional_link_flags, resolved_output_folder, dp->name);
    } else {
      additional_link_flags =
          cc_printf("%s %s/%s%s%s", additional_link_flags, resolved_output_folder,
                    project_type_prefix[dp->type], dp->name, project_type_suffix[dp->type]);
    }
  }
#endif

  fprintf(ninja_file, "\n##################\n# Project %s\n", p->name);

  const char** command_elements = NULL;
  {  // General rule to compile source files to object files
    fprintf(ninja_file, "\nrule compile_%s", p->name);
    array_push(command_elements, compiler_path);
    for (unsigned int di = 0; di < array_count(preprocessor_defines); ++di) {
#if defined(_WIN32)
      array_push(command_elements, cc_printf("/D \"%s\"", preprocessor_defines[di]));
#else
      array_push(command_elements, cc_printf("-D%s", preprocessor_defines[di]));
#endif
    }
    array_push(command_elements, includes_command);
    array_push(command_elements, additional_compiler_flags);
#if defined(_WIN32)
    array_push(command_elements, "/nologo /c $in /Fo:$out");
#else
    array_push(command_elements, "-c $in -o $out");
#endif

    fprintf(ninja_file, "\n  command = %s", command_elements[0]);
    for (unsigned int ci = 1; ci < array_count(command_elements); ++ci) {
      fprintf(ninja_file, " %s", command_elements[ci]);
    }
    fprintf(ninja_file, "\n  description = Compiling $in\n");
  }

  const char* desc = "";
  command_elements = NULL;

  fprintf(ninja_file, "\nrule link_%s", p->name);
#if defined(_WIN32)
  if (p->type == CCProjectTypeStaticLibrary) {
    array_push(command_elements, lib_linker_path);
    array_push(command_elements, additional_link_flags);
    array_push(command_elements, combined_link_folders);
    array_push(command_elements, link_additional_dependencies);
    array_push(command_elements, "$in /nologo /OUT:$out");
    desc = cc_printf("Linking static library %s", p->name);
  } else if (p->type == CCProjectTypeDynamicLibrary) {
    array_push(command_elements, linker_path);
    array_push(command_elements, additional_link_flags);
    array_push(command_elements, combined_link_folders);
    array_push(command_elements, link_additional_dependencies);
    array_push(command_elements, "$in /DLL /nologo /OUT:$dyn_lib /IMPLIB:$imp_lib");
    desc = cc_printf("Linking dynamic library %s", p->name);
  } else {
    array_push(command_elements, linker_path);
    array_push(command_elements, additional_link_flags);
    array_push(command_elements, combined_link_folders);
    array_push(command_elements, link_additional_dependencies);
    if (_internal.active_arch == EArchitectureX64) {
      array_push(command_elements, "/MACHINE:X64");
    }
    array_push(command_elements, "$in /nologo /OUT:$out");
    desc = cc_printf("Linking binary %s", p->name);
  }
#else
  if (p->type == CCProjectTypeStaticLibrary) {
    array_push(command_elements, lib_linker_path);
    array_push(command_elements, "rcs $out $in");
    array_push(command_elements, additional_link_flags);
    array_push(command_elements, combined_link_folders);
    array_push(command_elements, link_additional_dependencies);
    desc = cc_printf("Linking static library %s", p->name);
  } else if (p->type == CCProjectTypeDynamicLibrary) {
    array_push(command_elements, cpp_version > 0 ? cpp_compiler_path : compiler_path);
    array_push(command_elements, "$in -o $dyn_lib");
    array_push(command_elements, additional_link_flags);
    array_push(command_elements, combined_link_folders);
    array_push(command_elements, link_additional_dependencies);
  #if defined(__APPLE__)
    array_push(command_elements, "-dynamiclib -install_name @rpath/$rpath");
  #else
    array_push(command_elements, "-shared");
  #endif
    desc = cc_printf("Linking dynamic library %s", p->name);
  } else {
    array_push(command_elements, cpp_version > 0 ? cpp_compiler_path : compiler_path);
    array_push(command_elements, "$in -o $out");
    array_push(command_elements, additional_link_flags);
    array_push(command_elements, combined_link_folders);
    array_push(command_elements, link_additional_dependencies);
    desc = cc_printf("Linking binary %s", p->name);
  }

  for (unsigned i = 0; i < array_count(external_frameworks); ++i) {
    const char* dependantName = cc_printf("%s", strip_path(external_frameworks[i]));
    array_push(command_elements, cc_printf("-framework %s", strip_extension(dependantName)));
  }
#endif

  {
    fprintf(ninja_file, "\n  command = %s", command_elements[0]);
    for (unsigned int ci = 1; ci < array_count(command_elements); ++ci) {
      fprintf(ninja_file, " %s", command_elements[ci]);
    }
  }
  if (desc != NULL) {
    fprintf(ninja_file, "\n  description = %s", desc);
  }
  fprintf(ninja_file, "\n");

  for (unsigned fi = 0; fi < array_count(p->file_data); ++fi) {
    const char* f = p->file_data[fi]->path;
    // const char* relative_file_path = make_path_relative(in_output_folder, f);
    if (is_header_file(f)) {
    } else if (is_source_file(f)) {
      const char* obj_file_path = cc_printf("%s" OBJ_EXT, strip_extension(f));
      if (strncmp(obj_file_path, "../", 3) == 0) {
        obj_file_path += 3;
      }
      const char* src_file_path =
          make_uri(cc_printf("%s%s", build_to_base_path, p->file_data[fi]->path));
      fprintf(ninja_file, "\nbuild %s: compile_%s %s\n", obj_file_path, p->name, src_file_path);
      // Linked libs need to be after object files
      deps = cc_printf(" %s%s", obj_file_path, deps);
    } else {
    }
  }


  if (p->type == CCProjectTypeDynamicLibrary) {
#if defined(_WIN32)
    fprintf(ninja_file, "\nbuild %s/%s%s%s %s/%s%s%s: link_%s%s|%s", resolved_output_folder,
            project_type_prefix[p->type], p->name, project_type_suffix[p->type],
            resolved_output_folder, project_type_prefix[p->type], p->name, ".lib", p->name, deps,
            project_deps);
    fprintf(ninja_file, "\n  imp_lib = %s/%s%s%s", resolved_output_folder,
            project_type_prefix[p->type], p->name, ".lib");
#else
    fprintf(ninja_file, "\nbuild %s/%s%s%s: link_%s%s |%s", resolved_output_folder,
            project_type_prefix[p->type], p->name, project_type_suffix[p->type], p->name, deps,
            project_deps);
    fprintf(ninja_file, "\n  rpath = %s%s%s", project_type_prefix[p->type], p->name, ".dylib");
#endif
    fprintf(ninja_file, "\n  dyn_lib = %s/%s%s%s", resolved_output_folder,
            project_type_prefix[p->type], p->name, project_type_suffix[p->type]);
  } else {
    fprintf(ninja_file, "\nbuild %s/%s%s%s: link_%s%s |%s", resolved_output_folder,
            project_type_prefix[p->type], p->name, project_type_suffix[p->type], p->name, deps,
            project_deps);
  }
  fprintf(ninja_file, "\n");

  if (have_post_build_action) {
    fprintf(ninja_file, "\nbuild post_build_%s: postbuild_0_%s %s/%s%s\n", p->name, p->name,
            resolved_output_folder, p->name, project_type_suffix[p->type]);
  }

  fprintf(ninja_file, "\nbuild %s : phony || %s/%s%s%s \n", p->name, resolved_output_folder,
          project_type_prefix[p->type], p->name, project_type_suffix[p->type]);
}

void ninja_generateInFolder(const char* in_workspace_path) {
// Find compiler location
#if 0
  const char* VsDevCmd_bat = cc_find_VcDevCmd_bat_();
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
#endif

  LOG_VERBOSE("Compiler: '%s'\n", compiler_path);
  LOG_VERBOSE("Linker: '%s'\n", linker_path);
  LOG_VERBOSE("Lib-linker: '%s'\n", lib_linker_path);

  {  // Check for explicit arch
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const cc_architecture_impl_t* a = cc_data_.architectures[pi];
      if ((_internal.active_arch_label != NULL) &&
          strcmp(_internal.active_arch_label, cc_projectArch2String_(a->type)) == 0) {
        _internal.active_arch = a->type;
        break;
      }
    }
    if (_internal.active_arch == EArchitectureCount) {
      fprintf(stderr, "Unknown architecture: %s\n", _internal.active_arch_label);
      exit(1);
    }
  }

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

  const char* ninja_file_path = cc_printf("%s/build.ninja", output_folder);
  FILE* ninja_file            = fopen(ninja_file_path, "wt");
  fprintf(ninja_file, "ninja_required_version = 1.5\n\n");

#if defined(_WIN32)
  fprintf(ninja_file, "msvc_deps_prefix = Note: including file:\n\n");
#endif

  const char* platform_label = _internal.active_arch_label;

  fprintf(ninja_file, "configuration = %s\n", _internal.active_config);
  fprintf(ninja_file, "platform = %s\n", platform_label);
  fprintf(ninja_file, "workspace_folder = %s\n", ".");

  fprintf(ninja_file, "configuration = %s\n", _internal.active_config);
  fprintf(ninja_file, "platform = %s\n", platform_label);
  fprintf(ninja_file, "workspace_folder = %s\n", ".");

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const cc_project_impl_t* p = cc_data_.projects[i];
    ninja_createProjectFile(ninja_file, p, output_folder, build_to_base_path);
  }

  const char* workspace_abs =
      make_uri(cc_printf("%s%s", folder_path_only(_internal.config_file_path), in_workspace_path));
  const char* config_path_rel = make_path_relative(workspace_abs, _internal.config_file_path);

  const char* path_cconstruct_abs = cc_path_executable();
  const char* cconstruct_path_rel = make_path_relative(workspace_abs, path_cconstruct_abs);

  fprintf(ninja_file, "\n##################\n# CConstruct rebuild\n");
  {  // Add dependency on config file
    fprintf(ninja_file, "\nrule build_cconstruct");
#if defined(_WIN32)
    fprintf(ninja_file,
            "\n  command = %s /ZI /W4 /WX /DEBUG /FC /Fo:cconstruct" OBJ_EXT
            " /Fe%s "
            "/showIncludes /nologo %s $in",
            compiler_path, cconstruct_path_rel,
  #if __cplusplus
            "/TP"
  #else
            "/TC"
  #endif
    );
    fprintf(ninja_file, "\n  deps = msvc");
#else
    //-g -Wall -Wextra -Wpedantic -Werror
    fprintf(ninja_file,
            "\n  command = %s -MD -MF $out.d %s -Wno-deprecated-declarations -o ./%s $$PWD/$in",
  #if __cplusplus
            "clang++", "-std=c++11",
  #else
            "clang", "-x c",
  #endif
            cconstruct_path_rel);
    fprintf(ninja_file, "\n  depfile = $out.d");
    fprintf(ninja_file, "\n  deps = gcc");
#endif
    fprintf(ninja_file, "\n  description = Building CConstruct ...\n");

    fprintf(ninja_file, "\nbuild ./%s: build_cconstruct %s\n", cconstruct_path_rel,
            config_path_rel);

    fprintf(ninja_file, "\nrule RERUN_CCONSTRUCT\n");
    fprintf(ninja_file, "  command = ./%s --generator=ninja --generate-projects\n",
            cconstruct_path_rel);
    fprintf(ninja_file, "  description = Re-running CConstruct ...\n");
    fprintf(ninja_file, "  generator = 1\n");

    fprintf(ninja_file, "\nbuild build.ninja: RERUN_CCONSTRUCT %s\n", cconstruct_path_rel);
    fprintf(ninja_file, "  pool = console\n");
  }

  fclose(ninja_file);
}
