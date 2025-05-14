
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
  return cc_printf("%08x%08x%08x", uuid.uuid[2], uuid.uuid[1], uuid.uuid[0]);
}

xcode_uuid findUUIDForProject(const xcode_uuid* uuids, const cc_project_impl_t* project) {
  for (unsigned i = 0; i < array_count(uuids); ++i) {
    if (cc_data_.projects[i] == project) return uuids[i];
  }
  xcode_uuid empty = {0};
  return empty;
}

void add_build_setting(struct data_tree_t* dt, unsigned int node_build_settings, const char* k,
                       const char* v) {
  data_tree_api.set_object_value(dt, data_tree_api.create_object(dt, node_build_settings, k), v);
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

const char* xcodeFileTypeFromExtension(const char* ext) {
  if (strcmp(ext, "cpp") == 0) {
    return "sourcecode.cpp.cpp";
  } else if (strcmp(ext, "h") == 0) {
    return "sourcecode.c.h";
  } else if (strcmp(ext, "m") == 0) {
    return "sourcecode.c.objc";
  } else if (strcmp(ext, "mm") == 0) {
    return "sourcecode.cpp.objcpp";
  } else if (strcmp(ext, "metal") == 0) {
    return "sourcecode.metal";
  } else if (strcmp(ext, "plist") == 0) {
    return "text.plist.xml";
  } else if (strcmp(ext, "storyboard") == 0) {
    return "file.storyboard";
  } else if (strcmp(ext, "xcassets") == 0) {
    return "folder.assetcatalog";

    // Library file types
  } else if (strcmp(ext, "a") == 0) {
    return "archive.ar";
  } else if (strcmp(ext, "framework") == 0) {
    return "wrapper.framework";
  }

  // Else simply assume C file
  else {
    return "sourcecode.c.c";
  }
}

void export_tree_as_xcode_recursive_impl(FILE* f, const struct data_tree_t* dt, unsigned int node,
                                         unsigned int depth, bool no_newlines) {
  assert(node < array_count(dt->objects));

  const struct data_tree_object_t* obj = dt->objects + node;

  const bool is_root_section = (obj->name == NULL);

  bool child_no_newlines = no_newlines;

  // Skip the section if nothing is in it
  if (is_root_section) {
    if (obj->value_or_child.first_child == 0) {
      return;
    }

    // For clarity of build file section, keep those on a single line, just like XCode does
    if ((strstr(obj->comment, "PBXBuildFile") != NULL) ||
        (strstr(obj->comment, "PBXFileReference") != NULL)) {
      child_no_newlines = true;
    }
  }

  const char* preamble = "";
  if (!no_newlines) {
    for (unsigned int i = 0; i < depth; i++) {
      preamble = cc_printf("\t%s", preamble);
    }
  }

  if (obj->name != NULL) {
    fprintf(f, "%s%s", preamble, obj->name);
    if (obj->has_children && obj->comment) {
      fprintf(f, " /* %s */", obj->comment);
    }
    /*if (obj->first_parameter) {
      const struct data_tree_object_t* param_obj = dt->objects + obj->first_parameter;
      do {
        fprintf(f, " %s=\"%s\"", param_obj->name, param_obj->value_or_child.value);
      } while (param_obj->next_sibling && (param_obj = dt->objects + param_obj->next_sibling));
    }*/
  } else if (node != 0) {
    // Print as a top level comment in the file
    fprintf(f, "\n/* Begin %s section */\n", obj->comment);
  }

  const char newline_separator = (no_newlines ? ' ' : '\n');

  if (obj->has_children || obj->is_array) {
    if (obj->name) {
      if (obj->is_array) {
        fprintf(f, " = (%c", newline_separator);
      } else {
        fprintf(f, " = {%c", newline_separator);
      }
    }
    unsigned int child_index = obj->value_or_child.first_child;
    if (child_index) {
      const struct data_tree_object_t* child_obj = dt->objects + child_index;
      const char child_separator                 = obj->is_array ? ',' : ';';
      unsigned int child_depth                   = (obj->name) ? (depth + 1) : depth;
      do {
        if (!no_newlines && child_no_newlines) {
          fprintf(f, "%s", preamble);
        }
        export_tree_as_xcode_recursive_impl(f, dt, child_index, child_depth, child_no_newlines);
        if (child_obj->name) {
          fprintf(f, "%c%c", child_separator, newline_separator);
        }

        // Prepare for next iteration
        child_index = child_obj->next_sibling;
        child_obj   = (child_index > 0) ? (dt->objects + child_obj->next_sibling) : NULL;
      } while (child_obj);
    }
    if (obj->name) {
      const char node_closer = obj->is_array ? ')' : '}';
      fprintf(f, "%s%c", preamble, node_closer);
    }
  } else if (obj->value_or_child.value) {
    if (obj->name && obj->name[0]) {
      fprintf(f, " = %s", obj->value_or_child.value);
    } else {
      fprintf(f, "%s", obj->value_or_child.value);
    }
    if (obj->comment) {
      fprintf(f, " /* %s */", obj->comment);
    }
  }

  if (obj->name == NULL) {
    // Print as a top level comment in the file
    fprintf(f, "/* End %s section */\n", obj->comment);
  }
}

void export_tree_as_xcode(FILE* f, const struct data_tree_t* dt) {
  fprintf(f, "// !$*UTF8*$!\n");
  fprintf(f, "{\n");

  const struct data_tree_object_t* obj = dt->objects;
  unsigned int child_index             = obj->value_or_child.first_child;
  if (child_index) {
    const struct data_tree_object_t* child_obj = dt->objects + child_index;
    do {
      export_tree_as_xcode_recursive_impl(f, dt, child_index, 1, false);
      fprintf(f, ";\n");

      // Prepare for next iteration
      child_index = child_obj->next_sibling;
      child_obj   = (child_index > 0) ? (dt->objects + child_obj->next_sibling) : NULL;
    } while (child_obj);
  }
  fprintf(f, "}\n");
}

// Creates a child node with no name, and sets the value and comment on it. Used in XCode projects
// to create entries of an array with : ID /* comment */
void dt_api_add_child_value_and_comment(struct data_tree_t* dt, unsigned int parent_id,
                                        const char* value, const char* comment) {
  unsigned int node = data_tree_api.create_object(dt, parent_id, "");
  data_tree_api.set_object_value(dt, node, value);
  data_tree_api.set_object_comment(dt, node, comment);
}

void optionDeploymentTarget(struct data_tree_t* dt, unsigned int compile_group,
                            const char* remaining_flag_value) {
  data_tree_api.set_object_value(
      dt, data_tree_api.get_or_create_object(dt, compile_group, "MACOSX_DEPLOYMENT_TARGET"),
      remaining_flag_value);
}

typedef void (*func)(struct data_tree_t* dt, unsigned int compile_group,
                     const char* remaining_flag_value);

typedef struct xcode_compiler_flag {
  const char* flag;
  const func action;
} xcode_compiler_flag;

const xcode_compiler_flag xcode_known_compiler_flags_[] = {
    {"MACOSX_DEPLOYMENT_TARGET=", &optionDeploymentTarget}};

void xCodeCreateProjectFile(FILE* f, const cc_project_impl_t* in_project,
                            const xcode_uuid* projectFileReferenceUUIDs,
                            const char* in_output_folder, const char* build_to_base_path) {
  const cc_project_impl_t* p         = (cc_project_impl_t*)in_project;
  const struct data_tree_api* dt_api = &data_tree_api;
  struct data_tree_t dt              = dt_api->create();

  dt_api->set_object_value(&dt, dt_api->create_object(&dt, 0, "archiveVersion"), "1");
  dt.objects[dt_api->create_object(&dt, 0, "classes")].has_children = true;
  dt_api->set_object_value(&dt, dt_api->create_object(&dt, 0, "objectVersion"), "50");
  unsigned int nodeObjects = dt_api->create_object(&dt, 0, "objects");

  const char* substitution_keys[] = {
      "configuration",
      "platform",
      "workspace_folder",
  };
  // For MacOS currently the only allowed platform is  64-bit
  const char* substitution_values[] = {
      "$CONFIGURATION",
      "x64",
      in_output_folder,
  };

  const unsigned files_count  = array_count(p->file_data);
  const char** file_ref_paths = {0};
  for (unsigned i = 0; i < files_count; ++i) {
    array_push(file_ref_paths, cc_printf("%s%s", build_to_base_path, p->file_data[i]->path));
  }

  for (unsigned state_idx = 0; state_idx < array_count(p->state); state_idx++) {
    const cc_state_impl_t* state = p->state + state_idx;
    // Also all the include paths
    for (unsigned includes_idx = 0; includes_idx < array_count(state->include_folders);
         includes_idx++) {
      const char* include_path            = state->include_folders[includes_idx];
      const bool is_absolute_path         = (include_path[0] == '/') || (include_path[1] == ':');
      const bool starts_with_env_variable = (include_path[0] == '$');
      if (!is_absolute_path && !starts_with_env_variable) {
        state->include_folders[includes_idx] = cc_printf("%s%s", build_to_base_path, include_path);
      }
    }
    // And referenced external libs
    for (unsigned libs_idx = 0; libs_idx < array_count(state->external_libs); libs_idx++) {
      const char* lib_path                = state->external_libs[libs_idx];
      const bool is_absolute_path         = (lib_path[0] == '/') || (lib_path[1] == ':');
      const bool starts_with_env_variable = (lib_path[0] == '$');
      if (!is_absolute_path && !starts_with_env_variable) {
        state->external_libs[libs_idx] = cc_printf("%s%s", build_to_base_path, lib_path);
      }
    }
  }

  const char** fileReferenceUUID = {0};
  const char** fileUUID          = {0};
  for (unsigned fi = 0; fi < files_count; ++fi) {
    array_push(fileReferenceUUID, xCodeUUID2String(xCodeGenerateUUID()));
    array_push(fileUUID, xCodeUUID2String(xCodeGenerateUUID()));
  }

  const char** dependencyFileReferenceUUID   = {0};
  const char** dependencyBuildUUID           = {0};
  const char** dependencyEmbedLibrairiesUUID = {0};
  for (unsigned fi = 0; fi < array_count(p->dependantOn); ++fi) {
    array_push(dependencyFileReferenceUUID, xCodeUUID2String(xCodeGenerateUUID()));
    array_push(dependencyBuildUUID, xCodeUUID2String(xCodeGenerateUUID()));
    array_push(dependencyEmbedLibrairiesUUID, xCodeUUID2String(xCodeGenerateUUID()));
  }

  // Extract frameworks from list of external libs
  const char** external_frameworks = {0};
  for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
    const bool is_global_state = ((p->configs[ipc] == NULL) && (p->architectures[ipc] == NULL));

    const cc_state_impl_t* s = &(p->state[ipc]);
    for (unsigned fi = 0; fi < array_count(s->external_libs);) {
      const char* lib_path = s->external_libs[fi];
      if (strstr(lib_path, ".framework") == NULL) {
        fi++;
      } else {
        if (!is_global_state) {
          LOG_ERROR_AND_QUIT(
              ERR_CONFIGURATION,
              "Apple framework added to state with configuration or architecture. This is not yet "
              "supported");
        }
        array_push(external_frameworks, lib_path);
        array_remove_at_index(s->external_libs, fi);
      }
    }
  }
  // No frameworks left in the state now.

  const char** dependencyExternalLibraryFileReferenceUUID = {0};
  const char** dependencyExternalLibraryBuildUUID         = {0};
  for (unsigned fi = 0; fi < array_count(external_frameworks); ++fi) {
    array_push(dependencyExternalLibraryFileReferenceUUID, xCodeUUID2String(xCodeGenerateUUID()));
    array_push(dependencyExternalLibraryBuildUUID, xCodeUUID2String(xCodeGenerateUUID()));
  }

  xcode_uuid outputFileReferenceUIID = findUUIDForProject(projectFileReferenceUUIDs, p);
  xcode_uuid outputTargetUIID        = xCodeGenerateUUID();

  const char* outputName = p->name;
  if (p->type == CCProjectTypeStaticLibrary) {
    outputName = cc_printf("lib%s.a", outputName);
  } else if (p->type == CCProjectTypeDynamicLibrary) {
    outputName = cc_printf("%s.dylib", outputName);
  }

  // Create list of groups needed.
  const cc_group_impl_t** unique_groups = {0};
  const char** unique_groups_id         = {0};
  for (size_t ig = 0; ig < array_count(p->file_data); ++ig) {
    size_t g = p->file_data[ig]->parent_group_idx;
    while (g) {
      bool already_contains_group = false;
      for (size_t i = 0; i < array_count(unique_groups); ++i) {
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
  for (size_t ig = 0; ig < array_count(p->file_data_custom_command); ++ig) {
    size_t g = p->file_data_custom_command[ig]->parent_group_idx;
    while (g) {
      bool already_contains_group = false;
      for (size_t i = 0; i < array_count(unique_groups); ++i) {
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

  // Setup the top level sections in the file
  unsigned int PBXBuildFileSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXBuildFileSection, "PBXBuildFile");
  unsigned int PBXCopyFilesBuildPhaseSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXCopyFilesBuildPhaseSection, "PBXCopyFilesBuildPhase");
  unsigned int PBXFileReferenceSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXFileReferenceSection, "PBXFileReference");
  unsigned int PBXFrameworksBuildPhaseSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXFrameworksBuildPhaseSection, "PBXFrameworksBuildPhase");
  unsigned int PBXGroupSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXGroupSection, "PBXGroup");
  unsigned int PBXNativeTargetSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXNativeTargetSection, "PBXNativeTarget");
  unsigned int PBXProjectSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXProjectSection, "PBXProject");
  unsigned int PBXResourcesBuildPhaseSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXResourcesBuildPhaseSection, "PBXResourcesBuildPhase");
  unsigned int PBXSourcesBuildPhaseSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, PBXSourcesBuildPhaseSection, "PBXSourcesBuildPhase");
  unsigned int XCBuildConfigurationSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, XCBuildConfigurationSection, "XCBuildConfiguration");
  unsigned int XCConfigurationListSection = dt_api->create_object(&dt, nodeObjects, NULL);
  dt_api->set_object_comment(&dt, XCConfigurationListSection, "XCConfigurationList");
  {
    for (unsigned fi = 0; fi < files_count; ++fi) {
      const char* filename = p->file_data[fi]->path;
      if (is_source_file(filename) || is_buildable_resource_file(filename)) {
        unsigned int nodeFile = dt_api->create_object(&dt, PBXBuildFileSection, fileUUID[fi]);
        dt_api->set_object_comment(&dt, nodeFile,
                                   cc_printf("%s in Sources", strip_path(filename)));
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "isa"), "PBXBuildFile");
        unsigned int fr = dt_api->create_object(&dt, nodeFile, "fileRef");
        dt_api->set_object_value(&dt, fr, fileReferenceUUID[fi]);
        dt_api->set_object_comment(&dt, fr, strip_path(filename));
      }
    }
    for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
      const char* id             = dependencyFileReferenceUUID[i];
      const char* buildID        = dependencyBuildUUID[i];
      const char* dependantName  = NULL;
      bool is_dynamic_dependency = (p->dependantOn[i]->type == CCProjectTypeDynamicLibrary);
      if (!is_dynamic_dependency) {
        dependantName = cc_printf("lib%s.a", p->dependantOn[i]->name);
      } else {
        dependantName = cc_printf("lib%s.dylib", p->dependantOn[i]->name);
      }
      {
        unsigned int nodeFile = dt_api->create_object(&dt, PBXBuildFileSection, buildID);
        dt_api->set_object_comment(&dt, nodeFile, cc_printf("%s in Frameworks", dependantName));
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "isa"), "PBXBuildFile");
        unsigned int fr = dt_api->create_object(&dt, nodeFile, "fileRef");
        dt_api->set_object_value(&dt, fr, id);
        dt_api->set_object_comment(&dt, fr, dependantName);
      }
      if (is_dynamic_dependency) {
        unsigned int nodeFile =
            dt_api->create_object(&dt, PBXBuildFileSection, dependencyEmbedLibrairiesUUID[i]);
        dt_api->set_object_comment(&dt, nodeFile,
                                   cc_printf("%s in Embed Libraries", dependantName));
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "isa"), "PBXBuildFile");
        unsigned int fr = dt_api->create_object(&dt, nodeFile, "fileRef");
        dt_api->set_object_value(&dt, fr, id);
        dt_api->set_object_comment(&dt, fr, dependantName);

        unsigned int settings = dt_api->create_object(&dt, nodeFile, "settings");
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, settings, "ATTRIBUTES"),
                                 "(CodeSignOnCopy, )");
      }
    }
    for (unsigned i = 0; i < array_count(external_frameworks); ++i) {
      const char* id            = dependencyExternalLibraryFileReferenceUUID[i];
      const char* buildID       = dependencyExternalLibraryBuildUUID[i];
      const char* dependantName = cc_printf("%s", strip_path(external_frameworks[i]));
      unsigned int nodeFile     = dt_api->create_object(&dt, PBXBuildFileSection, buildID);
      dt_api->set_object_comment(&dt, nodeFile, cc_printf("%s in Frameworks", dependantName));
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "isa"), "PBXBuildFile");
      unsigned int fr = dt_api->create_object(&dt, nodeFile, "fileRef");
      dt_api->set_object_value(&dt, fr, id);
      dt_api->set_object_comment(&dt, fr, dependantName);
    }
  }

  {
    for (unsigned fi = 0; fi < files_count; ++fi) {
      const char* filename = p->file_data[fi]->path;
      const char* fileType = xcodeFileTypeFromExtension(file_extension(filename));
      unsigned int nodeFile =
          dt_api->create_object(&dt, PBXFileReferenceSection, fileReferenceUUID[fi]);
      dt_api->set_object_comment(&dt, nodeFile, strip_path(filename));
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "isa"),
                               "PBXFileReference");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "fileEncoding"), "4");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "lastKnownFileType"),
                               fileType);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "name"),
                               strip_path(filename));
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "path"),
                               file_ref_paths[fi]);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "sourceTree"),
                               "SOURCE_ROOT");

      if (_internal.is_verbose) {
        // printf("Adding file '%s' as '%s'\n", filename, file_ref_paths[fi]);
      }
    }

    {
      unsigned int nodeFile = dt_api->create_object(&dt, PBXFileReferenceSection,
                                                    xCodeUUID2String(outputFileReferenceUIID));
      dt_api->set_object_comment(&dt, nodeFile, outputName);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "isa"),
                               "PBXFileReference");
      if (p->type == CCProjectTypeConsoleApplication) {
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "explicitFileType"),
                                 "compiled.mach-o.executable");
      } else if (p->type == CCProjectTypeStaticLibrary) {
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "explicitFileType"),
                                 "archive.ar");
      } else if (p->type == CCProjectTypeDynamicLibrary) {
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "explicitFileType"),
                                 "compiled.mach-o.dylib");
      }
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "includeInIndex"), "0");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "path"), outputName);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "sourceTree"),
                               "BUILT_PRODUCTS_DIR");
    }

    for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
      const char* id               = dependencyFileReferenceUUID[i];
      unsigned int nodeFile        = dt_api->create_object(&dt, PBXFileReferenceSection, id);
      const char* dependantName    = NULL;
      const char* explicitFileType = NULL;
      if (p->dependantOn[i]->type == CCProjectTypeStaticLibrary) {
        dependantName    = cc_printf("lib%s.a", p->dependantOn[i]->name);
        explicitFileType = "archive.ar";
      } else {
        dependantName    = cc_printf("lib%s.dylib", p->dependantOn[i]->name);
        explicitFileType = "compiled.mach-o.dylib";
      }
      dt_api->set_object_value(&dt, nodeFile, id);
      dt_api->set_object_comment(&dt, nodeFile, dependantName);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "isa"),
                               "PBXFileReference");

      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "explicitFileType"),
                               explicitFileType);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "path"), dependantName);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "sourceTree"),
                               "BUILT_PRODUCTS_DIR");
    }
    for (unsigned i = 0; i < array_count(external_frameworks); ++i) {
      const char* id            = dependencyExternalLibraryFileReferenceUUID[i];
      const char* dependantName = cc_printf("%s", strip_path(external_frameworks[i]));
      const char* fileType      = xcodeFileTypeFromExtension(file_extension(dependantName));
      unsigned int nodeFile     = dt_api->create_object(&dt, PBXFileReferenceSection, id);
      dt_api->set_object_value(&dt, nodeFile, id);
      dt_api->set_object_comment(&dt, nodeFile, dependantName);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "isa"),
                               "PBXFileReference");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "explicitFileType"),
                               fileType);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "name"), dependantName);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "path"),
                               external_frameworks[i]);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFile, "sourceTree"),
                               "SDK_ROOT");
    }
  }

  const char* main_group_id      = xCodeUUID2String(xCodeGenerateUUID());
  const char* main_group_comment = "Main Group";
  {
    unsigned int group_children;
    {
      unsigned int nodeNT = dt_api->create_object(&dt, PBXGroupSection, main_group_id);
      dt_api->set_object_comment(&dt, nodeNT, main_group_comment);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeNT, "isa"), "PBXGroup");
      {
        unsigned int nodeChildren         = dt_api->create_object(&dt, nodeNT, "children");
        group_children                    = nodeChildren;
        dt.objects[nodeChildren].is_array = true;

        for (unsigned i = 0; i < array_count(unique_groups); ++i) {
          if (unique_groups[i]->parent_group_idx == 0) {
            unsigned int node = dt_api->create_object(&dt, nodeChildren, "");
            dt_api->set_object_value(
                &dt, node,
                xCodeStringFromGroup(unique_groups, unique_groups_id, unique_groups[i]));
          }
        }
        for (unsigned i = 0; i < array_count(p->file_data); ++i) {
          if (p->file_data[i]->parent_group_idx == 0) {
            dt_api_add_child_value_and_comment(&dt, nodeChildren, fileReferenceUUID[i],
                                               strip_path(p->file_data[i]->path));
          }
        }
      }
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeNT, "sourceTree"),
                               "\"<group>\"");
    }
    {
      for (unsigned i = 0; i < num_unique_groups; ++i) {
        const cc_group_impl_t* g = unique_groups[i];
        unsigned int nodeGroup   = dt_api->create_object(
            &dt, PBXGroupSection, xCodeStringFromGroup(unique_groups, unique_groups_id, g));
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "isa"), "PBXGroup");
        unsigned int nodeChildren         = dt_api->create_object(&dt, nodeGroup, "children");
        dt.objects[nodeChildren].is_array = true;

        for (unsigned fi = 0; fi < files_count; ++fi) {
          const char* filename              = p->file_data[fi]->path;
          const cc_group_impl_t* file_group = &cc_data_.groups[p->file_data[fi]->parent_group_idx];
          if (file_group == g) {
            dt_api_add_child_value_and_comment(&dt, nodeChildren, fileReferenceUUID[fi],
                                               strip_path(filename));
          }
        }
        for (unsigned gi = 0; gi < num_unique_groups; ++gi) {
          const cc_group_impl_t* child_group = unique_groups[gi];
          if (&cc_data_.groups[child_group->parent_group_idx] == g) {
            unsigned int node = dt_api->create_object(&dt, nodeChildren, "");
            dt_api->set_object_value(
                &dt, node, xCodeStringFromGroup(unique_groups, unique_groups_id, child_group));
          }
        }
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "name"),
                                 cc_printf("\"%s\"", g->name));
        dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "sourceTree"),
                                 "\"<group>\"");
      }
    }
    const bool references_libraries =
        (array_count(p->dependantOn) > 0) || (array_count(external_frameworks) > 0);
    if (references_libraries) {
      const char* id      = xCodeUUID2String(xCodeGenerateUUID());
      const char* comment = "Frameworks";
      dt_api_add_child_value_and_comment(&dt, group_children, id, comment);
      unsigned int nodeGroup = dt_api->create_object(&dt, PBXGroupSection, id);
      dt_api->set_object_comment(&dt, nodeGroup, comment);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "isa"), "PBXGroup");
      unsigned int nodeChildren         = dt_api->create_object(&dt, nodeGroup, "children");
      dt.objects[nodeChildren].is_array = true;
      for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
        const char* buildID       = dependencyFileReferenceUUID[i];
        const char* dependentName = NULL;
        if (p->dependantOn[i]->type == CCProjectTypeStaticLibrary) {
          dependentName = cc_printf("lib%s.a", strip_path(p->dependantOn[i]->name));
        } else if (p->dependantOn[i]->type == CCProjectTypeDynamicLibrary) {
          dependentName = cc_printf("lib%s.dylib", strip_path(p->dependantOn[i]->name));
        }
        dt_api_add_child_value_and_comment(&dt, nodeChildren, buildID, dependentName);
      }
      for (unsigned i = 0; i < array_count(external_frameworks); ++i) {
        const char* buildID       = dependencyExternalLibraryFileReferenceUUID[i];
        const char* dependantName = cc_printf("%s", strip_path(external_frameworks[i]));
        dt_api_add_child_value_and_comment(&dt, nodeChildren, buildID, dependantName);
      }
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "name"), "Frameworks");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "sourceTree"),
                               "\"<group>\"");
    }
    {
      const char* id = xCodeUUID2String(xCodeGenerateUUID());
      dt_api_add_child_value_and_comment(&dt, group_children, id, "Products");

      unsigned int nodeGroup = dt_api->create_object(&dt, PBXGroupSection, id);
      dt_api->set_object_comment(&dt, nodeGroup, "Products");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "isa"), "PBXGroup");
      unsigned int nodeChildren         = dt_api->create_object(&dt, nodeGroup, "children");
      dt.objects[nodeChildren].is_array = true;
      dt_api_add_child_value_and_comment(&dt, nodeChildren,
                                         xCodeUUID2String(outputFileReferenceUIID), outputName);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "name"), "Products");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeGroup, "sourceTree"),
                               "\"<group>\"");
    }
  }

  unsigned int node_build_phases;

  const char* build_configurations_id  = xCodeUUID2String(xCodeGenerateUUID());
  const char* build_configurations_id2 = xCodeUUID2String(xCodeGenerateUUID());
  const char* build_configurations_comment =
      cc_printf("Build configuration list for PBXNativeTarget \"%s\"", p->name);
  {
    unsigned int nodeNT =
        dt_api->create_object(&dt, PBXNativeTargetSection, xCodeUUID2String(outputTargetUIID));
    dt_api->set_object_comment(&dt, nodeNT, p->name);
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeNT, "isa"), "PBXNativeTarget");
    unsigned int nodeBCL = dt_api->create_object(&dt, nodeNT, "buildConfigurationList");
    dt_api->set_object_value(&dt, nodeBCL, build_configurations_id2);
    dt_api->set_object_comment(&dt, nodeBCL, build_configurations_comment);
    node_build_phases                      = dt_api->create_object(&dt, nodeNT, "buildPhases");
    dt.objects[node_build_phases].is_array = true;

    unsigned int nodeBR                   = dt_api->create_object(&dt, nodeNT, "buildRules");
    dt.objects[nodeBR].is_array           = true;
    unsigned int nodeDependencies         = dt_api->create_object(&dt, nodeNT, "dependencies");
    dt.objects[nodeDependencies].is_array = true;
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeNT, "name"), p->name);
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeNT, "productName"), p->name);
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeNT, "productReference"),
        cc_printf("%s /* %s */", xCodeUUID2String(outputFileReferenceUIID), outputName));
    unsigned int nodePT = dt_api->create_object(&dt, nodeNT, "productType");

    switch (p->type) {
      case CCProjectTypeConsoleApplication:
        dt_api->set_object_value(&dt, nodePT, "\"com.apple.product-type.tool\"");
        break;
      case CCProjectTypeWindowedApplication:
        dt_api->set_object_value(&dt, nodePT, "\"com.apple.product-type.application\"");
        break;
      case CCProjectTypeStaticLibrary:
        dt_api->set_object_value(&dt, nodePT, "\"com.apple.product-type.library.static\"");
        break;
      case CCProjectTypeDynamicLibrary:
        dt_api->set_object_value(&dt, nodePT, "\"com.apple.product-type.library.dynamic\"");
        break;
    }
  }

  bool has_dynamic_lib_dependency = false;
  {
    const char* id = xCodeUUID2String(xCodeGenerateUUID());
    dt_api_add_child_value_and_comment(&dt, node_build_phases, id, "Frameworks");
    unsigned int nodeFrameworks = dt_api->create_object(&dt, PBXFrameworksBuildPhaseSection, id);
    dt_api->set_object_comment(&dt, nodeFrameworks, "Frameworks");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFrameworks, "isa"),
                             "PBXFrameworksBuildPhase");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFrameworks, "buildActionMask"),
                             "2147483647");
    unsigned int nodeFiles         = dt_api->create_object(&dt, nodeFrameworks, "files");
    dt.objects[nodeFiles].is_array = true;
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeFrameworks, "runOnlyForDeploymentPostprocessing"),
        "0");

    for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
      const char* buildID       = dependencyBuildUUID[i];
      const char* dependentName = NULL;
      if (p->dependantOn[i]->type == CCProjectTypeStaticLibrary) {
        dependentName = cc_printf("lib%s.a", p->dependantOn[i]->name);
      } else if (p->dependantOn[i]->type == CCProjectTypeDynamicLibrary) {
        dependentName = cc_printf("lib%s.dylib", p->dependantOn[i]->name);
      }
      dt_api_add_child_value_and_comment(&dt, nodeFiles, buildID,
                                         cc_printf("%s in Frameworks", dependentName));
      if (p->dependantOn[i]->type == CCProjectTypeDynamicLibrary) {
        has_dynamic_lib_dependency = true;
      }
    }
    for (unsigned i = 0; i < array_count(external_frameworks); ++i) {
      const char* buildID       = dependencyExternalLibraryBuildUUID[i];
      const char* dependantName = cc_printf("%s", strip_path(external_frameworks[i]));
      dt_api_add_child_value_and_comment(&dt, nodeFiles, buildID,
                                         cc_printf("%s in Frameworks", dependantName));
    }
  }

  const char* project_object_id = xCodeUUID2String(xCodeGenerateUUID());
  {
    unsigned int nodeProject = dt_api->create_object(&dt, PBXProjectSection, project_object_id);
    dt_api->set_object_comment(&dt, nodeProject, "Project object");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeProject, "isa"), "PBXProject");
    unsigned int nodeAttributes = dt_api->create_object(&dt, nodeProject, "attributes");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeAttributes, "LastUpgradeCheck"),
                             "1130");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeAttributes, "ORGANIZATIONNAME"),
                             "\"Daedalus Development\"");
    unsigned int nodeTA = dt_api->create_object(&dt, nodeAttributes, "TargetAttributes");
    unsigned int node   = dt_api->create_object(&dt, nodeTA, xCodeUUID2String(outputTargetUIID));
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "CreatedOnToolsVersion"),
                             "11.3");

    unsigned int bcl = dt_api->create_object(&dt, nodeProject, "buildConfigurationList");
    dt_api->set_object_value(&dt, bcl, build_configurations_id);
    dt_api->set_object_comment(&dt, bcl, build_configurations_comment);
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeProject, "compatibilityVersion"),
                             "\"Xcode 9.3\"");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeProject, "developmentRegion"),
                             "en");
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeProject, "hasScannedForEncodings"), "0");
    unsigned int kr         = dt_api->create_object(&dt, nodeProject, "knownRegions");
    dt.objects[kr].is_array = true;
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, kr, ""), "en");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, kr, ""), "Base");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeProject, "mainGroup"),
                             main_group_id);
    unsigned int prg = dt_api->create_object(&dt, nodeProject, "productRefGroup");
    dt_api->set_object_value(&dt, prg, "403CC53C23EB479400558E07");
    dt_api->set_object_comment(&dt, prg, "Products");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeProject, "projectDirPath"),
                             "\"\"");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeProject, "projectRoot"), "\"\"");
    unsigned int nodeTargets         = dt_api->create_object(&dt, nodeProject, "targets");
    dt.objects[nodeTargets].is_array = true;
    dt_api_add_child_value_and_comment(&dt, nodeTargets, xCodeUUID2String(outputTargetUIID),
                                       p->name);
  }

  {
    const char* id = xCodeUUID2String(xCodeGenerateUUID());
    dt_api_add_child_value_and_comment(&dt, node_build_phases, id, "Resources");
    unsigned int nodeResources = dt_api->create_object(&dt, PBXResourcesBuildPhaseSection, id);
    dt_api->set_object_comment(&dt, nodeResources, "Resources");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeResources, "isa"),
                             "PBXResourcesBuildPhase");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeResources, "buildActionMask"),
                             "2147483647");
    {
      unsigned int nodeFiles         = dt_api->create_object(&dt, nodeResources, "files");
      dt.objects[nodeFiles].is_array = true;
      for (unsigned fi = 0; fi < files_count; ++fi) {
        const char* filename = p->file_data[fi]->path;
        if (is_buildable_resource_file(filename)) {
          dt_api_add_child_value_and_comment(&dt, nodeFiles, fileUUID[fi], strip_path(filename));
        }
      }
    }
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeResources, "runOnlyForDeploymentPostprocessing"), "0");
  }

  for (unsigned fi = 0; fi < array_count(p->file_data_custom_command); fi++) {
    const struct cc_file_custom_command_t_* file = p->file_data_custom_command[fi];

    const char* input_output_substitution_keys[]   = {"input", "output"};
    const char* input_output_substitution_values[] = {file->path, file->output_file};

    const char* custom_command = cc_substitute(file->command, substitution_keys,
                                               substitution_values, countof(substitution_keys));
    custom_command =
        cc_substitute(custom_command, input_output_substitution_keys,
                      input_output_substitution_values, countof(input_output_substitution_keys));

    const char* id      = xCodeUUID2String(xCodeGenerateUUID());
    const char* comment = "ShellScript - CustomCommand";
    dt_api_add_child_value_and_comment(&dt, node_build_phases, id, comment);
    unsigned int nodeShellScript = dt_api->create_object(&dt, nodeObjects, id);
    dt_api->set_object_comment(&dt, nodeShellScript, comment);
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeShellScript, "isa"),
                             "PBXShellScriptBuildPhase");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeShellScript, "buildActionMask"),
                             "2147483647");
    {
      unsigned int nodeFiles         = dt_api->create_object(&dt, nodeShellScript, "files");
      dt.objects[nodeFiles].is_array = true;
    }
    {
      unsigned int nodeFiles = dt_api->create_object(&dt, nodeShellScript, "inputFileListPaths");
      dt.objects[nodeFiles].is_array = true;
    }
    {
      unsigned int nodeFiles         = dt_api->create_object(&dt, nodeShellScript, "inputPaths");
      dt.objects[nodeFiles].is_array = true;
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFiles, ""),
                               cc_printf("\"$(SRCROOT)/%s\"", file->path));
    }
    {
      unsigned int nodeFiles = dt_api->create_object(&dt, nodeShellScript, "outputFileListPaths");
      dt.objects[nodeFiles].is_array = true;
    }
    {
      unsigned int nodeFiles         = dt_api->create_object(&dt, nodeShellScript, "outputPaths");
      dt.objects[nodeFiles].is_array = true;
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeFiles, ""),
                               cc_printf("\"$(SRCROOT)/%s\"", file->output_file));
    }
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeShellScript, "runOnlyForDeploymentPostprocessing"),
        "0");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeShellScript, "shellPath"),
                             "/bin/sh");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeShellScript, "shellScript"),
                             cc_printf("\"cd $SRCROOT && %s\"", custom_command));
  }

  {
    const char* id      = xCodeUUID2String(xCodeGenerateUUID());
    const char* comment = "Sources";
    dt_api_add_child_value_and_comment(&dt, node_build_phases, id, comment);
    unsigned int nodeSourcesBuildPhase =
        dt_api->create_object(&dt, PBXSourcesBuildPhaseSection, id);
    dt_api->set_object_comment(&dt, nodeSourcesBuildPhase, comment);
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeSourcesBuildPhase, "isa"),
                             "PBXSourcesBuildPhase");
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeSourcesBuildPhase, "buildActionMask"), "2147483647");
    unsigned int nodeFiles = dt_api->create_object(&dt, nodeSourcesBuildPhase, "files");
    dt_api->set_object_value(
        &dt,
        dt_api->create_object(&dt, nodeSourcesBuildPhase, "runOnlyForDeploymentPostprocessing"),
        "0");
    dt.objects[nodeFiles].is_array = true;
    for (unsigned fi = 0; fi < files_count; ++fi) {
      const char* filename = p->file_data[fi]->path;
      if (is_source_file(filename)) {
        dt_api_add_child_value_and_comment(&dt, nodeFiles, fileUUID[fi],
                                           cc_printf("%s in Sources", strip_path(filename)));
      }
    }
  }

  if (p->type == CCProjectTypeConsoleApplication) {
    // XCode builds to some internal location. Use an additional step to then copy the binary to
    // where CConstruct wants it.
    {
      const char* id      = xCodeUUID2String(xCodeGenerateUUID());
      const char* comment = "CopyFiles - Binary To CConstruct location";
      dt_api_add_child_value_and_comment(&dt, node_build_phases, id, comment);
      unsigned int node = dt_api->create_object(&dt, PBXCopyFilesBuildPhaseSection, id);
      dt_api->set_object_comment(&dt, node, comment);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "isa"),
                               "PBXCopyFilesBuildPhase");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "buildActionMask"),
                               "2147483647");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "dstPath"),
                               "/usr/share/man/man1/");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "dstSubfolderSpec"), "0");

      unsigned int nodeFiles         = dt_api->create_object(&dt, node, "files");
      dt.objects[nodeFiles].is_array = true;
      dt_api->set_object_value(
          &dt, dt_api->create_object(&dt, node, "runOnlyForDeploymentPostprocessing"), "1");
    }

    if (has_dynamic_lib_dependency) {
      const char* id      = xCodeUUID2String(xCodeGenerateUUID());
      const char* comment = "Embed Libraries";
      dt_api_add_child_value_and_comment(&dt, node_build_phases, id, comment);
      unsigned int node = dt_api->create_object(&dt, PBXCopyFilesBuildPhaseSection, id);
      dt_api->set_object_comment(&dt, node, comment);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "isa"),
                               "PBXCopyFilesBuildPhase");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "buildActionMask"),
                               "2147483647");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "dstPath"), "\"\"");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "dstSubfolderSpec"), "10");
      unsigned int nodeFiles         = dt_api->create_object(&dt, node, "files");
      dt.objects[nodeFiles].is_array = true;
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, node, "name"),
                               cc_printf("\"%s\"", comment));
      dt_api->set_object_value(
          &dt, dt_api->create_object(&dt, node, "runOnlyForDeploymentPostprocessing"), "0");

      for (unsigned i = 0; i < array_count(p->dependantOn); ++i) {
        if (p->dependantOn[i]->type == CCProjectTypeDynamicLibrary) {
          dt_api_add_child_value_and_comment(
              &dt, nodeFiles, dependencyEmbedLibrairiesUUID[i],
              cc_printf("lib%s.dylib in Embed Libraries", p->dependantOn[i]->name));
        }
      }
    }
  }

  // This actually has to be added after Sources so that the binary has been built before the
  // post-build action
  bool has_post_build_action = (p->postBuildAction != 0);
  if (has_post_build_action) {
    const char* id      = xCodeUUID2String(xCodeGenerateUUID());
    const char* comment = "ShellScript - Post-Build Action";
    dt_api_add_child_value_and_comment(&dt, node_build_phases, id, comment);

    unsigned int nodeShellScript = dt_api->create_object(&dt, nodeObjects, id);
    dt_api->set_object_comment(&dt, nodeShellScript, comment);
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeShellScript, "isa"),
                             "PBXShellScriptBuildPhase");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeShellScript, "buildActionMask"),
                             "2147483647");
    {
      unsigned int nodeFiles         = dt_api->create_object(&dt, nodeShellScript, "files");
      dt.objects[nodeFiles].is_array = true;
    }
    {
      unsigned int nodeFiles = dt_api->create_object(&dt, nodeShellScript, "inputFileListPaths");
      dt.objects[nodeFiles].is_array = true;
    }
    {
      unsigned int nodeFiles         = dt_api->create_object(&dt, nodeShellScript, "inputPaths");
      dt.objects[nodeFiles].is_array = true;
    }
    {
      unsigned int nodeFiles = dt_api->create_object(&dt, nodeShellScript, "outputFileListPaths");
      dt.objects[nodeFiles].is_array = true;
    }
    {
      unsigned int nodeFiles         = dt_api->create_object(&dt, nodeShellScript, "outputPaths");
      dt.objects[nodeFiles].is_array = true;
    }
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeShellScript, "runOnlyForDeploymentPostprocessing"),
        "0");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeShellScript, "shellPath"),
                             "/bin/sh");
    const char* postBuildAction = cc_printf("%s", p->postBuildAction);
    postBuildAction = cc_substitute(postBuildAction, substitution_keys, substitution_values,
                                    countof(substitution_keys));

    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeShellScript, "shellScript"),
                             cc_printf("\"%s\"", postBuildAction));
  }

  unsigned int node_project_configurations;
  unsigned int node_target_configurations;
  {
    unsigned int nodeConfigurationList =
        dt_api->create_object(&dt, XCConfigurationListSection, build_configurations_id);
    dt_api->set_object_comment(&dt, nodeConfigurationList, build_configurations_comment);
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeConfigurationList, "isa"),
                             "XCConfigurationList");
    node_project_configurations =
        dt_api->create_object(&dt, nodeConfigurationList, "buildConfigurations");
    dt.objects[node_project_configurations].is_array = true;
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeConfigurationList, "defaultConfigurationIsVisible"),
        "0");
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, nodeConfigurationList, "defaultConfigurationName"),
        "Release");
  }

  {
    unsigned int node_xc =
        dt_api->create_object(&dt, XCConfigurationListSection, build_configurations_id2);
    dt_api->set_object_comment(&dt, node_xc, build_configurations_comment);
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, node_xc, "isa"),
                             "XCConfigurationList");
    node_target_configurations = dt_api->create_object(&dt, node_xc, "buildConfigurations");
    dt.objects[node_target_configurations].is_array = true;
    dt_api->set_object_value(
        &dt, dt_api->create_object(&dt, node_xc, "defaultConfigurationIsVisible"), "0");
    dt_api->set_object_value(&dt, dt_api->create_object(&dt, node_xc, "defaultConfigurationName"),
                             "Release");
  }

  // Find Info.plist
  const char* info_plist_path = NULL;
  for (unsigned i = 0; i < files_count; i++) {
    const char* filename = strip_path(p->file_data[i]->path);
    if (strcmp(filename, "Info.plist") == 0) {
      info_plist_path = file_ref_paths[i];
    }
  }

  const unsigned num_configurations = array_count(cc_data_.configurations);
  for (unsigned i = 0; i < num_configurations; ++i) {
    const cc_configuration_impl_t* config = cc_data_.configurations[i];
    const char* config_name               = cc_data_.configurations[i]->label;

    EStateWarningLevel combined_warning_level = EStateWarningLevelDefault;
    bool shouldDisableWarningsAsError         = false;
    for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
      const cc_state_impl_t* flags = &(p->state[ipc]);

      // TODO ordering and combination so that more specific flags can override general ones
      if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;
      // if ((p->architectures[ipc] != arch) && (p->architectures[ipc] != NULL)) continue;

      shouldDisableWarningsAsError = flags->disableWarningsAsErrors;
      combined_warning_level       = flags->warningLevel;
    }

    {
      const char* config_id = xCodeUUID2String(xCodeGenerateUUID());

      dt_api_add_child_value_and_comment(&dt, node_project_configurations, config_id, config_name);

      const unsigned int nodeBuildConfigurationList =
          dt_api->create_object(&dt, XCBuildConfigurationSection, config_id);
      dt_api->set_object_comment(&dt, nodeBuildConfigurationList, config_name);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeBuildConfigurationList, "isa"),
                               "XCBuildConfiguration");
      const unsigned int nodeBuildSettings =
          dt_api->create_object(&dt, nodeBuildConfigurationList, "buildSettings");

      const unsigned int node_preprocessor_defines =
          dt_api->create_object(&dt, nodeBuildSettings, "GCC_PREPROCESSOR_DEFINITIONS");
      dt.objects[node_preprocessor_defines].is_array = true;
      dt_api_add_child_value_and_comment(&dt, node_preprocessor_defines, "\"$(inherited)\"", NULL);
      if (strcmp(config->label, "Debug") == 0) {
        dt_api_add_child_value_and_comment(&dt, node_preprocessor_defines, "\"DEBUG=1\"", NULL);
        add_build_setting(&dt, nodeBuildSettings, "DEBUG_INFORMATION_FORMAT", "dwarf");
        add_build_setting(&dt, nodeBuildSettings, "ENABLE_TESTABILITY", "YES");
        add_build_setting(&dt, nodeBuildSettings, "GCC_OPTIMIZATION_LEVEL", "0");
      } else {
        add_build_setting(&dt, nodeBuildSettings, "DEBUG_INFORMATION_FORMAT",
                          "\"dwarf-with-dsym\"");
        add_build_setting(&dt, nodeBuildSettings, "ENABLE_NS_ASSERTIONS", "NO");
      }

      const unsigned int node_header_search_paths =
          dt_api->create_object(&dt, nodeBuildSettings, "HEADER_SEARCH_PATHS");
      dt.objects[node_header_search_paths].is_array = true;

      const unsigned int node_additional_compiler_flags =
          dt_api->create_object(&dt, nodeBuildSettings, "OTHER_CFLAGS");
      dt.objects[node_additional_compiler_flags].is_array = true;

      assert(array_count(cc_data_.architectures) == 1);
      assert(cc_data_.architectures[0]->type == EArchitectureX64);
      const char* resolved_output_folder = cc_substitute(
          p->outputFolder, substitution_keys, substitution_values, countof(substitution_keys));

      const char* safe_output_folder = cc_printf("\"%s\"", resolved_output_folder);

      add_build_setting(&dt, nodeBuildSettings, "GCC_C_LANGUAGE_STANDARD",
                        "c11");  // TODO:Expose this?
      add_build_setting(&dt, nodeBuildSettings, "CLANG_CXX_LANGUAGE_STANDARD",
                        "\"c++0x\"");  // TODO:Expose this?

      if (!shouldDisableWarningsAsError) {
        add_build_setting(&dt, nodeBuildSettings, "GCC_TREAT_WARNINGS_AS_ERRORS", "YES");
      } else {
        add_build_setting(&dt, nodeBuildSettings, "GCC_TREAT_WARNINGS_AS_ERRORS", "NO");
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
          //"RUN_CLANG_STATIC_ANALYZER",
      };
      if (combined_warning_level == EStateWarningLevelHigh) {
        for (size_t ibs = 0; ibs < countof(high_enabled_warnings); ++ibs) {
          add_build_setting(&dt, nodeBuildSettings, high_enabled_warnings[ibs], "YES");
        }
        for (size_t ibs = 0; ibs < countof(default_enabled_warnings); ++ibs) {
          add_build_setting(&dt, nodeBuildSettings, default_enabled_warnings[ibs], "YES");
        }

        dt_api_add_child_value_and_comment(&dt, node_additional_compiler_flags, "\"-Wformat=2\"",
                                           NULL);
        dt_api_add_child_value_and_comment(&dt, node_additional_compiler_flags, "\"-Wextra\"",
                                           NULL);
        // disabling unused parameters needs to be done after -Wextra
        dt_api_add_child_value_and_comment(&dt, node_additional_compiler_flags,
                                           "\"-Wno-unused-parameter\"", NULL);
        // This warning can complain about my_struct s = {0};
        // Ref:
        // https://stackoverflow.com/questions/13905200/is-it-wise-to-ignore-gcc-clangs-wmissing-braces-warning
        dt_api_add_child_value_and_comment(&dt, node_additional_compiler_flags,
                                           "\"-Wno-missing-braces\"", NULL);

        // https://github.com/boredzo/Warnings-xcconfig/wiki/Warnings-Explained

      } else if (combined_warning_level == EStateWarningLevelNone) {
        add_build_setting(&dt, nodeBuildSettings, "GCC_WARN_INHIBIT_ALL_WARNINGS", "YES");
      } else if (combined_warning_level == EStateWarningLevelMedium) {
        for (size_t iw = 0; iw < countof(default_enabled_warnings); ++iw) {
          add_build_setting(&dt, nodeBuildSettings, default_enabled_warnings[iw], "YES");
        }
      }

      add_build_setting(&dt, nodeBuildSettings, "ALWAYS_SEARCH_USER_PATHS", "NO");
      add_build_setting(&dt, nodeBuildSettings, "CONFIGURATION_BUILD_DIR", safe_output_folder);

      add_build_setting(&dt, nodeBuildSettings, "ONLY_ACTIVE_ARCH", "YES");
      assert(array_count(cc_data_.platforms) == 1);
      switch (cc_data_.platforms[0]->type) {
        case EPlatformDesktop:
          add_build_setting(&dt, nodeBuildSettings, "MACOSX_DEPLOYMENT_TARGET", "10.14");
          add_build_setting(&dt, nodeBuildSettings, "SDKROOT", "macosx");
          break;
        case EPlatformPhone:
          add_build_setting(&dt, nodeBuildSettings, "IPHONEOS_DEPLOYMENT_TARGET", "13.2");
          add_build_setting(&dt, nodeBuildSettings, "SDKROOT", "iphoneos");
          add_build_setting(&dt, nodeBuildSettings, "CLANG_ENABLE_MODULES", "YES");
          add_build_setting(&dt, nodeBuildSettings, "CLANG_ENABLE_OBJC_ARC", "YES");
          add_build_setting(&dt, nodeBuildSettings, "CLANG_ENABLE_OBJC_WEAK", "YES");

          break;
      }

      for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
        const cc_state_impl_t* flags = &(p->state[ipc]);

        // TODO ordering and combination so that more specific flags can override general ones
        if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;
        // if ((p->architectures[ipc] != arch) && (p->architectures[ipc] != NULL)) continue;

        shouldDisableWarningsAsError = flags->disableWarningsAsErrors;
        combined_warning_level       = flags->warningLevel;

        for (unsigned pdi = 0; pdi < array_count(flags->defines); ++pdi) {
          dt_api_add_child_value_and_comment(&dt, node_preprocessor_defines,
                                             cc_printf("\"%s\"", flags->defines[pdi]), NULL);
        }

        for (unsigned cfi = 0; cfi < array_count(flags->compile_options); ++cfi) {
          // Index in known flags
          const char* current_flag = flags->compile_options[cfi];
          bool found               = false;
          for (unsigned kfi = 0; kfi < countof(xcode_known_compiler_flags_); kfi++) {
            const char* foundString = strstr(current_flag, xcode_known_compiler_flags_[kfi].flag);
            if (foundString) {
              found = true;
              xcode_known_compiler_flags_[kfi].action(
                  &dt, nodeBuildSettings,
                  foundString + strlen(xcode_known_compiler_flags_[kfi].flag));
            }
          }
          if (!found) {
            dt_api_add_child_value_and_comment(&dt, node_additional_compiler_flags,
                                               cc_printf("\"%s\"", flags->compile_options[cfi]),
                                               NULL);
          }
        }
        for (unsigned ifi = 0; ifi < array_count(flags->include_folders); ++ifi) {
          // Order matters here, so append
          add_build_setting(&dt, node_header_search_paths, flags->include_folders[ifi], NULL);
        }
      }
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeBuildConfigurationList, "name"),
                               config_name);
    }
    {
      const char* id2 = xCodeUUID2String(xCodeGenerateUUID());
      dt_api_add_child_value_and_comment(&dt, node_target_configurations, id2, config_name);
      unsigned int nodeBuildConfigurationList =
          dt_api->create_object(&dt, XCBuildConfigurationSection, id2);
      dt_api->set_object_comment(&dt, nodeBuildConfigurationList,
                                 cc_data_.configurations[i]->label);
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeBuildConfigurationList, "isa"),
                               "XCBuildConfiguration");
      unsigned int nodeBuildSettings =
          dt_api->create_object(&dt, nodeBuildConfigurationList, "buildSettings");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeBuildConfigurationList, "name"),
                               cc_data_.configurations[i]->label);

      switch (cc_data_.platforms[0]->type) {
        case EPlatformDesktop: {
          dt_api->set_object_value(
              &dt, dt_api->create_object(&dt, nodeBuildSettings, "CODE_SIGN_STYLE"), "Automatic");
          if (info_plist_path) {
            dt_api->set_object_value(
                &dt, dt_api->create_object(&dt, nodeBuildSettings, "INFOPLIST_FILE"),
                info_plist_path);
          }

          unsigned int nodeSearchPaths =
              dt_api->create_object(&dt, nodeBuildSettings, "LIBRARY_SEARCH_PATHS");
          dt.objects[nodeSearchPaths].is_array = true;
          unsigned int node                    = dt_api->create_object(&dt, nodeSearchPaths, "");
          dt_api->set_object_value(&dt, node, "\"$(inherited)\"");
          const char* link_additional_dependencies = "";
          for (unsigned ipc = 0; ipc < array_count(p->state); ++ipc) {
            const cc_state_impl_t* flags = &(p->state[ipc]);

            // TODO ordering and combination so that more specific flags can override general
            // ones
            if ((p->configs[ipc] != config) && (p->configs[ipc] != NULL)) continue;

            for (unsigned di = 0; di < array_count(flags->external_libs); di++) {
              const char* lib_path_from_base = flags->external_libs[di];
              const char* relative_lib_path =
                  make_path_relative(build_to_base_path, lib_path_from_base);
              const char* lib_name            = strip_path(relative_lib_path);
              const char* lib_folder          = make_uri(folder_path_only(lib_path_from_base));
              const char* resolved_lib_folder = cc_substitute(
                  lib_folder, substitution_keys, substitution_values, countof(substitution_keys));

              link_additional_dependencies =
                  cc_printf("%s -l%s", link_additional_dependencies, lib_name);
              node = dt_api->create_object(&dt, nodeSearchPaths, "");
              dt_api->set_object_value(&dt, node,
                                       cc_printf("\"$(PROJECT_DIR)/%s\"", resolved_lib_folder));
            }
          }
          if (link_additional_dependencies[0] != 0) {
            dt_api->set_object_value(
                &dt, dt_api->create_object(&dt, nodeBuildSettings, "OTHER_LDFLAGS"),
                cc_printf("\"%s\"", link_additional_dependencies));
          }
          if (p->type == CCProjectTypeDynamicLibrary) {
            dt_api->set_object_value(
                &dt, dt_api->create_object(&dt, nodeBuildSettings, "EXECUTABLE_PREFIX"), "lib");
            dt_api->set_object_value(&dt,
                                     dt_api->create_object(&dt, nodeBuildSettings, "INSTALL_PATH"),
                                     "\"@executable_path\"");
          }
        } break;
        case EPlatformPhone: {
          dt_api->set_object_value(&dt,
                                   dt_api->create_object(&dt, nodeBuildSettings, "INFOPLIST_FILE"),
                                   "../src/Info.plist");
          unsigned int nodeSearchPaths =
              dt_api->create_object(&dt, nodeBuildSettings, "LIBRARY_SEARCH_PATHS");
          dt.objects[nodeSearchPaths].is_array = true;
          unsigned int node                    = dt_api->create_object(&dt, nodeSearchPaths, "");
          dt_api->set_object_value(&dt, node, "\"$(inherited)\"");
          node = dt_api->create_object(&dt, nodeSearchPaths, "");
          dt_api->set_object_value(&dt, node, "\"@executable_path/Frameworks\"");
          dt_api->set_object_value(
              &dt, dt_api->create_object(&dt, nodeBuildSettings, "PRODUCT_BUNDLE_IDENTIFIER"),
              "\"net.daedalus-development.TestGame\"");
        } break;
      }
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeBuildSettings, "PRODUCT_NAME"),
                               "\"$(TARGET_NAME)\"");
      dt_api->set_object_value(&dt, dt_api->create_object(&dt, nodeBuildSettings, "SRCROOT"),
                               cc_printf("\"%s\"", build_to_base_path));
    }
  }

  unsigned int nodeRootObject = dt_api->create_object(&dt, 0, "rootObject");
  dt_api->set_object_value(&dt, nodeRootObject, project_object_id);
  dt_api->set_object_comment(&dt, nodeRootObject, "Project object");

  export_tree_as_xcode(f, &dt);
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

void xcode_generateInFolder() {
  const char* in_workspace_path = _internal.workspace_path;

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

  // If not only generating, then a new binary is built, that is run, and only then do we attempt
  // to clean up this existing version. This binary doesn't do any construction of projects.
  if (!_internal.only_generate) {
    printf("Rebuilding CConstruct ...");
    cc_recompile_binary_(_internal.config_file_path);
    printf(" done\n");
    int bresult = cc_runNewBuild_(_internal.argv, _internal.argc);
    if (bresult == 0) {
      cc_activateNewBuild_();
    }
    exit(bresult);
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
    (void)make_folder(project_path);

    const char* project_file_path = cc_printf("%s/project.pbxproj", project_path);

    FILE* f = fopen(project_file_path, "wb");

    if (f) {
      xCodeCreateProjectFile(f, p, projectFileReferenceUUIDs, in_workspace_path,
                             build_to_base_path);
      fclose(f);
    }

    printf("Constructed XCode project '%s'\n", project_path);
  }

  {
    const char* workspace_path = cc_printf("%s.xcworkspace", cc_data_.workspaceLabel);
    (void)make_folder(workspace_path);
    const char* workspace_file_path = cc_printf("%s/contents.xcworkspacedata", workspace_path);
    FILE* f                         = fopen(workspace_file_path, "wb");
    if (f) {
      xCodeCreateWorkspaceFile(f);
      fclose(f);
      printf("Constructed XCode workspace at '%s'\n", workspace_path);
    }
  }
}
