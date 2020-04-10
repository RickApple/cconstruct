
typedef struct xcode_compiler_setting {
  const char* key;
  const char* value;
} xcode_compiler_setting;

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
  xcode_uuid empty = {0};
  return empty;
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
    prepend_path = cc_printf("%s../", prepend_path);
  }

  unsigned files_count = array_count(p->files);

  const char** file_ref_paths = {0};
  for (unsigned i = 0; i < array_count(p->files); ++i) {
    array_push(file_ref_paths, cc_printf("%s%s", prepend_path, p->files[i]));
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

  fprintf(f,
          "// !$*UTF8*$!\n{\n	archiveVersion = 1;\n	classes = {\n	};\n	objectVersion = "
          "50;\n	objects = {\n\n/* Begin PBXBuildFile section */\n");

  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->files[fi];
    if (!is_header_file(filename))
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

  bool has_post_build_action = (p->postBuildAction != 0);
  if (p->type == CCProjectTypeConsoleApplication) {
    fprintf(f,
            "/* Begin PBXCopyFilesBuildPhase section */\n		403CC53923EB479400558E07 "
            "/* CopyFiles */ = {\n			isa = PBXCopyFilesBuildPhase;\n		"
            "	buildActionMask = 2147483647;\n			dstPath = "
            "/usr/share/man/man1/;\n			dstSubfolderSpec = 0;\n			"
            "files = (\n			);\n			"
            "runOnlyForDeploymentPostprocessing = 1;\n		};\n/* End PBXCopyFilesBuildPhase "
            "section */\n\n");
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
            "		%s /* lib%s.a */ = {isa = PBXFileReference; explicitFileType = "
            "archive.ar; path = lib%s.a; sourceTree = BUILT_PRODUCTS_DIR; };\n",
            id, p->dependantOn[i]->name, p->dependantOn[i]->name);
  }
  fprintf(f, "/* End PBXFileReference section */\n\n");

  fprintf(f,
          "/* Begin PBXFrameworksBuildPhase section */\n		403CC53823EB479400558E07 "
          "/* Frameworks */ = {\n			isa = PBXFrameworksBuildPhase;\n	"
          "		buildActionMask = 2147483647;\n			files = (\n");
  for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
    const char* buildID = dependencyBuildUUID[i];
    fprintf(f, "				%s /* lib%s.a in Frameworks */,\n", buildID,
            p->dependantOn[i]->name);
  }
  fprintf(f,
          "			);\n			runOnlyForDeploymentPostprocessing = "
          "0;\n		};\n/* End PBXFrameworksBuildPhase section */\n\n");

  fprintf(
      f,
      "/* Begin PBXGroup section */\n		4008B25F23EDACFC00FCB192 /* Frameworks */ = {\n	"
      "		isa = PBXGroup;\n			children = (\n			);\n	"
      "		name = Frameworks;\n			sourceTree = \"<group>\";\n		"
      "};\n		403CC53223EB479400558E07 = {\n			isa = PBXGroup;\n	"
      "		children = (\n				403CC53D23EB479400558E07 /* ");
  fprintf(f, "%s", p->name);
  fprintf(f,
          " */,\n				403CC53C23EB479400558E07 /* Products */,\n	"
          "			4008B25F23EDACFC00FCB192 /* Frameworks */,\n			"
          ");\n			sourceTree = \"<group>\";\n		};\n		"
          "403CC53C23EB479400558E07 /* Products */ = {\n			isa = "
          "PBXGroup;\n			children = (\n");
  fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(outputFileReferenceUIID),
          outputName);
  fprintf(f,
          "			);\n			name = Products;\n			"
          "sourceTree = \"<group>\";\n		};\n		403CC53D23EB479400558E07 /* ");
  fprintf(f, "%s", p->name);
  fprintf(f,
          " */ = {\n			isa = PBXGroup;\n			children = (\n	"
          "			403CC54523EB480800558E07 /* src */,\n			);\n	"
          "		path = ");
  fprintf(f, "%s", p->name);
  fprintf(f,
          ";\n			sourceTree = \"<group>\";\n		};\n		"
          "403CC54523EB480800558E07 /* src */ = {\n			isa = PBXGroup;\n	"
          "		children = (\n");
  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->files[fi];
    fprintf(f, "			    %s /* %s */,\n", fileReferenceUUID[fi],
            strip_path(filename));
  }
  fprintf(f,
          "			);\n			name = src;\n			path = "
          "../../src;\n			sourceTree = \"<group>\";\n		};\n/* End "
          "PBXGroup section */\n\n/* Begin PBXNativeTarget section */\n");
  fprintf(f, "		%s /* %s */ = {\n", xCodeUUID2String(outputTargetUIID), p->name);
  fprintf(
      f,
      "			isa = PBXNativeTarget;\n			buildConfigurationList = "
      "403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget \"");
  fprintf(f, "%s", p->name);
  fprintf(f,
          "\" */;\n			buildPhases = (\n				"
          "403CC53723EB479400558E07 /* Sources */,\n				"
          "403CC53823EB479400558E07 /* Frameworks */,\n				"
          "403CC53923EB479400558E07 /* CopyFiles */,\n				");
  if (has_post_build_action) {
    fprintf(f, "40C3D9692440AC2500C8EB40 /* ShellScript */,\n			");
  }
  fprintf(f,
          ");\n			"
          "buildRules = (\n			);\n			dependencies = (\n	"
          "		);\n");

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
  fprintf(f,
          ";\n		};\n/* End PBXNativeTarget section */\n\n/* Begin PBXProject section "
          "*/\n		403CC53323EB479400558E07 /* Project object */ = {\n			"
          "isa = PBXProject;\n			attributes = {\n				"
          "LastUpgradeCheck = 1130;\n				ORGANIZATIONNAME = \"Daedalus "
          "Development\";\n				TargetAttributes = {\n			"
          "		");
  fprintf(f, "%s", xCodeUUID2String(outputTargetUIID));
  fprintf(f,
          " = {\n						CreatedOnToolsVersion = 11.3;\n	"
          "				};\n				};\n			"
          "};\n			buildConfigurationList = 403CC53623EB479400558E07 /* Build "
          "configuration list for PBXProject \"");
  fprintf(f, "%s", p->name);
  fprintf(
      f,
      "\" */;\n			compatibilityVersion = \"Xcode 9.3\";\n			"
      "developmentRegion = en;\n			hasScannedForEncodings = 0;\n		"
      "	knownRegions = (\n				en,\n				Base,\n	"
      "		);\n			mainGroup = 403CC53223EB479400558E07;\n			"
      "productRefGroup = 403CC53C23EB479400558E07 /* Products */;\n			"
      "projectDirPath = \"\";\n			projectRoot = \"\""
      ";\n			targets = (\n");
  fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(outputTargetUIID),
          p->name);
  fprintf(f, "			);\n		};\n/* End PBXProject section */\n\n");

  if (has_post_build_action) {
    fprintf(f,
            "/* Begin PBXShellScriptBuildPhase section */\n		40C3D9692440AC2500C8EB40 "
            "/* ShellScript */ = {\n"
            "			isa = PBXShellScriptBuildPhase;\n"
            "			buildActionMask = 2147483647;\n"
            "			files = (\n"
            "			);\n"
            "			inputFileListPaths = (\n"
            "			);\n"
            "			inputPaths = (\n"
            "			);\n"
            "			outputFileListPaths = (\n"
            "			);\n"
            "			outputPaths = (\n"
            "			);\n"
            "			runOnlyForDeploymentPostprocessing = 0;\n"
            "			shellPath = /bin/sh;\n"
            "			shellScript = \"%s\";\n"
            "		};\n"
            "/* End PBXShellScriptBuildPhase section */\n\n",
            p->postBuildAction);
  }

  fprintf(f,
          "/* Begin "
          "PBXSourcesBuildPhase section */\n		403CC53723EB479400558E07 /* Sources */ = "
          "{\n			isa = PBXSourcesBuildPhase;\n			buildActionMask = "
          "2147483647;\n			files = (\n");

  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->files[fi];
    if (!is_header_file(filename)) {
      fprintf(f, "				%s /* %s in Sources */,\n", fileUUID[fi],
              strip_path(filename));
    }
  }

  fprintf(f,
          "			);\n			runOnlyForDeploymentPostprocessing = "
          "0;\n		};\n/* End PBXSourcesBuildPhase section */\n\n");

  xcode_uuid* configuration_ids = {0};
  configuration_ids = (xcode_uuid*)array_reserve(configuration_ids, sizeof(*configuration_ids),
                                                 array_count(privateData.configurations));
  for (unsigned i = 0; i < array_count(privateData.configurations); ++i) {
    array_push(configuration_ids, xCodeGenerateUUID());
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
          cc_printf("%s					  \"%s\",\n", additional_compiler_flags,
                    p->flags[ipc].compile_options[cfi]);
    }
    for (unsigned ifi = 0; ifi < array_count(p->flags[ipc].include_folders); ++ifi) {
      // Order matters here, so append
      additional_include_folders =
          cc_printf("%s					  \"%s\",\n", additional_include_folders,
                    p->flags[ipc].include_folders[ifi]);
    }
  }
  // if (additional_compiler_flags[0] != 0)
  { additional_compiler_flags = cc_printf("(\n%s               )", additional_compiler_flags); }
  additional_include_folders = cc_printf("%s               )", additional_include_folders);
  assert(array_count(privateData.platforms) == 1);
  assert(privateData.platforms[0]->type == EPlatformTypeX64);
  const char* substitution_keys[]   = {"configuration", "platform"};
  const char* substitution_values[] = {"$CONFIGURATION", "x64"};
  const char* resolved_output_folder =
      cc_substitute(privateData.outputFolder, substitution_keys, substitution_values,
                    countof(substitution_keys));

  const char* safe_output_folder          = cc_printf("\"%s\"", resolved_output_folder);
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
    const xcode_compiler_setting* config = config_data[i];
    for (unsigned ic = 0; ic < array_count(config); ++ic) {
      fprintf(f, "				%s = %s;\n", config[ic].key, config[ic].value);
    }
    fprintf(f, "			};\n");
    fprintf(f, "			name = %s;\n", config_name);
    fprintf(f, "		};\n");
  }
  fprintf(f,
          "		403CC54323EB479400558E07 /* Debug */ = {\n			isa = "
          "XCBuildConfiguration;\n			buildSettings = {\n			"
          "	CODE_SIGN_STYLE = Automatic;\n				PRODUCT_NAME = "
          "\"$(TARGET_NAME)\";\n			};\n			name = Debug;\n	"
          "	};\n		403CC54423EB479400558E07 /* Release */ = {\n			"
          "isa = XCBuildConfiguration;\n			buildSettings = {\n		"
          "		CODE_SIGN_STYLE = Automatic;\n				PRODUCT_NAME = "
          "\"$(TARGET_NAME)\";\n			};\n			name = "
          "Release;\n		};\n/* End XCBuildConfiguration section */\n\n");

  fprintf(f,
          "/* Begin XCConfigurationList section */\n		403CC53623EB479400558E07 /* Build "
          "configuration list for PBXProject \"");
  fprintf(f, "%s", p->name);
  fprintf(f,
          "\" */ = {\n			isa = XCConfigurationList;\n			"
          "buildConfigurations = (\n");
  for (unsigned i = 0; i < array_count(privateData.configurations); ++i) {
    const char* config_name = privateData.configurations[i]->label;
    xcode_uuid config_id    = configuration_ids[i];
    fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(config_id),
            config_name);
  }
  fprintf(f,
          "			);\n			defaultConfigurationIsVisible = 0;\n	"
          "		defaultConfigurationName = Release;\n		};\n		"
          "403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget \"");
  fprintf(f, "%s", p->name);
  fprintf(f,
          "\" */ = {\n			isa = XCConfigurationList;\n			"
          "buildConfigurations = (\n				403CC54323EB479400558E07 /* Debug "
          "*/,\n				403CC54423EB479400558E07 /* Release */,\n	"
          "		);\n			defaultConfigurationIsVisible = 0;\n		"
          "	defaultConfigurationName = Release;\n		};\n/* End XCConfigurationList "
          "section */\n	};\n	rootObject = 403CC53323EB479400558E07 /* Project object */;\n}\n");
}

void xCodeCreateWorkspaceFile(FILE* f) {
  fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Workspace\n   version = \"1.0\">\n");

  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
    const TProject* p = privateData.projects[i];
    fprintf(f, "  <FileRef\n");
    fprintf(f, "    location = \"group:%s.xcodeproj\">\n", p->name);
    fprintf(f, "  </FileRef>\n");
  }

  fprintf(f, "</Workspace>");
}

void xcode_generateInFolder(const char* generate_path) {
  int count_folder_depth = 1;
  {
    const char* c = generate_path;
    while (*c) {
      if (*c == '/') count_folder_depth += 1;
      ++c;
    }
    if (*(c - 1) == '/') count_folder_depth -= 1;
  }

  int result = make_folder(generate_path);
  if (result != 0) {
    fprintf(stderr, "Error %i creating path '%s'\n", result, generate_path);
  }
  (void)chdir(generate_path);

  // Before doing anything, generate a UUID for each projects output file
  xcode_uuid* projectFileReferenceUUIDs = 0;
  projectFileReferenceUUIDs =
      (xcode_uuid*)array_reserve(projectFileReferenceUUIDs, sizeof(*projectFileReferenceUUIDs),
                                 array_count(privateData.projects));
  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
    array_push(projectFileReferenceUUIDs, xCodeGenerateUUID());
  }

  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
    const TProject* p = privateData.projects[i];

    const char* project_path = cc_printf("%s.xcodeproj", p->name);
    int result               = make_folder(project_path);

    const char* project_file_path = cc_printf("%s/project.pbxproj", project_path);

    FILE* f = fopen(project_file_path, "wb");

    if (f) {
      xCodeCreateProjectFile(f, p, projectFileReferenceUUIDs, count_folder_depth);
      fclose(f);
    }

    printf("Constructed XCode project '%s' at '%s/%s'\n", p->name, generate_path, project_path);
  }

  {
    const char* workspace_path      = cc_printf("%s.xcworkspace", privateData.workspaceLabel);
    int result                      = make_folder(workspace_path);
    const char* workspace_file_path = cc_printf("%s/contents.xcworkspacedata", workspace_path);
    FILE* f                         = fopen(workspace_file_path, "wb");
    if (f) {
      xCodeCreateWorkspaceFile(f);
      fclose(f);
      printf("Constructed XCode workspace at '%s/%s'\n", generate_path, workspace_path);
    }
  }
}
