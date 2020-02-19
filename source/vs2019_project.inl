
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
}

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
    auto c = privateData.configurations[ci];
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      auto platform = privateData.platform_names[pi];
      fprintf(project_file, "    <ProjectConfiguration Include=\"%s|%s\">\n", c.c_str(),
              platform.c_str());
      fprintf(project_file, "      <Configuration>%s</Configuration>\n", c.c_str());
      fprintf(project_file, "      <Platform>%s</Platform>\n",
              platform2String(privateData.platforms[pi]).c_str());
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
    auto c = privateData.configurations[ci];
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      auto platform = privateData.platform_names[pi];
      fprintf(project_file,
              "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\" "
              "Label=\"Configuration\">\n",
              c.c_str(), platform.c_str());
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
    auto c = privateData.configurations[ci];
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi) {
      auto platform = privateData.platform_names[pi];
      fprintf(project_file,
              "  <ImportGroup Label=\"PropertySheets\" "
              "Condition=\"'$(Configuration)|$(Platform)'=='%s|%s'\">\n",
              c.c_str(), platform.c_str());
      fprintf(project_file,
              "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" "
              "Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" "
              "Label=\"LocalAppDataPlatform\" />\n");
      fprintf(project_file, "  </ImportGroup>\n");
    }
  }

  fprintf(project_file, R"lit(  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x86'">
    <LinkIncremental>true</LinkIncremental>
    <CustomBuildAfterTargets>Build</CustomBuildAfterTargets>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x86'">
    <LinkIncremental>false</LinkIncremental>
    <CustomBuildAfterTargets>Build</CustomBuildAfterTargets>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
  </PropertyGroup>
)lit");

  fprintf(project_file,
          R"lit(  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x86'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>Disabled</Optimization>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>WIN32;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <PrecompiledHeaderFile />
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <PrecompiledHeader>Use</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>Disabled</Optimization>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x86'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>MaxSpeed</Optimization>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>WIN32;NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <PrecompiledHeaderFile />
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <PrecompiledHeader>Use</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>MaxSpeed</Optimization>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
)lit");

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
