
const char* replaceSpacesWithUnderscores(const char* in) {
  unsigned length = strlen(in);
  char* out       = (char*)cc_alloc_(length + 1);  // +1 for terminating 0
  memcpy(out, in, length);
  out[length] = 0;
  char* it    = out;
  for (unsigned i = 0; i < length; ++i, ++it) {
    if (*it == ' ') {
      *it = '_';
    }
  }
  return in;
}

const char* solutionArch2String_(EArchitecture arch) {
  switch (arch) {
    case EArchitectureX86:
      return "x86";
    case EArchitectureX64:
      return "x64";
    case EArchitectureARM:
      return "ARM";
  }
  return "";
}

void vs2019_generateInFolder(const char* in_workspace_path) {
  in_workspace_path = make_uri(in_workspace_path);
  if (in_workspace_path[strlen(in_workspace_path) - 1] != '/')
    in_workspace_path = cc_printf("%s/", in_workspace_path);

  char* output_folder = make_uri(cc_printf("%s%s", cc_data_.base_folder, in_workspace_path));

  char* build_to_base_path = make_path_relative(output_folder, cc_data_.base_folder);

  for (unsigned project_idx = 0; project_idx < array_count(cc_data_.projects); project_idx++) {
    // Adjust all the files to be relative to the build output folder
    cc_project_impl_t* project = cc_data_.projects[project_idx];
    for (unsigned file_idx = 0; file_idx < array_count(project->file_data); file_idx++) {
      project->file_data[file_idx]->path =
          cc_printf("%s%s", build_to_base_path, project->file_data[file_idx]->path);
    }
    for (unsigned file_idx = 0; file_idx < array_count(project->file_data_custom_command);
         file_idx++) {
      project->file_data_custom_command[file_idx]->path =
          cc_printf("%s%s", build_to_base_path, project->file_data_custom_command[file_idx]->path);
      project->file_data_custom_command[file_idx]->output_file = cc_printf(
          "%s%s", build_to_base_path, project->file_data_custom_command[file_idx]->output_file);
    }
    for (unsigned state_idx = 0; state_idx < array_count(project->state); state_idx++) {
      const cc_state_impl_t* state = project->state + state_idx;
      // Also all the include paths
      for (unsigned includes_idx = 0; includes_idx < array_count(state->include_folders);
           includes_idx++) {
        const char* include_path            = state->include_folders[includes_idx];
        const bool is_absolute_path         = (include_path[0] == '/') || (include_path[1] == ':');
        const bool starts_with_env_variable = (include_path[0] == '$');
        if (!is_absolute_path && !starts_with_env_variable) {
          state->include_folders[includes_idx] =
              cc_printf("%s%s", build_to_base_path, include_path);
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
  }

  printf("Generating Visual Studio 2019 solution and projects in '%s'...\n", output_folder);

  int result = make_folder(output_folder);
  if (result != 0) {
    fprintf(stderr, "Error %i creating path '%s'\n", result, output_folder);
  }
  (void)chdir(output_folder);

  const char** project_ids =
      (const char**)cc_alloc_(sizeof(const char*) * array_count(cc_data_.projects));

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    project_ids[i] = vs_generateUUID();
  }

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const cc_project_impl_t* p = cc_data_.projects[i];
    const char* project_id     = project_ids[i];
    vs2019_createFilters(p, output_folder);
    vs2019_createProjectFile(p, project_id, project_ids, output_folder);

    printf("Constructed VS2019 project '%s.vcxproj'\n", p->name);
  }

  // Create list of groups needed.
  bool* groups_needed = (bool*)cc_alloc_(array_count(cc_data_.groups) * sizeof(bool));
  memset(groups_needed, 0, array_count(cc_data_.groups) * sizeof(bool));
  for (unsigned i = 0; i < array_count(cc_data_.projects); i++) {
    unsigned g = cc_data_.projects[i]->parent_group_idx;
    while (g) {
      groups_needed[g] = true;
      g                = cc_data_.groups[g].parent_group_idx;
    }
  }

  const char* workspace_file_path = cc_printf("%s.sln", cc_data_.workspaceLabel);
  FILE* workspace                 = fopen(workspace_file_path, "wb");
  if (workspace == NULL) {
    fprintf(stderr, "Couldn't create workspace.sln\n");
    return;
  }

  fprintf(workspace,
          "Microsoft Visual Studio Solution File, Format Version 12.00\n# Visual Studio Version "
          "16\nVisualStudioVersion = 16.0.29709.97\nMinimumVisualStudioVersion = 10.0.40219.1\n");

  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const char* projectId      = project_ids[i];
    const cc_project_impl_t* p = cc_data_.projects[i];
    fprintf(workspace,
            "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s.vcxproj\", "
            "\"{%s}\"\n",
            p->name, replaceSpacesWithUnderscores(p->name), projectId);
    fprintf(workspace, "EndProject\n");
  }

  // Add solution folders
  const char** unique_groups_id = {0};
  for (unsigned i = 0; i < array_count(cc_data_.groups); i++) {
    const char* id = NULL;
    if (groups_needed[i]) {
      id                       = vs_generateUUID();
      const cc_group_impl_t* g = &cc_data_.groups[i];
      fprintf(workspace,
              "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"%s\", \"%s\", "
              "\"{%s}\"\n",
              g->name, g->name, id);
      fprintf(workspace, "EndProject\n");
    }
    array_push(unique_groups_id, id);
  }

  fprintf(workspace, "Global\n");

  fprintf(workspace, "	GlobalSection(SolutionConfigurationPlatforms) = preSolution\n");
  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
      const char* p = solutionArch2String_(cc_data_.architectures[pi]->type);
      fprintf(workspace, "		%s|%s = %s|%s\n", c, p, c, p);
    }
  }
  fprintf(workspace, "	EndGlobalSection\n");

  fprintf(workspace, "	GlobalSection(ProjectConfigurationPlatforms) = postSolution\n");
  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const char* projectId = project_ids[i];
    for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
      const char* c = cc_data_.configurations[ci]->label;
      for (unsigned pi = 0; pi < array_count(cc_data_.architectures); ++pi) {
        const EArchitecture p  = cc_data_.architectures[pi]->type;
        const char* arch_label = vs_projectArch2String_(p);
        fprintf(workspace, "		{%s}.%s|%s.ActiveCfg = %s|%s\n", projectId, c,
                solutionArch2String_(p), c, arch_label);
        fprintf(workspace, "		{%s}.%s|%s.Build.0 = %s|%s\n", projectId, c,
                solutionArch2String_(p), c, arch_label);
      }
    }
  }
  fprintf(workspace, "	EndGlobalSection\n");

  fprintf(workspace,
          "	GlobalSection(SolutionProperties) = preSolution\n"
          "		HideSolutionNode = FALSE\n"
          "	EndGlobalSection\n");

  fprintf(workspace, "	GlobalSection(NestedProjects) = preSolution\n");
  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const char* projectId      = project_ids[i];
    const cc_project_impl_t* p = cc_data_.projects[i];
    if (p->parent_group_idx) {
      fprintf(workspace, "		{%s} = {%s}\n", projectId,
              unique_groups_id[p->parent_group_idx]);
    }
  }
  for (unsigned i = 0; i < array_count(cc_data_.groups); i++) {
    if (groups_needed[i]) {
      const cc_group_impl_t* g = &cc_data_.groups[i];
      if (g->parent_group_idx) {
        fprintf(workspace, "		{%s} = {%s}\n", unique_groups_id[i],
                unique_groups_id[g->parent_group_idx]);
      }
    }
  }
  fprintf(workspace, "	EndGlobalSection\n");

  fprintf(workspace,
          "	GlobalSection(ExtensibilityGlobals) = postSolution\n"
          "		SolutionGuid = "
          "{7354F2AC-FB49-4B5D-B080-EDD798F580A5}\n"
          "	EndGlobalSection\nEndGlobal\n");

  fclose(workspace);
  printf("Constructed VS2019 workspace at '%s'\n", workspace_file_path);
}
