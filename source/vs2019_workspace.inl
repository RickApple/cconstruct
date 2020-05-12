
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

const char* solutionPlatform2String(EPlatformType platform) {
  switch (platform) {
    case EPlatformTypeX86:
      return "x86";
    case EPlatformTypeX64:
      return "x64";
    case EPlatformTypeARM:
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
    // Also all the include paths
    for (unsigned state_idx = 0; state_idx < array_count(project->state); state_idx++) {
      const cc_state_impl_t* state = project->state + state_idx;
      for (unsigned includes_idx = 0; includes_idx < array_count(state->include_folders);
           includes_idx++) {
        const char* include_path = state->include_folders[includes_idx];
        if (include_path[0] != '/' && include_path[1] != ':') {
          state->include_folders[includes_idx] =
              cc_printf("%s%s", build_to_base_path, include_path);
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
  const cc_group_impl_t** unique_groups = {0};
  const char** unique_groups_id         = {0};
  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const cc_project_impl_t* p = cc_data_.projects[i];
    const cc_group_impl_t* g   = p->parent_group;
    while (g) {
      bool already_contains_group = false;
      for (unsigned i = 0; i < array_count(unique_groups); ++i) {
        if (g == unique_groups[i]) {
          already_contains_group = true;
        }
      }
      if (!already_contains_group) {
        array_push(unique_groups, g);
        array_push(unique_groups_id, vs_generateUUID());
      }
      g = g->parent_group;
    }
  }
  const unsigned num_unique_groups = array_count(unique_groups);

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
  for (unsigned i = 0; i < num_unique_groups; ++i) {
    const cc_group_impl_t* g = unique_groups[i];
    fprintf(workspace,
            "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"%s\", \"%s\", "
            "\"{%s}\"\n",
            g->name, g->name, unique_groups_id[i]);
    fprintf(workspace, "EndProject\n");
  }

  fprintf(workspace, "Global\n");

  fprintf(workspace, "	GlobalSection(SolutionConfigurationPlatforms) = preSolution\n");
  for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
    const char* c = cc_data_.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(cc_data_.platforms); ++pi) {
      const char* p = solutionPlatform2String(cc_data_.platforms[pi]->type);
      fprintf(workspace, "		%s|%s = %s|%s\n", c, p, c, p);
    }
  }
  fprintf(workspace, "	EndGlobalSection\n");

  fprintf(workspace, "	GlobalSection(ProjectConfigurationPlatforms) = postSolution\n");
  for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
    const char* projectId = project_ids[i];
    for (unsigned ci = 0; ci < array_count(cc_data_.configurations); ++ci) {
      const char* c = cc_data_.configurations[ci]->label;
      for (unsigned pi = 0; pi < array_count(cc_data_.platforms); ++pi) {
        const EPlatformType p      = cc_data_.platforms[pi]->type;
        const char* platform_label = vs_projectPlatform2String_(p);
        fprintf(workspace, "		{%s}.%s|%s.ActiveCfg = %s|%s\n", projectId, c,
                solutionPlatform2String(p), c, platform_label);
        fprintf(workspace, "		{%s}.%s|%s.Build.0 = %s|%s\n", projectId, c,
                solutionPlatform2String(p), c, platform_label);
      }
    }
  }
  fprintf(workspace, "	EndGlobalSection\n");

  fprintf(workspace,
          "	GlobalSection(SolutionProperties) = preSolution\n"
          "		HideSolutionNode = FALSE\n"
          "	EndGlobalSection\n");

  if (num_unique_groups > 0) {
    fprintf(workspace, "	GlobalSection(NestedProjects) = preSolution\n");
    for (unsigned i = 0; i < array_count(cc_data_.projects); ++i) {
      const char* projectId      = project_ids[i];
      const cc_project_impl_t* p = cc_data_.projects[i];
      if (p->parent_group) {
        for (unsigned ig = 0; ig < num_unique_groups; ++ig) {
          if (p->parent_group == unique_groups[ig]) {
            fprintf(workspace, "		{%s} = {%s}\n", projectId, unique_groups_id[ig]);
          }
        }
      }
    }
    for (unsigned i = 0; i < num_unique_groups; ++i) {
      const cc_group_impl_t* g = unique_groups[i];
      if (g->parent_group) {
        for (unsigned ig = 0; ig < num_unique_groups; ++ig) {
          if (g->parent_group == unique_groups[ig]) {
            fprintf(workspace, "		{%s} = {%s}\n", unique_groups_id[i],
                    unique_groups_id[ig]);
          }
        }
      }
    }
    fprintf(workspace, "	EndGlobalSection\n");
  }

  fprintf(workspace,
          "	GlobalSection(ExtensibilityGlobals) = postSolution\n"
          "		SolutionGuid = "
          "{7354F2AC-FB49-4B5D-B080-EDD798F580A5}\n"
          "	EndGlobalSection\nEndGlobal\n");

  fclose(workspace);
  printf("Constructed VS2019 workspace at '%s'\n", workspace_file_path);
}
