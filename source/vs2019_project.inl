
const char* vs_findUUIDForProject(const char** uuids, const TProject* project) {
  unsigned i = 0;
  while (privateData.projects[i] != project) {
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

void vs2019_createFilters(const TProject* in_project) {
  const char* projectfilters_file_path = cc_printf("%s.vcxproj.filters", in_project->name);
  FILE* filter_file                    = fopen(projectfilters_file_path, "wb");

  fprintf(filter_file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  fprintf(filter_file,
          "<Project ToolsVersion=\"4.0\" "
          "xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");

  // Remove duplicate groups
  const TProject* p          = (TProject*)in_project;
  const char** unique_groups = {0};
  for (unsigned ig = 0; ig < array_count(p->groups); ++ig) {
    const char* g = p->groups[ig];
    if (g[0]) {
      bool already_contains_group = false;
      for (unsigned i = 0; i < array_count(unique_groups); ++i) {
        if (strcmp(g, unique_groups[i]) == 0) {
          already_contains_group = true;
        }
      }
      if (!already_contains_group) {
        array_push(unique_groups, g);
      }
    }
  }

  fprintf(filter_file, "  <ItemGroup>\n");
  int count = 0;
  for (unsigned i = 0; i < array_count(unique_groups); ++i) {
    const char* g = unique_groups[i];
    fprintf(filter_file, "    <Filter Include=\"%s\">\n", g);
    fprintf(filter_file,
            "      <UniqueIdentifier>{d43b239c-1ecb-43eb-8aca-28bf989c811%i}</UniqueIdentifier>\n",
            ++count);
    fprintf(filter_file, "    </Filter>\n");
  }
  fprintf(filter_file, "  </ItemGroup>\n");

  for (unsigned i = 0; i < array_count(unique_groups); ++i) {
    const char* g = unique_groups[i];
    fprintf(filter_file, "  <ItemGroup>\n");
    for (unsigned fi = 0; fi < array_count(p->files); ++fi) {
      const char* f = p->files[fi];
      if (strcmp(f, g) == 0) {
        if (strstr(f, ".h")) {
          fprintf(filter_file, "    <ClInclude Include=\"%s\">\n", f);
          fprintf(filter_file, "      <Filter>%s</Filter>\n", g);
          fprintf(filter_file, "    </ClInclude>\n");
        } else {
          fprintf(filter_file, "    <ClCompile Include=\"%s\">\n", f);
          fprintf(filter_file, "      <Filter>%s</Filter>\n", g);
          fprintf(filter_file, "    </ClCompile>\n");
        }
      }
    }
    fprintf(filter_file, "  </ItemGroup>\n");
  }

  fprintf(filter_file, "</Project>\n");
}

void vs2019_createProjectFile(const TProject* p, const char* project_id, const char** project_ids,
                              int folder_depth) {
  const char* prepend_path = "";
  for (int i = 0; i < folder_depth; ++i) {
    prepend_path = cc_printf("%s../", prepend_path);
  }

  const char* project_file_path = cc_printf("%s.vcxproj", p->name);
  FILE* project_file            = fopen(project_file_path, "wb");
  fprintf(project_file,
          "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<Project DefaultTargets=\"Build\" "
          "ToolsVersion=\"15.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");

  fprintf(project_file, "  <ItemGroup Label=\"ProjectConfigurations\">\n");
  for (unsigned ci = 0; ci < array_count(privateData.configurations); ++ci) {
    const char* c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(privateData.platforms); ++pi) {
      const char* platform_label = vs_projectPlatform2String_(privateData.platforms[pi]->type);
      fprintf(project_file, "    <ProjectConfiguration Include=\"%s|%s\">\n", c, platform_label);
      fprintf(project_file, "      <Configuration>%s</Configuration>\n", c);
      fprintf(project_file, "      <Platform>%s</Platform>\n", platform_label);
      fprintf(project_file, "    </ProjectConfiguration>\n");
    }
  }
  fprintf(project_file, "  </ItemGroup>\n");

  fprintf(
      project_file,
      "  <PropertyGroup Label=\"Globals\">\n    <VCProjectVersion>15.0</VCProjectVersion>\n    "
      "<ProjectGuid>{%s}</ProjectGuid>\n    <Keyword>Win32Proj</Keyword>\n    "
      "<RootNamespace>builder</RootNamespace>\n    "
      "<WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\n  </PropertyGroup>\n",
      project_id);

  fprintf(project_file,
          "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n");

  for (unsigned ci = 0; ci < array_count(privateData.configurations); ++ci) {
    const char* c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(privateData.platforms); ++pi) {
      const char* platform_label = vs_projectPlatform2String_(privateData.platforms[pi]->type);
      fprintf(project_file,
              "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" "
              "Label=\"Configuration\">\n",
              c, platform_label);
      fprintf(project_file, "    <ConfigurationType>");
      if (p->type == CCProjectTypeConsoleApplication) {
        fprintf(project_file, "Application");
      } else {
        fprintf(project_file, "StaticLibrary");
      }
      fprintf(project_file, "</ConfigurationType>\n");
      fprintf(project_file,
              "    <UseDebugLibraries>true</UseDebugLibraries>\n"
              "    <PlatformToolset>v142</PlatformToolset>\n"
              "    <CharacterSet>Unicode</CharacterSet>\n"
              "  </PropertyGroup>\n");
    }
  }

  fprintf(project_file,
          "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n  <ImportGroup "
          "Label=\"ExtensionSettings\">\n  </ImportGroup>\n  <ImportGroup Label=\"Shared\">\n  "
          "</ImportGroup>\n");
  for (unsigned ci = 0; ci < array_count(privateData.configurations); ++ci) {
    const char* c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(privateData.platforms); ++pi) {
      const char* platform_label = vs_projectPlatform2String_(privateData.platforms[pi]->type);
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

  for (unsigned ci = 0; ci < array_count(privateData.configurations); ++ci) {
    const char* c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(privateData.platforms); ++pi) {
      const char* platform_label = vs_projectPlatform2String_(privateData.platforms[pi]->type);
      const char* substitution_keys[]   = {"configuration", "platform"};
      const char* substitution_values[] = {"$(Configuration)", "$(Platform)"};
      const char* resolved_output_folder =
          cc_substitute(privateData.outputFolder, substitution_keys, substitution_values,
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
      // VS2019 warns if multiple projects have the same intermediate directory, so void that here
      fprintf(project_file,
              "    <IntDir>$(SolutionDir)\\%s\\Intermediate\\$(ProjectName)\\</IntDir>\n",
              resolved_output_folder);

      fprintf(project_file, "  </PropertyGroup>\n");
    }
  }

  for (unsigned ci = 0; ci < array_count(privateData.configurations); ++ci) {
    const TConfiguration* config    = privateData.configurations[ci];
    const char* configuration_label = config->label;
    const bool is_debug_build       = (strcmp(configuration_label, "Debug") == 0);
    for (unsigned pi = 0; pi < array_count(privateData.platforms); ++pi) {
      const TPlatform* platform = privateData.platforms[pi];

      vs_compiler_setting* compiler_flags = {0};
      {
        vs_compiler_setting precompiled_setting     = {"PrecompiledHeader", "NotUsing"};
        vs_compiler_setting warning_setting         = {"WarningLevel", "Level3"};
        vs_compiler_setting sdlcheck_setting        = {"SDLCheck", "true"};
        vs_compiler_setting conformancemode_setting = {"ConformanceMode", "true"};
        array_push(compiler_flags, precompiled_setting);
        array_push(compiler_flags, warning_setting);
        array_push(compiler_flags, sdlcheck_setting);
        array_push(compiler_flags, conformancemode_setting);
      }

      vs_compiler_setting* linker_flags     = {0};
      vs_compiler_setting subsystem_setting = {"SubSystem", "Console"};
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

      const char* preprocessor_defines       = "_CONSOLE;%(PreprocessorDefinitions)";
      const char* additional_compiler_flags  = "%(AdditionalOptions)";
      const char* additional_include_folders = "";

      for (unsigned ipc = 0; ipc < array_count(p->flags); ++ipc) {
        // TODO ordering and combination so that more specific flags can override general ones
        if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;
        if ((p->platforms[ipc] != platform) && (p->platforms[ipc] != NULL)) continue;

        for (unsigned pdi = 0; pdi < array_count(p->flags[ipc].defines); ++pdi) {
          preprocessor_defines =
              cc_printf("%s;%s", p->flags[ipc].defines[pdi], preprocessor_defines);
        }
        for (unsigned cfi = 0; cfi < array_count(p->flags[ipc].compile_options); ++cfi) {
          additional_compiler_flags =
              cc_printf("%s %s", p->flags[ipc].compile_options[cfi], additional_compiler_flags);
        }
        for (unsigned ifi = 0; ifi < array_count(p->flags[ipc].include_folders); ++ifi) {
          // Order matters here, so append
          additional_include_folders =
              cc_printf("%s;%s", additional_include_folders, p->flags[ipc].include_folders[ifi]);
        }
      }

      if (is_debug_build) {
        preprocessor_defines = cc_printf("_DEBUG;%s", preprocessor_defines);
      } else {
        preprocessor_defines = cc_printf("NDEBUG;%s", preprocessor_defines);
      }
      const bool is_win32 = (privateData.platforms[pi]->type == EPlatformTypeX86);
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
        vs_compiler_setting additionalincludes_setting = {"AdditionalIncludeDirectories",
                                                          additional_include_folders + 1};
        array_push(compiler_flags, additionalincludes_setting);
      }

      const char* platform_label = vs_projectPlatform2String_(privateData.platforms[pi]->type);
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

  for (unsigned fi = 0; fi < array_count(p->files); ++fi) {
    fprintf(project_file, "  <ItemGroup>\n");
    const char* f = cc_printf("%s%s", prepend_path, p->files[fi]);
    if (strstr(f, ".h")) {
      fprintf(project_file, "    <ClInclude Include=\"%s\" />\n", f);
    } else {
      fprintf(project_file, "    <ClCompile Include=\"%s\" />\n", f);
    }
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
