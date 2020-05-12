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

const char* vs_projectPlatform2String_(EPlatformType platform) {
  switch (platform) {
    case EPlatformTypeX86:
      return "Win32";
    case EPlatformTypeX64:
      return "x64";
    case EPlatformTypeARM:
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

void vs2019_createFilters(const cc_project_impl_t* in_project, const char* in_output_folder) {
  const char* projectfilters_file_path = cc_printf("%s.vcxproj.filters", in_project->name);
  FILE* filter_file                    = fopen(projectfilters_file_path, "wb");

  fprintf(filter_file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  fprintf(filter_file,
          "<Project ToolsVersion=\"4.0\" "
          "xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");

  // Create list of groups needed.
  const cc_project_impl_t* p            = (cc_project_impl_t*)in_project;
  const cc_group_impl_t** unique_groups = {0};
  for (unsigned ig = 0; ig < array_count(p->file_data); ++ig) {
    const cc_group_impl_t* g = p->file_data[ig]->parent_group;
    while (g) {
      bool already_contains_group = false;
      for (unsigned i = 0; i < array_count(unique_groups); ++i) {
        if (g == unique_groups[i]) {
          already_contains_group = true;
        }
      }
      if (!already_contains_group) {
        array_push(unique_groups, g);
      }
      g = g->parent_group;
    }
  }

  // Create names for the needed groups. Nested groups append their name in the filter file
  //    Group A
  //    Group A\Nested Group
  //    Group B
  const unsigned num_unique_groups = array_count(unique_groups);
  const char** unique_group_names  = {0};
  for (unsigned i = 0; i < num_unique_groups; ++i) {
    const cc_group_impl_t* g = unique_groups[i];
    assert(g);
    const char* name = g->name[0] ? g->name : "<group>";
    while (g->parent_group) {
      g    = g->parent_group;
      name = cc_printf("%s\\%s", g->name[0] ? g->name : "<group>", name);
    }
    array_push(unique_group_names, name);
  }

  fprintf(filter_file, "  <ItemGroup>\n");
  for (unsigned i = 0; i < num_unique_groups; ++i) {
    const cc_group_impl_t* g = unique_groups[i];
    const char* group_name   = unique_group_names[i];
    fprintf(filter_file, "    <Filter Include=\"%s\">\n", group_name);
    fprintf(filter_file, "      <UniqueIdentifier>{%s}</UniqueIdentifier>\n", vs_generateUUID());
    fprintf(filter_file, "    </Filter>\n");
  }
  fprintf(filter_file, "  </ItemGroup>\n");

  for (unsigned fi = 0; fi < array_count(p->file_data); ++fi) {
    const struct cc_file_t_* file = p->file_data[fi];

    fprintf(filter_file, "  <ItemGroup>\n");
    const char* f                  = file->path;
    const cc_group_impl_t* g       = file->parent_group;
    const char* group_name         = NULL;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
    for (unsigned ug = 0; ug < num_unique_groups; ++ug) {
      if (unique_groups[ug] == g) {
        group_name = unique_group_names[ug];
      }
    }

    if (is_header_file(f)) {
      fprintf(filter_file, "    <ClInclude Include=\"%s\">\n", relative_file_path);
      fprintf(filter_file, "      <Filter>%s</Filter>\n", group_name);
      fprintf(filter_file, "    </ClInclude>\n");
    } else if (is_source_file(f)) {
      fprintf(filter_file, "    <ClCompile Include=\"%s\">\n", relative_file_path);
      fprintf(filter_file, "      <Filter>%s</Filter>\n", group_name);
      fprintf(filter_file, "    </ClCompile>\n");
    } else {
      fprintf(filter_file, "    <None Include=\"%s\">\n", relative_file_path);
      fprintf(filter_file, "      <Filter>%s</Filter>\n", group_name);
      fprintf(filter_file, "    </None>\n");
    }
    fprintf(filter_file, "  </ItemGroup>\n");
  }

  for (unsigned fi = 0; fi < array_count(p->file_data_custom_command); ++fi) {
    const struct cc_file_custom_command_t_* file = p->file_data_custom_command[fi];

    fprintf(filter_file, "  <ItemGroup>\n");
    const char* f                  = file->path;
    const cc_group_impl_t* g       = file->parent_group;
    const char* group_name         = NULL;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
    for (unsigned ug = 0; ug < num_unique_groups; ++ug) {
      if (unique_groups[ug] == g) {
        group_name = unique_group_names[ug];
      }
    }

    fprintf(filter_file, "    <CustomBuild Include=\"%s\">\n", relative_file_path);
    fprintf(filter_file, "      <Filter>%s</Filter>\n", group_name);
    fprintf(filter_file, "    </CustomBuild>\n");

    fprintf(filter_file, "  </ItemGroup>\n");
  }

  fprintf(filter_file, "</Project>\n");
}

void vs2019_createProjectFile(const cc_project_impl_t* p, const char* project_id,
                              const char** project_ids, const char* in_output_folder) {
  const char* project_file_path = cc_printf("%s.vcxproj", p->name);
  FILE* project_file            = fopen(project_file_path, "wb");
  fprintf(
      project_file,
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<Project DefaultTargets=\"Build\" "
      "ToolsVersion=\"15.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");

  fprintf(project_file, "  <ItemGroup Label=\"ProjectConfigurations\">\n");
  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.platforms); ++pi) {
      const char* platform_label = vs_projectPlatform2String_(cc_data_.platforms[pi]->type);
      fprintf(project_file, "    <ProjectConfiguration Include=\"%s|%s\">\n", c, platform_label);
      fprintf(project_file, "      <Configuration>%s</Configuration>\n", c);
      fprintf(project_file, "      <Platform>%s</Platform>\n", platform_label);
      fprintf(project_file, "    </ProjectConfiguration>\n");
    }
  }
  fprintf(project_file, "  </ItemGroup>\n");

  fprintf(project_file,
          "  <PropertyGroup Label=\"Globals\">\n"
          "    <VCProjectVersion>15.0</VCProjectVersion>\n"
          "    <ProjectGuid>{%s}</ProjectGuid>\n"
          "    <Keyword>Win32Proj</Keyword>\n"
          "    <RootNamespace>builder</RootNamespace>\n"
          "    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\n"
          "  </PropertyGroup>\n",
          project_id);

  fprintf(project_file,
          "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n");

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.platforms); ++pi) {
      const char* platform_label = vs_projectPlatform2String_(cc_data_.platforms[pi]->type);
      fprintf(project_file,
              "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" "
              "Label=\"Configuration\">\n",
              c, platform_label);
      fprintf(project_file, "    <ConfigurationType>");
      if ((p->type == CCProjectTypeConsoleApplication) ||
          (p->type == CCProjectTypeWindowedApplication)) {
        fprintf(project_file, "Application");
      } else {
        fprintf(project_file, "StaticLibrary");
      }
      fprintf(project_file, "</ConfigurationType>\n");
      fprintf(project_file,
              "    <UseDebugLibraries>true</UseDebugLibraries>\n"
              "    <PlatformToolset>v142</PlatformToolset>\n"
              "    <CharacterSet>MultiByte</CharacterSet>\n"
              "  </PropertyGroup>\n");
    }
  }

  fprintf(project_file,
          "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n  <ImportGroup "
          "Label=\"ExtensionSettings\">\n  </ImportGroup>\n  <ImportGroup Label=\"Shared\">\n  "
          "</ImportGroup>\n");
  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.platforms); ++pi) {
      const char* platform_label = vs_projectPlatform2String_(cc_data_.platforms[pi]->type);
      fprintf(project_file,
              "  <ImportGroup Label=\"PropertySheets\" "
              "Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n",
              c, platform_label);
      fprintf(project_file,
              "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" "
              "Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" "
              "Label=\"LocalAppDataPlatform\" />\n");
      fprintf(project_file, "  </ImportGroup>\n");
    }
  }

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.platforms); ++pi) {
      const char* platform_label        = vs_projectPlatform2String_(cc_data_.platforms[pi]->type);
      const char* substitution_keys[]   = {"configuration", "platform"};
      const char* substitution_values[] = {"$(Configuration)", "$(Platform)"};
      const char* resolved_output_folder =
          cc_substitute(cc_data_.outputFolder, substitution_keys, substitution_values,
                        countof(substitution_keys));
      vs_replaceForwardSlashWithBackwardSlashInPlace((char*)resolved_output_folder);
      fprintf(project_file, "  <PropertyGroup Label=\"UserMacros\" />\n");
      fprintf(project_file,
              "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", c,
              platform_label);
      // TODO: fix detection of type of config
      bool is_debug_build = (strcmp(c, "Debug") == 0);
      if (is_debug_build) {
        fprintf(project_file, "    <LinkIncremental>true</LinkIncremental>\n");
      } else {
        fprintf(project_file, "    <LinkIncremental>false</LinkIncremental>\n");
      }
      fprintf(project_file, "    <CustomBuildAfterTargets>Build</CustomBuildAfterTargets>\n");
      fprintf(project_file, "    <OutDir>$(SolutionDir)\\%s\\</OutDir>\n", resolved_output_folder);
      // VS2019 warns if multiple projects have the same intermediate directory, so void that
      // here
      fprintf(project_file,
              "    <IntDir>$(SolutionDir)\\%s\\Intermediate\\$(ProjectName)\\</IntDir>\n",
              resolved_output_folder);

      fprintf(project_file, "  </PropertyGroup>\n");
    }
  }

  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const cc_configuration_impl_t* config = cc_data_.configurations[ci];
    const char* configuration_label       = config->label;
    const bool is_debug_build             = (strcmp(configuration_label, "Debug") == 0);
    for (unsigned pi = 0; pi < array_count(cc_data_.platforms); ++pi) {
      const cc_platform_impl_t* platform = cc_data_.platforms[pi];

      vs_compiler_setting* compiler_flags = {0};
      {
        vs_compiler_setting precompiled_setting     = {"PrecompiledHeader", "NotUsing"};
        vs_compiler_setting sdlcheck_setting        = {"SDLCheck", "true"};
        vs_compiler_setting conformancemode_setting = {"ConformanceMode", "true"};
        array_push(compiler_flags, precompiled_setting);
        array_push(compiler_flags, sdlcheck_setting);
        array_push(compiler_flags, conformancemode_setting);
      }

      vs_compiler_setting* linker_flags     = {0};
      vs_compiler_setting subsystem_setting = {
          "SubSystem", ((p->type == CCProjectTypeWindowedApplication) ? "Windows" : "Console")};
      vs_compiler_setting debuginfo_setting = {"GenerateDebugInformation", "true"};
      array_push(linker_flags, subsystem_setting);
      array_push(linker_flags, debuginfo_setting);

      if (is_debug_build) {
        vs_compiler_setting setting = {"Optimization", "Disabled"};
        array_push(compiler_flags, setting);
      } else {
        vs_compiler_setting opt_setting   = {"Optimization", "MaxSpeed"};
        vs_compiler_setting check_setting = {"BasicRuntimeChecks", "Default"};
        array_push(compiler_flags, opt_setting);
        array_push(compiler_flags, check_setting);
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
        default:
          LOG_ERROR_AND_QUIT("Unknown project type for project '%s'\n", p->name);
      }
      const char* additional_compiler_flags  = "%(AdditionalOptions)";
      const char* additional_link_flags      = "%(AdditionalOptions)";
      const char* additional_include_folders = "";

      EStateWarningLevel combined_warning_level = EStateWarningLevelDefault;
      bool shouldDisableWarningsAsError         = false;
      for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
        const cc_state_impl_t* flags = &(p->state[ipc]);

        // TODO ordering and combination so that more specific flags can override general ones
        if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;
        if ((p->platforms[ipc] != platform) && (p->platforms[ipc] != NULL)) continue;

        shouldDisableWarningsAsError = flags->disableWarningsAsErrors;
        combined_warning_level       = flags->warningLevel;
        for (unsigned pdi = 0; pdi < array_count(flags->defines); ++pdi) {
          preprocessor_defines = cc_printf("%s;%s", flags->defines[pdi], preprocessor_defines);
        }
        for (unsigned cfi = 0; cfi < array_count(flags->compile_options); ++cfi) {
          additional_compiler_flags =
              cc_printf("%s %s", flags->compile_options[cfi], additional_compiler_flags);
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
      }

      const char* warning_strings[] = {"Level4", "Level3", "Level2", "EnableAllWarnings",
                                       "TurnOffAllWarnings"};
      assert(EStateWarningLevelHigh == 0);
      assert(EStateWarningLevelAll == 3);
      assert(EStateWarningLevelNone == 4);

      vs_compiler_setting warning_level_setting = {"WarningLevel",
                                                   warning_strings[combined_warning_level]};
      array_push(compiler_flags, warning_level_setting);
      vs_compiler_setting disable_unused_parameter_setting = {"DisableSpecificWarnings", "4100"};
      array_push(compiler_flags, disable_unused_parameter_setting);

      vs_compiler_setting warning_as_error_setting = {
          "TreatWarningAsError", shouldDisableWarningsAsError ? "false" : "true"};
      array_push(compiler_flags, warning_as_error_setting);

      if (is_debug_build) {
        preprocessor_defines = cc_printf("_DEBUG;%s", preprocessor_defines);
      } else {
        preprocessor_defines = cc_printf("NDEBUG;%s", preprocessor_defines);
      }
      const bool is_win32 = (cc_data_.platforms[pi]->type == EPlatformTypeX86);
      if (is_win32) {
        preprocessor_defines = cc_printf("WIN32;%s", preprocessor_defines);
      }

      vs_compiler_setting preprocessor_setting = {"PreprocessorDefinitions", preprocessor_defines};
      vs_compiler_setting additionaloptions_setting = {"AdditionalOptions",
                                                       additional_compiler_flags};
      array_push(compiler_flags, preprocessor_setting);
      array_push(compiler_flags, additionaloptions_setting);

      if (*additional_include_folders != 0) {
        // Skip the starting ;
        vs_replaceForwardSlashWithBackwardSlashInPlace((char*)additional_include_folders);
        vs_compiler_setting additionalincludes_setting = {"AdditionalIncludeDirectories",
                                                          additional_include_folders + 1};
        array_push(compiler_flags, additionalincludes_setting);
      }

      vs_compiler_setting additionallinkoptions_setting = {"AdditionalOptions",
                                                           additional_link_flags};
      array_push(linker_flags, additionallinkoptions_setting);

      const char* platform_label = vs_projectPlatform2String_(cc_data_.platforms[pi]->type);
      fprintf(project_file,
              "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n",
              configuration_label, platform_label);
      fprintf(project_file, "    <ClCompile>\n");

      for (unsigned cfi = 0; cfi < array_count(compiler_flags); ++cfi) {
        const char* key   = compiler_flags[cfi].key;
        const char* value = compiler_flags[cfi].value;
        fprintf(project_file, "      <%s>%s</%s>\n", key, value, key);
      }

      fprintf(project_file, "    </ClCompile>\n");
      fprintf(project_file, "    <Link>\n");
      for (unsigned cfi = 0; cfi < array_count(linker_flags); ++cfi) {
        const char* key   = linker_flags[cfi].key;
        const char* value = linker_flags[cfi].value;
        fprintf(project_file, "      <%s>%s</%s>\n", key, value, key);
      }
      fprintf(project_file, "    </Link>\n");

      bool have_post_build_action = (p->postBuildAction != 0);
      if (have_post_build_action) {
        const char* windowsPostBuildAction = cc_printf("%s", p->postBuildAction);
        vs_replaceForwardSlashWithBackwardSlashInPlace((char*)windowsPostBuildAction);
        fprintf(project_file, "    <PostBuildEvent>\n");
        fprintf(project_file, "      <Command>%s</Command>\n", windowsPostBuildAction);
        fprintf(project_file, "    </PostBuildEvent>\n");
      }
      fprintf(project_file, "  </ItemDefinitionGroup>\n");
    }
  }

  for (unsigned fi = 0; fi < array_count(p->file_data); ++fi) {
    fprintf(project_file, "  <ItemGroup>\n");
    const char* f                  = p->file_data[fi]->path;
    const char* relative_file_path = make_path_relative(in_output_folder, f);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_file_path);
    if (is_header_file(f)) {
      fprintf(project_file, "    <ClInclude Include=\"%s\" />\n", relative_file_path);
    } else if (is_source_file(f)) {
      fprintf(project_file, "    <ClCompile Include=\"%s\">\n", relative_file_path);
      fprintf(project_file, "      <CompileAs>%s</CompileAs>\n",
              ((strstr(f, ".cpp") != NULL) ? "CompileAsCpp" : "CompileAsC"));
      fprintf(project_file, "    </ClCompile>\n");
    } else {
      fprintf(project_file, "    <None Include=\"%s\" />\n", relative_file_path);
    }
    fprintf(project_file, "  </ItemGroup>\n");
  }

  const char* build_to_base_path = make_path_relative(in_output_folder, cc_data_.base_folder);
  vs_replaceForwardSlashWithBackwardSlashInPlace((char*)build_to_base_path);
  for (unsigned fi = 0; fi < array_count(p->file_data_custom_command); ++fi) {
    const struct cc_file_custom_command_t_* file = p->file_data_custom_command[fi];
    const char* relative_in_file_path = make_path_relative(in_output_folder, file->path);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_in_file_path);
    const char* relative_out_file_path = make_path_relative(in_output_folder, file->output_file);
    vs_replaceForwardSlashWithBackwardSlashInPlace((char*)relative_out_file_path);

    fprintf(project_file, "  <ItemGroup>\n");
    fprintf(project_file, "    <CustomBuild Include=\"%s\">\n", relative_in_file_path);
    fprintf(project_file, "      <Command>cd %s &amp;&amp; %s</Command>\n", build_to_base_path,
            file->command);
    fprintf(project_file, "      <Outputs>%s</Outputs>\n", relative_out_file_path);
    fprintf(project_file, "    </CustomBuild>\n");
    fprintf(project_file, "  </ItemGroup>\n");
  }

  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const char* id            = vs_findUUIDForProject(project_ids, p->dependantOn[i]);
    const char* project_label = p->dependantOn[i]->name;
    fprintf(project_file,
            "  <ItemGroup>\n    <ProjectReference Include=\"%s.vcxproj\">\n      "
            "<Project>{%s}</Project>\n    </ProjectReference>\n  </ItemGroup>\n",
            project_label, id);
  }

  fprintf(project_file,
          "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n  <ImportGroup "
          "Label=\"ExtensionTargets\">\n  </ImportGroup>\n</Project>");

  fclose(project_file);
}
