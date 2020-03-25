
const char* vs_findUUIDForProject(const std::vector<std::string>& uuids, const TProject* project) {
  for (unsigned i = 0; i < uuids.size(); ++i) {
    if (privateData.projects[i] == project) return uuids[i].c_str();
  }
  return "";
}

std::string platform2String(EPlatformType platform) {
  switch (platform) {
    case EPlatformTypeX86:
      return "x86";
    case EPlatformTypeX64:
      return "x64";
    case EPlatformTypeARM:
      return "ARM";
  }
  return "";
}

struct vs_compiler_setting {
  const char* key;
  const char* value;
};

void vs2019_createFilters(const TProject* in_project) {
  FILE* filter_file = fopen((in_project->name + std::string(".vcxproj.filters")).c_str(), "wb");

  fprintf(filter_file, R"lit(<?xml version="1.0" encoding="utf-8"?>)lit"
                       "\n");
  fprintf(filter_file,
          "<Project ToolsVersion=\"4.0\" "
          "xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\"\n");

  // Remove duplicate groups
  const TProject* p = (TProject*)in_project;
  std::set<std::string> unique_groups;
  for (auto& g : p->groups) unique_groups.insert(g);
  unique_groups.erase("");

  fprintf(filter_file, "  <ItemGroup>\n");
  int count = 0;
  for (auto& g : unique_groups) {
    fprintf(filter_file, "    <Filter Include=\"%s\">\n", g.c_str());
    fprintf(filter_file,
            "      <UniqueIdentifier>{d43b239c-1ecb-43eb-8aca-28bf989c811%i}</UniqueIdentifier>\n",
            ++count);
    fprintf(filter_file, "    </Filter>\n");
  }
  fprintf(filter_file, "  </ItemGroup>\n");

  for (auto& g : unique_groups) {
    fprintf(filter_file, "  <ItemGroup>\n");
    for (unsigned fi = 0; fi < p->files.size(); ++fi) {
      if (p->groups[fi] == g) {
        auto f = p->files[fi];
        if (f.find(".h") != std::string::npos) {
          fprintf(filter_file, "    <ClInclude Include=\"%s\">\n", p->files[fi].c_str());
          fprintf(filter_file, "      <Filter>%s</Filter>\n", g.c_str());
          fprintf(filter_file, "    </ClInclude>\n");
        } else {
          fprintf(filter_file, "    <ClCompile Include=\"%s\">\n", p->files[fi].c_str());
          fprintf(filter_file, "      <Filter>%s</Filter>\n", g.c_str());
          fprintf(filter_file, "    </ClCompile>\n");
        }
      }
    }
    fprintf(filter_file, "  </ItemGroup>\n");
  }

  fprintf(filter_file, "</Project>\n");
}

void vs2019_createProjectFile(const TProject* p, const char* project_id,
                              const std::vector<std::string>& project_ids, int folder_depth) {
  std::string prepend_path = "";
  for (int i = 0; i < folder_depth; ++i) {
    prepend_path += "../";
  }

  FILE* project_file = fopen((p->name + std::string(".vcxproj")).c_str(), "wb");
  fprintf(project_file, R"lit(<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
)lit");

  fprintf(project_file, "  <ItemGroup Label=\"ProjectConfigurations\">\n");
  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci) {
    auto c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      auto platform_label = privateData.platforms[pi]->label;
      fprintf(project_file, "    <ProjectConfiguration Include=\"%s|%s\">\n", c, platform_label);
      fprintf(project_file, "      <Configuration>%s</Configuration>\n", c);
      fprintf(project_file, "      <Platform>%s</Platform>\n",
              platform2String(privateData.platforms[pi]->type).c_str());
      fprintf(project_file, "    </ProjectConfiguration>\n");
    }
  }
  fprintf(project_file, "  </ItemGroup>\n");

  fprintf(project_file, R"lit(  <PropertyGroup Label="Globals">
    <VCProjectVersion>15.0</VCProjectVersion>
    <ProjectGuid>{%s}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>builder</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
)lit",
          project_id);

  fprintf(project_file,
          R"lit(  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
)lit");

  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci) {
    auto c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      auto platform_label = privateData.platforms[pi]->label;
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

  fprintf(project_file, R"lit(  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>
)lit");
  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci) {
    auto c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      auto platform_label = privateData.platforms[pi]->label;
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

  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci) {
    auto c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      auto platform_label = privateData.platforms[pi]->label;

      fprintf(project_file, "  <PropertyGroup Label=\"UserMacros\" />\n");
      fprintf(project_file,
              "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n", c,
              platform_label);
      bool is_debug_build = (stricmp(c, "debug") == 0);
      if (is_debug_build) {
        fprintf(project_file, "    <LinkIncremental>true</LinkIncremental>\n");
      } else {
        fprintf(project_file, "    <LinkIncremental>false</LinkIncremental>\n");
      }
      fprintf(project_file, "    <CustomBuildAfterTargets>Build</CustomBuildAfterTargets>\n");
      fprintf(project_file, "  </PropertyGroup>\n");
    }
  }

  std::vector<vs_compiler_setting> general_compiler_flags = {{"PrecompiledHeader", "NotUsing"},
                                                             {"WarningLevel", "Level3"},
                                                             {"SDLCheck", "true"},
                                                             {"ConformanceMode", "true"}};

  std::vector<vs_compiler_setting> general_linker_flags = {{"SubSystem", "Console"},
                                                           {"GenerateDebugInformation", "true"}};

  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci) {
    const auto config              = privateData.configurations[ci];
    const auto configuration_label = config->label;
    const bool is_debug_build      = (stricmp(configuration_label, "debug") == 0);
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      const auto platform = privateData.platforms[pi];
      auto compiler_flags = general_compiler_flags;  // Make copy
      if (is_debug_build) {
        compiler_flags.push_back({"Optimization", "Disabled"});
      } else {
        compiler_flags.push_back({"Optimization", "MaxSpeed"});
        compiler_flags.push_back({"BasicRuntimeChecks", "Default"});
      }

      std::string preprocessor_defines      = "_CONSOLE;%(PreprocessorDefinitions)";
      std::string additional_compiler_flags = "%(AdditionalOptions)";
      for (size_t ipc = 0; ipc < p->flags.size(); ++ipc) {
        // TODO ordering and combination so that more specific flags can override general ones
        if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;
        if ((p->platforms[ipc] != platform) && (p->platforms[ipc] != NULL)) continue;

        for (size_t pdi = 0; pdi < p->flags[ipc].defines.size(); ++pdi) {
          preprocessor_defines = p->flags[ipc].defines[pdi] + ";" + preprocessor_defines;
        }
        for (size_t cfi = 0; cfi < p->flags[ipc].compile_options.size(); ++cfi) {
          additional_compiler_flags =
              p->flags[ipc].compile_options[cfi] + " " + additional_compiler_flags;
        }
      }

      if (is_debug_build) {
        preprocessor_defines = "_DEBUG;" + preprocessor_defines;
      } else {
        preprocessor_defines = "NDEBUG;" + preprocessor_defines;
      }
      const bool is_win32 = (privateData.platforms[pi]->type == EPlatformTypeX86);
      if (is_win32) {
        preprocessor_defines = "WIN32;" + preprocessor_defines;
      }
      compiler_flags.push_back({"PreprocessorDefinitions", preprocessor_defines.c_str()});

      compiler_flags.push_back({"AdditionalOptions", additional_compiler_flags.c_str()});

      auto platform_label = privateData.platforms[pi]->label;
      fprintf(project_file,
              "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n",
              configuration_label, platform_label);
      fprintf(project_file, "    <ClCompile>\n");

      for (size_t cfi = 0; cfi < compiler_flags.size(); ++cfi) {
        const char* key   = compiler_flags[cfi].key;
        const char* value = compiler_flags[cfi].value;
        fprintf(project_file, "      <%s>%s</%s>\n", key, value, key);
      }

      fprintf(project_file, "    </ClCompile>\n");
      fprintf(project_file, "    <Link>\n");
      for (size_t cfi = 0; cfi < general_linker_flags.size(); ++cfi) {
        const char* key   = general_linker_flags[cfi].key;
        const char* value = general_linker_flags[cfi].value;
        fprintf(project_file, "      <%s>%s</%s>\n", key, value, key);
      }
      fprintf(project_file, "    </Link>\n");
      fprintf(project_file, "  </ItemDefinitionGroup>\n");
    }
  }

  for (unsigned fi = 0; fi < p->files.size(); ++fi) {
    fprintf(project_file, "  <ItemGroup>\n");
    auto f = prepend_path + p->files[fi];
    if (f.find(".h") != std::string::npos) {
      fprintf(project_file, "    <ClInclude Include=\"%s\" />\n", f.c_str());
    } else {
      fprintf(project_file, "    <ClCompile Include=\"%s\" />\n", f.c_str());
    }
    fprintf(project_file, "  </ItemGroup>\n");
  }

  for (size_t i = 0; i < p->dependantOn.size(); ++i) {
    const char* id = vs_findUUIDForProject(project_ids, p->dependantOn[i]);
    fprintf(project_file, R"lit(  <ItemGroup>
    <ProjectReference Include="my_library.vcxproj">
      <Project>{%s}</Project>
    </ProjectReference>
  </ItemGroup>
)lit",
            id);
  }

  fprintf(project_file, R"lit(  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>)lit");

  fclose(project_file);
}
