
const char* replaceSpacesWithUnderscores(const char* in) {
  unsigned length = strlen(in);
  char* out = (char*)cc_alloc_(length+1);  // +1 for terminating 0
  memcpy(out, in, length);
  out[length] = 0;
  char* it = out;
  for( unsigned i=0; i<length; ++i, ++it ) {
    if( *it ==' ' ) {
      *it = '_';
    }
  }
  return in;
}

const char* vs_generateUUID() {
  static size_t count = 0;
  ++count;

  char* buffer = (char*)cc_alloc_(37);
  sprintf(buffer, "00000000-0000-0000-0000-%012zi", count);
  return buffer;
};

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

void vs2019_generateInFolder(const char* workspace_path) {
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

  const char* workspace_file_path = cc_printf("%s.sln", privateData.workspaceLabel);
  FILE* workspace = fopen(workspace_file_path, "wb");
  if (workspace == NULL) {
    fprintf(stderr, "Couldn't create workspace.sln\n");
    return;
  }

  fprintf(workspace, R"lit(Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 16
VisualStudioVersion = 16.0.29709.97
MinimumVisualStudioVersion = 10.0.40219.1
)lit");

  const char** project_ids = (const char**)cc_alloc_(sizeof(const char*)*array_count(privateData.projects));

  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
    project_ids[i] = vs_generateUUID();
    auto projectId = project_ids[i];
    auto p         = privateData.projects[i];
    fprintf(workspace,
            "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s.vcxproj\", "
            "\"{%s}\"\n",
            p->name, replaceSpacesWithUnderscores(p->name), projectId);
    fprintf(workspace, "EndProject\n");
  }

  fprintf(workspace, R"lit(Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
)lit");
  for (unsigned ci = 0; ci < array_count(privateData.configurations); ++ci) {
    auto c = privateData.configurations[ci]->label;
    for (unsigned pi = 0; pi < array_count(privateData.platforms); ++pi) {
      auto p = privateData.platforms[pi]->type;
      fprintf(workspace, "\t\t%s|%s = %s|%s\n", c, solutionPlatform2String(p), c,
              solutionPlatform2String(p));
    }
  }
  fprintf(workspace, R"lit(	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
)lit");
  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
    auto projectId = project_ids[i];
    for (unsigned ci = 0; ci < array_count(privateData.configurations); ++ci) {
      auto c = privateData.configurations[ci]->label;
      for (unsigned pi = 0; pi < array_count(privateData.platforms); ++pi) {
        auto p = privateData.platforms[pi]->type;
        auto platform_label = vs_projectPlatform2String_(p);
        fprintf(workspace, "\t\t{%s}.%s|%s.ActiveCfg = %s|%s\n", projectId, c,
                solutionPlatform2String(p), c, platform_label);
        fprintf(workspace, "\t\t{%s}.%s|%s.Build.0 = %s|%s\n", projectId, c,
                solutionPlatform2String(p), c, platform_label);
      }
    }
  }

  fprintf(workspace, R"lit(	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {7354F2AC-FB49-4B5D-B080-EDD798F580A5}
	EndGlobalSection
EndGlobal
)lit");

  fclose(workspace);
  printf("Created workspace at '%s'\n", workspace_path);

  for (unsigned i = 0; i < array_count(privateData.projects); ++i) {
    auto p          = privateData.projects[i];
    auto project_id = project_ids[i];
    vs2019_createProjectFile(p, project_id, project_ids, count_folder_depth);
    vs2019_createFilters(p);

    printf("Created project '%s' at '%s'\n", p->name, workspace_path);
  }
}
