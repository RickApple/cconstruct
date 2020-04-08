
struct xcode_compiler_setting {
  const char* key;
  const char* value;
};

typedef struct xcode_uuid {
  unsigned int uuid[3];
} xcode_uuid;

static_assert(sizeof(xcode_uuid) == 12, "Incorrect size of UUID");

xcode_uuid xCodeGenerateUUID() {
  static unsigned count = 0;

  xcode_uuid out = {0};
  out.uuid[0]    = ++count;
  return out;
}
const char* xCodeUUID2String(xcode_uuid uuid) {
  // Incorrect byte order, but don't care for now
  return cc_printf("%08x%08x%08x", uuid.uuid[0], uuid.uuid[1], uuid.uuid[2]);
}

xcode_uuid findUUIDForProject(const xcode_uuid* uuids, const TProject* project) {
  for (unsigned i = 0; i < array_count(uuids); ++i) {
    if (privateData.projects[i] == project) return uuids[i];
  }
  return {};
}

#define add_setting(a, k, v)                 \
  {                                          \
    xcode_compiler_setting setting = {k, v}; \
    array_push(a, setting);                  \
  }

void xCodeCreateProjectFile(FILE* f, const TProject* in_project,
                            const xcode_uuid* projectFileReferenceUUIDs, int folder_depth) {
  const TProject* p = (TProject*)in_project;

  const char* prepend_path = "";
  for (int i = 0; i < folder_depth; ++i) {
    prepend_path = cc_string_append(prepend_path, "../");
  }

  unsigned files_count = array_count(p->files);

  const char** file_ref_paths = {0};
  for (unsigned i = 0; i < array_count(p->files); ++i) {
    array_push(file_ref_paths, cc_string_append(prepend_path, p->files[i]));
  }

  const char** fileReferenceUUID = {0};
  const char** fileUUID          = {0};
  for (unsigned fi = 0; fi < files_count; ++fi) {
    array_push(fileReferenceUUID, xCodeUUID2String(xCodeGenerateUUID()));
    array_push(fileUUID, xCodeUUID2String(xCodeGenerateUUID()));
  }

  const char** dependencyFileReferenceUUID = {0};
  const char** dependencyBuildUUID         = {0};
  for (unsigned fi = 0; fi < array_count(p->dependantOn); ++fi) {
    array_push(dependencyFileReferenceUUID, xCodeUUID2String(xCodeGenerateUUID()));
    array_push(dependencyBuildUUID, xCodeUUID2String(xCodeGenerateUUID()));
  }

  xcode_uuid outputFileReferenceUIID = findUUIDForProject(projectFileReferenceUUIDs, p);
  xcode_uuid outputTargetUIID        = xCodeGenerateUUID();

  const char* outputName = p->name;
  if (p->type == CCProjectTypeStaticLibrary) {
    outputName = cc_printf("lib%s.a", outputName);
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

  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->files[fi];
    fprintf(f,
            "		%s /* %s in Sources */ = {isa = PBXBuildFile; fileRef = %s /* %s */; };\n",
            fileUUID[fi], strip_path(filename), fileReferenceUUID[fi], strip_path(filename));
  }
  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const char* id            = dependencyFileReferenceUUID[i];
    const char* buildID       = dependencyBuildUUID[i];
    const char* dependantName = cc_printf("lib%s.a", p->dependantOn[i]->name);
    fprintf(f,
            "		%s /* %s in Frameworks */ = {isa = PBXBuildFile; fileRef = %s /* %s */; "
            "};\n",
            buildID, dependantName, id, dependantName);
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
  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->files[fi];
    fprintf(f,
            "		%s /* %s */ = {isa = PBXFileReference; fileEncoding = 4; "
            "lastKnownFileType "
            "= sourcecode.cpp.cpp; name = %s; path = %s; sourceTree = SOURCE_ROOT; };\n",
            fileReferenceUUID[fi], strip_path(filename), strip_path(filename), file_ref_paths[fi]);
    printf("Adding file '%s' as '%s'\n", filename, file_ref_paths[fi]);
  }
  fprintf(f, "		%s /* %s */ = {isa = PBXFileReference; explicitFileType = \"",
          xCodeUUID2String(outputFileReferenceUIID), outputName);
  if (p->type == CCProjectTypeConsoleApplication) {
    fprintf(f, "compiled.mach-o.executable");
  } else {
    fprintf(f, "archive.ar");
  }
  fprintf(f, "\"; includeInIndex = 0; path = %s; sourceTree = BUILT_PRODUCTS_DIR; };\n",
          outputName);

  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const char* id = dependencyFileReferenceUUID[i];
    fprintf(f,
            "		%s /* libmy_library.a */ = {isa = PBXFileReference; explicitFileType = "
            "archive.ar; path = libmy_library.a; sourceTree = BUILT_PRODUCTS_DIR; };\n",
            id);
  }
  fprintf(f, "/* End PBXFileReference section */\n\n");

  fprintf(f, R"lit(/* Begin PBXFrameworksBuildPhase section */
		403CC53823EB479400558E07 /* Frameworks */ = {
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
)lit");
  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const char* buildID = dependencyBuildUUID[i];
    fprintf(f, "				%s /* lib%s.a in Frameworks */,\n", buildID,
            p->dependantOn[i]->name);
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
          outputName);
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
  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->files[fi];
    fprintf(f, "			    %s /* %s */,\n", fileReferenceUUID[fi],
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
          xCodeUUID2String(outputFileReferenceUIID), outputName);
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

  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->files[fi];
    fprintf(f, "				%s /* %s in Sources */,\n", fileUUID[fi],
            strip_path(filename));
  }

  fprintf(f, R"lit(			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXSourcesBuildPhase section */

)lit");

  xcode_uuid* configuration_ids = {0};
  configuration_ids =
      (xcode_uuid*)array_grow(configuration_ids, array_count(privateData.configurations));
  for (unsigned i = 0; i < array_count(privateData.configurations); ++i) {
    configuration_ids[i] = xCodeGenerateUUID();
  }

  const char* preprocessor_defines       = "					\"$(inherited)\",\n";
  const char* additional_compiler_flags  = "";
  const char* additional_include_folders = "(\n";

  for (unsigned ipc = 0; ipc < array_count(p->flags); ++ipc) {
    // TODO ordering and combination so that more specific flags can override general ones
    // if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;
    // if ((p->platforms[ipc] != platform) && (p->platforms[ipc] != NULL)) continue;

    for (unsigned pdi = 0; pdi < array_count(p->flags[ipc].defines); ++pdi) {
      preprocessor_defines = cc_printf("					\"%s\",\n%s",
                                       p->flags[ipc].defines[pdi], preprocessor_defines);
    }

    for (unsigned cfi = 0; cfi < array_count(p->flags[ipc].compile_options); ++cfi) {
      additional_compiler_flags =
          cc_string_append(additional_compiler_flags,
                           cc_printf("					  \"%s\",\n",
                                     p->flags[ipc].compile_options[cfi]));
    }
    for (unsigned ifi = 0; ifi < array_count(p->flags[ipc].include_folders); ++ifi) {
      // Order matters here, so append
      additional_include_folders =
          cc_string_append(additional_include_folders,
                           cc_printf("					  \"%s\",\n",
                                     p->flags[ipc].include_folders[ifi]));
    }
  }
  // if (additional_compiler_flags[0] != 0)
  { additional_compiler_flags = cc_printf("(\n%s               )", additional_compiler_flags); }
  additional_include_folders = cc_string_append(additional_include_folders, "               )");
  const char* safe_output_folder          = cc_printf("\"%s\"", privateData.outputFolder);
  const char* combined_preprocessor_debug = cc_printf(
      "(\n					\"DEBUG=1\",\n%s				)",
      preprocessor_defines);
  const char* combined_preprocessor_release =
      cc_printf("(\n%s				)", preprocessor_defines);
  xcode_compiler_setting* debug_config_data = {0};
  add_setting(debug_config_data, "ALWAYS_SEARCH_USER_PATHS", "NO");
  add_setting(debug_config_data, "CONFIGURATION_BUILD_DIR", safe_output_folder);
  add_setting(debug_config_data, "DEBUG_INFORMATION_FORMAT", "dwarf");
  add_setting(debug_config_data, "ENABLE_TESTABILITY", "YES");
  add_setting(debug_config_data, "GCC_OPTIMIZATION_LEVEL", "0");
  add_setting(debug_config_data, "GCC_PREPROCESSOR_DEFINITIONS", combined_preprocessor_debug);
  add_setting(debug_config_data, "HEADER_SEARCH_PATHS", additional_include_folders);
  add_setting(debug_config_data, "MACOSX_DEPLOYMENT_TARGET", "10.14");
  add_setting(debug_config_data, "ONLY_ACTIVE_ARCH", "YES");
  add_setting(debug_config_data, "OTHER_CFLAGS", additional_compiler_flags);
  add_setting(debug_config_data, "SDKROOT", "macosx");
  xcode_compiler_setting* release_config_data = {0};
  add_setting(release_config_data, "ALWAYS_SEARCH_USER_PATHS", "NO");
  add_setting(release_config_data, "CONFIGURATION_BUILD_DIR", safe_output_folder);
  add_setting(release_config_data, "DEBUG_INFORMATION_FORMAT", "\"dwarf-with-dsym\"");
  add_setting(release_config_data, "ENABLE_NS_ASSERTIONS", "NO");
  add_setting(release_config_data, "GCC_PREPROCESSOR_DEFINITIONS", combined_preprocessor_release);
  add_setting(release_config_data, "HEADER_SEARCH_PATHS", additional_include_folders);
  add_setting(release_config_data, "MACOSX_DEPLOYMENT_TARGET", "10.14");
  add_setting(release_config_data, "ONLY_ACTIVE_ARCH", "YES");
  add_setting(release_config_data, "OTHER_CFLAGS", additional_compiler_flags);
  add_setting(release_config_data, "SDKROOT", "macosx");

  xcode_compiler_setting** config_data = {0};
  array_push(config_data, debug_config_data);
  array_push(config_data, release_config_data);
  fprintf(f, "/* Begin XCBuildConfiguration section */\n");
  for (unsigned i = 0; i < array_count(privateData.configurations); ++i) {
    const char* config_name = privateData.configurations[i]->label;
    xcode_uuid config_id    = configuration_ids[i];

    fprintf(f, "		%s /* %s */ = {\n", xCodeUUID2String(config_id), config_name);
    fprintf(f, "			isa = XCBuildConfiguration;\n");
    fprintf(f, "			buildSettings = {\n");
    auto config = config_data[i];
    for (unsigned ic = 0; ic < array_count(config); ++ic) {
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
  for (unsigned i = 0; i < array_count(privateData.configurations); ++i) {
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

  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
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
  xcode_uuid* projectFileReferenceUUIDs = 0;
  projectFileReferenceUUIDs =
      (xcode_uuid*)array_grow(projectFileReferenceUUIDs, array_count(privateData.projects));
  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
    projectFileReferenceUUIDs[i] = xCodeGenerateUUID();
  }

  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
    auto p = privateData.projects[i];

    const char* projectFilePath = cc_printf("%s.xcodeproj", p->name);
    int result                  = make_folder(projectFilePath);

    projectFilePath = cc_printf("%s/project.pbxproj", projectFilePath);

    FILE* f = fopen(projectFilePath, "wb");

    if (f) {
      xCodeCreateProjectFile(f, p, projectFileReferenceUUIDs, count_folder_depth);
      fclose(f);
    }
  }

  {
    const char* workspaceFilePath = cc_printf("%s.xcworkspace", privateData.workspaceLabel);
    int result                    = make_folder(workspaceFilePath);
    workspaceFilePath             = cc_printf("%s/contents.xcworkspacedata", workspaceFilePath);
    FILE* f                       = fopen(workspaceFilePath, "wb");
    if (f) {
      xCodeCreateWorkspaceFile(f);
      fclose(f);
    }
  }
}
