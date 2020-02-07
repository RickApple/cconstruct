void createFilters(const TProject *in_project)
{
  std::ofstream vcxproj_filters_file("test/builder.vcxproj.filters");

  vcxproj_filters_file
      << R"lit(<?xml version="1.0" encoding="utf-8"?>)lit" << std::endl
      << R"lit(<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">)lit"
      << std::endl;
  // Remove duplicate groups
  const TProject *p = (TProject *)in_project;
  std::set<std::string> unique_groups;
  for (auto &g : p->groups)
    unique_groups.insert(g);
  unique_groups.erase("");

  vcxproj_filters_file << "  <ItemGroup>" << std::endl;
  int count = 0;
  for (auto &g : unique_groups)
  {
    vcxproj_filters_file << "    <Filter Include=\"" << g << "\">" << std::endl;
    vcxproj_filters_file << "      <UniqueIdentifier>{d43b239c-1ecb-43eb-8aca-28bf989c811" << ++count
                         << "}</UniqueIdentifier>" << std::endl;
    vcxproj_filters_file << "    </Filter>" << std::endl;
  }
  vcxproj_filters_file << "  </ItemGroup>" << std::endl;

  for (auto &g : unique_groups)
  {
    vcxproj_filters_file << "  <ItemGroup>" << std::endl;
    for (unsigned fi = 0; fi < p->files.size(); ++fi)
    {
      if (p->groups[fi] == g)
      {
        auto f = p->files[fi];
        if (f.find(".h") != std::string::npos)
        {
          vcxproj_filters_file << "    <ClInclude Include=\"" << p->files[fi] << "\">" << std::endl;
          vcxproj_filters_file << "      <Filter>" << g << "</Filter>" << std::endl;
          vcxproj_filters_file << "    </ClInclude>" << std::endl;
        }
        else
        {
          vcxproj_filters_file << "    <ClCompile Include=\"" << p->files[fi] << "\">" << std::endl;
          vcxproj_filters_file << "      <Filter>" << g << "</Filter>" << std::endl;
          vcxproj_filters_file << "    </ClCompile>" << std::endl;
        }
      }
    }
    vcxproj_filters_file << "  </ItemGroup>" << std::endl;
  }

  vcxproj_filters_file << "</Project>" << std::endl;
}
void createProjectFile(const TProject *in_project)
{
  std::ofstream vcxproj_file("test/builder.vcxproj");
  vcxproj_file << R"lit(<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
)lit";
  vcxproj_file << "  <ItemGroup Label=\"ProjectConfigurations\">" << std::endl;

  const char *platform_types[] = {"Win32", "x64", "ARM"};
  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci)
  {
    auto c = privateData.configurations[ci];
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi)
    {
      auto platform = privateData.platform_names[pi];
      vcxproj_file << "    <ProjectConfiguration Include=\"" << c << "|" << platform << "\">" << std::endl
                   << "      <Configuration>" << c << "</Configuration>" << std::endl
                   << "      <Platform>" << platform_types[privateData.platforms[pi]] << "</Platform>" << std::endl
                   << "    </ProjectConfiguration>" << std::endl;
    }
  }
  vcxproj_file << "  </ItemGroup>" << std::endl;
  vcxproj_file << R"lit(  <PropertyGroup Label="Globals">
    <VCProjectVersion>15.0</VCProjectVersion>
    <ProjectGuid>{4470604D-2B04-466E-A39B-9E49BA6DA261}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>builder</RootNamespace>
    <WindowsTargetPlatformVersion>10.0.16299.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />)lit"
               << std::endl;
  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci)
  {
    auto c = privateData.configurations[ci];
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi)
    {
      auto platform = privateData.platform_names[pi];
      vcxproj_file << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='" << c << "|" << platform
                   << "'\" Label=\"Configuration\">" << std::endl
                   << "    <ConfigurationType>Application</ConfigurationType>" << std::endl
                   << "    <UseDebugLibraries>true</UseDebugLibraries>" << std::endl
                   << "    <PlatformToolset>v141</PlatformToolset>" << std::endl
                   << "    <CharacterSet>Unicode</CharacterSet>" << std::endl
                   << "  </PropertyGroup>" << std::endl;
    }
  }
  vcxproj_file << R"lit(  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="Shared">
  </ImportGroup>)lit"
               << std::endl;
  for (unsigned ci = 0; ci < privateData.configurations.size(); ++ci)
  {
    auto c = privateData.configurations[ci];
    for (unsigned pi = 0; pi < privateData.platforms.size(); ++pi)
    {
      auto platform = privateData.platform_names[pi];
      vcxproj_file << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='" << c
                   << "|" << platform << "'\">" << std::endl
                   << "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" "
                      "Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" "
                      "Label=\"LocalAppDataPlatform\" />"
                   << std::endl
                   << "  </ImportGroup>" << std::endl;
    }
  }

  vcxproj_file << R"lit(  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <LinkIncremental>true</LinkIncremental>
    <CustomBuildAfterTargets>Build</CustomBuildAfterTargets>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <LinkIncremental>false</LinkIncremental>
    <CustomBuildAfterTargets>Build</CustomBuildAfterTargets>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
  </PropertyGroup>)lit"
               << std::endl;

  vcxproj_file << R"lit(  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
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
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
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
)lit";

  const TProject *p = (TProject *)in_project;
  for (unsigned fi = 0; fi < p->files.size(); ++fi)
  {
    vcxproj_file << "  <ItemGroup>" << std::endl;
    auto f = p->files[fi];
    if (f.find(".h") != std::string::npos)
    {
      vcxproj_file << "    <ClInclude Include=\"" << f << "\" />" << std::endl;
    }
    else
    {
      vcxproj_file << "    <ClCompile Include=\"" << f << "\" />" << std::endl;
    }
    vcxproj_file << "  </ItemGroup>" << std::endl;
  }

  vcxproj_file << R"lit(  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>)lit" << std::endl;

  createFilters(in_project);
}
