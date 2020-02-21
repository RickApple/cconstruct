
struct xcode_compiler_setting {
  const char* key;
  const char* value;
};

typedef struct xcode_uuid {
  unsigned int uuid[3];
} xcode_uuid;

static_assert(sizeof(xcode_uuid) == 12, "Incorrect size of UUID");

xcode_uuid xCodeGenerateUUID() {
  static size_t count = 0;

  xcode_uuid out = {0};
  out.uuid[0]    = ++count;
  return out;
}
const char* xCodeUUID2String(xcode_uuid uuid) {
  static char out[25] = {0};

  // Incorrect byte order, but don't care for now
  sprintf(out, "%08x%08x%08x", uuid.uuid[0], uuid.uuid[1], uuid.uuid[2]);

  return out;
}

xcode_uuid findUUIDForProject(const std::vector<xcode_uuid>& uuids, const TProject* project) {
  for (unsigned i = 0; i < uuids.size(); ++i) {
    if (privateData.projects[i] == project) return uuids[i];
  }
  return {};
}

void xCodeCreateProjectFile(FILE* f, const TProject* in_project,
                            const std::vector<xcode_uuid>& projectFileReferenceUUIDs,
                            int folder_depth) {
  const TProject* p = (TProject*)in_project;

  std::string prepend_path = "";
  for (int i = 0; i < folder_depth; ++i) {
    prepend_path += "../";
  }

  std::vector<std::string> file_ref_paths(p->files.size());
  std::transform(p->files.begin(), p->files.end(), file_ref_paths.begin(),
                 [&](const std::string& f) { return prepend_path + f; });

  std::vector<std::string> fileReferenceUUID(p->files.size());
  std::vector<std::string> fileUUID(p->files.size());
  for (unsigned fi = 0; fi < p->files.size(); ++fi) {
    fileReferenceUUID[fi] = xCodeUUID2String(xCodeGenerateUUID());
    fileUUID[fi]          = xCodeUUID2String(xCodeGenerateUUID());
  }

  std::vector<std::string> dependencyFileReferenceUUID(p->dependantOn.size());
  std::vector<std::string> dependencyBuildUUID(p->dependantOn.size());
  for (unsigned fi = 0; fi < p->dependantOn.size(); ++fi) {
    dependencyFileReferenceUUID[fi] = xCodeUUID2String(xCodeGenerateUUID());
    dependencyBuildUUID[fi]         = xCodeUUID2String(xCodeGenerateUUID());
  }

  xcode_uuid outputFileReferenceUIID = findUUIDForProject(projectFileReferenceUUIDs, p);
  xcode_uuid outputTargetUIID        = xCodeGenerateUUID();

  std::string outputName = p->name;
  if (p->type == CCProjectTypeStaticLibrary) {
    outputName = "lib" + outputName + ".a";
  }

  fprintf(f, R"lit(// !$*UTF8*$!
{
	archiveVersion = 1;
	classes = {
	};
	objectVersion = 50;
	objects = {

/* Begin PBXBuildFile section */
)lit");

  for (unsigned fi = 0; fi < p->files.size(); ++fi) {
    const char* filename = p->files[fi].c_str();
    fprintf(f,
            "		%s /* %s in Sources */ = {isa = PBXBuildFile; fileRef = %s /* %s */; };\n",
            fileUUID[fi].c_str(), strip_path(filename), fileReferenceUUID[fi].c_str(),
            strip_path(filename));
  }
  for (size_t i = 0; i < p->dependantOn.size(); ++i) {
    std::string id            = dependencyFileReferenceUUID[i];
    std::string buildID       = dependencyBuildUUID[i];
    std::string dependantName = std::string("lib") + p->dependantOn[i]->name + ".a";
    fprintf(f,
            "		%s /* %s in Frameworks */ = {isa = PBXBuildFile; fileRef = %s /* %s */; "
            "};\n",
            buildID.c_str(), dependantName.c_str(), id.c_str(), dependantName.c_str());
  }
  fprintf(f, "/* End PBXBuildFile section */\n\n");

  if (p->type == CCProjectTypeConsoleApplication) {
    fprintf(f, R"lit(/* Begin PBXCopyFilesBuildPhase section */
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

  fprintf(f, "/* Begin PBXFileReference section */\n");
  for (unsigned fi = 0; fi < p->files.size(); ++fi) {
    const char* filename = p->files[fi].c_str();
    fprintf(f,
            "		%s /* %s */ = {isa = PBXFileReference; fileEncoding = 4; "
            "lastKnownFileType "
            "= sourcecode.cpp.cpp; name = %s; path = %s; sourceTree = SOURCE_ROOT; };\n",
            fileReferenceUUID[fi].c_str(), strip_path(filename), strip_path(filename),
            file_ref_paths[fi].c_str());
    printf("Adding file '%s' as '%s'\n", filename, file_ref_paths[fi].c_str());
  }
  fprintf(f, "		%s /* %s */ = {isa = PBXFileReference; explicitFileType = \"",
          xCodeUUID2String(outputFileReferenceUIID), outputName.c_str());
  if (p->type == CCProjectTypeConsoleApplication) {
    fprintf(f, "compiled.mach-o.executable");
  } else {
    fprintf(f, "archive.ar");
  }
  fprintf(f, "\"; includeInIndex = 0; path = %s; sourceTree = BUILT_PRODUCTS_DIR; };\n",
          outputName.c_str());

  for (size_t i = 0; i < p->dependantOn.size(); ++i) {
    std::string id = dependencyFileReferenceUUID[i];
    fprintf(f,
            "		%s /* libmy_library.a */ = {isa = PBXFileReference; explicitFileType = "
            "archive.ar; path = libmy_library.a; sourceTree = BUILT_PRODUCTS_DIR; };\n",
            id.c_str());
  }
  fprintf(f, "/* End PBXFileReference section */\n\n");

  fprintf(f, R"lit(/* Begin PBXFrameworksBuildPhase section */
		403CC53823EB479400558E07 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
)lit");
  for (size_t i = 0; i < p->dependantOn.size(); ++i) {
    std::string buildID = dependencyBuildUUID[i];
    fprintf(f, "				%s /* lib%s.a in Frameworks */,\n",
            buildID.c_str(), p->dependantOn[i]->name);
  }
  fprintf(f, R"lit(			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXFrameworksBuildPhase section */

)lit");

  fprintf(f, R"lit(/* Begin PBXGroup section */
		4008B25F23EDACFC00FCB192 /* Frameworks */ = {
			isa = PBXGroup;
			children = (
			);
			name = Frameworks;
			sourceTree = "<group>";
		};
		403CC53223EB479400558E07 = {
			isa = PBXGroup;
			children = (
				403CC53D23EB479400558E07 /* )lit");
  fprintf(f, "%s", p->name);
  fprintf(f, R"lit( */,
				403CC53C23EB479400558E07 /* Products */,
				4008B25F23EDACFC00FCB192 /* Frameworks */,
			);
			sourceTree = "<group>";
		};
		403CC53C23EB479400558E07 /* Products */ = {
			isa = PBXGroup;
			children = (
)lit");
  fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(outputFileReferenceUIID),
          outputName.c_str());
  fprintf(f, R"lit(			);
			name = Products;
			sourceTree = "<group>";
		};
		403CC53D23EB479400558E07 /* )lit");
  fprintf(f, "%s", p->name);
  fprintf(f, R"lit( */ = {
			isa = PBXGroup;
			children = (
				403CC54523EB480800558E07 /* src */,
			);
			path = )lit");
  fprintf(f, "%s", p->name);
  fprintf(f, R"lit(;
			sourceTree = "<group>";
		};
		403CC54523EB480800558E07 /* src */ = {
			isa = PBXGroup;
			children = (
)lit");
  for (unsigned fi = 0; fi < p->files.size(); ++fi) {
    const char* filename = p->files[fi].c_str();
    fprintf(f, "			    %s /* %s */,\n", fileReferenceUUID[fi].c_str(),
            strip_path(filename));
  }
  fprintf(f, R"lit(			);
			name = src;
			path = ../../src;
			sourceTree = "<group>";
		};
/* End PBXGroup section */

/* Begin PBXNativeTarget section */
)lit");
  fprintf(f, "		%s /* %s */ = {\n", xCodeUUID2String(outputTargetUIID), p->name);
  fprintf(f, R"lit(			isa = PBXNativeTarget;
			buildConfigurationList = 403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget ")lit");
  fprintf(f, "%s", p->name);
  fprintf(f, R"lit(" */;
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

  fprintf(f, "			name = %s;\n", p->name);
  fprintf(f, "			productName = %s;\n", p->name);
  fprintf(f, "			productReference = %s /* %s */;\n",
          xCodeUUID2String(outputFileReferenceUIID), outputName.c_str());
  fprintf(f, "			productType = ");
  if (p->type == CCProjectTypeConsoleApplication) {
    fprintf(f, "\"com.apple.product-type.tool\"");
  } else {
    fprintf(f, "\"com.apple.product-type.library.static\"");
  }
  fprintf(f, R"lit(;
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
  fprintf(f, "%s", xCodeUUID2String(outputTargetUIID));
  fprintf(f, R"lit( = {
						CreatedOnToolsVersion = 11.3;
					};
				};
			};
			buildConfigurationList = 403CC53623EB479400558E07 /* Build configuration list for PBXProject ")lit");
  fprintf(f, "%s", p->name);
  fprintf(f, R"lit(" */;
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
  fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(outputTargetUIID),
          p->name);
  fprintf(f, R"lit(			);
		};
/* End PBXProject section */

/* Begin PBXSourcesBuildPhase section */
		403CC53723EB479400558E07 /* Sources */ = {
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
)lit");

  for (unsigned fi = 0; fi < p->files.size(); ++fi) {
    const char* filename = p->files[fi].c_str();
    fprintf(f, "				%s /* %s in Sources */,\n", fileUUID[fi].c_str(),
            strip_path(filename));
  }

  fprintf(f, R"lit(			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXSourcesBuildPhase section */

)lit");

  std::vector<xcode_uuid> configuration_ids(privateData.configurations.size());
  for (size_t i = 0; i < privateData.configurations.size(); ++i) {
    configuration_ids[i] = xCodeGenerateUUID();
  }
  std::string safe_output_folder = "\"" + std::string(privateData.outputFolder) + "\"";
  std::vector<struct xcode_compiler_setting> debug_config_data = {
      {"CONFIGURATION_BUILD_DIR", safe_output_folder.c_str()},
      {"DEBUG_INFORMATION_FORMAT", "dwarf"},
      {"ENABLE_TESTABILITY", "YES"},
      {"GCC_OPTIMIZATION_LEVEL", "0"},
      {"GCC_PREPROCESSOR_DEFINITIONS", R"lit((
					"DEBUG=1",
					"$(inherited)",
				))lit"},
      {"MACOSX_DEPLOYMENT_TARGET", "10.14"},
      {"ONLY_ACTIVE_ARCH", "YES"},
      {"SDKROOT", "macosx"}};
  std::vector<struct xcode_compiler_setting> release_config_data = {
      {"CONFIGURATION_BUILD_DIR", safe_output_folder.c_str()},
      {"DEBUG_INFORMATION_FORMAT", "\"dwarf-with-dsym\""},
      {"ENABLE_NS_ASSERTIONS", "NO"},
      {"MACOSX_DEPLOYMENT_TARGET", "10.14"},
      {"ONLY_ACTIVE_ARCH", "YES"},
      {"SDKROOT", "macosx"}};

  std::vector<std::vector<struct xcode_compiler_setting>> config_data = {debug_config_data,
                                                                         release_config_data};
  fprintf(f, "/* Begin XCBuildConfiguration section */\n");
  for (size_t i = 0; i < privateData.configurations.size(); ++i) {
    const char* config_name = privateData.configurations[i]->label;
    xcode_uuid config_id    = configuration_ids[i];

    fprintf(f, "		%s /* %s */ = {\n", xCodeUUID2String(config_id), config_name);
    fprintf(f, "			isa = XCBuildConfiguration;\n");
    fprintf(f, "			buildSettings = {\n");
    auto config = config_data[i];
    for (size_t ic = 0; ic < config.size(); ++ic) {
      fprintf(f, "				%s = %s;\n", config[ic].key, config[ic].value);
    }
    fprintf(f, "			};\n");
    fprintf(f, "			name = %s;\n", config_name);
    fprintf(f, "		};\n");
  }
  fprintf(f, R"lit(		403CC54323EB479400558E07 /* Debug */ = {
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

)lit");

  fprintf(f, R"lit(/* Begin XCConfigurationList section */
		403CC53623EB479400558E07 /* Build configuration list for PBXProject ")lit");
  fprintf(f, "%s", p->name);
  fprintf(f, R"lit(" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
)lit");
  for (size_t i = 0; i < privateData.configurations.size(); ++i) {
    const char* config_name = privateData.configurations[i]->label;
    xcode_uuid config_id    = configuration_ids[i];
    fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(config_id),
            config_name);
  }
  fprintf(f, R"lit(			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget ")lit");
  fprintf(f, "%s", p->name);
  fprintf(f, R"lit(" */ = {
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
}

void xCodeCreateWorkspaceFile(FILE* f) {
  fprintf(f, R"lit(<?xml version="1.0" encoding="UTF-8"?>
<Workspace
   version = "1.0">
)lit");

  for (unsigned i = 0; i < privateData.projects.size(); ++i) {
    auto p = privateData.projects[i];
    fprintf(f, "  <FileRef\n");
    fprintf(f, "    location = \"group:%s.xcodeproj\">\n", p->name);
    fprintf(f, "  </FileRef>\n");
  }

  fprintf(f, "</Workspace>");
}

void xcode_generateInFolder(const char* workspace_path) {
  int count_folder_depth = 1;
  {
    const char* c = workspace_path;
    while (*c) {
      if (*c == '/') count_folder_depth += 1;
      ++c;
    }
    if (*(c - 1) == '/') count_folder_depth -= 1;
  }

  int result = make_folder(workspace_path);
  if (result != 0) {
    fprintf(stderr, "Error %i creating path '%s'\n", result, workspace_path);
  }
  (void)chdir(workspace_path);

  // Before doing anything, generate a UUID for each projects output file
  std::vector<xcode_uuid> projectFileReferenceUUIDs(privateData.projects.size());
  for (unsigned i = 0; i < privateData.projects.size(); ++i) {
    projectFileReferenceUUIDs[i] = xCodeGenerateUUID();
  }

  for (unsigned i = 0; i < privateData.projects.size(); ++i) {
    auto p = privateData.projects[i];

    std::string projectFilePath = p->name;
    projectFilePath += ".xcodeproj";
    int result = make_folder(projectFilePath.c_str());

    projectFilePath += "/project.pbxproj";

    FILE* f = fopen(projectFilePath.c_str(), "wb");

    if (f) {
      xCodeCreateProjectFile(f, p, projectFileReferenceUUIDs, count_folder_depth);
      fclose(f);
    }
  }

  {
    std::string workspaceFilePath = privateData.workspaceLabel;
    workspaceFilePath += ".xcworkspace";
    int result = make_folder(workspaceFilePath.c_str());
    workspaceFilePath += "/contents.xcworkspacedata";
    FILE* f = fopen(workspaceFilePath.c_str(), "wb");
    if (f) {
      xCodeCreateWorkspaceFile(f);
      fclose(f);
    }
  }
}
