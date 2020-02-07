char *append_string(char *destination, const char *source)
{
  size_t length = strlen(source);
  strcpy(destination, source);
  return destination + length;
}

void xCodeCreateProjectFile(const TProject *in_project)
{
#if 0
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
#endif
  const char *projectContents = R"lit(// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 50;
	objects = {

/* Begin PBXBuildFile section */
		40151AD123EB48D600D4236A /* main.cpp in Sources */ = {isa = PBXBuildFile; fileRef = 40151AD023EB48D600D4236A /* main.cpp */; };
/* End PBXBuildFile section */

/* Begin PBXCopyFilesBuildPhase section */
		403CC53923EB479400558E07 /* CopyFiles */ = {
			isa = PBXCopyFilesBuildPhase;
			buildActionMask = 2147483647;
			dstPath = /usr/share/man/man1/;
			dstSubfolderSpec = 0;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 1;
		};
/* End PBXCopyFilesBuildPhase section */

/* Begin PBXFileReference section */
)lit";

  FILE *f = fopen("hello_world.xcodeproj/project.pbxproj", "wb");
  fwrite(projectContents, 1, strlen(projectContents), f);

  std::vector<char> buffer(1024 * 1024 * 10);
  char *appendBuffer = buffer.data();

  const TProject *p = (TProject *)in_project;
  for (unsigned fi = 0; fi < p->files.size(); ++fi)
  {
    const char *filename = p->files[fi].c_str();
    appendBuffer = append_string(appendBuffer, "		40151AD023EB48D600D4236A /* main.cpp */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = main.cpp; path = ");
    appendBuffer = append_string(appendBuffer, filename);
    appendBuffer = append_string(appendBuffer, "; sourceTree = SOURCE_ROOT; };\n");
    fwrite(buffer.data(), 1, appendBuffer - buffer.data(), f);
  }

  const char *projectContentsAfterFiles = R"lit(
		403CC53B23EB479400558E07 /* hello_world */ = {isa = PBXFileReference; explicitFileType = "compiled.mach-o.executable"; includeInIndex = 0; path = hello_world; sourceTree = BUILT_PRODUCTS_DIR; };
/* End PBXFileReference section */

/* Begin PBXFrameworksBuildPhase section */
		403CC53823EB479400558E07 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
		403CC53223EB479400558E07 = {
			isa = PBXGroup;
			children = (
				403CC53D23EB479400558E07 /* hello_world */,
				403CC53C23EB479400558E07 /* Products */,
			);
			sourceTree = "<group>";
		};
		403CC53C23EB479400558E07 /* Products */ = {
			isa = PBXGroup;
			children = (
				403CC53B23EB479400558E07 /* hello_world */,
			);
			name = Products;
			sourceTree = "<group>";
		};
		403CC53D23EB479400558E07 /* hello_world */ = {
			isa = PBXGroup;
			children = (
				403CC54523EB480800558E07 /* src */,
			);
			path = hello_world;
			sourceTree = "<group>";
		};
		403CC54523EB480800558E07 /* src */ = {
			isa = PBXGroup;
			children = (
				40151AD023EB48D600D4236A /* main.cpp */,
			);
			name = src;
			path = ../../src;
			sourceTree = "<group>";
		};
/* End PBXGroup section */

/* Begin PBXNativeTarget section */
		403CC53A23EB479400558E07 /* hello_world */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget "hello_world" */;
			buildPhases = (
				403CC53723EB479400558E07 /* Sources */,
				403CC53823EB479400558E07 /* Frameworks */,
				403CC53923EB479400558E07 /* CopyFiles */,
			);
			buildRules = (
			);
			dependencies = (
			);
			name = hello_world;
			productName = hello_world;
			productReference = 403CC53B23EB479400558E07 /* hello_world */;
			productType = "com.apple.product-type.tool";
		};
/* End PBXNativeTarget section */

/* Begin PBXProject section */
		403CC53323EB479400558E07 /* Project object */ = {
			isa = PBXProject;
			attributes = {
				LastUpgradeCheck = 1130;
				ORGANIZATIONNAME = "Daedalus Development";
				TargetAttributes = {
					403CC53A23EB479400558E07 = {
						CreatedOnToolsVersion = 11.3;
					};
				};
			};
			buildConfigurationList = 403CC53623EB479400558E07 /* Build configuration list for PBXProject "hello_world" */;
			compatibilityVersion = "Xcode 9.3";
			developmentRegion = en;
			hasScannedForEncodings = 0;
			knownRegions = (
				en,
				Base,
			);
			mainGroup = 403CC53223EB479400558E07;
			productRefGroup = 403CC53C23EB479400558E07 /* Products */;
			projectDirPath = "";
			projectRoot = "";
			targets = (
				403CC53A23EB479400558E07 /* hello_world */,
			);
		};
/* End PBXProject section */

/* Begin PBXSourcesBuildPhase section */
		403CC53723EB479400558E07 /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
				40151AD123EB48D600D4236A /* main.cpp in Sources */,
			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXSourcesBuildPhase section */

/* Begin XCBuildConfiguration section */
		403CC54023EB479400558E07 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++14";
				CLANG_CXX_LIBRARY = "libc++";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
				COPY_PHASE_STRIP = NO;
				DEBUG_INFORMATION_FORMAT = dwarf;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				ENABLE_TESTABILITY = YES;
				GCC_C_LANGUAGE_STANDARD = gnu11;
				GCC_DYNAMIC_NO_PIC = NO;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_OPTIMIZATION_LEVEL = 0;
				GCC_PREPROCESSOR_DEFINITIONS = (
					"DEBUG=1",
					"$(inherited)",
				);
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				MACOSX_DEPLOYMENT_TARGET = 10.14;
				MTL_ENABLE_DEBUG_INFO = INCLUDE_SOURCE;
				MTL_FAST_MATH = YES;
				ONLY_ACTIVE_ARCH = YES;
				SDKROOT = macosx;
			};
			name = Debug;
		};
		403CC54123EB479400558E07 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "gnu++14";
				CLANG_CXX_LIBRARY = "libc++";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
				COPY_PHASE_STRIP = NO;
				DEBUG_INFORMATION_FORMAT = "dwarf-with-dsym";
				ENABLE_NS_ASSERTIONS = NO;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				GCC_C_LANGUAGE_STANDARD = gnu11;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				MACOSX_DEPLOYMENT_TARGET = 10.14;
				MTL_ENABLE_DEBUG_INFO = NO;
				MTL_FAST_MATH = YES;
				SDKROOT = macosx;
			};
			name = Release;
		};
		403CC54323EB479400558E07 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				CODE_SIGN_STYLE = Automatic;
				PRODUCT_NAME = "$(TARGET_NAME)";
			};
			name = Debug;
		};
		403CC54423EB479400558E07 /* Release */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				CODE_SIGN_STYLE = Automatic;
				PRODUCT_NAME = "$(TARGET_NAME)";
			};
			name = Release;
		};
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
		403CC53623EB479400558E07 /* Build configuration list for PBXProject "hello_world" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				403CC54023EB479400558E07 /* Debug */,
				403CC54123EB479400558E07 /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget "hello_world" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				403CC54323EB479400558E07 /* Debug */,
				403CC54423EB479400558E07 /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
/* End XCConfigurationList section */
	};
	rootObject = 403CC53323EB479400558E07 /* Project object */;
}
)lit";

  fwrite(projectContentsAfterFiles, 1, strlen(projectContentsAfterFiles), f);
  fclose(f);
}

void xcode_generate()
{
  for (unsigned i = 0; i < privateData.projects.size(); ++i)
  {
    auto p = privateData.projects[i];
    xCodeCreateProjectFile(p);
  }
}

TBuilder cc_xcode_builder = {
    {
        ::createProject,
        ::addFileToProject,
    },
    {
        ::setOutputFolder,
        ::addProject,
        ::addConfiguration,
        ::addPlatform,
    },
    xcode_generate};