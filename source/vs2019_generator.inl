const char* vs_generateUUID() {
  static size_t count = 0;
  ++count;

  char* buffer = (char*)cc_alloc_(37);
  sprintf(buffer, "00000000-0000-0000-0000-%012zi", count);
  return buffer;
};

const char* vs_findUUIDForProject(const char** uuids, const cc_project_impl_t* project) {
  unsigned i = 0;
  while (cc_data_.projects[i] != project) {
    ++i;
  }

  return uuids[i];
}

const char* vs_projectArch2String_(EArchitecture arch) {
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

void vs_replaceForwardSlashWithBackwardSlashInPlace(char* in_out) {
  if (in_out == 0) return;

  while (*in_out) {
    if (*in_out == '/') *in_out = '\\';
    in_out++;
  }
}

typedef struct vs_compiler_setting {
  const char* key;
  const char* value;
} vs_compiler_setting;

void export_tree_as_xml(FILE* f, const struct data_tree_t* dt, unsigned int node,
                        unsigned int depth) {
  assert(node < array_count(dt->objects));

  const int SPACES_PER_DEPTH = 2;

  const struct data_tree_object_t* obj = dt->objects + node;
  if (obj->name) {
    fprintf(f, "%*s<%s", depth * SPACES_PER_DEPTH, "", obj->name);
    if (obj->first_parameter) {
      const struct data_tree_object_t* param_obj = dt->objects + obj->first_parameter;
      do {
        fprintf(f, " %s=\"%s\"", param_obj->name, param_obj->value);
      } while (param_obj->next_sibling && (param_obj = dt->objects + param_obj->next_sibling));
    }
  }

  if (obj->has_children) {
    if (obj->name) {
      fprintf(f, ">\n");
    }
    unsigned int child_index                   = obj->first_child;
    const struct data_tree_object_t* child_obj = dt->objects + child_index;
    do {
      export_tree_as_xml(f, dt, child_index, (node == 0) ? 0 : (depth + 1));
    } while (child_obj->next_sibling && (child_index = child_obj->next_sibling) &&
             (child_obj = dt->objects + child_obj->next_sibling));
    fprintf(f, "%*s", depth * SPACES_PER_DEPTH, "");

    if (obj->name) {
      fprintf(f, "</%s>\n", obj->name);
    }
  } else if (obj->value) {
    fprintf(f, ">%s</%s>\n", obj->value, obj->name);
  } else {
    fprintf(f, " />\n");
  }
}

void vs2019_createFilters(const cc_project_impl_t* in_project, const char* in_output_folder) {
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
  for (unsigned fi = 0; fi < array_count(p->file_data); fi++) {
    unsigned gi = p->file_data[fi]->parent_group_idx;
    while (gi) {
      groups_needed[gi] = true;
      gi                = cc_data_.groups[gi].parent_group_idx;
    }
  }
  for (unsigned fi = 0; fi < array_count(p->file_data_custom_command); fi++) {
    unsigned gi = p->file_data_custom_command[fi]->parent_group_idx;
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
      data_tree_api.set_object_value(&dt, uid, vs_generateUUID());
    }
  }

  for (unsigned fi = 0; fi < array_count(p->file_data); ++fi) {
    const struct cc_file_t_* file = p->file_data[fi];

    itemgroup                      = data_tree_api.create_object(&dt, project, "ItemGroup");
    const char* f                  = file->path;
    const unsigned gi              = file->parent_group_idx;
    const char* group_name         = NULL;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
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
    const unsigned gi              = file->parent_group_idx;
    const char* group_name         = NULL;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
    group_name = unique_group_names[gi];

    itemgroup       = data_tree_api.create_object(&dt, project, "ItemGroup");
    unsigned int cb = data_tree_api.create_object(&dt, itemgroup, "CustomBuild");
    data_tree_api.set_object_parameter(&dt, cb, "Include", relative_file_path);
    unsigned int fil = data_tree_api.create_object(&dt, cb, "Filter");
    data_tree_api.set_object_value(&dt, fil, group_name);
  }

  FILE* filter_file = fopen(projectfilters_file_path, "wb");
  fprintf(filter_file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  export_tree_as_xml(filter_file, &dt, 0, 0);
  fclose(filter_file);
}

typedef void (*func)(struct data_tree_t* dt, unsigned int compile_group);

void optionZi(struct data_tree_t* dt, unsigned int compile_group) {
  data_tree_api.set_object_value(
      dt, data_tree_api.get_or_create_object(dt, compile_group, "DebugInformationFormat"),
      "ProgramDatabase");
}
void optionZI(struct data_tree_t* dt, unsigned int compile_group) {
  data_tree_api.set_object_value(
      dt, data_tree_api.get_or_create_object(dt, compile_group, "DebugInformationFormat"),
      "EditAndContinue");
}

typedef struct vs_compiler_flag {
  const char* flag;
  const func action;
} vs_compiler_flag;

const vs_compiler_flag known_flags[] = {{"/Zi", &optionZi}, {"/ZI", &optionZI}};

void vs2019_createProjectFile(const cc_project_impl_t* p, const char* project_id,
                              const char** project_ids, const char* in_output_folder) {
  const char* project_file_path = cc_printf("%s.vcxproj", p->name);

  struct data_tree_t dt = data_tree_api.create();
  unsigned int project  = data_tree_api.create_object(&dt, 0, "Project");
  data_tree_api.set_object_parameter(&dt, project, "DefaultTargets", "Build");
  data_tree_api.set_object_parameter(&dt, project, "ToolsVersion", "15.0");
  data_tree_api.set_object_parameter(&dt, project, "xmlns",
                                     "http://schemas.microsoft.com/developer/msbuild/2003");

  unsigned int itemgroup = data_tree_api.create_object(&dt, project, "ItemGroup");
  data_tree_api.set_object_parameter(&dt, itemgroup, "Label", "ProjectConfigurations");
  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const char* platform_label = vs_projectArch2String_(cc_data_.architectures[pi]->type);

      unsigned int pc = data_tree_api.create_object(&dt, itemgroup, "ProjectConfiguration");
      data_tree_api.set_object_parameter(&dt, pc, "Include",
                                         cc_printf("%s|%s", c, platform_label));
      unsigned int cobj = data_tree_api.create_object(&dt, pc, "Configuration");
      data_tree_api.set_object_value(&dt, cobj, c);
      unsigned int platformobj = data_tree_api.create_object(&dt, pc, "Platform");
      data_tree_api.set_object_value(&dt, platformobj, platform_label);
    }
  }

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

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const char* platform_label = vs_projectArch2String_(cc_data_.architectures[pi]->type);
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
                                     "v142");
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
      const char* platform_label = vs_projectArch2String_(cc_data_.architectures[pi]->type);
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

  const char* substitution_keys[]   = {"configuration", "platform"};
  const char* substitution_values[] = {"$(Configuration)", "$(Platform)"};

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const char* platform_label = vs_projectArch2String_(cc_data_.architectures[pi]->type);
      const char* resolved_output_folder = cc_substitute(
          p->outputFolder, substitution_keys, substitution_values, countof(substitution_keys));
      vs_replaceForwardSlashWithBackwardSlashInPlace((char*)resolved_output_folder);

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
      const char* platform_label = vs_projectArch2String_(arch->type);
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
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "Optimization"), "Disabled");
      } else {
        data_tree_api.set_object_value(
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "Optimization"), "MaxSpeed");
        data_tree_api.set_object_value(
            &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "BasicRuntimeChecks"),
            "Default");
      }

      const char* preprocessor_defines = "%(PreprocessorDefinitions)";
      switch (p->type) {
        case CCProjectTypeConsoleApplication:
          preprocessor_defines = cc_printf("%s;%s", "_CONSOLE", preprocessor_defines);
          break;
        case CCProjectTypeWindowedApplication:
          preprocessor_defines = cc_printf("%s;%s", "_WINDOWS", preprocessor_defines);
          break;
        case CCProjectTypeStaticLibrary:
          preprocessor_defines = cc_printf("%s;%s", "_LIB", preprocessor_defines);
          break;
        case CCProjectTypeDynamicLibrary: {
          preprocessor_defines =
              cc_printf("%s%s;%s", str_strip_spaces(p->name), "_EXPORTS", preprocessor_defines);
        } break;
        default:
          LOG_ERROR_AND_QUIT(ERR_CONFIGURATION, "Unknown project type for project '%s'\n",
                             p->name);
      }
      const char* additional_compiler_flags    = "%(AdditionalOptions)";
      const char* additional_link_flags        = "%(AdditionalOptions)";
      const char* additional_include_folders   = "";
      const char* link_additional_directories  = "";
      const char* link_additional_dependencies = "";

      EStateWarningLevel combined_warning_level = EStateWarningLevelDefault;
      bool shouldDisableWarningsAsError         = false;
      for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
        const cc_state_impl_t* flags = &(p->state[ipc]);

        // TODO ordering and combination so that more specific flags can override general ones
        if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;
        if ((p->architectures[ipc] != arch) && (p->architectures[ipc] != NULL)) continue;

        shouldDisableWarningsAsError = flags->disableWarningsAsErrors;
        combined_warning_level       = flags->warningLevel;
        for (unsigned pdi = 0; pdi < array_count(flags->defines); ++pdi) {
          preprocessor_defines = cc_printf("%s;%s", flags->defines[pdi], preprocessor_defines);
        }
        for (unsigned cfi = 0; cfi < array_count(flags->compile_options); ++cfi) {
          // Index in known flags
          bool found = false;
          for (unsigned kfi = 0; kfi < countof(known_flags); kfi++) {
            if (strcmp(known_flags[kfi].flag, flags->compile_options[cfi]) == 0) {
              found = true;
              known_flags[kfi].action(&dt, compile_obj);
            }
          }
          if (!found) {
            additional_compiler_flags =
                cc_printf("%s %s", flags->compile_options[cfi], additional_compiler_flags);
          }
        }
        for (unsigned lfi = 0; lfi < array_count(flags->link_options); ++lfi) {
          additional_link_flags =
              cc_printf("%s %s", flags->link_options[lfi], additional_link_flags);
        }
        for (unsigned ifi = 0; ifi < array_count(flags->include_folders); ++ifi) {
          // Order matters here, so append
          additional_include_folders =
              cc_printf("%s;%s", additional_include_folders, flags->include_folders[ifi]);
        }

        for (unsigned di = 0; di < array_count(flags->external_libs); di++) {
          const char* lib_path_from_base = flags->external_libs[di];
          const char* relative_lib_path = make_path_relative(in_output_folder, lib_path_from_base);
          const char* lib_name          = strip_path(relative_lib_path);
          const char* lib_folder        = make_uri(folder_path_only(relative_lib_path));
          const char* resolved_lib_folder = cc_substitute(
              lib_folder, substitution_keys, substitution_values, countof(substitution_keys));

          vs_replaceForwardSlashWithBackwardSlashInPlace((char*)resolved_lib_folder);
          link_additional_dependencies =
              cc_printf("%s;%s", link_additional_dependencies, lib_name);
          link_additional_directories =
              cc_printf("%s;%s", link_additional_directories, resolved_lib_folder);
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

      vs_compiler_setting preprocessor_setting = {"PreprocessorDefinitions", preprocessor_defines};
      vs_compiler_setting additionaloptions_setting = {"AdditionalOptions",
                                                       additional_compiler_flags};
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "PreprocessorDefinitions"),
          preprocessor_defines);
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, compile_obj, "AdditionalOptions"),
          additional_compiler_flags);

      if (*additional_include_folders != 0) {
        vs_replaceForwardSlashWithBackwardSlashInPlace((char*)additional_include_folders);
        // Skip the starting ;
        data_tree_api.set_object_value(
            &dt,
            data_tree_api.get_or_create_object(&dt, compile_obj, "AdditionalIncludeDirectories"),
            additional_include_folders + 1);
      }

      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, link_obj, "AdditionalOptions"),
          additional_link_flags);

      link_additional_dependencies =
          cc_printf("%s;%%(AdditionalDependencies)", link_additional_dependencies);
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, link_obj, "AdditionalLibraryDirectories"),
          link_additional_directories);
      data_tree_api.set_object_value(
          &dt, data_tree_api.get_or_create_object(&dt, link_obj, "AdditionalDependencies"),
          link_additional_dependencies);

      bool have_post_build_action = (p->postBuildAction != 0);
      if (have_post_build_action) {
        const char* windowsPostBuildAction = cc_printf("%s", p->postBuildAction);
        windowsPostBuildAction = cc_substitute(windowsPostBuildAction, substitution_keys,
                                               substitution_values, countof(substitution_keys));
        vs_replaceForwardSlashWithBackwardSlashInPlace((char*)windowsPostBuildAction);
        unsigned int pbe = data_tree_api.create_object(&dt, idg, "PostBuildEvent");
        data_tree_api.set_object_value(&dt, data_tree_api.create_object(&dt, pbe, "Command"),
                                       windowsPostBuildAction);
      }
    }
  }

  for (unsigned fi = 0; fi < array_count(p->file_data); ++fi) {
    const char* f                  = p->file_data[fi]->path;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
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

  const char* build_to_base_path = make_path_relative(in_output_folder, cc_data_.base_folder);
  vs_replaceForwardSlashWithBackwardSlashInPlace((char*)build_to_base_path);
  for (unsigned fi = 0; fi < array_count(p->file_data_custom_command); ++fi) {
    const struct cc_file_custom_command_t_* file = p->file_data_custom_command[fi];
    const char* relative_in_file_path = make_path_relative(in_output_folder, file->path);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_in_file_path);
    const char* relative_out_file_path = make_path_relative(in_output_folder, file->output_file);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_out_file_path);

    /*
    Apparently VS2019 Custom Build Tool does not reset paths between multiple invocations of Custom
    Build Tool. That's why there needs to be an absolute path here, instead of only the simpler
    relative build_to_base_path.
    */
    const char* in_file_path_from_base = make_path_relative(
        cc_data_.base_folder, make_uri(cc_printf("%s/%s", in_output_folder, file->path)));
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)in_file_path_from_base);
    const char* out_file_path_from_base = make_path_relative(
        cc_data_.base_folder, make_uri(cc_printf("%s/%s", in_output_folder, file->output_file)));
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)out_file_path_from_base);
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

  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const char* id            = vs_findUUIDForProject(project_ids, p->dependantOn[i]);
    const char* project_label = p->dependantOn[i]->name;

    unsigned int itemgroup = data_tree_api.create_object(&dt, project, "ItemGroup");
    unsigned int pr        = data_tree_api.create_object(&dt, itemgroup, "ProjectReference");
    data_tree_api.set_object_parameter(&dt, pr, "Include", cc_printf("%s.vcxproj", project_label));
    unsigned int project = data_tree_api.create_object(&dt, pr, "Project");
    data_tree_api.set_object_value(&dt, project, cc_printf("{%s}", id));
  }

  unsigned int importobj = data_tree_api.create_object(&dt, project, "Import");
  data_tree_api.set_object_parameter(&dt, importobj, "Project",
                                     "$(VCTargetsPath)\\Microsoft.Cpp.targets");
  unsigned int importgroup = data_tree_api.create_object(&dt, project, "ImportGroup");
  data_tree_api.set_object_parameter(&dt, importgroup, "Label", "ExtensionTargets");

  FILE* project_file = fopen(project_file_path, "wb");
  fprintf(project_file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  export_tree_as_xml(project_file, &dt, 0, 0);
  fclose(project_file);
}

const char* replaceSpacesWithUnderscores(const char* in) {
  unsigned length = strlen(in);
  char* out       = (char*)cc_alloc_(length + 1);  // +1 for terminating 0
  memcpy(out, in, length);
  out[length] = 0;
  char* it    = out;
  for (unsigned i = 0; i < length; ++i, ++it) {
    if (*it == ' ') {
      *it = '_';
    }
  }
  return in;
}

const char* solutionArch2String_(EArchitecture arch) {
  switch (arch) {
    case EArchitectureX86:
      return "x86";
    case EArchitectureX64:
      return "x64";
    case EArchitectureARM:
      return "ARM";
  }
  return "";
}

void vs2019_generateInFolder(const char* in_workspace_path) {
  in_workspace_path = make_uri(in_workspace_path);
  if (in_workspace_path[strlen(in_workspace_path) - 1] != '/')
    in_workspace_path = cc_printf("%s/", in_workspace_path);

  char* output_folder = make_uri(cc_printf("%s%s", cc_data_.base_folder, in_workspace_path));

  char* build_to_base_path = make_path_relative(output_folder, cc_data_.base_folder);

  for (unsigned project_idx = 0; project_idx < array_count(cc_data_.projects); project_idx++) {
    // Adjust all the files to be relative to the build output folder
    cc_project_impl_t* project = cc_data_.projects[project_idx];
    for (unsigned file_idx = 0; file_idx < array_count(project->file_data); file_idx++) {
      project->file_data[file_idx]->path =
          cc_printf("%s%s", build_to_base_path, project->file_data[file_idx]->path);
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
      // And referenced external libs
      for (unsigned libs_idx = 0; libs_idx < array_count(state->external_libs); libs_idx++) {
        const char* lib_path                = state->external_libs[libs_idx];
        const bool is_absolute_path         = (lib_path[0] == '/') || (lib_path[1] == ':');
        const bool starts_with_env_variable = (lib_path[0] == '$');
        if (!is_absolute_path && !starts_with_env_variable) {
          state->external_libs[libs_idx] = cc_printf("%s%s", build_to_base_path, lib_path);
        }
      }
    }
  }

  printf("Generating Visual Studio 2019 solution and projects in '%s'...\n", output_folder);

  int result = make_folder(output_folder);
  if (result != 0) {
    fprintf(stderr, "Error %i creating path '%s'\n", result, output_folder);
  }
  (void)chdir(output_folder);

  const char** project_ids =
      (const char**)cc_alloc_(sizeof(const char*) * array_count(cc_data_.projects));

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    project_ids[i] = vs_generateUUID();
  }

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const cc_project_impl_t* p = cc_data_.projects[i];
    const char* project_id     = project_ids[i];
    vs2019_createFilters(p, output_folder);
    vs2019_createProjectFile(p, project_id, project_ids, output_folder);

    printf("Constructed VS2019 project '%s.vcxproj'\n", p->name);
  }

  // Create list of groups needed.
  bool* groups_needed = (bool*)cc_alloc_(array_count(cc_data_.groups) * sizeof(bool));
  memset(groups_needed, 0, array_count(cc_data_.groups) * sizeof(bool));
  for (unsigned i = 0; i < array_count(cc_data_.projects); i++) {
    unsigned g = cc_data_.projects[i]->parent_group_idx;
    while (g) {
      groups_needed[g] = true;
      g                = cc_data_.groups[g].parent_group_idx;
    }
  }

  const char* workspace_file_path = cc_printf("%s.sln", cc_data_.workspaceLabel);
  FILE* workspace                 = fopen(workspace_file_path, "wb");
  if (workspace == NULL) {
    fprintf(stderr, "Couldn't create workspace.sln\n");
    return;
  }

  fprintf(workspace,
          "Microsoft Visual Studio Solution File, Format Version 12.00\n# Visual Studio Version "
          "16\nVisualStudioVersion = 16.0.29709.97\nMinimumVisualStudioVersion = 10.0.40219.1\n");

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const char* projectId      = project_ids[i];
    const cc_project_impl_t* p = cc_data_.projects[i];
    fprintf(workspace,
            "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s.vcxproj\", "
            "\"{%s}\"\n",
            p->name, replaceSpacesWithUnderscores(p->name), projectId);
    fprintf(workspace, "EndProject\n");
  }

  // Add solution folders
  const char** unique_groups_id = {0};
  for (unsigned i = 0; i < array_count(cc_data_.groups); i++) {
    const char* id = NULL;
    if (groups_needed[i]) {
      id                       = vs_generateUUID();
      const cc_group_impl_t* g = &cc_data_.groups[i];
      fprintf(workspace,
              "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"%s\", \"%s\", "
              "\"{%s}\"\n",
              g->name, g->name, id);
      fprintf(workspace, "EndProject\n");
    }
    array_push(unique_groups_id, id);
  }

  fprintf(workspace, "Global\n");

  fprintf(workspace, "	GlobalSection(SolutionConfigurationPlatforms) = preSolution\n");
  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const char* p = solutionArch2String_(cc_data_.architectures[pi]->type);
      fprintf(workspace, "		%s|%s = %s|%s\n", c, p, c, p);
    }
  }
  fprintf(workspace, "	EndGlobalSection\n");

  fprintf(workspace, "	GlobalSection(ProjectConfigurationPlatforms) = postSolution\n");
  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const char* projectId = project_ids[i];
    for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
      const char* c = cc_data_.configurations[ci]->label;
      for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
        const EArchitecture p  = cc_data_.architectures[pi]->type;
        const char* arch_label = vs_projectArch2String_(p);
        fprintf(workspace, "		{%s}.%s|%s.ActiveCfg = %s|%s\n", projectId, c,
                solutionArch2String_(p), c, arch_label);
        fprintf(workspace, "		{%s}.%s|%s.Build.0 = %s|%s\n", projectId, c,
                solutionArch2String_(p), c, arch_label);
      }
    }
  }
  fprintf(workspace, "	EndGlobalSection\n");

  fprintf(workspace,
          "	GlobalSection(SolutionProperties) = preSolution\n"
          "		HideSolutionNode = FALSE\n"
          "	EndGlobalSection\n");

  fprintf(workspace, "	GlobalSection(NestedProjects) = preSolution\n");
  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const char* projectId      = project_ids[i];
    const cc_project_impl_t* p = cc_data_.projects[i];
    if (p->parent_group_idx) {
      fprintf(workspace, "		{%s} = {%s}\n", projectId,
              unique_groups_id[p->parent_group_idx]);
    }
  }
  for (unsigned i = 0; i < array_count(cc_data_.groups); i++) {
    if (groups_needed[i]) {
      const cc_group_impl_t* g = &cc_data_.groups[i];
      if (g->parent_group_idx) {
        fprintf(workspace, "		{%s} = {%s}\n", unique_groups_id[i],
                unique_groups_id[g->parent_group_idx]);
      }
    }
  }
  fprintf(workspace, "	EndGlobalSection\n");

  fprintf(workspace,
          "	GlobalSection(ExtensibilityGlobals) = postSolution\n"
          "		SolutionGuid = "
          "{7354F2AC-FB49-4B5D-B080-EDD798F580A5}\n"
          "	EndGlobalSection\nEndGlobal\n");

  fclose(workspace);
  printf("Constructed VS2019 workspace at '%s'\n", workspace_file_path);
}
