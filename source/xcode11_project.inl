
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

xcode_uuid findUUIDForProject(const xcode_uuid* uuids, const cc_project_impl_t* project) {
  for (unsigned i = 0; i < array_count(uuids); ++i) {
    if (cc_data_.projects[i] == project) return uuids[i];
  }
  xcode_uuid empty = {0};
  return empty;
}

#define add_setting(a, k, v)                 \
  {                                          \
    xcode_compiler_setting setting = {k, v}; \
    array_push(a, setting);                  \
  }

const char* xCodeStringFromGroup(const cc_group_impl_t** unique_groups, const char** group_ids,
                                 const cc_group_impl_t* group) {
  const unsigned num_groups = array_count(unique_groups);
  for (unsigned i = 0; i < num_groups; ++i) {
    if (unique_groups[i] == group) {
      return cc_printf("%s /* %s */", group_ids[i], group->name);
    }
  }

  assert(false && "Couldn't find group name");
  return "";
}

void xCodeCreateProjectFile(FILE* f, const cc_project_impl_t* in_project,
                            const xcode_uuid* projectFileReferenceUUIDs,
                            const char* build_to_base_path) {
  const cc_project_impl_t* p = (cc_project_impl_t*)in_project;

  const unsigned files_count = array_count(p->file_data);

  const char** file_ref_paths = {0};
  for (unsigned i = 0; i < files_count; ++i) {
    array_push(file_ref_paths, cc_printf("%s%s", build_to_base_path, p->file_data[i]->path));
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

  // Create list of groups needed.
  const cc_group_impl_t** unique_groups = {0};
  const char** unique_groups_id         = {0};
  for (unsigned ig = 0; ig < array_count(p->file_data); ++ig) {
    unsigned g = p->file_data[ig]->parent_group_idx;
    while (g) {
      bool already_contains_group = false;
      for (unsigned i = 0; i < array_count(unique_groups); ++i) {
        if (&cc_data_.groups[g] == unique_groups[i]) {
          already_contains_group = true;
        }
      }
      if (!already_contains_group) {
        array_push(unique_groups, &cc_data_.groups[g]);
        array_push(unique_groups_id, xCodeUUID2String(xCodeGenerateUUID()));
      }
      g = cc_data_.groups[g].parent_group_idx;
    }
  }
  for (unsigned ig = 0; ig < array_count(p->file_data_custom_command); ++ig) {
    unsigned g = p->file_data_custom_command[ig]->parent_group_idx;
    while (g) {
      bool already_contains_group = false;
      for (unsigned i = 0; i < array_count(unique_groups); ++i) {
        if (&cc_data_.groups[g] == unique_groups[i]) {
          already_contains_group = true;
        }
      }
      if (!already_contains_group) {
        array_push(unique_groups, &cc_data_.groups[g]);
        array_push(unique_groups_id, xCodeUUID2String(xCodeGenerateUUID()));
      }
      g = cc_data_.groups[g].parent_group_idx;
    }
  }
  const unsigned num_unique_groups = array_count(unique_groups);

  fprintf(f,
          "// !$*UTF8*$!\n{\n	archiveVersion = 1;\n	classes = {\n	};\n	objectVersion = "
          "50;\n	objects = {\n\n");

  fprintf(f, "/* Begin PBXBuildFile section */\n");
  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->file_data[fi]->path;
    if (is_source_file(filename))
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
    const char* filename = p->file_data[fi]->path;
    fprintf(f,
            "		%s /* %s */ = {isa = PBXFileReference; fileEncoding = 4; "
            "lastKnownFileType "
            "= %s; name = %s; path = %s; sourceTree = SOURCE_ROOT; };\n",
            fileReferenceUUID[fi], strip_path(filename),
            (strstr(filename, ".cpp") != NULL ? "sourcecode.cpp.cpp" : "sourcecode.c.c"),
            strip_path(filename), file_ref_paths[fi]);
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

  fprintf(f, "/* Begin PBXGroup section */\n");
  fprintf(f,
          "		403CC53223EB479400558E07 = {\n"
          "			isa = PBXGroup;\n"
          "			children = (\n");
  for (unsigned i = 0; i < array_count(unique_groups); ++i) {
    if (unique_groups[i]->parent_group_idx == 0) {
      fprintf(f, "				%s,\n",
              xCodeStringFromGroup(unique_groups, unique_groups_id, unique_groups[i]));
    }
  }
  for (unsigned i = 0; i < array_count(p->file_data); ++i) {
    if (p->file_data[i]->parent_group_idx == 0) {
      fprintf(f, "				%s /* %s */,\n", fileReferenceUUID[i],
              strip_path(p->file_data[i]->path));
    }
  }
  fprintf(f, "				403CC53C23EB479400558E07 /* Products */,\n");
  if (array_count(p->dependantOn) > 0) {
    fprintf(f, "				4008B25F23EDACFC00FCB192 /* Frameworks */,\n");
  }
  fprintf(f,
          "			);\n"
          "			sourceTree = \"<group>\";\n"
          "		};\n");
  for (unsigned i = 0; i < num_unique_groups; ++i) {
    const cc_group_impl_t* g = unique_groups[i];
    fprintf(f,
            "		%s = {\n"
            "			isa = PBXGroup;\n"
            "			children = (\n",
            xCodeStringFromGroup(unique_groups, unique_groups_id, g));
    for (unsigned fi = 0; fi < files_count; ++fi) {
      const char* filename              = p->file_data[fi]->path;
      const cc_group_impl_t* file_group = &cc_data_.groups[p->file_data[fi]->parent_group_idx];
      if (file_group == g) {
        fprintf(f, "			    %s /* %s */,\n", fileReferenceUUID[fi],
                strip_path(filename));
      }
    }
    for (unsigned gi = 0; gi < num_unique_groups; ++gi) {
      const cc_group_impl_t* child_group = unique_groups[gi];
      if (&cc_data_.groups[child_group->parent_group_idx] == g) {
        fprintf(f, "			    %s,\n",
                xCodeStringFromGroup(unique_groups, unique_groups_id, child_group));
      }
    }
    fprintf(f,
            "			);\n"
            "			name = \"%s\";\n"
            "			sourceTree = \"<group>\";\n"
            "		};\n",
            g->name);
  }
  if (array_count(p->dependantOn) > 0) {
    fprintf(f,
            "		4008B25F23EDACFC00FCB192 /* Frameworks */ = {\n"
            "			isa = PBXGroup;\n"
            "			children = (\n"
            "			);\n"
            "			name = Frameworks;\n"
            "			sourceTree = \"<group>\";\n"
            "		};\n");
  }
  fprintf(f,
          "		403CC53C23EB479400558E07 /* Products */ = {\n"
          "			isa = PBXGroup;\n"
          "			children = (\n");
  fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(outputFileReferenceUIID),
          outputName);
  fprintf(f,
          "			);\n"
          "			name = Products;\n"
          "			sourceTree = \"<group>\";\n"
          "		};\n");
  fprintf(f, "/* End PBXGroup section */\n\n");

  fprintf(f, "/* Begin PBXNativeTarget section */\n");
  fprintf(f, "		%s /* %s */ = {\n", xCodeUUID2String(outputTargetUIID), p->name);
  fprintf(f,
          "			isa = PBXNativeTarget;\n			"
          "buildConfigurationList = "
          "403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget \"");
  fprintf(f, "%s", p->name);
  fprintf(f,
          "\" */;\n"
          "			buildPhases = (\n"
          "				403CC53723EB479400558E07 /* Sources */,\n"
          "				403CC53823EB479400558E07 /* Frameworks */,\n"
          "				403CC53923EB479400558E07 /* CopyFiles */,\n");
  if (has_post_build_action) {
    fprintf(f, "				40C3D9692440AC2500C8EB40 /* ShellScript */,\n");
  }
  fprintf(f,
          "			);\n"
          "			buildRules = (\n"
          "			);\n"
          "			dependencies = (\n"
          "			);\n");
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
  fprintf(f, ";\n		};\n/* End PBXNativeTarget section */\n\n");

  fprintf(f,
          "/* Begin PBXProject section "
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
  fprintf(f,
          "\" */;\n			compatibilityVersion = \"Xcode 9.3\";\n			"
          "developmentRegion = en;\n			hasScannedForEncodings = 0;\n		"
          "	knownRegions = (\n				en,\n				"
          "Base,\n	"
          "		);\n			mainGroup = 403CC53223EB479400558E07;\n		"
          "	"
          "productRefGroup = 403CC53C23EB479400558E07 /* Products */;\n			"
          "projectDirPath = \"\";\n			projectRoot = \"\""
          ";\n			targets = (\n");
  fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(outputTargetUIID),
          p->name);
  fprintf(f, "			);\n		};\n/* End PBXProject section */\n\n");

  if (has_post_build_action) {
    const char* postBuildAction     = cc_printf("%s", p->postBuildAction);
    const char* substitution_keys[] = {"configuration", "platform"};
    // For MacOS currently the only allowed platform is  64-bit
    const char* substitution_values[] = {"$CONFIGURATION", "x64"};
    postBuildAction = cc_substitute(postBuildAction, substitution_keys, substitution_values,
                                    countof(substitution_keys));
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
            postBuildAction);
  }

  fprintf(f,
          "/* Begin PBXSourcesBuildPhase section */\n"
          "		403CC53723EB479400558E07 /* Sources */ = {\n"
          "			isa = PBXSourcesBuildPhase;\n"
          "			buildActionMask = 2147483647;\n"
          "			files = (\n");
  for (unsigned fi = 0; fi < files_count; ++fi) {
    const char* filename = p->file_data[fi]->path;
    if (is_source_file(filename)) {
      fprintf(f, "				%s /* %s in Sources */,\n", fileUUID[fi],
              strip_path(filename));
    }
  }
  fprintf(f,
          "			);\n"
          "			runOnlyForDeploymentPostprocessing = 0;\n"
          "		};\n"
          "/* End PBXSourcesBuildPhase section */\n\n");

  const unsigned num_configurations = array_count(cc_data_.configurations);
  xcode_uuid* configuration_ids     = {0};
  for (unsigned i = 0; i < num_configurations; ++i) {
    array_push(configuration_ids, xCodeGenerateUUID());
  }
  xcode_compiler_setting** config_datas = {0};

  for (unsigned i = 0; i < num_configurations; ++i) {
    const cc_configuration_impl_t* config = cc_data_.configurations[i];

    const char* preprocessor_defines =
        "					"
        "\"$(inherited)\",\n";
    const char* additional_compiler_flags  = "";
    const char* additional_include_folders = "(\n";

    EStateWarningLevel combined_warning_level = EStateWarningLevelDefault;
    bool shouldDisableWarningsAsError         = false;
    for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
      const cc_state_impl_t* flags = &(p->state[ipc]);

      // TODO ordering and combination so that more specific flags can override general ones
      if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;
      // if ((p->platforms[ipc] != platform) && (p->platforms[ipc] != NULL)) continue;

      shouldDisableWarningsAsError = flags->disableWarningsAsErrors;
      combined_warning_level       = flags->warningLevel;

      for (unsigned pdi = 0; pdi < array_count(flags->defines); ++pdi) {
        preprocessor_defines = cc_printf("					\"%s\",\n%s",
                                         flags->defines[pdi], preprocessor_defines);
      }

      for (unsigned cfi = 0; cfi < array_count(flags->compile_options); ++cfi) {
        additional_compiler_flags =
            cc_printf("%s					  \"%s\",\n",
                      additional_compiler_flags, flags->compile_options[cfi]);
      }
      for (unsigned ifi = 0; ifi < array_count(flags->include_folders); ++ifi) {
        // Order matters here, so append
        additional_include_folders =
            cc_printf("%s					  \"%s%s\",\n",
                      additional_include_folders, build_to_base_path, flags->include_folders[ifi]);
      }
    }
    additional_include_folders = cc_printf("%s               )", additional_include_folders);
    assert(array_count(cc_data_.platforms) == 1);
    assert(cc_data_.platforms[0]->type == EPlatformTypeX64);
    const char* substitution_keys[]    = {"configuration", "platform"};
    const char* substitution_values[]  = {"$CONFIGURATION", "x64"};
    const char* resolved_output_folder = cc_substitute(
        cc_data_.outputFolder, substitution_keys, substitution_values, countof(substitution_keys));

    const char* safe_output_folder = cc_printf("\"%s\"", resolved_output_folder);

    const char* combined_preprocessor   = NULL;
    xcode_compiler_setting* config_data = {0};
    if (strcmp(config->label, "Debug") == 0) {
      combined_preprocessor = cc_printf(
          "(\n					\"DEBUG=1\",\n%s				)",
          preprocessor_defines);
      add_setting(config_data, "DEBUG_INFORMATION_FORMAT", "dwarf");
      add_setting(config_data, "ENABLE_TESTABILITY", "YES");
      add_setting(config_data, "GCC_OPTIMIZATION_LEVEL", "0");
    } else {
      combined_preprocessor = cc_printf("(\n%s				)", preprocessor_defines);
      add_setting(config_data, "DEBUG_INFORMATION_FORMAT", "\"dwarf-with-dsym\"");
      add_setting(config_data, "ENABLE_NS_ASSERTIONS", "NO");
    }
    add_setting(config_data, "GCC_PREPROCESSOR_DEFINITIONS", combined_preprocessor);
    add_setting(config_data, "GCC_C_LANGUAGE_STANDARD", "c11");            // TODO:Expose this?
    add_setting(config_data, "CLANG_CXX_LANGUAGE_STANDARD", "\"c++0x\"");  // TODO:Expose this?

    if (!shouldDisableWarningsAsError) {
      add_setting(config_data, "GCC_TREAT_WARNINGS_AS_ERRORS", "YES");
    } else {
      add_setting(config_data, "GCC_TREAT_WARNINGS_AS_ERRORS", "NO");
    }
    const char* default_enabled_warnings[] = {
        "CLANG_WARN_DELETE_NON_VIRTUAL_DTOR",
        "CLANG_WARN_DIRECT_OBJC_ISA_USAGE",
        "CLANG_WARN_MISSING_NOESCAPE",
        "CLANG_WARN_OBJC_ROOT_CLASS",
        "CLANG_WARN_PRAGMA_PACK",
        "CLANG_WARN_PRIVATE_MODULE",
        "CLANG_WARN_UNGUARDED_AVAILABILITY",
        "CLANG_WARN_VEXING_PARSE",
        "CLANG_WARN__ARC_BRIDGE_CAST_NONARC",
        "GCC_WARN_ABOUT_DEPRECATED_FUNCTIONS",
        "GCC_WARN_ABOUT_INVALID_OFFSETOF_MACRO",
        "GCC_WARN_ABOUT_POINTER_SIGNEDNESS",
        "GCC_WARN_ALLOW_INCOMPLETE_PROTOCOL",
        "GCC_WARN_CHECK_SWITCH_STATEMENTS",
        "GCC_WARN_MISSING_PARENTHESES",
        "GCC_WARN_TYPECHECK_CALLS_TO_PRINTF",
    };
    const char* high_enabled_warnings[] = {
        "CLANG_WARN_EMPTY_BODY",
        "CLANG_WARN_IMPLICIT_SIGN_CONVERSION",
        "CLANG_WARN_SEMICOLON_BEFORE_METHOD_BODY",
        "CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION",
        "CLANG_WARN_UNREACHABLE_CODE",
        "CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION",
        "CLANG_WARN_EMPTY_BODY",
        "CLANG_WARN_IMPLICIT_SIGN_CONVERSION",
        "CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION",
        "CLANG_WARN_UNREACHABLE_CODE",
        "GCC_WARN_64_TO_32_BIT_CONVERSION",
        "GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS",
        "GCC_WARN_ABOUT_MISSING_NEWLINE",
        "GCC_WARN_ABOUT_RETURN_TYPE",
        "GCC_WARN_CHECK_SWITCH_STATEMENTS",
        "GCC_WARN_HIDDEN_VIRTUAL_FUNCTIONS",
        "GCC_WARN_INITIALIZER_NOT_FULLY_BRACKETED",
        "GCC_WARN_MISSING_PARENTHESES",
        "GCC_WARN_PEDANTIC",
        "GCC_WARN_SHADOW",
        "GCC_WARN_SIGN_COMPARE",
        "GCC_WARN_TYPECHECK_CALLS_TO_PRINTF",
        "GCC_WARN_UNINITIALIZED_AUTOS",
        "GCC_WARN_UNKNOWN_PRAGMAS",
        "GCC_WARN_UNUSED_VALUE",
        "GCC_WARN_UNUSED_FUNCTION",
        "GCC_WARN_UNUSED_LABEL",
        "GCC_WARN_UNUSED_VARIABLE",
        "RUN_CLANG_STATIC_ANALYZER",
    };
    if (combined_warning_level == EStateWarningLevelHigh) {
      for (unsigned i = 0; i < countof(high_enabled_warnings); ++i) {
        add_setting(config_data, high_enabled_warnings[i], "YES");
      }
      for (unsigned i = 0; i < countof(default_enabled_warnings); ++i) {
        add_setting(config_data, default_enabled_warnings[i], "YES");
      }

      additional_compiler_flags = cc_printf("%s					  \"%s\",\n",
                                            additional_compiler_flags, "-Wformat=2");
      additional_compiler_flags = cc_printf("%s					  \"%s\",\n",
                                            additional_compiler_flags, "-Wextra");
      // disabling unused parameters needs to be done after -Wextra
      additional_compiler_flags = cc_printf("%s					  \"%s\",\n",
                                            additional_compiler_flags, "-Wno-unused-parameter");
      // This warning can complain about my_struct s = {0};
      // Ref:
      // https://stackoverflow.com/questions/13905200/is-it-wise-to-ignore-gcc-clangs-wmissing-braces-warning
      additional_compiler_flags = cc_printf("%s					  \"%s\",\n",
                                            additional_compiler_flags, "-Wno-missing-braces");

      // https://github.com/boredzo/Warnings-xcconfig/wiki/Warnings-Explained

    } else if (combined_warning_level == EStateWarningLevelNone) {
      add_setting(config_data, "GCC_WARN_INHIBIT_ALL_WARNINGS", "YES");
    } else if (combined_warning_level == EStateWarningLevelMedium) {
      for (unsigned i = 0; i < countof(default_enabled_warnings); ++i) {
        add_setting(config_data, default_enabled_warnings[i], "YES");
      }
    }
    // if (additional_compiler_flags[0] != 0)
    { additional_compiler_flags = cc_printf("(\n%s               )", additional_compiler_flags); }
    add_setting(config_data, "OTHER_CFLAGS", additional_compiler_flags);

    add_setting(config_data, "ALWAYS_SEARCH_USER_PATHS", "NO");
    add_setting(config_data, "CONFIGURATION_BUILD_DIR", safe_output_folder);
    add_setting(config_data, "HEADER_SEARCH_PATHS", additional_include_folders);
    add_setting(config_data, "MACOSX_DEPLOYMENT_TARGET", "10.14");
    add_setting(config_data, "ONLY_ACTIVE_ARCH", "YES");
    add_setting(config_data, "SDKROOT", "macosx");

    array_push(config_datas, config_data);
  }

  fprintf(f, "/* Begin XCBuildConfiguration section */\n");
  for (unsigned i = 0; i < num_configurations; ++i) {
    const char* config_name = cc_data_.configurations[i]->label;
    xcode_uuid config_id    = configuration_ids[i];

    fprintf(f, "		%s /* %s */ = {\n", xCodeUUID2String(config_id), config_name);
    fprintf(f, "			isa = XCBuildConfiguration;\n");
    fprintf(f, "			buildSettings = {\n");
    const xcode_compiler_setting* config = config_datas[i];
    for (unsigned ic = 0; ic < array_count(config); ++ic) {
      fprintf(f, "				%s = %s;\n", config[ic].key, config[ic].value);
    }
    fprintf(f, "			};\n");
    fprintf(f, "			name = %s;\n", config_name);
    fprintf(f, "		};\n");
  }

  const char** native_target_ids = {0};
  for (unsigned i = 0; i < num_configurations; ++i) {
    array_push(native_target_ids, xCodeUUID2String(xCodeGenerateUUID()));
  }
  for (unsigned i = 0; i < num_configurations; ++i) {
    fprintf(f,
            "		%s /* %s */ = {\n"
            "			isa = XCBuildConfiguration;\n"
            "			buildSettings = {\n"
            "				CODE_SIGN_STYLE = Automatic;\n"
            "				PRODUCT_NAME = \"$(TARGET_NAME)\";\n"
            "			};\n"
            "			name = %s;\n"
            "		};\n",
            native_target_ids[i], cc_data_.configurations[i]->label,
            cc_data_.configurations[i]->label);
  }
  fprintf(f, "/* End XCBuildConfiguration section */\n\n");

  fprintf(f,
          "/* Begin XCConfigurationList section */\n"
          "		403CC53623EB479400558E07 /* Build "
          "configuration list for PBXProject \"");
  fprintf(f, "%s", p->name);
  fprintf(f,
          "\" */ = {\n			isa = XCConfigurationList;\n			"
          "buildConfigurations = (\n");
  for (unsigned i = 0; i < num_configurations; ++i) {
    const char* config_name = cc_data_.configurations[i]->label;
    xcode_uuid config_id    = configuration_ids[i];
    fprintf(f, "				%s /* %s */,\n", xCodeUUID2String(config_id),
            config_name);
  }
  fprintf(f,
          "			);\n"
          "			defaultConfigurationIsVisible = 0;\n"
          "			defaultConfigurationName = Release;\n"
          "		};\n");
  fprintf(f,
          "		403CC54223EB479400558E07 /* Build configuration list for PBXNativeTarget "
          "\"%s\" */ = {\n",
          p->name);
  fprintf(f,
          "			isa = XCConfigurationList;\n"
          "			buildConfigurations = (\n");
  for (unsigned i = 0; i < num_configurations; ++i) {
    const char* config_name = cc_data_.configurations[i]->label;
    fprintf(f, "				%s /* %s */,\n", native_target_ids[i],
            config_name);
  }
  fprintf(f,
          "			);\n"
          "			defaultConfigurationIsVisible = 0;\n"
          "			defaultConfigurationName = Release;\n"
          "		};\n"
          "/* End XCConfigurationList section */\n"
          "	};\n"
          "	rootObject = 403CC53323EB479400558E07 /* Project object */;\n"
          "}\n");
}

void xCode_addWorkspaceFolder(FILE* f, const size_t* unique_groups, const size_t parent_group,
                              int folder_depth) {
  const char* prepend_path = "";
  for (int i = 0; i < folder_depth; ++i) {
    prepend_path = cc_printf("%s  ", prepend_path);
  }

  for (unsigned i = 0; i < array_count(unique_groups); ++i) {
    if (cc_data_.groups[unique_groups[i]].parent_group_idx == parent_group) {
      fprintf(f, "%s  <Group\n", prepend_path);
      fprintf(f, "%s    location = \"container:\"\n", prepend_path);
      fprintf(f, "%s    name = \"%s\">\n", prepend_path, cc_data_.groups[unique_groups[i]].name);
      xCode_addWorkspaceFolder(f, unique_groups, unique_groups[i], folder_depth + 1);
      fprintf(f, "%s  </Group>\n", prepend_path);
    }
  }

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const cc_project_impl_t* p = cc_data_.projects[i];
    if (p->parent_group_idx == parent_group) {
      fprintf(f, "%s  <FileRef\n", prepend_path);
      fprintf(f, "%s    location = \"group:%s.xcodeproj\">\n", prepend_path, p->name);
      fprintf(f, "%s  </FileRef>\n", prepend_path);
    }
  }
}

void xCodeCreateWorkspaceFile(FILE* f) {
  fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Workspace\n   version = \"1.0\">\n");

  // Create list of groups needed.
  bool* groups_needed = (bool*)cc_alloc_(array_count(cc_data_.groups) * sizeof(bool));
  memset(groups_needed, 0, array_count(cc_data_.groups) * sizeof(bool));
  for (unsigned i = 0; i < array_count(cc_data_.projects); i++) {
    size_t g = cc_data_.projects[i]->parent_group_idx;
    while (g) {
      groups_needed[g] = true;
      g                = cc_data_.groups[g].parent_group_idx;
    }
  }

  size_t* unique_groups = {0};
  for (unsigned i = 0; i < array_count(cc_data_.groups); i++) {
    if (groups_needed[i]) {
      array_push(unique_groups, i);
    }
  }

  xCode_addWorkspaceFolder(f, unique_groups, 0, 0);

  fprintf(f, "</Workspace>");
}

void xcode_generateInFolder(const char* in_workspace_path) {
  in_workspace_path = make_uri(in_workspace_path);
  if (in_workspace_path[strlen(in_workspace_path) - 1] != '/')
    in_workspace_path = cc_printf("%s/", in_workspace_path);

  char* output_folder = make_uri(cc_printf("%s%s", cc_data_.base_folder, in_workspace_path));

  char* build_to_base_path = make_path_relative(output_folder, cc_data_.base_folder);

  printf("Generating XCode workspace and projects in '%s'...\n", output_folder);

  int result = make_folder(output_folder);
  if (result != 0) {
    fprintf(stderr, "Error %i creating path '%s'\n", result, output_folder);
  }
  (void)chdir(output_folder);

  // Before doing anything, generate a UUID for each projects output file
  xcode_uuid* projectFileReferenceUUIDs = 0;
  projectFileReferenceUUIDs =
      (xcode_uuid*)array_reserve(projectFileReferenceUUIDs, sizeof(*projectFileReferenceUUIDs),
                                 array_count(cc_data_.projects));
  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    array_push(projectFileReferenceUUIDs, xCodeGenerateUUID());
  }

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const cc_project_impl_t* p = cc_data_.projects[i];

    const char* project_path = cc_printf("%s.xcodeproj", p->name);
    int result               = make_folder(project_path);

    const char* project_file_path = cc_printf("%s/project.pbxproj", project_path);

    FILE* f = fopen(project_file_path, "wb");

    if (f) {
      xCodeCreateProjectFile(f, p, projectFileReferenceUUIDs, build_to_base_path);
      fclose(f);
    }

    printf("Constructed XCode project '%s'\n", project_path);
  }

  {
    const char* workspace_path      = cc_printf("%s.xcworkspace", cc_data_.workspaceLabel);
    int result                      = make_folder(workspace_path);
    const char* workspace_file_path = cc_printf("%s/contents.xcworkspacedata", workspace_path);
    FILE* f                         = fopen(workspace_file_path, "wb");
    if (f) {
      xCodeCreateWorkspaceFile(f);
      fclose(f);
      printf("Constructed XCode workspace at '%s'\n", workspace_path);
    }
  }
}
