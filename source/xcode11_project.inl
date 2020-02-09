
char *append_string(char *destination, const char *source)
{
	size_t length = strlen(source);
	strcpy(destination, source);
	return destination + length;
}

typedef struct xcode_uuid
{
	unsigned int uuid[3];
} xcode_uuid;

static_assert(sizeof(xcode_uuid) == 12, "Incorrect size of UUID");

xcode_uuid xCodeGenerateUUID()
{
	static size_t count = 0;

	xcode_uuid out = {0};
	out.uuid[0] = ++count;
	return out;
}
const char *xCodeUUID2String(xcode_uuid uuid)
{
	static char out[25] = {0};

	// Incorrect byte order, but don't care for now
	sprintf(out, "%08x%08x%08x", uuid.uuid[0], uuid.uuid[1], uuid.uuid[2]);

	return out;
}

xcode_uuid findUUIDForProject(const std::vector<xcode_uuid> &uuids, const TProject *project)
{
	for (unsigned i = 0; i < uuids.size(); ++i)
	{
		if (privateData.projects[i] == project)
			return uuids[i];
	}
	return {};
}

void xCodeCreateProjectFile(const TProject *in_project, const std::vector<xcode_uuid> &projectFileReferenceUUIDs, int folder_depth)
{
	const TProject *p = (TProject *)in_project;

	std::string prepend_path = "";
	for (int i = 0; i < folder_depth; ++i)
	{
		prepend_path += "../";
	}

	std::vector<std::string> file_ref_paths(p->files.size());
	std::transform(p->files.begin(), p->files.end(), file_ref_paths.begin(), [&](const std::string &f) {
		return prepend_path + f;
	});

	std::vector<xcode_uuid> fileReferenceUUID(p->files.size());
	std::vector<xcode_uuid> fileUUID(p->files.size());
	for (unsigned fi = 0; fi < p->files.size(); ++fi)
	{
		fileReferenceUUID[fi] = xCodeGenerateUUID();
		fileUUID[fi] = xCodeGenerateUUID();
	}

	std::vector<xcode_uuid> dependencyFileReferenceUUID(p->dependantOn.size());
	std::vector<xcode_uuid> dependencyBuildUUID(p->dependantOn.size());
	for (unsigned fi = 0; fi < p->dependantOn.size(); ++fi)
	{
		dependencyFileReferenceUUID[fi] = xCodeGenerateUUID();
		dependencyBuildUUID[fi] = xCodeGenerateUUID();
	}

	xcode_uuid outputFileReferenceUIID = findUUIDForProject(projectFileReferenceUUIDs, p);
	xcode_uuid outputTargetUIID = xCodeGenerateUUID();

	std::string outputName = p->name;
	if (p->type == CCProjectTypeStaticLibrary)
	{
		outputName = "lib" + outputName + ".a";
	}

	std::vector<char> buffer(1024 * 1024 * 10);
	char *appendBuffer = buffer.data();

	appendBuffer = append_string(appendBuffer, R"lit(// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 50;
	objects = {

/* Begin PBXBuildFile section */
)lit");

	for (unsigned fi = 0; fi < p->files.size(); ++fi)
	{
		const char *filename = p->files[fi].c_str();
		appendBuffer = append_string(appendBuffer, "		");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(fileUUID[fi]));
		appendBuffer = append_string(appendBuffer, " /* ");
		appendBuffer = append_string(appendBuffer, strip_path(filename));
		appendBuffer = append_string(appendBuffer, " in Sources */ = {isa = PBXBuildFile; fileRef = ");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(fileReferenceUUID[fi]));
		appendBuffer = append_string(appendBuffer, " /* ");
		appendBuffer = append_string(appendBuffer, strip_path(filename));
		appendBuffer = append_string(appendBuffer, " */; };\n");
	}
	for (size_t i = 0; i < p->dependantOn.size(); ++i)
	{
		xcode_uuid id = dependencyFileReferenceUUID[i];
		xcode_uuid buildID = dependencyBuildUUID[i];
		std::string dependantName = "lib" + p->dependantOn[i]->name + ".a";
		appendBuffer = append_string(appendBuffer, "		");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(buildID));
		appendBuffer = append_string(appendBuffer, " /* ");
		appendBuffer = append_string(appendBuffer, dependantName.c_str());
		appendBuffer = append_string(appendBuffer, " in Frameworks */ = {isa = PBXBuildFile; fileRef = ");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(id));
		appendBuffer = append_string(appendBuffer, " /* ");
		appendBuffer = append_string(appendBuffer, dependantName.c_str());
		appendBuffer = append_string(appendBuffer, " */; };\n");
	}
	appendBuffer = append_string(appendBuffer, "/* End PBXBuildFile section */\n\n");

	if (p->type == CCProjectTypeConsoleApplication)
	{
		appendBuffer = append_string(appendBuffer, R"lit(/* Begin PBXCopyFilesBuildPhase section */
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

)lit");
	}

	appendBuffer = append_string(appendBuffer, "/* Begin PBXFileReference section */\n");
	for (unsigned fi = 0; fi < p->files.size(); ++fi)
	{
		const char *filename = p->files[fi].c_str();
		appendBuffer = append_string(appendBuffer, "		");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(fileReferenceUUID[fi]));
		appendBuffer = append_string(appendBuffer, " /* ");
		appendBuffer = append_string(appendBuffer, strip_path(filename));
		appendBuffer = append_string(appendBuffer, " */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = ");
		appendBuffer = append_string(appendBuffer, strip_path(filename));
		appendBuffer = append_string(appendBuffer, "; path = ");
		appendBuffer = append_string(appendBuffer, file_ref_paths[fi].c_str());
		appendBuffer = append_string(appendBuffer, "; sourceTree = SOURCE_ROOT; };\n");
		printf("Adding file '%s' as '%s'\n", filename, file_ref_paths[fi].c_str());
	}

	appendBuffer = append_string(appendBuffer, "		");
	appendBuffer = append_string(appendBuffer, xCodeUUID2String(outputFileReferenceUIID));
	appendBuffer = append_string(appendBuffer, " /* ");
	appendBuffer = append_string(appendBuffer, outputName.c_str());
	if (p->type == CCProjectTypeConsoleApplication)
	{
		appendBuffer = append_string(appendBuffer, " */ = {isa = PBXFileReference; explicitFileType = \"compiled.mach-o.executable\"; includeInIndex = 0; path = ");
	}
	else
	{
		appendBuffer = append_string(appendBuffer, " */ = {isa = PBXFileReference; explicitFileType = \"archive.ar\"; includeInIndex = 0; path = ");
	}
	appendBuffer = append_string(appendBuffer, outputName.c_str());
	appendBuffer = append_string(appendBuffer, "; sourceTree = BUILT_PRODUCTS_DIR; };\n");

	for (size_t i = 0; i < p->dependantOn.size(); ++i)
	{
		xcode_uuid id = dependencyFileReferenceUUID[i];
		appendBuffer = append_string(appendBuffer, "		");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(id));
		appendBuffer = append_string(appendBuffer, " /* libmy_library.a */ = {isa = PBXFileReference; explicitFileType = archive.ar; path = libmy_library.a; sourceTree = BUILT_PRODUCTS_DIR; };\n");
	}
	appendBuffer = append_string(appendBuffer, "/* End PBXFileReference section */\n\n");

	appendBuffer = append_string(appendBuffer, R"lit(/* Begin PBXFrameworksBuildPhase section */
		403CC53823EB479400558E07 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
)lit");
	for (size_t i = 0; i < p->dependantOn.size(); ++i)
	{
		xcode_uuid buildID = dependencyBuildUUID[i];
		appendBuffer = append_string(appendBuffer, "				");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(buildID));
		appendBuffer = append_string(appendBuffer, " /* lib");
		appendBuffer = append_string(appendBuffer, p->dependantOn[i]->name.c_str());
		appendBuffer = append_string(appendBuffer, ".a in Frameworks */,\n");
	}
	appendBuffer = append_string(appendBuffer, R"lit(			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
		4008B25F23EDACFC00FCB192 /* Frameworks */ = {
			isa = PBXGroup;
			children = (
)lit");
	for (size_t i = 0; i < p->dependantOn.size(); ++i)
	{
		xcode_uuid id = dependencyFileReferenceUUID[i];
	}
	appendBuffer = append_string(appendBuffer, R"lit(			);
			name = Frameworks;
			sourceTree = "<group>";
		};
		403CC53223EB479400558E07 = {
			isa = PBXGroup;
			children = (
				403CC53D23EB479400558E07 /* )lit");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit( */,
				403CC53C23EB479400558E07 /* Products */,
				4008B25F23EDACFC00FCB192 /* Frameworks */,
			);
			sourceTree = "<group>";
		};
		403CC53C23EB479400558E07 /* Products */ = {
			isa = PBXGroup;
			children = (
				)lit");
	appendBuffer = append_string(appendBuffer, xCodeUUID2String(outputFileReferenceUIID));
	appendBuffer = append_string(appendBuffer, " /* ");
	appendBuffer = append_string(appendBuffer, outputName.c_str());
	appendBuffer = append_string(appendBuffer, R"lit( */,
			);
			name = Products;
			sourceTree = "<group>";
		};
		403CC53D23EB479400558E07 /* )lit");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit( */ = {
			isa = PBXGroup;
			children = (
				403CC54523EB480800558E07 /* src */,
			);
			path = )lit");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit(;
			sourceTree = "<group>";
		};
		403CC54523EB480800558E07 /* src */ = {
			isa = PBXGroup;
			children = (
)lit");
	for (unsigned fi = 0; fi < p->files.size(); ++fi)
	{
		const char *filename = p->files[fi].c_str();
		appendBuffer = append_string(appendBuffer, "			    ");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(fileReferenceUUID[fi]));
		appendBuffer = append_string(appendBuffer, " /* ");
		appendBuffer = append_string(appendBuffer, strip_path(filename));
		appendBuffer = append_string(appendBuffer, " */,\n");
	}
	appendBuffer = append_string(appendBuffer, R"lit(			);
			name = src;
			path = ../../src;
			sourceTree = "<group>";
		};
/* End PBXGroup section */

/* Begin PBXNativeTarget section */
)lit");
	appendBuffer = append_string(appendBuffer, "		");
	appendBuffer = append_string(appendBuffer, xCodeUUID2String(outputTargetUIID));
	appendBuffer = append_string(appendBuffer, " /* ");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit( */ = {
			isa = PBXNativeTarget;
			buildConfigurationList = 403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget ")lit");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit(" */;
			buildPhases = (
				403CC53723EB479400558E07 /* Sources */,
				403CC53823EB479400558E07 /* Frameworks */,
				403CC53923EB479400558E07 /* CopyFiles */,
			);
			buildRules = (
			);
			dependencies = (
			);
)lit");

	appendBuffer = append_string(appendBuffer, "			name = ");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, ";\n");
	appendBuffer = append_string(appendBuffer, "			productName = ");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, ";\n");
	appendBuffer = append_string(appendBuffer, "			productReference = ");
	appendBuffer = append_string(appendBuffer, xCodeUUID2String(outputFileReferenceUIID));
	appendBuffer = append_string(appendBuffer, " /* ");
	appendBuffer = append_string(appendBuffer, outputName.c_str());
	appendBuffer = append_string(appendBuffer, " */;\n");
	appendBuffer = append_string(appendBuffer, "			productType = ");
	if (p->type == CCProjectTypeConsoleApplication)
	{
		appendBuffer = append_string(appendBuffer, "\"com.apple.product-type.tool\"");
	}
	else
	{
		appendBuffer = append_string(appendBuffer, "\"com.apple.product-type.library.static\"");
	}
	appendBuffer = append_string(appendBuffer, R"lit(;
		};
/* End PBXNativeTarget section */

/* Begin PBXProject section */
		403CC53323EB479400558E07 /* Project object */ = {
			isa = PBXProject;
			attributes = {
				LastUpgradeCheck = 1130;
				ORGANIZATIONNAME = "Daedalus Development";
				TargetAttributes = {
					)lit");
	appendBuffer = append_string(appendBuffer, xCodeUUID2String(outputTargetUIID));
	appendBuffer = append_string(appendBuffer, R"lit( = {
						CreatedOnToolsVersion = 11.3;
					};
				};
			};
			buildConfigurationList = 403CC53623EB479400558E07 /* Build configuration list for PBXProject ")lit");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit(" */;
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
				)lit");
	appendBuffer = append_string(appendBuffer, xCodeUUID2String(outputTargetUIID));
	appendBuffer = append_string(appendBuffer, R"lit( /* )lit");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit( */,
			);
		};
/* End PBXProject section */

/* Begin PBXSourcesBuildPhase section */
		403CC53723EB479400558E07 /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
)lit");

	for (unsigned fi = 0; fi < p->files.size(); ++fi)
	{
		const char *filename = p->files[fi].c_str();
		appendBuffer = append_string(appendBuffer, "\t\t\t\t");
		appendBuffer = append_string(appendBuffer, xCodeUUID2String(fileUUID[fi]));
		appendBuffer = append_string(appendBuffer, " /* ");
		appendBuffer = append_string(appendBuffer, strip_path(filename));
		appendBuffer = append_string(appendBuffer, " in Sources */,\n");
	}

	appendBuffer = append_string(appendBuffer, R"lit(			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXSourcesBuildPhase section */

/* Begin XCBuildConfiguration section */
		403CC54023EB479400558E07 /* Debug */ = {
			isa = XCBuildConfiguration;
			buildSettings = {
				CONFIGURATION_BUILD_DIR = ")lit");
	appendBuffer = append_string(appendBuffer, privateData.outputFolder);
	appendBuffer = append_string(appendBuffer, R"lit(";
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
				CONFIGURATION_BUILD_DIR = ")lit");
	appendBuffer = append_string(appendBuffer, privateData.outputFolder);
	appendBuffer = append_string(appendBuffer, R"lit(";
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
		403CC53623EB479400558E07 /* Build configuration list for PBXProject ")lit");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit(" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				403CC54023EB479400558E07 /* Debug */,
				403CC54123EB479400558E07 /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget ")lit");
	appendBuffer = append_string(appendBuffer, p->name.c_str());
	appendBuffer = append_string(appendBuffer, R"lit(" */ = {
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
)lit");

	std::string projectFilePath = p->name;
	projectFilePath += ".xcodeproj";
	int result = mkdir(projectFilePath.c_str(), 0777);

	projectFilePath += "/project.pbxproj";

	FILE *f = fopen(projectFilePath.c_str(), "wb");
	fwrite(buffer.data(), 1, appendBuffer - buffer.data(), f);
	fclose(f);
}

void xCodeCreateWorkspaceFile()
{
	std::vector<char> buffer(1024 * 1024 * 10);
	char *appendBuffer = buffer.data();

	appendBuffer = append_string(appendBuffer,
								 R"lit(<?xml version="1.0" encoding="UTF-8"?>
<Workspace
   version = "1.0">)lit");

	for (unsigned i = 0; i < privateData.projects.size(); ++i)
	{
		auto p = privateData.projects[i];
		appendBuffer = append_string(appendBuffer, "  <FileRef");
		appendBuffer = append_string(appendBuffer, "    location = \"group:");
		appendBuffer = append_string(appendBuffer, p->name.c_str());
		appendBuffer = append_string(appendBuffer, ".xcodeproj\">");
		appendBuffer = append_string(appendBuffer, "  </FileRef>");
	}
	appendBuffer = append_string(appendBuffer, "</Workspace>");

	std::string workspaceFilePath = privateData.workspaceLabel;
	workspaceFilePath += ".xcworkspace";
	int result = mkdir(workspaceFilePath.c_str(), 0777);
	workspaceFilePath += "/contents.xcworkspacedata";
	FILE *f = fopen(workspaceFilePath.c_str(), "wb");
	fwrite(buffer.data(), 1, appendBuffer - buffer.data(), f);
	fclose(f);
}

void xcode_generateInFolder(const char *workspace_path)
{
	int count_folder_depth = 1;
	{
		const char *c = workspace_path;
		while (*c)
		{
			if (*c == '/')
				count_folder_depth += 1;
			++c;
		}
		if (*(c - 1) == '/')
			count_folder_depth -= 1;
	}

	int result = make_folder(workspace_path);
	if (result != 0)
	{
		fprintf(stderr, "Error %i creating path '%s'\n", result, workspace_path);
	}
	(void)chdir(workspace_path);

	// Before doing anything, generate a UUID for each projects output file
	std::vector<xcode_uuid> projectFileReferenceUUIDs(privateData.projects.size());
	for (unsigned i = 0; i < privateData.projects.size(); ++i)
	{
		projectFileReferenceUUIDs[i] = xCodeGenerateUUID();
	}

	for (unsigned i = 0; i < privateData.projects.size(); ++i)
	{
		auto p = privateData.projects[i];
		xCodeCreateProjectFile(p, projectFileReferenceUUIDs, count_folder_depth);
	}

	xCodeCreateWorkspaceFile();
}

CConstruct cc_xcode_builder = {
	{createProject,
	 addFileToProject,
	 addInputProject},
	{
		setWorkspaceLabel,
		setOutputFolder,
		addProject,
		addConfiguration,
		addPlatform,
	},
	xcode_generateInFolder};